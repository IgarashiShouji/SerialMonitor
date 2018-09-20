/**
 * Cyclic Timer Service Control Class on boost library
 *
 * @file    CyclicTimer.cpp
 * @brief   Cyclic Timer Control
 * @author  Shouji, Igarashi
 *
 * (c) 2018 Shouji, Igarashi.
 */

#include "CyclicTimer.hpp"
#include <boost/thread.hpp>

using namespace MyBoost;
using namespace boost;

TimerThread * TimerThread::obj = 0;
asio::io_service TimerThread::io_service;
posix_time::milliseconds TimerThread::interval(10);
boost::asio::deadline_timer TimerThread::timer(io_service, interval);

TimerThread::TimerThread( TimerHandler & obj_handle )
  : handle(obj_handle), th_timer( &TimerThread::main, this )
{
    loop = true;
    obj = this;
}

TimerThread::TimerThread( TimerThread & obj )
  : handle(obj.handle), th_timer( &TimerThread::main, this )
{
}

TimerThread::~TimerThread(void)
{
    th_timer.join();
}

void TimerThread::tick(const boost::system::error_code& /*e*/)
{
    TimerThread * obj = TimerThread::getObject();
    if( obj == 0 )
    {
        return;
    }
    if( obj->loop )
    {
        obj->handle.handler();
        timer.expires_at(timer.expires_at() + interval);
        timer.async_wait(tick);
    }
}

TimerThread * TimerThread::getObject(void)
{
    return obj;
}

void TimerThread::stop(void)
{
    loop = false;
}

void TimerThread::main( void )
{
    timer.async_wait(tick);
    io_service.run();
}
