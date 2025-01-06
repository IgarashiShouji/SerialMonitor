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
#include <iconv.h>
#include <iostream>
#include <list>
#include <memory>
#include <regex>
#include <stdio.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <linux/serial.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>


using namespace MyEntity;
using namespace std;


/* --------------------------------------------------------------------------------<< Serial Control >>-------------------------------------------------------------------------------- */
class SerialControlLinux : public SerialControl
{
protected:
    int fd;
    bool ctrl;
    bool rts;

public:
    SerialControlLinux(const char * name, unsigned int bd, Parity pt, StopBit st, bool rts_ctrl);
    virtual ~SerialControlLinux(void);
    virtual std::size_t read(unsigned char * data, std::size_t size);
    virtual std::size_t send(unsigned char * data, std::size_t size);
    virtual bool rts_status(void) const;
    virtual void setRTS(void);
    virtual void clearRTS(void);
    virtual void close(void);
};
SerialControlLinux::SerialControlLinux(const char * name, unsigned int bd, Parity pt, StopBit st, bool rts_ctrl)
  : fd(-1), ctrl(rts_ctrl), rts(false)
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
    fd = open(name, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if(0 <= fd)
    {
        // シリアルポートの設定
        struct termios tty;
        if(0 == tcgetattr(fd, &tty))
        {
            cfsetospeed(&tty, bd); // 出力ボーレート
            cfsetispeed(&tty, bd); // 入力ボーレート

            tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8ビットデータ
            tty.c_iflag &= ~IGNBRK;                     // ブレークを無視しない
            tty.c_lflag = 0;                            // 非カノニカルモード
            tty.c_oflag = 0;                            // ローデータ出力
            tty.c_cc[VMIN] = 1;                         // 最低1文字の入力を要求
            tty.c_cc[VTIME] = 0;                        // タイムアウト無効
            switch(pt)
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
            switch(st)
            {
            case one:   tty.c_cflag &= ~CSTOPB; break;
            case two:   tty.c_cflag |= CSTOPB;  break;
            }
            if(0 != tcsetattr(fd, TCSANOW, &tty)) { close(); }
        }
        else { close(); }
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
        FD_SET(fd, &readfds);    // シリアル通信を追加
        int max_fd = fd;

        // `select` を呼び出し
        int activity = select(max_fd + 1, &readfds, nullptr, nullptr, nullptr);
        if(0 <= activity)
        {
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
        ::close(fd);
        fd = -1;
    }
}

SerialControl * SerialControl::createObject(const string & name, unsigned int baud, Parity pt, StopBit st, bool rts)
{
    SerialControl * com = new SerialControlLinux(name.c_str(), baud, pt, st, rts);
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




#if 0
void test(void)
{
        // シリアルポートを開く
    const char* serial_port = "/dev/ttyS0"; // 使用するシリアルポートを指定
    int serial_fd = open(serial_port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (serial_fd < 0) {
        std::cerr << "Failed to open serial port: " << strerror(errno) << std::endl;
        return;
    }

    // シリアルポートの設定
    struct termios tty;
    if (tcgetattr(serial_fd, &tty) != 0) {
        std::cerr << "Failed to get serial attributes: " << strerror(errno) << std::endl;
        close(serial_fd);
        return;
    }

    cfsetospeed(&tty, B9600); // 出力ボーレート
    cfsetispeed(&tty, B9600); // 入力ボーレート

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8ビットデータ
    tty.c_iflag &= ~IGNBRK;                     // ブレークを無視しない
    tty.c_lflag = 0;                            // 非カノニカルモード
    tty.c_oflag = 0;                            // ローデータ出力
    tty.c_cc[VMIN] = 1;                         // 最低1文字の入力を要求
    tty.c_cc[VTIME] = 0;                        // タイムアウト無効

    if (tcsetattr(serial_fd, TCSANOW, &tty) != 0) {
        std::cerr << "Failed to set serial attributes: " << strerror(errno) << std::endl;
        close(serial_fd);
        return;
    }

    // `select` で複数の入力を待つ
    while (true) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(fileno(stdin), &readfds); // 標準入力を追加
        FD_SET(serial_fd, &readfds);    // シリアル通信を追加

        int max_fd = std::max(fileno(stdin), serial_fd);

        // `select` を呼び出し
        int activity = select(max_fd + 1, &readfds, nullptr, nullptr, nullptr);
        if (activity < 0) {
            std::cerr << "select() error: " << strerror(errno) << std::endl;
            break;
        }

        // 標準入力からのデータを処理
        if (FD_ISSET(fileno(stdin), &readfds)) {
            char buffer[256];
            if (fgets(buffer, sizeof(buffer), stdin) != nullptr) {
                std::cout << "Standard Input: " << buffer;
            }
        }

        // シリアル通信からのデータを処理
        if (FD_ISSET(serial_fd, &readfds)) {
            char buffer[256];
            ssize_t bytes_read = read(serial_fd, buffer, sizeof(buffer) - 1);
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                std::cout << "Serial Input: " << buffer;
            }
        }
    }

    close(serial_fd);
}
#endif
