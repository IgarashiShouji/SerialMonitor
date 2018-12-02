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

#include "CyclicTimer.hpp"
#include <boost/asio.hpp>
#include <string>
#include <stdlib.h>

namespace MyBoost
{
    class SerialSignal
    {
    public:
        virtual void rcvIntarval(unsigned int tick) = 0;
    };

    class SerialControl : public TimerHandler
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
            enum BaudRate   baud;
            enum Parity     parity;
            enum StopBit    stop;
        };
        class RtsContorl
        {
        public:
            virtual void set(void) = 0;
            virtual void clear(void) = 0;
        };
    protected:
        boost::asio::io_service io;
        boost::asio::serial_port port;
        SerialSignal & rcv;
        RtsContorl * rts;
        unsigned int tick;
        unsigned int latest;
        bool is_send;
    public:
        SerialControl(const char * pname, SerialSignal & obj, BaudRate baud=BR1200, Parity pt=odd, StopBit st=one);
        virtual ~SerialControl(void);
        virtual std::size_t read(unsigned char * data, std::size_t size);
        virtual std::size_t send(unsigned char * data, std::size_t size);
        virtual void setRTS(void);
        virtual void clearRTS(void);
        virtual void handler(void);
        inline bool isSend(void);
        virtual void close(void);
        static bool hasBaudRate(std::string & baud, Profile & info);
    protected:
        RtsContorl * createRtsControl(void);
    };

    inline bool SerialControl::isSend(void)
    {
        return is_send;
    }
};


#endif
