/**
 * Serial Monitor Application
 *
 * @file    main.cpp
 * @brief   Serial Monitor Application
 * @author  Shouji, Igarashi
 *
 * (c) 2018 Shouji, Igarashi.
 */

#include "Entity.hpp"
#include "SerialControl.hpp"
#include "ComList.hpp"

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <sstream>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <regex>
#include <filesystem>
#include <time.h>
#include <algorithm>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

#include <mruby.h>
#include <mruby/proc.h>
#include <mruby/data.h>
#include <mruby/compile.h>
#include <mruby/string.h>
#include <mruby/array.h>
#include <mruby/hash.h>
#include <mruby/range.h>
#include <mruby/proc.h>
#include <mruby/data.h>
#include <mruby/class.h>
#include <mruby/value.h>
#include <mruby/variable.h>
#include <mruby/dump.h>

#include <OpenXLSX.hpp>

/* class Options */
static mrb_value mrb_opt_initialize(mrb_state * mrb, mrb_value self);
static mrb_value mrb_opt_get(mrb_state * mrb, mrb_value self);
static mrb_value mrb_opt_size(mrb_state * mrb, mrb_value self);

/* class Clac */
static mrb_value mrb_core_crc16(mrb_state* mrb, mrb_value self)
{
    static const unsigned short modbusCRC[256] =
    {
        0x0000, 0xC1C0, 0x81C1, 0x4001, 0x01C3, 0xC003, 0x8002, 0x41C2, 0x01C6, 0xC006, 0x8007, 0x41C7, 0x0005, 0xC1C5, 0x81C4,
        0x4004, 0x01CC, 0xC00C, 0x800D, 0x41CD, 0x000F, 0xC1CF, 0x81CE, 0x400E, 0x000A, 0xC1CA, 0x81CB, 0x400B, 0x01C9, 0xC009,
        0x8008, 0x41C8, 0x01D8, 0xC018, 0x8019, 0x41D9, 0x001B, 0xC1DB, 0x81DA, 0x401A, 0x001E, 0xC1DE, 0x81DF, 0x401F, 0x01DD,
        0xC01D, 0x801C, 0x41DC, 0x0014, 0xC1D4, 0x81D5, 0x4015, 0x01D7, 0xC017, 0x8016, 0x41D6, 0x01D2, 0xC012, 0x8013, 0x41D3,
        0x0011, 0xC1D1, 0x81D0, 0x4010, 0x01F0, 0xC030, 0x8031, 0x41F1, 0x0033, 0xC1F3, 0x81F2, 0x4032, 0x0036, 0xC1F6, 0x81F7,
        0x4037, 0x01F5, 0xC035, 0x8034, 0x41F4, 0x003C, 0xC1FC, 0x81FD, 0x403D, 0x01FF, 0xC03F, 0x803E, 0x41FE, 0x01FA, 0xC03A,
        0x803B, 0x41FB, 0x0039, 0xC1F9, 0x81F8, 0x4038, 0x0028, 0xC1E8, 0x81E9, 0x4029, 0x01EB, 0xC02B, 0x802A, 0x41EA, 0x01EE,
        0xC02E, 0x802F, 0x41EF, 0x002D, 0xC1ED, 0x81EC, 0x402C, 0x01E4, 0xC024, 0x8025, 0x41E5, 0x0027, 0xC1E7, 0x81E6, 0x4026,
        0x0022, 0xC1E2, 0x81E3, 0x4023, 0x01E1, 0xC021, 0x8020, 0x41E0, 0x01A0, 0xC060, 0x8061, 0x41A1, 0x0063, 0xC1A3, 0x81A2,
        0x4062, 0x0066, 0xC1A6, 0x81A7, 0x4067, 0x01A5, 0xC065, 0x8064, 0x41A4, 0x006C, 0xC1AC, 0x81AD, 0x406D, 0x01AF, 0xC06F,
        0x806E, 0x41AE, 0x01AA, 0xC06A, 0x806B, 0x41AB, 0x0069, 0xC1A9, 0x81A8, 0x4068, 0x0078, 0xC1B8, 0x81B9, 0x4079, 0x01BB,
        0xC07B, 0x807A, 0x41BA, 0x01BE, 0xC07E, 0x807F, 0x41BF, 0x007D, 0xC1BD, 0x81BC, 0x407C, 0x01B4, 0xC074, 0x8075, 0x41B5,
        0x0077, 0xC1B7, 0x81B6, 0x4076, 0x0072, 0xC1B2, 0x81B3, 0x4073, 0x01B1, 0xC071, 0x8070, 0x41B0, 0x0050, 0xC190, 0x8191,
        0x4051, 0x0193, 0xC053, 0x8052, 0x4192, 0x0196, 0xC056, 0x8057, 0x4197, 0x0055, 0xC195, 0x8194, 0x4054, 0x019C, 0xC05C,
        0x805D, 0x419D, 0x005F, 0xC19F, 0x819E, 0x405E, 0x005A, 0xC19A, 0x819B, 0x405B, 0x0199, 0xC059, 0x8058, 0x4198, 0x0188,
        0xC048, 0x8049, 0x4189, 0x004B, 0xC18B, 0x818A, 0x404A, 0x004E, 0xC18E, 0x818F, 0x404F, 0x018D, 0xC04D, 0x804C, 0x418C,
        0x0044, 0xC184, 0x8185, 0x4045, 0x0187, 0xC047, 0x8046, 0x4186, 0x0182, 0xC042, 0x8043, 0x4183, 0x0041, 0xC181, 0x8180,
        0x4040
    };
    char * arg;
    mrb_get_args(mrb, "z", &arg);
    std::string data(arg);
    auto max = data.size();
    if(0 == (max % 2))
    {
        MyEntity::CalcCRC16 crc(modbusCRC);
        for(unsigned int idx = 0; idx < max; idx += 2)
        {
            std::stringstream ss;
            ss << std::hex << data.substr(idx, 2);
            int val;
            ss >> val;
            crc << val;
        }
        char temp[5] = { 0 };
        sprintf(temp, "%04X", *crc);
        return mrb_str_new_cstr(mrb, temp);
    }
    return mrb_nil_value();
}
static mrb_value mrb_core_crc8(mrb_state* mrb, mrb_value self)
{
    static const unsigned char crctab8[16] = { 0x00,0x9B,0xAD,0x36,0xC1,0x5A,0x6C,0xF7, 0x19,0x82,0xB4,0x2F,0xD8,0x43,0x75,0xEE };
    char * arg;
    mrb_get_args(mrb, "z", &arg);
    std::string data(arg);
    auto max = data.size();
    if(0 == ( max % 2 ))
    {
        unsigned char crc = 0;
        unsigned char high = 0;
        for(unsigned int idx = 0; idx < max; idx += 2)
        {
            std::stringstream ss;
            ss << std::hex << data.substr(idx, 2);
            int val;
            ss >> val;

            unsigned char data = static_cast<unsigned char>(val);
            high = crc >> 4;
            crc <<= 4;
            crc ^= crctab8[ high ^ (data >> 4) ];

            high = crc >> 4;
            crc <<= 4;
            crc ^= crctab8[ high ^ (data & 0x0f) ];
        }
        char temp[3] = { 0 };
        sprintf(temp, "%02x", crc);
        return mrb_str_new_cstr(mrb, temp);
    }
    return mrb_nil_value();
}
static mrb_value mrb_core_sum(mrb_state* mrb, mrb_value self)
{
    char * arg;
    mrb_get_args(mrb, "z", &arg);
    std::string data(arg);
    auto max = data.size();
    if(0 == (max % 2))
    {
        unsigned char sum = 0;
        for(unsigned int idx = 0; idx < max; idx += 2)
        {
            std::stringstream ss;
            ss << std::hex << data.substr(idx, 2);
            int val;
            ss >> val;
            sum ^= static_cast<unsigned char>(val);
        }
        char temp[3] = {0};
        sprintf(temp, "%02x", sum);
        return mrb_str_new_cstr(mrb, temp);
    }
    return mrb_nil_value();
}
static mrb_value mrb_core_float(mrb_state* mrb, mrb_value self)
{
    char * arg;
    mrb_get_args(mrb, "z", &arg);
    std::string data(arg);
    if(8 == data.size())
    {
        union
        {
            float           f;
            unsigned long   dword;
        } fval;
        fval.dword = 0;
        unsigned char sum = 0;
        for(unsigned int idx=0, max=data.size();idx<max; idx += 2)
        {
            std::stringstream ss;
            ss << std::hex << data.substr(idx, 2);
            int val;
            ss >> val;
            fval.dword <<= 8;
            fval.dword |= static_cast<unsigned long>(val);
        }
        return mrb_float_value( mrb, fval.f );
    }
    return mrb_nil_value();
}
static mrb_value mrb_core_float_l(mrb_state* mrb, mrb_value self)
{
    char * arg;
    mrb_get_args(mrb, "z", &arg);
    std::string org_data(arg);
    if(org_data.size() == 8)
    {
        std::string data("");
        data  = org_data.substr(6, 2);
        data += org_data.substr(4, 2);
        data += org_data.substr(2, 2);
        data += org_data.substr(0, 2);
        union
        {
            float           f;
            unsigned long   dword;
        } fval;
        fval.dword = 0;
        unsigned char sum = 0;
        for(unsigned int idx=0, max=data.size();idx<max; idx += 2)
        {
            std::stringstream ss;
            ss << std::hex << data.substr(idx, 2);
            int val;
            ss >> val;
            fval.dword <<= 8;
            fval.dword |= static_cast<unsigned long>(val);
        }
        return mrb_float_value( mrb, fval.f );
    }
    return mrb_nil_value();
}
static mrb_value mrb_core_to_hex(mrb_state* mrb, mrb_value self)
{
    union
    {
        float       f;
        mrb_int     val;
        uint8_t     data[4];
        uint16_t    data16[2];
        uint32_t    data32;
    } num;
    char str_data[8+1];
    char * type_ptr;
    mrb_get_args(mrb, "zi", &type_ptr, &num);
    std::string type(type_ptr);
    if(("int16" == type) || ("uint16" == type))
    {
        sprintf(str_data, "%04X", num.data16[0]);
        return mrb_str_new_cstr( mrb, str_data );
    }
    if("float" == type)
    {
        mrb_float mrb_f;
        mrb_get_args(mrb, "zf", &type_ptr, &mrb_f);
        num.f = mrb_f;
    }
    sprintf(str_data, "%08X", num.data32);
    return mrb_str_new_cstr( mrb, str_data );
}
static mrb_value mrb_core_reg_match(mrb_state* mrb, mrb_value self)
{
    char * org_ptr, * reg_ptr;
    mrb_get_args(mrb, "zz", &org_ptr, &reg_ptr);
    if( std::regex_search(org_ptr, std::regex(reg_ptr)))
    {
        return mrb_bool_value(true);
    }
    return mrb_bool_value(false);
}
static mrb_value mrb_core_reg_replace(mrb_state* mrb, mrb_value self)
{
    char * org_ptr, * reg_ptr, * rep_ptr;
    mrb_get_args(mrb, "zzz", &org_ptr, &reg_ptr, &rep_ptr);
    auto result = std::regex_replace(org_ptr, std::regex(reg_ptr), rep_ptr);
    return mrb_str_new_cstr( mrb, result.c_str() );
}
static mrb_value mrb_core_gets(mrb_state* mrb, mrb_value self);
static mrb_value mrb_core_exists(mrb_state* mrb, mrb_value self)
{
    char * arg;
    mrb_get_args(mrb, "z", &arg);
    return mrb_bool_value(std::filesystem::exists(arg));
}
static mrb_value mrb_core_file_timestamp(mrb_state* mrb, mrb_value self)
{
    char * arg;
    mrb_get_args(mrb, "z", &arg);
#if 0
    std::filesystem::path path(arg);
    auto ftime = std::filesystem::last_write_time(path);
    auto time = ( std::chrono::duration_cast<std::chrono::seconds>(ftime.time_since_epoch()) ).count();
    const std::tm * ltime = std::localtime(&time);
    std::ostringstream timestamp;
    timestamp << std::put_time(ltime, "%c");
    return mrb_str_new_cstr( mrb, (timestamp.str()).c_str() );
#else
    boost::filesystem::path path(arg);
    auto ftime = boost::filesystem::last_write_time(path);
    std::ostringstream timestamp;
    timestamp << ctime(&ftime);
    return mrb_str_new_cstr( mrb, (timestamp.str()).c_str() );
#endif
}
static mrb_value mrb_core_comlist(mrb_state* mrb, mrb_value self)
{
    mrb_value proc;
    mrb_get_args(mrb, "&", &proc);
    if (!mrb_nil_p(proc))
    {
        ComList com;
        std::vector<std::string> & list = com.ref();
        for(auto & com_name: list)
        {
            mrb_value argv[1];
            argv[0] = mrb_str_new_cstr(mrb, com_name.c_str());
            mrb_yield_argv(mrb, proc, 1, &(argv[0]));
        }
        return mrb_int_value(mrb, list.size());
    }
    return mrb_int_value(mrb, 0);
}


