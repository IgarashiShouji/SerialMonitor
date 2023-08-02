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
    virtual bool status(void) const { return rts; }
};

class SerialControlBoost : public SerialControl
{
protected:
        boost::asio::io_service     io;
        boost::asio::serial_port    port;
        BaudRate                    _baudrate;
        unsigned char               bit_num;
        RtsContorl                  rts;

public:
        SerialControlBoost(const char * name, BaudRate bd, Parity pt, StopBit st, bool rts);
        virtual ~SerialControlBoost(void);
        virtual std::size_t read(unsigned char * data, std::size_t size);
        virtual std::size_t send(unsigned char * data, std::size_t size);
        virtual void setRTS(void);
        virtual void clearRTS(void);
        virtual void close(void);
};

SerialControlBoost::SerialControlBoost(const char * name, BaudRate bd, Parity pt, StopBit st, bool rts_ctrl)
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

void SerialControlBoost::setRTS(void)   { rts.set();   }
void SerialControlBoost::clearRTS(void) { rts.clear(); }

SerialControl * SerialControl::createObject(const string & name, BaudRate baud, Parity pt, StopBit st, bool rts)
{
    SerialControl * com = new SerialControlBoost(name.c_str(), baud, pt, st, rts);
    return com;
}

bool SerialControl::hasBaudRate(std::string & baud, Profile & info)
{
    static const char * list[] =
    {
        "115200E1", "115200E2", "115200N1", "115200N2", "115200O1", "115200O2",
        "1200E1",   "1200E2",   "1200N1",   "1200N2",   "1200O1",   "1200O2",
        "19200E1",  "19200E2",  "19200N1",  "19200N2",  "19200O1",  "19200O2",
        "38400E1",  "38400E2",  "38400N1",  "38400N2",  "38400O1",  "38400O2",
        "9600E1",   "9600E2",   "9600N1",   "9600N2",   "9600O1",   "9600O2"
    };
    enum {ListMax=(sizeof(list)/sizeof(list[0]))};
    static const Profile prof[ListMax] =
    {
        { BR115200, even, one }, { BR115200, even, two }, { BR115200, none, one }, { BR115200, none, two }, { BR115200, odd,  one }, { BR115200, odd,  two },
        { BR1200,   even, one }, { BR1200,   even, two }, { BR1200,   none, one }, { BR1200,   none, two }, { BR1200,   odd,  one }, { BR1200,   odd,  two },
        { BR19200,  even, one }, { BR19200,  even, two }, { BR19200,  none, one }, { BR19200,  none, two }, { BR19200,  odd,  one }, { BR19200,  odd,  two },
        { BR38400,  even, one }, { BR38400,  even, two }, { BR38400,  none, one }, { BR38400,  none, two }, { BR38400,  odd,  one }, { BR38400,  odd,  two },
        { BR9600,   even, one }, { BR9600,   even, two }, { BR9600,   none, one }, { BR9600,   none, two }, { BR9600,   odd,  one }, { BR9600,   odd,  two }
    };

    ConstArray<const char *> arry(list, (sizeof(list)/sizeof(list[0])));
    ConstCString target(baud.c_str(), baud.size());
    size_t idx = getIndexArray<const char *>(arry, target);
    if(idx < __ArrayCount(list))
    {
        info = prof[idx];
        return true;
    }
    return false;
}
