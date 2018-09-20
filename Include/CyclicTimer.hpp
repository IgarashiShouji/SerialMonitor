/**
 * Cyclic Timer Control Class
 *
 * @file   CyclicTimer.hpp
 * @brief  Cyclic Timer Service on boost library
 * @author Shouji, Igarashi
 *
 * (c) 2018 Shouji, Igarashi.
 */

#ifndef __CyclicTimer_hpp__
#define __CyclicTimer_hpp__

#include <boost/thread.hpp>
#include <boost/asio.hpp>

namespace MyBoost
{
    class TimerHandler
    {
    public:
        virtual void handler(void) = 0;
    };
    
    class TimerThread
    {
    private:
        bool loop;
        TimerHandler & handle;
        boost::thread th_timer;
        static TimerThread * obj;
        static boost::asio::io_service io_service;
        static boost::posix_time::milliseconds interval;
        static boost::asio::deadline_timer timer;
    
    public:
        TimerThread(TimerHandler & obj_handle);
        TimerThread(TimerThread & obj);
        ~TimerThread(void);
        static TimerThread * getObject(void);
        void stop(void);
    private:
        static void tick(const boost::system::error_code& /*e*/);
        void main(void);
    };
};


#endif
