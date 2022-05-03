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
#include "Timer.hpp"
#include "SerialControl.hpp"
#include <boost/asio.hpp>
#include <thread>
#include <chrono>
#include <stdio.h>

using namespace MyApplications;
using namespace MyEntity;
using namespace boost::asio;
using namespace std;

SerialControl::SerialControl(const char * pname, SerialSignal & obj, BaudRate baud, Parity parity, StopBit stop, bool RTS_control)
  : port(io, pname), rcv(obj), tick(0), latest(0), is_send(false), _baudrate(BR1200), bit_num(11), is_control_RTS(true)
{
    _baudrate = baud;
    bit_num   = 1 + 8 + stop;
    if(parity != none) bit_num ++;
    port.set_option(serial_port_base::baud_rate(baud));
    port.set_option(serial_port_base::character_size(8));
    port.set_option(serial_port_base::flow_control(serial_port_base::flow_control::none));
    switch(parity)
    {
        case none:
            port.set_option(serial_port_base::parity(serial_port_base::parity::none));
            break;
        case odd:
            port.set_option(serial_port_base::parity(serial_port_base::parity::odd));
            break;
        case even:
            port.set_option(serial_port_base::parity(serial_port_base::parity::even));
            break;
        default:
            break;
    }
    switch(stop)
    {
        case one:
            port.set_option(serial_port_base::stop_bits(serial_port_base::stop_bits::one));
            break;
        case two:
            port.set_option(serial_port_base::stop_bits(serial_port_base::stop_bits::two));
            break;
    }
    rts = createRtsControl();
    clearRTS();
}

SerialControl::~SerialControl(void)
{
    delete rts;
}

std::size_t SerialControl::read(unsigned char * data, std::size_t size)
{
    std::size_t len = 0;
    if( port.is_open() )
    {
        len = port.read_some(buffer(data, size));
        latest = tick;
    }
    return len;
}

std::size_t SerialControl::send(unsigned char * data, std::size_t size)
{
    if(size<=0)
    {
        return 0;
    }
    latest = tick;
    setRTS();
    std::size_t wlen = port.write_some(buffer(data, size));
    unsigned int wait_time = (1000 * bit_num * (wlen+1)) / _baudrate;
    std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));
    clearRTS();
    latest = tick;
    return wlen;
}

void SerialControl::clearRTS(void)
{
    if(is_control_RTS)
    {
        rts->clear();
    }
    is_send = false;
}

void SerialControl::setRTS(void)
{
    if(is_control_RTS)
    {
        rts->set();
    }
    is_send = true;
}

void SerialControl::handler(void)
{
    tick ++;
    if( !is_send )
    {
        unsigned int diff = tick - latest;
        rcv.rcvIntarval(diff);
    }
}

void SerialControl::close(void)
{
    port.close();
}

bool SerialControl::hasBaudRate(std::string & baud, Profile & info)
{
    static const char * list[] =
    {
        "B115200E1", "B115200E2", "B115200N1", "B115200N2", "B115200O1", "B115200O2",
        "B1200E1",   "B1200E2",   "B1200N1",   "B1200N2",   "B1200O1",   "B1200O2",
        "B19200E1",  "B19200E2",  "B19200N1",  "B19200N2",  "B19200O1",  "B19200O2",
        "B38400E1",  "B38400E2",  "B38400N1",  "B38400N2",  "B38400O1",  "B38400O2",
        "B9600E1",   "B9600E2",   "B9600N1",   "B9600N2",   "B9600O1",   "B9600O2"
    };
    enum {ListMax=(sizeof(list)/sizeof(list[0]))};
    static const Profile pro[ListMax] =
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
    if(idx < (sizeof(list)/sizeof(list[0])) )
    {
        info = pro[idx];
        return true;
    }
    return false;
}
