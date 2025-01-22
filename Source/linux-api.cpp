/**
 * Serial Control Class on linux system
 *
 * @file    SerialControl.cpp
 * @brief   Serial Control Class
 * @author  Shouji, Igarashi
 *
 * (c) 2018 Shouji, Igarashi.
 */
#include "Entity.hpp"
#include "SerialControl.hpp"
#include "ComList.hpp"
#include "PipeList.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <list>
#include <memory>
#include <regex>
#include <string>
#include <thread>

#include <fcntl.h>
#include <iconv.h>
#include <linux/serial.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

using namespace MyEntity;
using namespace std;


/* --------------------------------------------------------------------------------<< Serial Control >>-------------------------------------------------------------------------------- */
class SerialControlLinux : public SerialControl
{
protected:
    int fd;
    bool ctrl;
    bool rts;
    int  rfd;
    int  wfd;

public:
    SerialControlLinux(const char * name, struct Profile & profile);
    virtual ~SerialControlLinux(void);
    virtual std::size_t read(unsigned char * data, std::size_t size);
    virtual std::size_t send(unsigned char * data, std::size_t size);
    virtual bool rts_status(void) const;
    virtual void setRTS(void);
    virtual void clearRTS(void);
    virtual void close(void);
};

SerialControlLinux::SerialControlLinux(const char * name, struct Profile & profile)
  : fd(-1), ctrl(profile.rts), rts(false), rfd(-1), wfd(-1)
{
#if 0
    50  B50     1200    B1200   57600   B57600  1000000 B1000000
    75  B75     1800    B1800   115200  B115200 1152000 B1152000
    110 B110    2400    B2400   230400  B230400 1500000 B1500000
    134 B134    4800    B4800   460800  B460800 2000000 B2000000
    150 B150    9600    B9600   500000  B500000 2500000 B2500000
    200 B200    19200   B19200  576000  B576000 3000000 B3000000
    300 B300    38400   B38400  921600  B921600 3500000 B3500000
    600 B600                                    4000000 B4000000
#endif
    int fds[2]; auto presult = ::pipe(&fds[0]); rfd = fds[0]; wfd = fds[1];
    fd = open(name, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if(0 <= fd)
    {
        struct termios tty;
        if(0 == tcgetattr(fd, &tty))
        {
            cfsetospeed(&tty, profile.baud);
            cfsetispeed(&tty, profile.baud);
            tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
            tty.c_iflag &= ~IGNBRK;
            tty.c_lflag = 0;
            tty.c_oflag = 0;
            tty.c_cc[VMIN] = 1;
            tty.c_cc[VTIME] = 0;
            switch(profile.parity)
            {
            case odd:
                tty.c_cflag |= PARENB;
                tty.c_cflag |= PARODD;
                break;
            case even:
                tty.c_cflag |= PARENB;
                tty.c_cflag &= ~PARODD;
                break;
            case none:
            default:
                tty.c_cflag &= ~PARENB;
                break;
            }
            switch(profile.stop)
            {
            case one:   tty.c_cflag &= ~CSTOPB; break;
            case two:   tty.c_cflag |= CSTOPB;  break;
            }
            if(0 != tcsetattr(fd, TCSANOW, &tty)) { close(); }
        } else { close(); }
    }
}

SerialControlLinux::~SerialControlLinux(void)
{
    close();
}

std::size_t SerialControlLinux::send(unsigned char * data, std::size_t size)
{
    size_t wr_size = 0;
    if(0 <= fd)
    {
        wr_size = ::write(fd, data, size);
    }
    return wr_size;
}

std::size_t SerialControlLinux::read(unsigned char * data, std::size_t size)
{
    size_t rd_size = 0;
    if(0 <= fd)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(rfd, &readfds);
        FD_SET(fd, &readfds);
        int max_fd = max(fd, rfd);
        int activity = select(max_fd + 1, &readfds, nullptr, nullptr, nullptr);
        if(0 <= activity)
        {
            if(FD_ISSET(rfd, &readfds))
            {
                ::close(wfd);
                ::close(rfd);
                return rd_size;
            }
            if(FD_ISSET(fd, &readfds))
            {
                rd_size = ::read(fd, data, size);
            }
        }
    }
    return rd_size;
}

bool SerialControlLinux::rts_status(void) const
{
    if(ctrl) { return rts; }
    return false;
}