/* class thread */
static mrb_value mrb_thread_initialize(mrb_state * mrb, mrb_value self);
static mrb_value mrb_thread_run(mrb_state * mrb, mrb_value self);
static mrb_value mrb_thread_join(mrb_state * mrb, mrb_value self);
static mrb_value mrb_thread_is_run(mrb_state * mrb, mrb_value self);
static mrb_value mrb_thread_state(mrb_state * mrb, mrb_value self);
static mrb_value mrb_thread_sync(mrb_state * mrb, mrb_value self);
static mrb_value mrb_thread_wait(mrb_state * mrb, mrb_value self);
static mrb_value mrb_thread_notify(mrb_state * mrb, mrb_value self);
static mrb_value mrb_thread_ms_sleep(mrb_state * mrb, mrb_value self)
{
    mrb_int tick;
    mrb_get_args(mrb, "i", &tick);
    std::this_thread::sleep_for(std::chrono::milliseconds(tick));
    return mrb_nil_value();
}
static void mrb_thread_context_free(mrb_state * mrb, void * ptr);

/* class SerialMonitor */
static mrb_value mrb_smon_initialize(mrb_state * mrb, mrb_value self);
static mrb_value mrb_smon_wait(mrb_state * mrb, mrb_value self);
static mrb_value mrb_smon_send(mrb_state * mrb, mrb_value self);
static mrb_value mrb_smon_close(mrb_state * mrb, mrb_value self);
static void mrb_smon_context_free(mrb_state * mrb, void * ptr);

/* class OpenXLSX */
static mrb_value mrb_xlsx_initialize(mrb_state * mrb, mrb_value self);
static mrb_value mrb_xlsx_create(mrb_state * mrb, mrb_value self);
static mrb_value mrb_xlsx_open(mrb_state * mrb, mrb_value self);
static mrb_value mrb_xlsx_worksheet(mrb_state * mrb, mrb_value self);
static mrb_value mrb_xlsx_sheet_names(mrb_state * mrb, mrb_value self);
static mrb_value mrb_xlsx_set_seet_name(mrb_state * mrb, mrb_value self);
static mrb_value mrb_xlsx_set_value(mrb_state * mrb, mrb_value self);
static mrb_value mrb_xlsx_cell(mrb_state * mrb, mrb_value self);
static void mrb_xlsx_context_free(mrb_state * mrb, void * ptr);

