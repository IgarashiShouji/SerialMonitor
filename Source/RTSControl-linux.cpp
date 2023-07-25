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

using namespace boost;
using namespace boost::asio;

class RTSContorlLinux : public SerialControl::RtsContorl 
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

SerialControl::RtsContorl * SerialControl::createRtsControl(unsigned long int fd, bool ctrl)
{
    return new RTSContorlLinux(static_cast<int>(fd), ctrl);
}
