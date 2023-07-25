/**
 * RTS signal control class on mingw base. Sub Class of Serial Control 
 *
 * @file    RTSControl-mingw.cpp
 * @brief   RTS signal control class
 * @author  Shouji, Igarashi
 *
 * (c) 2018 Shouji, Igarashi.
 */

#include "SerialControl.hpp"
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <windows.h>

using namespace MyApplications;
using namespace boost;
using namespace boost::asio;

class RTSContorlMingw : public SerialControl::RtsContorl
{
protected:
    const HANDLE commDevice;
    const DWORD clear_RTS = 4;
    const DWORD set_RTS = 3;

public:
    RTSContorlMingw( HANDLE commDeviceIn );
    virtual ~RTSContorlMingw(void);
    virtual void set(void);
    virtual void clear(void);
};

RTSContorlMingw::RTSContorlMingw( HANDLE commDeviceIn ) : commDevice(commDeviceIn)  { }
RTSContorlMingw::~RTSContorlMingw(void)                                             { }
void RTSContorlMingw::set(void)                                                     { EscapeCommFunction( commDevice, set_RTS );    }
void RTSContorlMingw::clear(void)                                                   { EscapeCommFunction( commDevice, clear_RTS );  }

// ----------<< SerialControl >>----------
SerialControl::RtsContorl * SerialControl::createRtsControl(unsigned long int handle)
{
    RtsContorl * rts = new RTSContorlMingw(port.native_handle(handle));
    return rts;
}
