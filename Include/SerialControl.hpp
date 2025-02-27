/**
 * Serial Control Class on Boost Library
 *
 * @file   SerialControl.hpp
 * @brief  Serial Control for half-duplex on boost library
 * @author Shouji, Igarashi
 *
 * (c) 2018 Shouji, Igarashi.
 */

#ifndef __SerialControl_cpp__
#define __SerialControl_cpp__

#include <string>
#include <stdlib.h>

class SerialControl
{
public:
    enum BaudRate
    {
        BR1200   = 1200,
        BR9600   = 9600,
        BR19200  = 19200,
        BR38400  = 38400,
        BR115200 = 115200
    };
    enum Parity
    {
        none,
        odd,
        even
    };
    enum StopBit
    {
        one = 1,
        two = 2
    };
    struct Profile
    {
        unsigned int    baud;
        enum Parity     parity;
        enum StopBit    stop;
        bool            rtsctrl;
    };

public:
    static SerialControl * createObject(const std::string & name, SerialControl::Profile & profile);
    virtual std::size_t read(unsigned char * data, std::size_t size) = 0;
    virtual std::size_t send(unsigned char * data, std::size_t size) = 0;
    virtual bool rts_status(void) const = 0;
    virtual void setRTS(bool rts) = 0;
    virtual void close(void) = 0;
    static void setHiPriorityThread(void);
};


class Core
{
public:
    virtual void gets(std::string & str) = 0;
    static Core * createObject(void);
};


#endif
