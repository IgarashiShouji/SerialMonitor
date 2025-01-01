/**
 * Serial Control Class on boost library
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
#include <boost/asio.hpp>
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


using namespace MyEntity;
using namespace boost::asio;
using namespace std;


/**
 * convert to UTF8 string from CP932
 *
 * @param   dst
 * @param   icd
 * @param   src
 * @param   size
 */
static void toUTF8(string & dst, iconv_t & icd, char * src, size_t size)
{
    size_t sz = size;
    while(0 < sz)
    {
        char utf8[size*2];
        char * p_dst = utf8;
        size_t sz_utf8 = sizeof(utf8);
        iconv(icd, &src, &sz, &p_dst, &sz_utf8);
        *p_dst = '\0';
        dst += utf8;
    }
}


/* --------------------------------------------------------------------------------<< Serial Control >>-------------------------------------------------------------------------------- */
class RtsContorl
{
protected:
    int     fd;
    bool    ctrl;
    bool    rts;
public:
    RtsContorl(int _fd, bool _ctrl) : fd(_fd), ctrl(_ctrl), rts(false) { }
    virtual ~RtsContorl(void)                                          { }
    virtual void set(void)              { if(ctrl) { int data = TIOCM_RTS; ioctl(fd, TIOCMBIS, &data); } rts = true;   }
    virtual void clear(void)            { if(ctrl) { int data = TIOCM_RTS; ioctl(fd, TIOCMBIC, &data); } rts = false;  }
    virtual bool status(void) const     { if(ctrl) { return rts; } return false; }
};

class SerialControlBoost : public SerialControl
{
protected:
    boost::asio::io_service     io;
    boost::asio::serial_port    port;
    unsigned int                _baudrate;
    unsigned char               bit_num;
    RtsContorl                  rts;
    int                         fd_pipe[2];

public:
    SerialControlBoost(const char * name, unsigned int bd, Parity pt, StopBit st, bool rts_ctrl);
    virtual ~SerialControlBoost(void);
    virtual std::size_t read(unsigned char * data, std::size_t size);
    virtual std::size_t send(unsigned char * data, std::size_t size);
    virtual bool rts_status(void) const;
    virtual void setRTS(void);
    virtual void clearRTS(void);
    virtual void close(void);
};

SerialControlBoost::SerialControlBoost(const char * name, unsigned int bd, Parity pt, StopBit st, bool rts_ctrl)
  : port(io, name), _baudrate(bd), bit_num(1 + 8 + st), rts(port.native_handle(), rts_ctrl)
{
    auto result =  pipe(fd_pipe);
    if(pt != none) bit_num ++;
    port.set_option(serial_port_base::baud_rate(_baudrate));
    port.set_option(serial_port_base::character_size(8));
    port.set_option(serial_port_base::flow_control(serial_port_base::flow_control::none));
    switch(pt)
    {
        case none:  port.set_option(serial_port_base::parity(serial_port_base::parity::none));  break;
        case odd:   port.set_option(serial_port_base::parity(serial_port_base::parity::odd));   break;
        case even:  port.set_option(serial_port_base::parity(serial_port_base::parity::even));  break;
        default: break;
    }
    switch(st)
    {
        case one:   port.set_option(serial_port_base::stop_bits(serial_port_base::stop_bits::one)); break;
        case two: port.set_option(serial_port_base::stop_bits(serial_port_base::stop_bits::two));   break;
        default: break;
    }
    clearRTS();
}

static void fdclose(int fd)
{
    close(fd);
}
SerialControlBoost::~SerialControlBoost(void)
{
    close();
    fdclose(fd_pipe[0]);
    fdclose(fd_pipe[1]);
}

void SerialControlBoost::close(void)
{
    static const uint8_t cmd = 1;
    auto w_size = write(fd_pipe[1], &cmd, sizeof(cmd));
    port.close();
}

std::size_t SerialControlBoost::read(unsigned char * data, std::size_t size)
{
    std::size_t len = 0;
    try
    {
        if( port.is_open() )
        {
            auto fd = port.native_handle();
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(fd, &fds);
            FD_SET(fd_pipe[0], &fds);
            auto max =fd; 
            if( max < fd_pipe[0] ) { max = fd_pipe[0]; }
            auto result = select( max+1, &fds, NULL, NULL, NULL );
            if( FD_ISSET(fd_pipe[0], &fds) )    { return 0; }
            if( FD_ISSET(fd, &fds) )            { len = port.read_some(buffer(data, size)); }
        }
    } catch(...) { }
    return len;
}

std::size_t SerialControlBoost::send(unsigned char * data, std::size_t size)
{
    if(0 < size)
    {
        setRTS();
        unsigned int    send_time = (1000 * bit_num * (size+1)) / _baudrate;
        std::size_t     wlen      = port.write_some(buffer(data, size));
        std::this_thread::sleep_for(std::chrono::milliseconds(send_time));
        clearRTS();
        return wlen;
    }
    return 0;
}

bool SerialControlBoost::rts_status(void) const { return rts.status(); }
void SerialControlBoost::setRTS(void)           { rts.set();   }
void SerialControlBoost::clearRTS(void)         { rts.clear(); }

SerialControl * SerialControl::createObject(const string & name, unsigned int baud, Parity pt, StopBit st, bool rts)
{
    SerialControl * com = new SerialControlBoost(name.c_str(), baud, pt, st, rts);
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
