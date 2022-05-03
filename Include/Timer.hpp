/**
 * Timer Control Class
 *
 * @file   Timer.hpp
 * @brief  Timer Service on boost library
 * @author Shouji, Igarashi
 *
 * (c) 2018 Shouji, Igarashi.
 */

#ifndef __Timer_hpp__
#define __Timer_hpp__

#include <thread>
#include <chrono>
#include <atomic>
#include <list>

namespace MyApplications
{
    class TimerHandler
    {
    public:
        virtual void handler(void) = 0;
    };
    class TimerThread
    {
    private:
        std::atomic<bool>           active{false};
        std::list<std::jthread>     list_th;
    public:
        inline TimerThread(void);
        inline ~TimerThread(void);
        inline void timer(TimerHandler & obj, int interval);
        inline void cyclic(TimerHandler & obj, int interval);
        inline void stop(void);
    };
    inline TimerThread::TimerThread(void) { }
    inline TimerThread::~TimerThread(void) { stop(); };
    inline void TimerThread::timer(TimerHandler & sig_obj, int interval)
    {
        active = true;
        auto lamda = [this, &sig_obj, interval]
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
            if(active.load())
            {
                sig_obj.handler();
            }
        };
        list_th.push_back(std::jthread(lamda));
    }
    inline void TimerThread::cyclic(TimerHandler & sig_obj, int interval)
    {
        active = true;
        auto lamda = [this, &sig_obj, interval]
        {
            while(active.load())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(interval));
                if(active.load())
                {
                    sig_obj.handler();
                }
            }
        };
        list_th.push_back(std::jthread(lamda));
    }
    inline void TimerThread::stop(void)
    {
        if(active)
        {
            active = false;
            for(auto & th : list_th)
            {
                auto ss = th.get_stop_source();
                ss.request_stop();
            }
            for(auto & th : list_th)
            {
                th.join();
            }
            list_th.clear();
        }
    }
};


#endif
