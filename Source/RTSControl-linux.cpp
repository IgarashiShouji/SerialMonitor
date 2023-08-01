/**
 * RTS signal control class on linux base. Sub Class of Serial Control 
 *
 * @file    RTSControl-linux.cpp
 * @brief   RTS signal control class
 * @author  Shouji, Igarashi
 *
 * (c) 2018 Shouji, Igarashi.
 */

#include "RtsControl.hpp"
#include "SerialControl.hpp"
#include <boost/asio.hpp>
#include <sys/ioctl.h>

extern boost::asio::serial_port & getPortObject(SerialControl * obj);

class RTSContorlLinux : public RtsContorl
{
protected:
    int     fd;
    bool    ctrl;
    bool    rts;
public:
    RTSContorlLinux(int fd, bool ctrl);
    virtual ~RTSContorlLinux(void);
    virtual void set(void);
    virtual void clear(void);
    virtual bool status(void) const;
};

RTSContorlLinux::RTSContorlLinux(int _fd, bool _ctrl)
  : fd(_fd), ctrl(_ctrl), rts(false)
{
}
RTSContorlLinux::~RTSContorlLinux(void)     { }
void RTSContorlLinux::set(void)             { if(ctrl) { int data = TIOCM_RTS; ioctl(fd, TIOCMBIS, &data); } rts = true;   }
void RTSContorlLinux::clear(void)           { if(ctrl) { int data = TIOCM_RTS; ioctl(fd, TIOCMBIC, &data); } rts = false;  }
bool RTSContorlLinux::status(void) const    { return rts; }

RtsContorl * SerialControl::createRtsControl(bool ctrl)
{
    boost::asio::serial_port & port = getPortObject(this);
    auto fd = port.native_handle();
    return new RTSContorlLinux(fd, ctrl);
}
