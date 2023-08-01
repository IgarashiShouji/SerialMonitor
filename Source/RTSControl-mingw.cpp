/**
 * RTS signal control class on mingw base. Sub Class of Serial Control 
 *
 * @file    RTSControl-mingw.cpp
 * @brief   RTS signal control class
 * @author  Shouji, Igarashi
 *
 * (c) 2018 Shouji, Igarashi.
 */

#include "RtsControl.hpp"
#include "SerialControl.hpp"
#include <boost/asio.hpp>
#include <windows.h>

extern boost::asio::serial_port & getPortObject(SerialControl * obj);

class RTSContorlMinGW : public RtsContorl
{
protected:
    static const DWORD clear_RTS = 4;
    static const DWORD set_RTS   = 3;
    HANDLE  commDevice;
    bool    ctrl;
    bool    rts;
public:
    RTSContorlMinGW(HANDLE handle, bool ctrl);
    virtual ~RTSContorlMinGW(void);
    virtual void set(void);
    virtual void clear(void);
    virtual bool status(void) const;
};

RTSContorlMinGW::RTSContorlMinGW(HANDLE handle, bool _ctrl)
  : commDevice(handle), ctrl(_ctrl), rts(false)
{
}
RTSContorlMinGW::~RTSContorlMinGW(void)     { }
void RTSContorlMinGW::set(void)             { if(ctrl) { EscapeCommFunction( commDevice, set_RTS );   } rts = true;   }
void RTSContorlMinGW::clear(void)           { if(ctrl) { EscapeCommFunction( commDevice, clear_RTS ); } rts = false;  }
bool RTSContorlMinGW::status(void) const    { return rts; }

RtsContorl * SerialControl::createRtsControl(bool ctrl)
{
    boost::asio::serial_port & port = getPortObject(this);
    auto handle = port.native_handle();
    return new RTSContorlMinGW(handle, ctrl);
}