void SerialControlLinux::setRTS(void)
{
    if(ctrl)
    {
        int data = TIOCM_RTS;
        ioctl(fd, TIOCMBIS, &data);
    }
    rts = true;
}

void SerialControlLinux::clearRTS(void)
{
    if(ctrl)
    {
        int data = TIOCM_RTS;
        ioctl(fd, TIOCMBIC, &data);
    }
    rts = false;
}

void SerialControlLinux::close(void)
{
    if(0 <= fd)
    {
        uint8_t data[] = {"close"};
        auto wlen = ::write(wfd, data, sizeof(data));
        ::close(fd);
        fd = -1;
    }
}

SerialControl * SerialControl::createObject(const std::string & name, SerialControl::Profile & profile)
{
    static SerialControl * com = nullptr;
    if(nullptr == com)
    {
        com = new SerialControlLinux(name.c_str(), profile);
    }
    return com;
}


/* --------------------------------------------------------------------------------<< ComList >>-------------------------------------------------------------------------------- */
/**
 * constractor on ComList: create COM list
 */
ComList::ComList(void)
  : list(0)
{
    auto fp = popen("/bin/ls -l /sys/class/tty/*/device/driver | grep serial |  /bin/awk '{print $9}' | /bin/awk -F'/' '{print $5}'", "r");
    if( nullptr != fp)
    {
        char temp[1024] = {0};
        for(auto msg = fgets(temp, sizeof(temp), fp); (nullptr != msg) ; msg = fgets(temp, sizeof(temp), fp) )
        {
            string dev_name_(msg);
            auto dev_name = std::regex_replace(dev_name_, std::regex("\\n"), "");
            if( std::regex_search(dev_name, std::regex("USB")))
            {
                std::string info("");
                char cmd[1024];
                sprintf(cmd, "/bin/udevadm info /dev/%s | /bin/grep ID_SERIAL | /bin/sed -e 's/^.*ID_SERIAL.*=//'", dev_name.c_str());
                auto fp2 = popen(cmd, "r");
                if(nullptr != fp2)
                {
                    char temp[1024] = {0};
                    auto msg = fgets(temp, sizeof(temp), fp2);
                    if(nullptr != msg)
                    {
                        info = ": ";
                        info += msg;
                        info = std::regex_replace(info, std::regex("\\n"), "");
                    }
                    pclose(fp2);
                }
                dev_name = "/dev/" + dev_name + info;
            }
            else
            {
                dev_name = "/dev/" + dev_name;
            }
            list.push_back(dev_name);
        }
        pclose(fp);
    }
}

/**
 * destractor on comlist
 */
ComList::~ComList(void)
{
}

/**
 * reference of COM list
 */
std::list<std::string> & ComList::ref(void)
{
    return list;
}


/* --------------------------------------------------------------------------------<< PipeList >>-------------------------------------------------------------------------------- */
/**
 * constractor on PipeList: create pipe name list
 */
PipeList::PipeList(void)
{
}

/**
 * destractor on PipeList
 */
PipeList::~PipeList(void)
{
}

/**
 * reference of COM list
 */
std::list<std::string> & PipeList::ref(void)
{
    return list;
}


class CorePosix : public Core
{
protected:
    int  rfd;
    int  wfd;

public:
    CorePosix(void);
    virtual ~CorePosix(void);
    virtual void gets(std::string & str);
};
CorePosix::CorePosix(void)
{
    int fds[2];
    auto presult = ::pipe(&fds[0]);
    rfd = fds[0];
    wfd = fds[1];
}

CorePosix::~CorePosix(void)
{
    uint8_t data[] = {"close"};
    auto wlen = ::write(wfd, data, sizeof(data));
}

void CorePosix::gets(std::string & str)
{
    size_t rd_size = 0;
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(rfd, &readfds);
    FD_SET(0, &readfds);
    int max_fd = max(0, rfd);
    int activity = select(max_fd + 1, &readfds, nullptr, nullptr, nullptr);
    if(0 <= activity)
    {
        if(FD_ISSET(rfd, &readfds))
        {
            ::close(wfd);
            ::close(rfd);
            return;
        }
        if(FD_ISSET(0, &readfds))
        {
            std::getline(std::cin, str);
        }
    }
}

Core * Core::createObject(void)
{
    static Core * core = nullptr;
    if(nullptr == core)
    {
        core = new CorePosix();
    }
    return core;
}
