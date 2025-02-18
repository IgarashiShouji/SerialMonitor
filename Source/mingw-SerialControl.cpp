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

class SerialControlBoost : public SerialControl
{
private:
    enum RtsCtrl
    {
        set_RTS   = 3,
        clear_RTS = 4,
    };
protected:
    boost::asio::io_service     io;
    boost::asio::serial_port    port;
    struct Profile profile;
    bool rts;
    unsigned char bit_num;
    HANDLE  handle;

public:
    SerialControlBoost(const char * name, struct Profile & prof);
    virtual ~SerialControlBoost(void);
    virtual std::size_t read(unsigned char * data, std::size_t size);
    virtual std::size_t send(unsigned char * data, std::size_t size);
    virtual bool rts_status(void) const;
    virtual void setRTS(bool rts);
    virtual void close(void);
};

SerialControlBoost::SerialControlBoost(const char * name, struct Profile & prof)
  : port(io, name), profile(prof), rts(true), bit_num(1 + 8), handle(port.native_handle())
{
    bit_num += profile.stop;
    bit_num += (profile.parity != none ? 1 : 0);
    port.set_option(serial_port_base::baud_rate(profile.baud));
    port.set_option(serial_port_base::character_size(8));
    port.set_option(serial_port_base::flow_control(serial_port_base::flow_control::none));
    switch(profile.parity)
    {
        case none:  port.set_option(serial_port_base::parity(serial_port_base::parity::none));  break;
        case odd:   port.set_option(serial_port_base::parity(serial_port_base::parity::odd));   break;
        case even:  port.set_option(serial_port_base::parity(serial_port_base::parity::even));  break;
        default: break;
    }
    switch(profile.stop)
    {
        case one: port.set_option(serial_port_base::stop_bits(serial_port_base::stop_bits::one)); break;
        case two: port.set_option(serial_port_base::stop_bits(serial_port_base::stop_bits::two)); break;
        default: break;
    }
    setRTS(false);
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
        if(port.is_open())
        {
            len = port.read_some(buffer(data, size));
        }
    } catch(...) { }
    return len;
}

std::size_t SerialControlBoost::send(unsigned char * data, std::size_t size)
{
    size_t wr_size = 0;
    try
    {
        if(port.is_open())
        {
            setRTS(true);
            unsigned int send_time = (((bit_num * 1000000) / profile.baud) * size) + ((2 * 100000) / profile.baud);
            wr_size = port.write_some(buffer(data, size));
            std::this_thread::sleep_for(std::chrono::microseconds(send_time));
            setRTS(false);
        }
    } catch(...) { }
    return wr_size;
}

bool SerialControlBoost::rts_status(void) const
{
    return rts;
}

void SerialControlBoost::setRTS(bool rts)
{
    if(profile.rtsctrl)
    {
        if(INVALID_HANDLE_VALUE != handle)
        {
            if(rts)
            {
                if(!this->rts)
                {   /* RTS: false -> true */
                    EscapeCommFunction(handle, set_RTS);
                }
            }
            else
            {
                if(this->rts)
                {   /* RTS: true -> false */
                    EscapeCommFunction(handle, clear_RTS);
                }
            }
        }
    }
    this->rts = rts;
}

void SerialControlBoost::close(void)
{
    try
    {
        if(port.is_open())
        {
            if(INVALID_HANDLE_VALUE != handle)
            {
                SetCommMask(handle, EV_ERR);
                PurgeComm(handle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
                handle = INVALID_HANDLE_VALUE;
            }
            port.close();
        }
    } catch(...) { }
}


SerialControl * SerialControl::createObject(const std::string & name, SerialControl::Profile & profile)
{
    return new SerialControlBoost(name.c_str(), profile);
}