/* class BinEdit */
static mrb_value mrb_bedit_initialize(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_length(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_load(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_save(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_write(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_dump(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_toItems(mrb_state * mrb, mrb_value self);
static void mrb_bedit_context_free(mrb_state * mrb, void * ptr);


auto split = [](std::string & src, auto pat)
{
    std::vector<std::string> result{};
    std::regex reg{pat};
    std::copy( std::sregex_token_iterator{src.begin(), src.end(), reg, -1}, std::sregex_token_iterator{}, std::back_inserter(result) );
    return result;
};

static unsigned char toValue(unsigned char data)
{
    unsigned char val=0;
    if(('0' <= data) && (data<='9'))
    {
        val = data - '0';
    }
    else if(('a' <= data) && (data<='f'))
    {
        val = 10 + data - 'a';
    }
    else if(('A' <= data) && (data<='F'))
    {
        val = 10 + data - 'A';
    }
    return val;
}

class WorkerThread
{
public:
    enum Status
    {
        Stop,
        Wakeup,
        Run,
        Wait,
        WaitJoin
    };
protected:
    enum Status             state;
    std::thread             th_ctrl;
    std::mutex              mtx;
    std::condition_variable cond;
    mrb_state *             mrb;
    mrb_value               proc;
public:
    WorkerThread(void) : state(Stop), mrb(nullptr) { }
    virtual ~WorkerThread(void)
    {
        std::lock_guard<std::mutex> lock(mtx);
        if(nullptr != this->mrb)
        {
            mrb_close(this->mrb);
            this->mrb = nullptr;
        }
    }
    bool run(mrb_state * mrb, mrb_value self)
    {
        bool result = false;
        std::unique_lock<std::mutex> lock(mtx);
        if(nullptr == this->mrb)
        {
            this->mrb = mrb_open();
            if(nullptr != this->mrb)
            {
                mrb_get_args(mrb, "&", &proc);
                if (!mrb_nil_p(proc))
                {
                    result = true;
                    state = Wakeup;
                    std::thread temp(&WorkerThread::run_context, this, 0);
                    th_ctrl.swap(temp);
                    auto lamda = [this]
                    {
                        if(state == Wakeup) { return false; }
                        return true;
                    };
                    state = Wakeup;
                    cond.wait(lock, lamda);
                }
            }
        }
        lock.unlock();
        return result;
    }
    void run_context(size_t id)
    {
        {
            std::lock_guard<std::mutex> lock(mtx);
            state = Run;
            cond.notify_all();
        }
        mrb_yield_argv(mrb, proc, 0, NULL);
        {
            std::lock_guard<std::mutex> lock(mtx);
            state = WaitJoin;
            cond.notify_all();
        }
    }
    void join(void)
    {
        auto lamda = [this]
        {
            switch(state)
            {
            case WaitJoin:
            case Stop:
                return true;
            default:
                break;
            }
            return false;
        };
        std::unique_lock<std::mutex> lock(mtx);
        cond.wait(lock, lamda);
        if(state == WaitJoin ) { th_ctrl.join(); }
        state = Stop;
        if(nullptr != this->mrb)
        {
            mrb_close(this->mrb);
            this->mrb = nullptr;
        }
    }
    enum Status get_state(void) const { return state;      }
    void wait(mrb_state * mrb, mrb_value & proc)
    {
        auto lamda = [this, &mrb, &proc]
        {
            if(Stop == state)     { return true; }
            if(WaitJoin == state) { return true; }
            auto result = mrb_yield_argv(mrb, proc, 0, NULL);
            if(mrb_bool(result))
            {
                state = Run;
                return true;
            }
            state = Wait;
            return false;
        };
        std::unique_lock<std::mutex> lock(mtx);
        cond.wait(lock, lamda);
    }
    mrb_value notify(mrb_state * mrb, mrb_value & proc)
    {
        std::lock_guard<std::mutex> lock(mtx);
        auto result = mrb_yield_argv(mrb, proc, 0, NULL);
        cond.notify_all();
        return result;
    }
    mrb_value sync(mrb_state * mrb, mrb_value & proc)
    {
        std::lock_guard<std::mutex> lock(mtx);
        auto result = mrb_yield_argv(mrb, proc, 0, NULL);
        return result;
    }
};

class SerialMonitor : public MyEntity::TimerHandler
{
public:
    enum State
    {
        GAP = 0,
        TIME_OUT_1,
        TIME_OUT_2,
        TIME_OUT_3,
        CACHE_FULL,
        CLOSE,
        NONE
    };
    struct ReciveInfo
    {
        enum State      state;
        unsigned int    cnt;
        unsigned char * buff;
    };
protected:
    std::string                              arg;
    std::vector<size_t>                      timer;
    std::map<SerialMonitor *, std::string> & res_list;
    SerialControl *                          com;
    MyEntity::OneShotTimerEventer<size_t> *  tevter;
    unsigned int                             cache_size;
    bool                                     rcv_enable;
    ReciveInfo                               cache;
    std::thread                              th_recive;
    std::mutex                               mtx;
    std::condition_variable                  cond;
    std::list<ReciveInfo>                    rcv_cache;
public:
    SerialMonitor(mrb_value & self, const char * arg_, std::vector<size_t> & timer_default, std::map<SerialMonitor *, std::string> & list, std::string def_boud, bool rts_ctrl)
      : arg(arg_), timer(timer_default), res_list(list), com(nullptr), tevter(nullptr), cache_size(1024), rcv_enable(true)
    {   /* split */
        std::vector<std::string> args;
        for( auto&& item : split( arg, "," ) )
        {
            args.push_back(item);
        }
        std::string name(args[0]);
        std::string baud(def_boud);
        if(1 < args.size())
        {
            for(auto idx = 1; idx < args.size(); idx ++)
            {
                auto & arg = args[idx];
                if( std::regex_match(arg, std::regex("GAP=[0-9]+")))
                {
                    auto str_num = std::regex_replace(arg, std::regex("GAP=([0-9]+)"), "$1");
                    timer[0] = stoi(str_num);
                }
                else if( std::regex_match(arg, std::regex("TO1=[0-9]+")))
                {
                    auto str_num = std::regex_replace(arg, std::regex("TO1=([0-9]+)"), "$1");
                    timer[1] = stoi(str_num);
                }
                else if( std::regex_match(arg, std::regex("TO2=[0-9]+")))
                {
                    auto str_num = std::regex_replace(arg, std::regex("TO2=([0-9]+)"), "$1");
                    timer[2] = stoi(str_num);
                }
                else if( std::regex_match(arg, std::regex("TO3=[0-9]+")))
                {
                    auto str_num = std::regex_replace(arg, std::regex("TO3=([0-9]+)"), "$1");
                    timer[3] = stoi(str_num);
                }
                else
                {
                    baud = arg;
                }
            }
        }
        SerialControl::Profile info;
        if( SerialControl::hasBaudRate(baud, info) )
        {
            com = SerialControl::createObject(name.c_str(), info.baud, info.parity, info.stop, rts_ctrl);
        }
        cache.buff  = new unsigned char [cache_size];
        cache.cnt   = 0;
        cache.state = NONE;
        std::thread temp(&SerialMonitor::recive, this, 0);
        th_recive.swap(temp);
    }
    virtual ~SerialMonitor(void)
    {
        delete com;
        delete [] cache.buff;
        cache.cnt   = 0;
        for(auto & item : rcv_cache)
        {
            delete [] item.buff;
        }
        rcv_cache.clear();
        res_list.erase(this);
        th_recive.join();
    }
    void close(void)
    {
        com->close();
        delete com;
        com = nullptr;
    }
    void send(std::string data, unsigned int timer)
    {
        if( nullptr != com)
        {
            for(auto pos = data.find_first_of(" "); pos != std::string::npos; pos = data.find_first_of(" "))
            {
                data.erase(pos, 1);
            }
            auto s_len = data.size();
            if( ( 1 < s_len ) && !( 1 & s_len ) )
            {   /* even charactor count and more size of 1 byte */
                auto max = s_len / 2;
                auto len = 0;
                auto bin = new unsigned  char [max];
                for(auto idx = 0; len < max; len ++)
                {
                    unsigned char val = (toValue(data.at(idx ++)) << 4);
                    val |= toValue(data.at(idx ++));
                    bin[len] = val;
                }
                com->send(bin, len);
                std::this_thread::sleep_for(std::chrono::milliseconds(timer));
                if( nullptr != tevter)
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    tevter->restart();
                }
                delete [] bin;
            }
        }
    }
    SerialMonitor::State recive_wait(std::string & data)
    {
        ReciveInfo rcv_info = { NONE, 0, nullptr };
        auto lamda = [this, &rcv_info]
        {
            if(!rcv_enable)
            {
                cache.state = CLOSE;
                cache.cnt   = 0;
                cache.buff  = nullptr;
                return true;
            }
            if(0 < rcv_cache.size())
            {
                rcv_info = *(rcv_cache.begin());
                rcv_cache.pop_front();
                return true;
            }
            return false;
        };
        std::unique_lock<std::mutex> lock(mtx);
        cond.wait(lock, lamda);
        for(unsigned int idx = 0; idx < rcv_info.cnt; idx ++)
        {
            char temp[4];
            sprintf(temp, "%02X", rcv_info.buff[idx]);
            temp[2] = '\0';
            data += temp;
        }
        delete [] rcv_info.buff;
        return rcv_info.state;
    }
    void recive(size_t id)
    {
        MyEntity::OneShotTimerEventer timeout_evter(timer, *this);
        tevter = &timeout_evter;
        while(rcv_enable)
        {
            if(nullptr == com) { break; }
            unsigned char data;
            size_t len = com->read(&(data), 1);
            if(0 == len) { break; }
            std::lock_guard<std::mutex> lock(mtx);
            timeout_evter.restart();
            if(0 < len)
            {
                cache.buff[cache.cnt] = data;
                cache.cnt += len;
                if(cache_size <= cache.cnt)
                {
                    cache.state = CACHE_FULL;
                    rcv_cache.push_back(cache);
                    cache.buff  = new unsigned char [cache_size];
                    cache.cnt   = 0;
                    cond.notify_all();
                }
            }
        }
        std::lock_guard<std::mutex> lock(mtx);
        rcv_enable = false;
        cache.state = CLOSE;
        cache.cnt   = 0;
        rcv_cache.push_back(cache);
        cache.buff  = new unsigned char [cache_size];
        cache.cnt   = 0;
        cond.notify_all();
    }
    virtual void handler(unsigned int idx)
    {
        static const enum State evt_state[] = { GAP, TIME_OUT_1, TIME_OUT_2, TIME_OUT_3 };
        if(idx < __ArrayCount(evt_state))
        {
            if( !com->rts_status() )
            {
                std::lock_guard<std::mutex> lock(mtx);
                cache.state = evt_state[idx];
                rcv_cache.push_back(cache);
                cache.buff = new unsigned char [cache_size];
                cache.cnt  = 0;
                cond.notify_all();
            }
        }
    }
};

class OpenXLSXCtrl
{
public:
    enum Type
    {
        Empty,
        Boolean,
        Integer,
        Float,
        Error,
        String
    };
protected:
    OpenXLSX::XLDocument  xlsx;
    OpenXLSX::XLWorkbook  book;
    OpenXLSX::XLWorksheet sheet;

public:
    OpenXLSXCtrl(void)          { }
    virtual ~OpenXLSXCtrl(void) { }
    void create(char * fname)                        { xlsx.create(fname);           }
    void open(char * fname)                          { xlsx.open(fname);             }
    void workbook(void)                              { book = xlsx.workbook();       }
    std::vector<std::string> getWorkSheetNames(void) { return book.worksheetNames(); }
    void worksheet(char * sheet_name)
    {
        auto list = book.worksheetNames();
        for(auto & name: list)
        {
            if( name == sheet_name )
            {
                sheet = book.worksheet(sheet_name);
                return;
            }
        }
        book.addWorksheet(sheet_name);
        sheet = book.worksheet(sheet_name);
    }
    void set_sheet_name(char * sheet_name)            { sheet.setName(sheet_name);                                      }
    void set_cell_value(char * cell_name, int val)    { sheet.cell(OpenXLSX::XLCellReference(cell_name)).value() = val; }
    void set_cell_value(char * cell_name, float val)  { sheet.cell(OpenXLSX::XLCellReference(cell_name)).value() = val; }
    void set_cell_value(char * cell_name, char * val) { sheet.cell(OpenXLSX::XLCellReference(cell_name)).value() = val; }
    OpenXLSXCtrl::Type get_cell(char * cell_name, int & val_int, float & val_float, std::string & str) const
    {
        switch( (sheet.cell(OpenXLSX::XLCellReference(cell_name)).value()).type() )
        {
        case OpenXLSX::XLValueType::Empty:
            return Empty;
        case OpenXLSX::XLValueType::Boolean:
            if( (sheet.cell(OpenXLSX::XLCellReference(cell_name)).value()).get<bool>() ) { val_int = 1; }
            else                    { val_int = 0; }
            return Boolean;
        case OpenXLSX::XLValueType::Integer:
            val_int = static_cast<int>((sheet.cell(OpenXLSX::XLCellReference(cell_name)).value()).get<int64_t>());
            return Integer;
        case OpenXLSX::XLValueType::Float:
            val_float = static_cast<float>((sheet.cell(OpenXLSX::XLCellReference(cell_name)).value()).get<double>());
            return Float;
        case OpenXLSX::XLValueType::String:
            str = (sheet.cell(OpenXLSX::XLCellReference(cell_name)).value()).get<std::string>();
            return String;
        case OpenXLSX::XLValueType::Error:
        default:
            break;
        }
        return Error;
    }
    void save(void)  { xlsx.save();  }
    void close(void) { xlsx.close(); }
};

class BinaryControl
{
protected:
    size_t length;
    unsigned char * data;
    std::mutex mtx;
public:
    BinaryControl(void) : length(0), data(nullptr) { }
    virtual ~BinaryControl(void)
    {
        std::lock_guard<std::mutex> lock(mtx);
        if(nullptr!=data)
        {
            delete [] data;
        }
    }
    size_t loadBinaryFile(std::string & fname)
    {
        std::lock_guard<std::mutex> lock(mtx);
        std::filesystem::path path(fname);
        length = std::filesystem::file_size(path);
        if(nullptr != data) { delete [] data; }
        data = new unsigned char[length];
        std::ifstream fin(fname);
        if(fin.is_open())
        {
            fin.read(reinterpret_cast<char *>(data), length);
            return length;
        }
        return 0;
    }
    void saveBinaryFile(std::string & fname)
    {
        std::lock_guard<std::mutex> lock(mtx);
        std::ofstream fout(fname);
        fout.write(reinterpret_cast<char *>(data), length);
    }
    void alloc(size_t size)
    {
        std::lock_guard<std::mutex> lock(mtx);
        length = size;
        if( nullptr != data )   { delete [] data;                   data = nullptr; }
        if( 0 < size )          { data = new unsigned char [size];                  }
    }
    size_t size(void) const
    {
        return length;
    }
    uint32_t write(mrb_int address, mrb_int size, std::string & in_data)
    {
        uint32_t cnt = 0;
        std::lock_guard<std::mutex> lock(mtx);
        for(auto pos = 0; ((cnt < size) && (address < length)); cnt ++, address ++, pos += 2)
        {
            auto temp = in_data.substr(pos, 2);
            auto val  = stoi(temp, 0, 16);
            data[address] = static_cast<unsigned char>(val);
        }
        return cnt;
    }
    uint32_t dump(mrb_int address, mrb_int size, std::string & output)
    {
        uint32_t idx = 0;
        std::lock_guard<std::mutex> lock(mtx);
        for( ; (idx < size) && (address < length); idx ++, address ++)
        {
            char temp[4] = { 0 };
            sprintf(temp, "%02X", data[address]);
            output += temp;
        }
        return idx;
    }
    bool toItems(mrb_state * mrb, uint32_t address, std::string & format_, mrb_value & array)
    {
        auto state = ' ';
        std::string num;
        auto format = format_ + ' ';
        for(auto t: format)
        {
            if(('0' <= t) && (t <= '9'))
            {
                num += t;
            }
            else
            {
                switch(state)
                {
                case 'A':
                    {
                        size_t size = stoi(num);
                        char str[size + 1];
                        str[size] = '\0';
                        memcpy(str, &(data[address]), size);
                        mrb_ary_push(mrb, array, mrb_str_new_cstr(mrb, str));
                        address += size;
                    }
                    break;
                case 'h':
                    {
                        mrb_int addr = address;
                        mrb_int size = static_cast<unsigned int>(stoi(num));
                        std::string hex;
                        dump(addr, size, hex);
                        mrb_ary_push(mrb, array, mrb_str_new_cstr(mrb, hex.c_str()));
                        address += size;
                    }
                    break;
                default:
                    break;
                }
                num.clear();
                state = ' ';
                switch(t)
                {
                case 'b': {  Byte temp; temp.buff[0] = data[address]; mrb_ary_push(mrb, array, mrb_int_value(mrb, temp.data)); address += 1; } break;
                case 'c': {  Byte temp; temp.buff[0] = data[address]; mrb_ary_push(mrb, array, mrb_int_value(mrb, temp.val));  address += 1; } break;
                case 'w': {  Word temp; memcpy(&(temp.buff[0]), &(data[address]), 2); mrb_ary_push(mrb, array, mrb_int_value(mrb, temp.uint16)); address += 2; } break;
                case 's': {  Word temp; memcpy(&(temp.buff[0]), &(data[address]), 2); mrb_ary_push(mrb, array, mrb_int_value(mrb, temp.int16));  address += 2; } break;
                case 'd': { DWord temp; memcpy(&(temp.buff[0]), &(data[address]), 4); mrb_ary_push(mrb, array, mrb_int_value(mrb, temp.uint32));  address += 4; } break;
                case 'i': { DWord temp; memcpy(&(temp.buff[0]), &(data[address]), 4); mrb_ary_push(mrb, array, mrb_int_value(mrb, temp.int32));   address += 4; } break;
                case 'f': { DWord temp; memcpy(&(temp.buff[0]), &(data[address]), 4); mrb_ary_push(mrb, array, mrb_float_value(mrb, temp.value)); address += 4; } break;
                case 'W': { Word temp;  for(auto idx = 0; idx < 2; idx ++) { temp.buff[idx] = data[address + (1-idx)]; } mrb_ary_push(mrb, array, mrb_int_value(mrb, temp.uint16));  address += 2; } break;
                case 'S': { Word temp;  for(auto idx = 0; idx < 2; idx ++) { temp.buff[idx] = data[address + (1-idx)]; } mrb_ary_push(mrb, array, mrb_int_value(mrb, temp.int16));   address += 2; } break;
                case 'D': { DWord temp; for(auto idx = 0; idx < 4; idx ++) { temp.buff[idx] = data[address + (3-idx)]; } mrb_ary_push(mrb, array, mrb_int_value(mrb, temp.uint32));  address += 4; } break;
                case 'I': { DWord temp; for(auto idx = 0; idx < 4; idx ++) { temp.buff[idx] = data[address + (3-idx)]; } mrb_ary_push(mrb, array, mrb_int_value(mrb, temp.int32));   address += 4; } break;
                case 'F': { DWord temp; for(auto idx = 0; idx < 4; idx ++) { temp.buff[idx] = data[address + (3-idx)]; } mrb_ary_push(mrb, array, mrb_float_value(mrb, temp.value)); address += 4; } break;
                case 'A': state = t; break;
                case 'H': state = 'h'; break;
                case 'h': state = t; break;
                default:
                    break;
                }
            }
        }
        return true;
    }
};

class Application
{
protected:
    static Application *                    obj;
    boost::program_options::variables_map & opts;
    std::vector<std::string> &              args;
    std::vector<size_t>                     timer;
    std::map<SerialMonitor *, std::string>  res_list;
    std::mutex                              mtx;
    std::condition_variable                 cond;
    std::mutex                              com_wait_mtx;
public:
    static Application * getObject(void);
    Application( boost::program_options::variables_map & optmap, std::vector<std::string> & arg)
      : opts(optmap), args(arg), timer(4)
    {
        obj = this;
        timer[0] =   30;
        timer[1] =  300;
        timer[2] =  500;
        timer[3] = 1000;
    }
    virtual ~Application(void)
    {
        std::list<SerialMonitor *> keys;
        for( auto & p : res_list )  keys.push_back(p.first);
        for( auto & ptr : keys )    delete ptr;
    }
    void exit(int code) { }
    mrb_value core_gets(mrb_state* mrb, mrb_value self)
    {
        try
        {
            std::string str("");
            std::getline(std::cin, str);
            return mrb_str_new_cstr( mrb, str.c_str() );
        }
        catch(std::exception & exp)
        {
            return mrb_nil_value();
        }
    }
    mrb_value opt_init(mrb_state * mrb, mrb_value self)
    {
        return self;
    }
    mrb_value opt_size(mrb_state * mrb, mrb_value self)
    {
        return mrb_int_value( mrb, args.size() );
    }
    mrb_value opt_get(mrb_state * mrb, mrb_value self)
    {
        mrb_value y = mrb_get_arg1(mrb);
        switch( mrb_type( y ) )
        {
            case MRB_TT_FLOAT:
            case MRB_TT_RATIONAL:
            case MRB_TT_COMPLEX:
                break;
            case MRB_TT_INTEGER:
                {
                    mrb_int index = mrb_integer(y);
                    if( index < args.size() )
                    {
                        return mrb_str_new_cstr( mrb, (args[index]).c_str() );
                    }
                }
                break;
            default:
                {
                    char * key;
                    mrb_get_args( mrb, "z", &key );
                    if( opts.count( key ) )
                    {
                        std::string opt_val = opts[key].as<std::string>();
                        return mrb_str_new_cstr( mrb, opt_val.c_str() );
                    }
                }
                break;
        }
        return mrb_nil_value();
    }
    mrb_value thread_init(mrb_state * mrb, mrb_value self)
    {
        static const struct mrb_data_type mrb_thread_context_type =
        {
            "mrb_cpp_thread_context", mrb_thread_context_free,
        };
        mrb_value       proc = mrb_nil_value();
        mrb_int         argc;
        mrb_value *     argv;
        mrb_get_args(mrb, "&*", &proc, &argv, &argc);
        WorkerThread * th_ctrl = new WorkerThread();
        mrb_data_init(self, th_ctrl, &mrb_thread_context_type);
        return self;
    }
    mrb_value thread_run(mrb_state * mrb, mrb_value self)
    {
        mrb_value ret  = mrb_nil_value();
        mrb_value proc = mrb_nil_value();
        mrb_get_args(mrb, "&", &proc);
        if (!mrb_nil_p(proc))
        {
            WorkerThread * th_ctrl = static_cast<WorkerThread *>(DATA_PTR(self));
            if(nullptr != th_ctrl )
            {
                th_ctrl->run(mrb, self);
            }
        }
        return ret;
    }
    mrb_value thread_join(mrb_state * mrb, mrb_value self)
    {
        mrb_value ret = mrb_nil_value();
        WorkerThread * th_ctrl = static_cast<WorkerThread *>(DATA_PTR(self));
        if(nullptr != th_ctrl)
        {
            th_ctrl->join();
        }
        return ret;
    }
    mrb_value thread_state(mrb_state * mrb, mrb_value self)
    {
        mrb_value ret = mrb_nil_value();
        WorkerThread * th_ctrl = static_cast<WorkerThread *>(DATA_PTR(self));
        if(nullptr != th_ctrl)
        {
            return mrb_fixnum_value(th_ctrl->get_state());
        }
        return mrb_nil_value();
    }
    mrb_value thread_wait(mrb_state * mrb, mrb_value self)
    {
        mrb_value proc = mrb_nil_value();
        mrb_get_args(mrb, "&", &proc);
        if (!mrb_nil_p(proc))
        {
            WorkerThread * th_ctrl = static_cast<WorkerThread *>(DATA_PTR(self));
            if(nullptr != th_ctrl)
            {
                th_ctrl->wait(mrb, proc);
            }
        }
        return mrb_nil_value();
    }
    mrb_value thread_notiry(mrb_state * mrb, mrb_value self)
    {
        mrb_value proc = mrb_nil_value();
        mrb_get_args(mrb, "&", &proc);
        if (!mrb_nil_p(proc))
        {
            WorkerThread * th_ctrl = static_cast<WorkerThread *>(DATA_PTR(self));
            if(nullptr != th_ctrl)
            {
                th_ctrl->notify(mrb, proc);
            }
        }
        return mrb_nil_value();
    }
    mrb_value thread_sync(mrb_state * mrb, mrb_value self)
    {
        mrb_value proc = mrb_nil_value();
        mrb_get_args(mrb, "&", &proc);
        if (!mrb_nil_p(proc))
        {
            WorkerThread * th_ctrl = static_cast<WorkerThread * >(DATA_PTR(self));
            if(nullptr != th_ctrl)
            {
                auto result = th_ctrl->sync(mrb, proc);
                return result;
            }
        }
        return mrb_nil_value();
    }

    mrb_value smon_init(mrb_state * mrb, mrb_value self)
    {
        static const struct mrb_data_type mrb_smon_context_type =
        {
            "mrb_smon_context", mrb_smon_context_free,
        };
        char * arg;
        mrb_get_args(mrb, "z", &arg);
        std::string boud("1200O1");
        bool rts_ctrl = true;
        if(opts.count("baud"))
        {
            boud = opts["baud"].as<std::string>();
        }
        if(opts.count("no-rts"))
        {
            rts_ctrl = false;
        }
        SerialMonitor * smon = new SerialMonitor(self, arg, timer, res_list, boud, rts_ctrl);
        mrb_data_init(self, smon, &mrb_smon_context_type);
        return self;
    }
    mrb_value smon_wait(mrb_state * mrb, mrb_value self)
    {
        mrb_value proc = mrb_nil_value();
        mrb_get_args(mrb, "&", &proc);
        if (!mrb_nil_p(proc))
        {
            SerialMonitor * smon = static_cast<SerialMonitor *>(DATA_PTR(self));
            if(nullptr != smon)
            {
                std::string data;
                auto state = smon->recive_wait(data);
                mrb_value argv[2];
                argv[0] = mrb_fixnum_value(state);
                switch(state)
                {
                case SerialMonitor::CACHE_FULL:
                case SerialMonitor::GAP:
                    argv[1] = mrb_str_new_cstr(mrb, data.c_str());
                    break;
#if 0
                case SerialMonitor::TIME_OUT_1:
                case SerialMonitor::TIME_OUT_2:
                case SerialMonitor::TIME_OUT_3:
                case SerialMonitor::CLOSE:
                case SerialMonitor::NONE:
#endif
                default:
                    argv[1] = mrb_str_new_cstr(mrb, "");
                     break;
                }
                std::lock_guard<std::mutex> lock(com_wait_mtx);
                auto ret = mrb_yield_argv(mrb, proc, 2, &(argv[0]));
                return ret;
            }
        }
        return mrb_nil_value();
    }
    mrb_value smon_send(mrb_state * mrb, mrb_value self)
    {
        char *      msg;
        mrb_int     timer;
        mrb_get_args(mrb, "zi", &msg, &timer);

        SerialMonitor * smon = static_cast<SerialMonitor *>(DATA_PTR(self));
        if(nullptr != smon)
        {
            smon->send(msg, timer);
        }
        return self;
    }
    mrb_value smon_close(mrb_state * mrb, mrb_value self)
    {
        SerialMonitor * smon = static_cast<SerialMonitor *>(DATA_PTR(self));
        if(nullptr != smon)
        {
            smon->close();
        }
        return self;
    }
    mrb_value xlsx_init(mrb_state * mrb, mrb_value self)
    {
        static const struct mrb_data_type mrb_xlsx_context_type =
        {
            "mrb_open_xlsx_context", mrb_xlsx_context_free,
        };
        OpenXLSXCtrl * xlsx = new OpenXLSXCtrl();
        mrb_data_init(self, xlsx, &mrb_xlsx_context_type);
        return self;
    }
    mrb_value xlsx_create(mrb_state * mrb, mrb_value self)
    {
        mrb_int argc;
        mrb_value * argv;
        mrb_value proc = mrb_nil_value();
        mrb_get_args(mrb, "&*", &proc, &argv, &argc);
        if((1 == argc) && (!mrb_nil_p(proc)))
        {
            OpenXLSXCtrl * xlsx = static_cast<OpenXLSXCtrl *>(DATA_PTR(self));
            if(nullptr != xlsx)
            {
                struct RString * s = mrb_str_ptr(argv[0]);
                xlsx->create(RSTR_PTR(s));
                xlsx->workbook();
                mrb_value ret = mrb_yield_argv(mrb, proc, 0, nullptr);
                xlsx->save();
                xlsx->close();
                return ret;
            }
        }
        return mrb_nil_value();
    }
    mrb_value xlsx_open(mrb_state * mrb, mrb_value self)
    {
        mrb_int argc;
        mrb_value * argv;
        mrb_value proc = mrb_nil_value();
        mrb_get_args(mrb, "&*", &proc, &argv, &argc);
        if((1 == argc) && (!mrb_nil_p(proc)))
        {
            OpenXLSXCtrl * xlsx = static_cast<OpenXLSXCtrl *>(DATA_PTR(self));
            if(nullptr != xlsx)
            {
                struct RString * s = mrb_str_ptr(argv[0]);
                xlsx->open(RSTR_PTR(s));
                xlsx->workbook();
                mrb_value ret = mrb_yield_argv(mrb, proc, 0, nullptr);
                xlsx->close();
                return ret;
            }
        }
        return mrb_nil_value();
    }
    mrb_value xlsx_worksheet(mrb_state * mrb, mrb_value self)
    {
        mrb_int argc;
        mrb_value * argv;
        mrb_value proc = mrb_nil_value();
        mrb_get_args(mrb, "&*", &proc, &argv, &argc);
        if((1 == argc) && (!mrb_nil_p(proc)))
        {
            OpenXLSXCtrl * xlsx = static_cast<OpenXLSXCtrl *>(DATA_PTR(self));
            if(nullptr != xlsx)
            {
                struct RString * sheet_name = mrb_str_ptr(argv[0]);
                xlsx->worksheet(RSTR_PTR(sheet_name ));
                mrb_value ret = mrb_yield_argv(mrb, proc, 0, nullptr);
                return ret;
            }
        }
        return mrb_nil_value();
    }
    mrb_value xlsx_work_sheet_names(mrb_state * mrb, mrb_value self)
    {
        mrb_int     argc;
        mrb_value * argv;
        mrb_value   proc = mrb_nil_value();
        mrb_get_args(mrb, "&*", &proc, &argv, &argc);
        if((0 == argc) && (!mrb_nil_p(proc)))
        {
            OpenXLSXCtrl * xlsx = static_cast<OpenXLSXCtrl *>(DATA_PTR(self));
            if(nullptr != xlsx)
            {
                auto list = xlsx->getWorkSheetNames();
                for( auto & name: list)
                {
                    mrb_value argv[1];
                    argv[0] = mrb_str_new_cstr(mrb, name.c_str());
                    auto ret = mrb_yield_argv(mrb, proc, 1, argv);
                    if(mrb_type(ret) == MRB_TT_FALSE) { return ret; }
                }
            }
        }
        return mrb_nil_value();
    }
    mrb_value xlsx_set_sheet_name(mrb_state * mrb, mrb_value self)
    {
        char * sheet_name;
        mrb_get_args(mrb, "z", &sheet_name);
        OpenXLSXCtrl * xlsx = static_cast<OpenXLSXCtrl *>(DATA_PTR(self));
        if(nullptr != xlsx )
        {
            xlsx->set_sheet_name(sheet_name);
        }
        return mrb_nil_value();
    }
    mrb_value xlsx_set_value(mrb_state * mrb, mrb_value self)
    {
        mrb_int argc;
        mrb_value * argv;
        mrb_get_args(mrb, "*", &argv, &argc);
        if(2 == argc)
        {
            OpenXLSXCtrl * xlsx = static_cast<OpenXLSXCtrl *>(DATA_PTR(self));
            if(nullptr != xlsx)
            {
                struct RString * cell_name = mrb_str_ptr(argv[0]);
                char * cel = RSTR_PTR(cell_name);
                switch( mrb_type( argv[1] ) )
                {
                case MRB_TT_INTEGER:
                    {
                        int value = static_cast<int>(mrb_integer(argv[1]));
                        xlsx->set_cell_value(cel, value);
                    }
                    break;
                case MRB_TT_FLOAT:
                    {
                        float value = static_cast<float>(mrb_float(argv[1]));
                        xlsx->set_cell_value(cel, value);
                    }
                    break;
                case MRB_TT_STRING:
                    {
                        struct RString * str_value = mrb_str_ptr(argv[1]);
                        char * value = RSTR_PTR(str_value);
                        xlsx->set_cell_value(cel, value);
                    }
                    break;
                case MRB_TT_TRUE:   xlsx->set_cell_value(cel, 1); break;
                case MRB_TT_FALSE:  xlsx->set_cell_value(cel, 0); break;
                default:
                    break;
                }
            }
        }
        return mrb_nil_value();
    }
    mrb_value xlsx_cell(mrb_state * mrb, mrb_value self)
    {
        char * cell_name;
        mrb_get_args(mrb, "z", &cell_name);
        OpenXLSXCtrl * xlsx = static_cast<OpenXLSXCtrl *>(DATA_PTR(self));
        if(nullptr != xlsx )
        {
            int         val_int;
            float       val_float;
            std::string str;
            auto type = xlsx->get_cell(cell_name, val_int, val_float, str);
            switch(type)
            {
            case OpenXLSXCtrl::Boolean:
                return mrb_bool_value(val_int != 0);
            case OpenXLSXCtrl::Integer:
                return mrb_int_value( mrb, val_int );
            case OpenXLSXCtrl::Float:
                return mrb_float_value( mrb, val_float );
            case OpenXLSXCtrl::String:
                return mrb_str_new_cstr( mrb, str.c_str() );
            case OpenXLSXCtrl::Empty:
            case OpenXLSXCtrl::Error:
            default:
                break;
            }
        }
        return mrb_nil_value();
    }
    mrb_value bedit_init(mrb_state * mrb, mrb_value self)
    {
        static const struct mrb_data_type mrb_bedit_context_type =
        {
            "mrb_open_bedit_context", mrb_bedit_context_free,
        };
        BinaryControl * bedit = new BinaryControl();
        mrb_int argc;
        mrb_value * argv;
        mrb_get_args(mrb, "*", &argv, &argc);
        if(1 == argc)
        {
            switch( mrb_type( argv[0] ) )
            {
            case MRB_TT_INTEGER:
                {
                    int size = static_cast<int>(mrb_integer(argv[0]));
                    bedit->alloc(size);
                }
                break;
            case MRB_TT_STRING:
                {
                    struct RString * str = mrb_str_ptr(argv[0]);
                    std::string data(RSTR_PTR(str));
                    for(auto pos = data.find_first_of(" "); pos != std::string::npos; pos = data.find_first_of(" "))
                    {
                        data.erase(pos, 1);
                    }
                    mrb_int size = (data.size() / 2);
                    bedit->alloc(size);

                    if(0 == bedit->write(0, size, data)) { bedit->alloc(0); }
                }
                break;
            default:
                break;
            }
        }
        mrb_data_init(self, bedit, &mrb_bedit_context_type);
        return self;
    }
    mrb_value bedit_length(mrb_state * mrb, mrb_value self)
    {
        BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
        if(nullptr != bedit)
        {
            return mrb_int_value(mrb, bedit->size());
        }
        return mrb_nil_value();
    }
    mrb_value bedit_load(mrb_state * mrb, mrb_value self)
    {
        BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
        if(nullptr != bedit)
        {
            char * fname_ptr;
            mrb_get_args(mrb, "z", &fname_ptr);
            std::string fname(fname_ptr);
            auto sz = bedit->loadBinaryFile(fname);
            return mrb_int_value(mrb, sz);
        }
        return mrb_int_value(mrb, 0);
    }
    mrb_value bedit_save(mrb_state * mrb, mrb_value self)
    {
        BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
        if(nullptr != bedit)
        {
            char * fname_ptr;
            mrb_get_args(mrb, "z", &fname_ptr);
            std::string fname(fname_ptr);
            bedit->saveBinaryFile(fname);
        }
        return self;
    }
    mrb_value bedit_write(mrb_state * mrb, mrb_value self)
    {
        BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
        if(nullptr != bedit)
        {
            mrb_int address = 0;
            mrb_int size    = 0;
            char * data_ptr = nullptr;
            mrb_int argc;
            mrb_value * argv;
            mrb_get_args(mrb, "*", &argv, &argc);
            switch(argc)
            {
            case 1:
                if(MRB_TT_STRING == mrb_type(argv[0]))
                {
                    struct RString * str = mrb_str_ptr(argv[0]);
                    data_ptr = RSTR_PTR(str);
                }
                break;
            case 2:
                if(  (MRB_TT_INTEGER == mrb_type(argv[0]))
                   &&(MRB_TT_STRING  == mrb_type(argv[1])))
                {
                    address = mrb_integer(argv[0]);
                    struct RString * str = mrb_str_ptr(argv[1]);
                    data_ptr = RSTR_PTR(str);
                }
                break;
            case 3:
                if(  (MRB_TT_INTEGER == mrb_type(argv[0]))
                   &&(MRB_TT_INTEGER == mrb_type(argv[1]))
                   &&(MRB_TT_STRING  == mrb_type(argv[2])))
                {
                    address = mrb_integer(argv[0]);
                    size    = mrb_integer(argv[1]);
                    struct RString * str = mrb_str_ptr(argv[2]);
                    data_ptr = RSTR_PTR(str);
                }
                break;
            default:
                break;
            }
            if(nullptr != data_ptr)
            {
                std::string data(data_ptr);
                for(auto pos = data.find_first_of(" "); pos != std::string::npos; pos = data.find_first_of(" "))
                {
                    data.erase(pos, 1);
                }
                if(0 == size) { size = data.size() / 2; }
                if(0 < size)
                {
                    auto wsize = bedit->write(address, size, data);
                    return mrb_int_value(mrb, wsize);
                }
            }
        }
        return mrb_int_value(mrb, 0);
    }
    mrb_value bedit_dump(mrb_state * mrb, mrb_value self)
    {
        BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
        if(nullptr != bedit)
        {
            mrb_int address = 0;
            mrb_int size    = bedit->size();
            mrb_int argc;
            mrb_value * argv;
            mrb_get_args(mrb, "*", &argv, &argc);
            switch(argc)
            {
            case 1:
                mrb_get_args(mrb, "i", &size);
                break;
            case 2:
                mrb_get_args(mrb, "ii", &address, &size);
                break;
            default:
                break;
            }
            if(0 < size)
            {
                std::string output;
                if( 0 < bedit->dump(address, size, output) )
                {
                    return mrb_str_new_cstr(mrb, output.c_str());
                }
            }
        }
        return mrb_nil_value();
    }
    mrb_value bedit_toItems(mrb_state * mrb, mrb_value self)
    {
        BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
        if(nullptr != bedit)
        {
            mrb_int address = 0;
            char *  fmt_ptr = nullptr;
            mrb_int argc;
            mrb_value * argv;
            mrb_get_args(mrb, "*", &argv, &argc);
            switch(argc)
            {
            case 1:
                mrb_get_args(mrb, "z", &fmt_ptr);
                break;
            case 2:
                mrb_get_args(mrb, "iz", &address, &fmt_ptr);
                break;
            default:
                break;
            }
            if(nullptr != fmt_ptr)
            {
                std::string fmt(fmt_ptr);
                mrb_value arry = mrb_ary_new(mrb);
                if( bedit->toItems(mrb, address, fmt, arry) )
                {
                    return arry;
                }
            }
        }
        return mrb_nil_value();
    }

    void main(void)
    {
        if(opts.count("gap"))
        {
            unsigned int val = opts["gap"].as<unsigned int>();
            timer[0] = val;
        }
        if(opts.count("timer"))
        {
            unsigned int val = opts["timer"].as<unsigned int>();
            timer[1] = val;
        }
        if(opts.count("timer2"))
        {
            unsigned int val = opts["timer2"].as<unsigned int>();
            timer[2] = val;
        }
        if(opts.count("timer3"))
        {
            unsigned int val = opts["timer3"].as<unsigned int>();
            timer[3] = val;
        }
        mrb_state * mrb = mrb_open();
        if( nullptr != mrb )
        {
            /* Class Core */
            struct RClass * calc_class = mrb_define_class_under( mrb, mrb->kernel_module, "Core", mrb->object_class );
            mrb_define_module_function(mrb, calc_class, "crc16",        mrb_core_crc16,         MRB_ARGS_ARG( 1, 1 )    );
            mrb_define_module_function(mrb, calc_class, "crc8",         mrb_core_crc8,          MRB_ARGS_ARG( 1, 1 )    );
            mrb_define_module_function(mrb, calc_class, "sum",          mrb_core_sum,           MRB_ARGS_ARG( 1, 1 )    );
            mrb_define_module_function(mrb, calc_class, "float",        mrb_core_float,         MRB_ARGS_ARG( 1, 1 )    );
            mrb_define_module_function(mrb, calc_class, "float_l",      mrb_core_float_l,       MRB_ARGS_ARG( 1, 1 )    );
            mrb_define_module_function(mrb, calc_class, "to_hex",       mrb_core_to_hex,        MRB_ARGS_ARG( 2, 1 )    );
            mrb_define_module_function(mrb, calc_class, "reg_match",    mrb_core_reg_match,     MRB_ARGS_ARG( 2, 1 )    );
            mrb_define_module_function(mrb, calc_class, "reg_replace",  mrb_core_reg_replace,   MRB_ARGS_ARG( 3, 1 )    );
            mrb_define_module_function(mrb, calc_class, "gets",         mrb_core_gets,          MRB_ARGS_ANY()          );
            mrb_define_module_function(mrb, calc_class, "exists",       mrb_core_exists,        MRB_ARGS_ARG( 1, 1 )    );
            mrb_define_module_function(mrb, calc_class, "timestamp",    mrb_core_file_timestamp,MRB_ARGS_ARG( 1, 1 )    );
            mrb_define_module_function(mrb, calc_class, "comlist",      mrb_core_comlist,       MRB_ARGS_ANY()          );

            /* Class options */
            struct RClass * opt_class = mrb_define_class_under( mrb, mrb->kernel_module, "Args", mrb->object_class );
            mrb_define_method( mrb, opt_class, "initialize",            mrb_opt_initialize,     MRB_ARGS_ANY()          );
            mrb_define_method( mrb, opt_class, "size",                  mrb_opt_size,           MRB_ARGS_NONE()         );
            mrb_define_method( mrb, opt_class, "[]",                    mrb_opt_get,            MRB_ARGS_ARG( 1, 1 )    );

            /* Class Thread */
            struct RClass * thread_class = mrb_define_class_under( mrb, mrb->kernel_module, "WorkerThread", mrb->object_class );
            mrb_define_const(  mrb, thread_class, "STOP",               mrb_fixnum_value(WorkerThread::Stop)            );
            mrb_define_const(  mrb, thread_class, "WAKEUP",             mrb_fixnum_value(WorkerThread::Wakeup)          );
            mrb_define_const(  mrb, thread_class, "RUN",                mrb_fixnum_value(WorkerThread::Run)             );
            mrb_define_const(  mrb, thread_class, "WAIT_JOIN",          mrb_fixnum_value(WorkerThread::WaitJoin)        );
            mrb_define_module_function( mrb, thread_class, "ms_sleep",  mrb_thread_ms_sleep,    MRB_ARGS_ARG( 1, 1 )    );
            mrb_define_method( mrb, thread_class, "initialize",         mrb_thread_initialize,  MRB_ARGS_ANY()          );
            mrb_define_method( mrb, thread_class, "run",                mrb_thread_run,         MRB_ARGS_ANY()          );
            mrb_define_method( mrb, thread_class, "join",               mrb_thread_join,        MRB_ARGS_ANY()          );
            mrb_define_method( mrb, thread_class, "state",              mrb_thread_state,       MRB_ARGS_ANY()          );
            mrb_define_method( mrb, thread_class, "wait",               mrb_thread_wait,        MRB_ARGS_NONE()         );
            mrb_define_method( mrb, thread_class, "synchronize",        mrb_thread_sync,        MRB_ARGS_NONE()         );
            mrb_define_method( mrb, thread_class, "notify",             mrb_thread_notify,      MRB_ARGS_NONE()         );

            /* Class Smon */
            struct RClass * smon_class = mrb_define_class_under( mrb, mrb->kernel_module, "Smon", mrb->object_class );
            mrb_define_const(  mrb, smon_class, "GAP",                  mrb_fixnum_value(SerialMonitor::GAP)            );
            mrb_define_const(  mrb, smon_class, "TO1",                  mrb_fixnum_value(SerialMonitor::TIME_OUT_1)     );
            mrb_define_const(  mrb, smon_class, "TO2",                  mrb_fixnum_value(SerialMonitor::TIME_OUT_2)     );
            mrb_define_const(  mrb, smon_class, "TO3",                  mrb_fixnum_value(SerialMonitor::TIME_OUT_3)     );
            mrb_define_const(  mrb, smon_class, "CLOSE",                mrb_fixnum_value(SerialMonitor::CLOSE)          );
            mrb_define_const(  mrb, smon_class, "CACHE_FULL",           mrb_fixnum_value(SerialMonitor::CACHE_FULL)     );
            mrb_define_const(  mrb, smon_class, "NONE",                 mrb_fixnum_value(SerialMonitor::NONE)           );
            mrb_define_method( mrb, smon_class, "initialize",           mrb_smon_initialize,    MRB_ARGS_REQ( 2 )       );
            mrb_define_method( mrb, smon_class, "wait",                 mrb_smon_wait,          MRB_ARGS_ARG( 2, 1 )    );
            mrb_define_method( mrb, smon_class, "send",                 mrb_smon_send,          MRB_ARGS_ARG( 2, 1 )    );
            mrb_define_method( mrb, smon_class, "close",                mrb_smon_close,         MRB_ARGS_NONE()         );

            /* Class OpenXLSX */
            struct RClass * xlsx_class = mrb_define_class_under( mrb, mrb->kernel_module, "OpenXLSX", mrb->object_class );
            mrb_define_method( mrb, xlsx_class, "initialize",       mrb_xlsx_initialize,        MRB_ARGS_REQ( 2 )       );
            mrb_define_method( mrb, xlsx_class, "create",           mrb_xlsx_create,            MRB_ARGS_ARG( 1, 1 )    );
            mrb_define_method( mrb, xlsx_class, "open",             mrb_xlsx_open,              MRB_ARGS_ARG( 1, 1 )    );
            mrb_define_method( mrb, xlsx_class, "sheet",            mrb_xlsx_worksheet,         MRB_ARGS_ARG( 1, 1 )    );
            mrb_define_method( mrb, xlsx_class, "sheet_names",      mrb_xlsx_sheet_names,       MRB_ARGS_NONE()         );
            mrb_define_method( mrb, xlsx_class, "setSheetName",     mrb_xlsx_set_seet_name,     MRB_ARGS_ARG( 1, 1 )    );
            mrb_define_method( mrb, xlsx_class, "set_value",        mrb_xlsx_set_value,         MRB_ARGS_ARG( 2, 1 )    );
            mrb_define_method( mrb, xlsx_class, "cell",             mrb_xlsx_cell,              MRB_ARGS_ARG( 1, 1 )    );

            /* Class BinEdit */
            struct RClass * bedit_class = mrb_define_class_under( mrb, mrb->kernel_module, "BinEdit", mrb->object_class );
            mrb_define_method( mrb, bedit_class, "initialize",      mrb_bedit_initialize,       MRB_ARGS_ANY()          );
            mrb_define_method( mrb, bedit_class, "length",          mrb_bedit_length,           MRB_ARGS_ARG( 1, 1 )    );
            mrb_define_method( mrb, bedit_class, "load",            mrb_bedit_load,             MRB_ARGS_ARG( 1, 1 )    );
            mrb_define_method( mrb, bedit_class, "save",            mrb_bedit_save,             MRB_ARGS_ARG( 1, 1 )    );
            mrb_define_method( mrb, bedit_class, "write",           mrb_bedit_write,            MRB_ARGS_ARG( 2, 1 )    );
            mrb_define_method( mrb, bedit_class, "dump",            mrb_bedit_dump,             MRB_ARGS_ARG( 2, 1 )    );
            mrb_define_method( mrb, bedit_class, "toItems",         mrb_bedit_toItems,          MRB_ARGS_ARG( 2, 1 )    );

            /* exec mRuby Script */
            extern const uint8_t default_options[];
            mrb_load_irep(mrb, default_options);
            if(opts.count("mruby-script"))
            {
                std::string mruby_fnames = opts["mruby-script"].as<std::string>();
                for( auto&& fname : split( mruby_fnames, "," ) )
                {
                    std::ifstream fin(fname);
                    if(fin.is_open())
                    {
                        std::string script((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());
                        mrb_load_string(mrb, script.c_str());
                    }
                    else
                    {
                        printf("file open error: %s\n", fname.c_str());
                        break;
                    }
                }
            }
            else
            {
                extern const uint8_t default_script[];
                mrb_load_irep(mrb, default_script);
            }
            mrb_close(mrb);
        }
    }
};
Application * Application::obj = nullptr;
Application * Application::getObject(void)
{
    return Application::obj;
}

mrb_value mrb_core_gets(mrb_state* mrb, mrb_value self)             { Application * app = Application::getObject(); return app->core_gets(mrb, self);           }

mrb_value mrb_opt_initialize(mrb_state * mrb, mrb_value self)       { Application * app = Application::getObject(); return app->opt_init(mrb, self);            }
mrb_value mrb_opt_size(mrb_state * mrb, mrb_value self)             { Application * app = Application::getObject(); return app->opt_size(mrb, self);            }
mrb_value mrb_opt_get(mrb_state * mrb, mrb_value self)              { Application * app = Application::getObject(); return app->opt_get(mrb, self);             }

mrb_value mrb_thread_initialize(mrb_state * mrb, mrb_value self)    { Application * app = Application::getObject(); return app->thread_init(mrb, self);         }
mrb_value mrb_thread_run(mrb_state * mrb, mrb_value self)           { Application * app = Application::getObject(); return app->thread_run(mrb, self);          }
mrb_value mrb_thread_join(mrb_state * mrb, mrb_value self)          { Application * app = Application::getObject(); return app->thread_join(mrb, self);         }
mrb_value mrb_thread_state(mrb_state * mrb, mrb_value self)         { Application * app = Application::getObject(); return app->thread_state(mrb, self);        }
mrb_value mrb_thread_wait(mrb_state * mrb, mrb_value self)          { Application * app = Application::getObject(); return app->thread_wait(mrb, self);         }
mrb_value mrb_thread_notify(mrb_state * mrb, mrb_value self)        { Application * app = Application::getObject(); return app->thread_notiry(mrb, self);       }
mrb_value mrb_thread_sync(mrb_state * mrb, mrb_value self)          { Application * app = Application::getObject(); return app->thread_sync(mrb, self);         }

mrb_value mrb_smon_initialize(mrb_state * mrb, mrb_value self)      { Application * app = Application::getObject(); return app->smon_init(mrb, self);           }
mrb_value mrb_smon_wait(mrb_state * mrb, mrb_value self)            { Application * app = Application::getObject(); return app->smon_wait(mrb, self);           }
mrb_value mrb_smon_send(mrb_state * mrb, mrb_value self)            { Application * app = Application::getObject(); return app->smon_send(mrb, self);           }
mrb_value mrb_smon_close(mrb_state * mrb, mrb_value self)           { Application * app = Application::getObject(); return app->smon_close(mrb, self);          }

mrb_value mrb_xlsx_initialize(mrb_state * mrb, mrb_value self)      { Application * app = Application::getObject(); return app->xlsx_init(mrb, self);           }
mrb_value mrb_xlsx_create(mrb_state * mrb, mrb_value self)          { Application * app = Application::getObject(); return app->xlsx_create(mrb, self);         }
mrb_value mrb_xlsx_open(mrb_state * mrb, mrb_value self)            { Application * app = Application::getObject(); return app->xlsx_open(mrb, self);           }
mrb_value mrb_xlsx_worksheet(mrb_state * mrb, mrb_value self)       { Application * app = Application::getObject(); return app->xlsx_worksheet(mrb, self);      }
mrb_value mrb_xlsx_sheet_names(mrb_state * mrb, mrb_value self)     { Application * app = Application::getObject(); return app->xlsx_work_sheet_names(mrb, self); }
mrb_value mrb_xlsx_set_seet_name(mrb_state * mrb, mrb_value self)   { Application * app = Application::getObject(); return app->xlsx_set_sheet_name(mrb, self); }
mrb_value mrb_xlsx_set_value(mrb_state * mrb, mrb_value self)       { Application * app = Application::getObject(); return app->xlsx_set_value(mrb, self);      }
mrb_value mrb_xlsx_cell(mrb_state * mrb, mrb_value self)            { Application * app = Application::getObject(); return app->xlsx_cell(mrb, self);           }

mrb_value mrb_bedit_initialize(mrb_state * mrb, mrb_value self)     { Application * app = Application::getObject(); return app->bedit_init(mrb, self);          }
mrb_value mrb_bedit_length(mrb_state * mrb, mrb_value self)         { Application * app = Application::getObject(); return app->bedit_length(mrb, self);        }
mrb_value mrb_bedit_load(mrb_state * mrb, mrb_value self)           { Application * app = Application::getObject(); return app->bedit_load(mrb, self);          }
mrb_value mrb_bedit_save(mrb_state * mrb, mrb_value self)           { Application * app = Application::getObject(); return app->bedit_save(mrb, self);          }
mrb_value mrb_bedit_write(mrb_state * mrb, mrb_value self)          { Application * app = Application::getObject(); return app->bedit_write(mrb, self);         }
mrb_value mrb_bedit_dump(mrb_state * mrb, mrb_value self)           { Application * app = Application::getObject(); return app->bedit_dump(mrb, self);          }
mrb_value mrb_bedit_toItems(mrb_state * mrb, mrb_value self)        { Application * app = Application::getObject(); return app->bedit_toItems(mrb, self);       }


void mrb_thread_context_free(mrb_state * mrb, void * ptr)
{
//    printf("%s:%d: %s\n", __FILE__, __LINE__, __FUNCTION__);
    if(nullptr != ptr)
    {
        WorkerThread * th_ctrl = static_cast<WorkerThread *>(ptr);
        delete th_ctrl;
    }
}

void mrb_smon_context_free(mrb_state * mrb, void * ptr)
{
//    printf("%s:%d: %s\n", __FILE__, __LINE__, __FUNCTION__);
    if(nullptr != ptr)
    {
        SerialMonitor * smon = static_cast<SerialMonitor *>(ptr);
        delete smon;
    }
}
void mrb_xlsx_context_free(mrb_state * mrb, void * ptr)
{
//    printf("%s:%d: %s\n", __FILE__, __LINE__, __FUNCTION__);
    if(nullptr != ptr)
    {
        OpenXLSXCtrl * xlsx = static_cast<OpenXLSXCtrl *>(ptr);
        delete xlsx;
    }
}
void mrb_bedit_context_free(mrb_state * mrb, void * ptr)
{
//    printf("%s:%d: %s\n", __FILE__, __LINE__, __FUNCTION__);
    if(nullptr != ptr)
    {
        BinaryControl * bedit = static_cast<BinaryControl *>(ptr);
        delete bedit;
    }
}

int main(int argc, char * argv[])
{
    try
    {
        boost::program_options::options_description desc("smon.exe [Options]");
        desc.add_options()
            ("baud,b",          boost::program_options::value<std::string>(),   "baud rate      Default 1200O1 ex) -b 9600E1")
            ("gap,g",           boost::program_options::value<unsigned int>(),  "time out tick. Default   30 ( 30 [ms])")
            ("timer,t",         boost::program_options::value<unsigned int>(),  "time out tick. Default  300 (300 [ms])")
            ("timer2",          boost::program_options::value<unsigned int>(),  "time out tick. Default  500 (500 [ms])")
            ("timer3",          boost::program_options::value<unsigned int>(),  "time out tick. Default 1000 (  1 [s])")
            ("no-rts",                                                          "no control RTS signal")
            ("crc,c",          boost::program_options::value<std::string>(),    "calclate modbus RTU CRC")
            ("crc8",           boost::program_options::value<std::string>(),    "calclate CRC8")
            ("sum,s",          boost::program_options::value<std::string>(),    "calclate checksum of XOR")
            ("float,f",        boost::program_options::value<std::string>(),    "hex to float value")
            ("floatl,F",       boost::program_options::value<std::string>(),    "litle endian hex to float value")
            ("mruby-script,m", boost::program_options::value<std::string>(),    "execute mruby script")
            ("help,h",                                                          "help");
        boost::program_options::variables_map argmap;

        auto const parsing_result = parse_command_line( argc, argv, desc );
        store( parsing_result, argmap );
        notify( argmap );

        if(argmap.count("help"))
        {
            std::cout << "smon Software revision 0.9.0c" << std::endl;
            std::cout << std::endl;
            std::cout << desc << std::endl;
            return 0;
        }

        std::vector<std::string> arg;
        for(auto const& str : collect_unrecognized(parsing_result.options, boost::program_options::include_positional))
        {
            arg.push_back(str);
        }
        Application app( argmap, arg );
        app.main();
    }
    catch(std::exception & exp) { std::cerr << "exeption: " << exp.what() << std::endl; }
    catch(...)                  { std::cerr << "unknown exeption" << std::endl;         }
    return 0;
}
