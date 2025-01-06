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
#include <boost/asio.hpp>
#include <thread>
#include <chrono>
#include <stdio.h>
#include <windows.h>
#include <iostream>

using namespace MyEntity;
using namespace boost::asio;
using namespace std;

class RtsContorl
{
protected:
    static const DWORD clear_RTS = 4;
    static const DWORD set_RTS   = 3;
    HANDLE  commDevice;
    bool    ctrl;
    bool    rts;
public:
    RtsContorl(HANDLE handle, bool _ctrl) : commDevice(handle), ctrl(_ctrl), rts(false) { }
    virtual ~RtsContorl(void)                                                           { }
    virtual void set(void)          { if(ctrl) { EscapeCommFunction( commDevice, set_RTS );   } rts = true;  }
    virtual void clear(void)        { if(ctrl) { EscapeCommFunction( commDevice, clear_RTS ); } rts = false; }
    virtual bool status(void) const     { if(ctrl) { return rts; } return false; }
};

class SerialControlBoost : public SerialControl
{
protected:
    boost::asio::io_service     io;
    boost::asio::serial_port    port;
    unsigned int                    _baudrate;
    unsigned char               bit_num;
    RtsContorl                  rts;

public:
    SerialControlBoost(const char * name, unsigned int bd, Parity pt, StopBit st, bool rts);
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

void SerialControlBoost::close(void)
{
    port.close();
}
SerialControlBoost::~SerialControlBoost(void)
{
    close();
}

std::size_t SerialControlBoost::read(unsigned char * data, std::size_t size)
{
    std::size_t len = 0;
    try
    {
        if( port.is_open() )
        {
            len = port.read_some(buffer(data, size));
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

#if 1
SerialControl * SerialControl::createObject(const string & name, unsigned int baud, Parity pt, StopBit st, bool rts)
{
    SerialControl * com = new SerialControlBoost(name.c_str(), baud, pt, st, rts);
    return com;
}
#endif
