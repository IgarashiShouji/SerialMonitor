/**
 * RTS signal control class on linux base. Sub Class of Serial Control 
 *
 * @file    RTSControl-linux.cpp
 * @brief   RTS signal control class
 * @author  Shouji, Igarashi
 *
 * (c) 2018 Shouji, Igarashi.
 */

#include "SerialControl.hpp"
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <sys/ioctl.h>

using namespace MyApplications;
using namespace boost;
using namespace boost::asio;

class RTSContorlLinux : public SerialControl::RtsContorl 
{
protected:
    int fd;
public:
    RTSContorlLinux(int fd);
    virtual ~RTSContorlLinux(void);
    virtual void set(void);
    virtual void clear(void);
};

RTSContorlLinux::RTSContorlLinux(int _fd)
  : fd(_fd)
{
}

RTSContorlLinux::~RTSContorlLinux(void)
{
}

void RTSContorlLinux::set(void)
{
    int data = TIOCM_RTS;
    ioctl(fd, TIOCMBIS, &data);
}

void RTSContorlLinux::clear(void)
{
    int data = TIOCM_RTS;
    ioctl(fd, TIOCMBIC, &data);
}

// ----------<< SerialControl >>----------
SerialControl::RtsContorl * SerialControl::createRtsControl(void)
{
    RtsContorl * rts = new RTSContorlLinux(port.native_handle());
    return rts;
}
