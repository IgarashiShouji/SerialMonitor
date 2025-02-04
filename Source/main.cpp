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
#include "PipeList.hpp"
#include "lz4.h"
#include "qrcodegen.hpp"

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <sstream>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
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
#include <mruby/array.h>
#include <mruby/class.h>
#include <mruby/compile.h>
#include <mruby/data.h>
#include <mruby/data.h>
#include <mruby/dump.h>
#include <mruby/hash.h>
#include <mruby/proc.h>
#include <mruby/range.h>
#include <mruby/string.h>
#include <mruby/value.h>
#include <mruby/variable.h>

#include <OpenXLSX.hpp>


/* -- static const & functions -- */
class Object
{
protected:
    unsigned int id;
    std::map<unsigned int, Object *> * res;
public:
    inline Object(void);
    virtual ~Object(void);
    inline void setControlID(unsigned int id, std::map<unsigned int, Object *> * res);
    inline unsigned int ControlID(void);
};

class BinaryControl : public Object
{
protected:
    size_t length;
    size_t compress_size;
    size_t pos;
    unsigned char * data;

public:
    inline BinaryControl(void);
    inline BinaryControl(BinaryControl & src);
    inline BinaryControl(size_t size_);
    inline BinaryControl(size_t size_, unsigned char data);
    BinaryControl(std::string & data);
    BinaryControl(std::list<BinaryControl *> & list_bin);
    virtual ~BinaryControl(void);
    inline unsigned char * ptr(void);
    inline size_t size(void) const;
    inline size_t get_pos(void) const;
    inline size_t ref_pos(void) const;
    void chg_compress(void);
    void alloc(size_t size);
    size_t resize(size_t size);
    void clone(mrb_int address, mrb_int size, const BinaryControl & src);
    inline void attach(unsigned char * ptr, size_t size);
    size_t loadBinaryFile(std::string & fname);
    void saveBinaryFile(std::string & fname);
    uint32_t compress(void);
    uint32_t uncompress(void);
    uint32_t memset(mrb_int address, mrb_int set_data, mrb_int sz);
    uint32_t memcpy(mrb_int address_dst, mrb_int address_src, mrb_int len, BinaryControl & src);
    int32_t memcmp(mrb_int address_dst, mrb_int address_src, mrb_int len, BinaryControl & src);
    uint32_t write(mrb_int address, mrb_int size, std::string & data);
    uint32_t write_big(mrb_int address, mrb_int size, std::string & src);
    uint32_t dump(mrb_int address, mrb_int size, std::string & output);
    uint32_t dump_big(mrb_int address, mrb_int size, std::string & output);
    bool get(mrb_state * mrb, uint32_t address, std::string & format_, mrb_value & array);
    mrb_int set(mrb_state * mrb, uint32_t address, std::string & format_, mrb_value & array);
    unsigned short modbus_crc16(void);
    unsigned char crc8(void);
    uint32_t crc32(void);
    unsigned char sum(void);
    unsigned char xsum(void);
};

class CppRegexp: public Object
{
private:
    std::vector<std::regex> regs;
public:
    inline CppRegexp(void);
    inline CppRegexp(std::string & str);
    inline CppRegexp(const char * str_);
    inline CppRegexp(const std::list<std::string> & arg);
    virtual ~CppRegexp(void);
    inline void reg_add(const std::string & str);
    inline void reg_add(const std::list<std::string> & arg);
    inline unsigned int length(void);
    bool match(const std::string & str);
    std::list<std::string> match(std::list<std::string> & text);
    std::list<std::string> grep(std::list<std::string> & text);
    void replace(std::list<std::string> & text, const char * rep);
    unsigned int select(const char * str);
    std::list<std::string> split(std::string & org);
};

class WorkerThread : public Object
{
public:
    enum Status { Stop, Wakeup, Run, Wait, WaitStop };
protected:
    enum Status             state;
    mrb_state *             mrb;
    size_t                  loop_cnt;
    mrb_value               proc;
    std::thread             th_ctrl;
    std::mutex              mtx;
    std::condition_variable cond;
protected:
    struct FifoItem
    {
        std::string                 str;
        unsigned char *             data;
        size_t                      len;
    };
    std::list<FifoItem>             fifo;
    std::mutex                      mtx_fifo;
    std::condition_variable         cond_fifo;
public:
    inline WorkerThread(void);
    virtual ~WorkerThread(void);
    bool start(mrb_state * mrb, mrb_value self);
    void run_context(size_t id);
    void join(void);
    inline enum Status get_state(void) const;
    void wait(mrb_state * mrb);
    inline void notify(void);
    inline mrb_value notify(mrb_state * mrb, mrb_value & proc);
    inline mrb_value sync(mrb_state * mrb, mrb_value & proc);
    void stop(mrb_state * mrb);

    mrb_value fifo_push(mrb_state * mrb, mrb_value self);
    mrb_value fifo_pop(mrb_state * mrb, mrb_value self);
    mrb_value fifo_len(mrb_state * mrb, mrb_value self);
    mrb_value fifo_wait(mrb_state * mrb, mrb_value self);
};

class OneShotTimer
{
private:
    std::atomic<bool>                       running;
    std::atomic<unsigned int>               idx;
    std::vector<size_t>                     timer;
    std::function<void(size_t)>             act;
    std::atomic<std::chrono::system_clock::time_point>               begin;
    std::thread                             task;
    std::mutex                              mtx;        /*! mutex object                */
private:
    inline void start(void);
    inline unsigned long int get_timeout_tick(std::chrono::system_clock::time_point now) const;
public:
    inline OneShotTimer(std::vector<size_t> & timer_list, std::function<void(size_t)> callback);
    inline ~OneShotTimer();
    inline void restart(void);
    inline void stop(void);
};
inline unsigned long int OneShotTimer::get_timeout_tick(std::chrono::system_clock::time_point now) const
{
    if(idx.load() < timer.size())
    {
        auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(now - begin.load()).count();
        auto timeout_value = timer[idx.load()];
        if(interval < timeout_value) { return (timeout_value - interval); }
    }
    return 0;
}
inline void OneShotTimer::start(void)
{
    auto run = [this]()
    {
        running.store(true);
        while(running.load())
        {
            auto max = timer.size();
            if(idx.load() < max)
            {
                auto now = std::chrono::system_clock::now();
                auto tick = this->get_timeout_tick(now);
                if(200 < tick) { std::this_thread::sleep_for(std::chrono::milliseconds(200)); }
                else
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(tick));
                    if(running.load())
                    {
                        auto idx = this->idx.load();
                        if(idx < max)
                        {
                            act(idx);
                            this->idx.store(idx+1);
                        }
                    }
                }
            }
            else
            {
                if(200 < timer[0]) { std::this_thread::sleep_for(std::chrono::milliseconds(200));      }
                else               { std::this_thread::sleep_for(std::chrono::milliseconds(timer[0])); }
            }
        }
        running.store(false);
    };
    task = std::thread(run);
}
inline void OneShotTimer::restart(void)
{
    this->idx.store(timer.size());
    begin.store(std::chrono::system_clock::now());
    if(!running.load())
    {
        if(task.joinable()) { task.join(); }
        start();
    } else { this->idx.store(0); }
}
inline void OneShotTimer::stop(void)
{
    running.store(false);
    if(task.joinable()) { task.join(); }
}
inline OneShotTimer::OneShotTimer(std::vector<size_t> & timer_list, std::function<void(size_t)> callback)
  : running(true), idx(0), timer(timer_list), act(callback), begin(std::chrono::system_clock::now())
{
    start();
}
inline OneShotTimer::~OneShotTimer(void)
{
    stop();
}

class CyclicTimer
{
private:
    std::atomic<bool>       running;
    std::thread             task;
    std::function<void()>   act;
    std::chrono::system_clock::time_point begin;
    unsigned int            count;
    unsigned int            interval;
public:
    CyclicTimer()
      : running(false)
    {
    }
    unsigned long int get_timeout_tick(std::chrono::system_clock::time_point now) const
    {
        auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(now - begin).count();
        auto timeout_value = count;
        if(interval < timeout_value) { return (timeout_value - interval); }
        return 0;
    }
    void start(unsigned int interval_ms, std::function<void()> callback)
    {
        if(running.load()) { return; }
        running.store(true);
        begin    = std::chrono::system_clock::now();
        interval = interval_ms;
        count    = interval;
        act      = callback;
        auto run = [this]()
        {
            while(running.load())
            {
                auto now = std::chrono::system_clock::now();
                auto tick = this->get_timeout_tick(now);
                if(200 < tick) { std::this_thread::sleep_for(std::chrono::milliseconds(200)); }
                else
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(tick));
                    if(running.load())
                    {
                        act();
                        if((0xffffffff-interval) < count) { begin = now; count = interval;  }
                        else                              {              count += interval; }
                    }
                }
            }
        };
        task = std::thread(run);
    }
    void stop()
    {
        running.store(false);
        if(task.joinable()) { task.join(); }
    }
    ~CyclicTimer()
    {
        stop();
    }
};

class SerialMonitor : public Object
{
public:
    enum State { GAP = 0, TIME_OUT_1, TIME_OUT_2, TIME_OUT_3, CACHE_FULL, CLOSE, CTIMER, UTIMER, WAKEUP, NONE };
    struct ReciveInfo
    {
        size_t          idx;
        enum State      state;
        unsigned int    cnt;
        unsigned char * buff;
    };
    struct Sync
    {
        std::mutex              mtx;
        std::condition_variable cond;
    };
protected:
    std::vector<SerialControl *>    com;
    std::vector<OneShotTimer *>     oneshot;
    std::vector<ReciveInfo>         cache;
    struct Sync *                   sync;
    unsigned int                    cache_size;
    bool                            rcv_enable;
    CyclicTimer                     cyclic;
    CyclicTimer                     utimer;
    std::mutex                      mtx;
    std::condition_variable         cond;
    std::list<ReciveInfo>           rcv_cache;
    std::vector<size_t>             timer;
public:
    SerialMonitor(mrb_value & self, std::list<std::string> & ports, std::vector<size_t> & timer_def);
    virtual ~SerialMonitor(void);
    void close(void);
    void send(size_t idx, BinaryControl & bin, uint32_t timer);
    SerialMonitor::ReciveInfo read(BinaryControl & bin);
    SerialMonitor::State read_wait(BinaryControl & bin);
    void reciver(size_t id);
    void user_timer(int mode, unsigned int tick);
    inline void set_sync(SerialMonitor::Sync * sync);
    inline SerialMonitor::ReciveInfo refInfo(void);
};

class OpenXLSXCtrl : public Object
{
public:
    enum Type { Empty, Boolean, Integer, Float, Error, String };
    union Data { int vint; float vfloat; std::string * str; };
protected:
    OpenXLSX::XLDocument  doc;
    OpenXLSX::XLWorkbook  book;
    OpenXLSX::XLWorksheet sheet;

public:
    inline OpenXLSXCtrl(void);
    virtual ~OpenXLSXCtrl(void);
//  void create(const std::string & fname)           { doc.create(fname, true);     }
    inline void create(const std::string & fname);
    inline void open(const std::string & fname);
    inline void workbook(void);
    inline std::vector<std::string> getWorkSheetNames(void);
    void worksheet(std::string & sheet_name);
    inline void set_sheet_name(std::string & sheet_name);
    inline void set_cell_value(std::string & cell, int val);
    inline void set_cell_value(std::string & cell, char * val);
    inline void set_cell_value(std::string & cell, float val);
    OpenXLSXCtrl::Type get_cell(std::string & cell, OpenXLSXCtrl::Data & val) const;
    inline void save(void);
    inline void close(void);
};

/* -- inline methods -- */
inline unsigned char toValue(unsigned char data)
{
    unsigned char val=0xff;
         if(('0' <= data) && (data<='9')) { val = data - '0';      }
    else if(('a' <= data) && (data<='f')) { val = 10 + data - 'a'; }
    else if(('A' <= data) && (data<='F')) { val = 10 + data - 'A'; }
    else                                  { }
    return val;
}

inline auto str_split = [](std::string & src, std::regex & reg)
{
    std::list<std::string> result;
    std::copy( std::sregex_token_iterator{src.begin(), src.end(), reg, -1}, std::sregex_token_iterator{}, std::back_inserter(result) );
    return result;
};

inline Object::Object(void)
  : id(0), res(nullptr)
{
}

Object::~Object(void)
{
    if(nullptr != res)
    {
        res->erase(id);
    }
}

inline void Object::setControlID(unsigned int id, std::map<unsigned int, Object *> * res)
{
    if(nullptr != res)
    {
        this->id = id;
        this->res = res;
        (*res)[id] = this;
    }
}

inline unsigned int Object::ControlID(void)
{
    return this->id;
}

inline BinaryControl::BinaryControl(void)
  : length(0), compress_size(0), pos(0), data(nullptr)
{
}

inline BinaryControl::BinaryControl(BinaryControl & src)
  : length(0), compress_size(0), pos(0), data(nullptr)
{
    clone(0, src.length, src);
}

inline BinaryControl::BinaryControl(std::list<BinaryControl *> & list_bin)
  : length(0), compress_size(0), pos(0), data(nullptr)
{
    for(auto & bin : list_bin)
    {
        length += bin->size();
    }
    alloc(length);
    size_t addr = 0;
    for(auto & bin : list_bin)
    {
        std::memcpy(&(data[addr]), &(bin->data[0]), bin->length);
        addr += bin->length;
    }
}

inline BinaryControl::BinaryControl(size_t size_)
  : length(0), compress_size(0), pos(0), data(nullptr)
{
    alloc(size_);
}

inline BinaryControl::BinaryControl(size_t size_, unsigned char data)
  : length(0), compress_size(0), pos(0), data(nullptr)
{
    alloc(size_);
    std::memset(this->data, data, size_);
}

inline unsigned char * BinaryControl::ptr(void)
{
    return data;
}

inline size_t BinaryControl::size(void) const
{
    if(0 < compress_size)
    {
        return compress_size;
    }
    return length;
}

inline size_t BinaryControl::get_pos(void) const
{
    if(pos < size())
    {
        return pos;
    }
    return 0;
}

inline size_t BinaryControl::ref_pos(void) const
{
    return pos;
}

inline void BinaryControl::attach(unsigned char * ptr, size_t size)
{
    compress_size = pos = 0;
    length = size;
    data = ptr;
}

inline CppRegexp::CppRegexp(void)
  : regs(0)
{
}

inline CppRegexp::CppRegexp(std::string & str)
  : regs(1)
{
    std::regex reg(str);
    regs[0] = reg;
}

inline CppRegexp::CppRegexp(const char * arg)
  : regs(1)
{
    std::string str(arg);
    std::regex reg(str);
    regs[0] = reg;
}

inline CppRegexp::CppRegexp(const std::list<std::string> & arg)
  : regs(arg.size())
{
    unsigned int idx = 0;
    for(auto & str : arg)
    {
        std::regex reg(str);
        regs[idx] = reg;
        idx ++;
    }
}

inline void CppRegexp::reg_add(const std::string & str)
{

    std::regex reg(str);
    regs.push_back(reg);
}

inline void CppRegexp::reg_add(const std::list<std::string> & arg)
{
    for(auto & str : arg)
    {
        reg_add(str);
    }
}

inline unsigned int CppRegexp::length(void)
{
    return regs.size();
}

inline WorkerThread::WorkerThread(void)
  : state(Stop), mrb(nullptr), loop_cnt(0)
{
}

inline enum WorkerThread::Status WorkerThread::get_state(void) const
{
    return state;
}

inline void WorkerThread::notify(void)
{
    std::lock_guard<std::mutex> lock(mtx);
    state = Run;
    cond.notify_all();
}

inline mrb_value WorkerThread::notify(mrb_state * mrb, mrb_value & proc)
{
    std::lock_guard<std::mutex> lock(mtx);
    auto result = mrb_yield_argv(mrb, proc, 0, NULL);
    state = Run;
    cond.notify_all();
    return result;
}

inline mrb_value WorkerThread::sync(mrb_state * mrb, mrb_value & proc)
{
    std::lock_guard<std::mutex> lock(mtx);
    auto result = mrb_yield_argv(mrb, proc, 0, NULL);
    return result;
}

inline void SerialMonitor::set_sync(SerialMonitor::Sync * sync)
{
    std::lock_guard<std::mutex> lock(mtx);
    this->sync = sync;
}

inline SerialMonitor::ReciveInfo SerialMonitor::refInfo(void)
{
    if(0 < rcv_cache.size())
    {
        return *(rcv_cache.begin());
    }
    ReciveInfo info = { 0, NONE, 0, nullptr };
    return info;
}

inline OpenXLSXCtrl::OpenXLSXCtrl(void)
{
}

inline void OpenXLSXCtrl::create(const std::string & fname)
{
    doc.create(fname);
}

inline void OpenXLSXCtrl::open(const std::string & fname)
{
    doc.open(fname);
}

inline void OpenXLSXCtrl::workbook(void)
{
    book = doc.workbook();
}

inline std::vector<std::string> OpenXLSXCtrl::getWorkSheetNames(void)
{
    return book.worksheetNames();
}

inline void OpenXLSXCtrl::set_sheet_name(std::string & sheet_name)
{
    sheet.setName(sheet_name);
}

inline void OpenXLSXCtrl::set_cell_value(std::string & cell, int val)
{
    sheet.cell(OpenXLSX::XLCellReference(cell)).value() = val;
}

inline void OpenXLSXCtrl::set_cell_value(std::string & cell, char * val)
{
    sheet.cell(OpenXLSX::XLCellReference(cell)).value() = val;
}

inline void OpenXLSXCtrl::set_cell_value(std::string & cell, float val)
{
    DWord temp = { .value = val };
    if((temp.uint32 & 0x7fffff) != 0x7fffff)
    {
        sheet.cell(OpenXLSX::XLCellReference(cell)).value() = val;
    }
    else
    {
        sheet.cell(OpenXLSX::XLCellReference(cell)).value() = "NaN";
    }
}

inline void OpenXLSXCtrl::save(void)
{
    doc.save();
}

inline void OpenXLSXCtrl::close(void)
{
    doc.close();
}


/* -- protorypes for mruby interfaces -- */
static mrb_value mrb_core_tick(mrb_state* mrb, mrb_value self);
static mrb_value mrb_core_date(mrb_state* mrb, mrb_value self);
static mrb_value mrb_core_gets(mrb_state* mrb, mrb_value self);
static mrb_value mrb_core_exists(mrb_state* mrb, mrb_value self);
static mrb_value mrb_core_file_timestamp(mrb_state* mrb, mrb_value self);
static mrb_value mrb_core_make_qr(mrb_state* mrb, mrb_value self);
static mrb_value mrb_core_get_args(mrb_state * mrb, mrb_value self);
static mrb_value mrb_core_get_opts(mrb_state * mrb, mrb_value self);
static mrb_value mrb_core_prog(mrb_state * mrb, mrb_value self);
static void mrb_core_context_free(mrb_state * mrb, void * ptr);

static mrb_value mrb_bedit_initialize(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_length(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_dump(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_write(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_memset(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_memcpy(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_memcmp(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_get(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_set(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_pos(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_crc32(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_crc16(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_crc8(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_sum(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_xsum(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_compress(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_uncompress(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_save(mrb_state * mrb, mrb_value self);
static void mrb_bedit_context_free(mrb_state * mrb, void * ptr);
static BinaryControl * get_bedit_ptr(mrb_value & argv);

static mrb_value mrb_cppregexp_reg_match(mrb_state* mrb, mrb_value self);
static mrb_value mrb_cppregexp_reg_replace(mrb_state* mrb, mrb_value self);
static mrb_value mrb_cppregexp_reg_split(mrb_state* mrb, mrb_value self);
static mrb_value mrb_cppregexp_initialize(mrb_state * mrb, mrb_value self);
static mrb_value mrb_cppregexp_length(mrb_state * mrb, mrb_value self);
static mrb_value mrb_cppregexp_match(mrb_state * mrb, mrb_value self);
static mrb_value mrb_cppregexp_grep(mrb_state * mrb, mrb_value self);
static mrb_value mrb_cppregexp_replace(mrb_state * mrb, mrb_value self);
static mrb_value mrb_cppregexp_select(mrb_state * mrb, mrb_value self);
static mrb_value mrb_cppregexp_split(mrb_state * mrb, mrb_value self);
static void mrb_regexp_context_free(mrb_state * mrb, void * ptr);

static mrb_value mrb_thread_initialize(mrb_state * mrb, mrb_value self);
static mrb_value mrb_thread_state(mrb_state * mrb, mrb_value self);
static mrb_value mrb_thread_start(mrb_state * mrb, mrb_value self);
static mrb_value mrb_thread_wait(mrb_state * mrb, mrb_value self);
static mrb_value mrb_thread_notify(mrb_state * mrb, mrb_value self);
static mrb_value mrb_thread_sync(mrb_state * mrb, mrb_value self);
static mrb_value mrb_thread_join(mrb_state * mrb, mrb_value self);
static mrb_value mrb_thread_stop(mrb_state * mrb, mrb_value self);
static mrb_value mrb_thread_fifo_push(mrb_state * mrb, mrb_value self);
static mrb_value mrb_thread_fifo_pop(mrb_state * mrb, mrb_value self);
static mrb_value mrb_thread_fifo_len(mrb_state * mrb, mrb_value self);
static mrb_value mrb_thread_fifo_wait(mrb_state * mrb, mrb_value self);
static mrb_value mrb_thread_ms_sleep(mrb_state * mrb, mrb_value self);
static void mrb_thread_context_free(mrb_state * mrb, void * ptr);

static mrb_value mrb_smon_comlist(mrb_state* mrb, mrb_value self);
static mrb_value mrb_smon_pipelist(mrb_state* mrb, mrb_value self);
static mrb_value mrb_smon_initialize(mrb_state * mrb, mrb_value self);
static mrb_value mrb_smon_send(mrb_state * mrb, mrb_value self);
static mrb_value mrb_smon_read_wait(mrb_state * mrb, mrb_value self);
static mrb_value mrb_smon_close(mrb_state * mrb, mrb_value self);
static mrb_value mrb_smon_timer(mrb_state * mrb, mrb_value self);
static void mrb_smon_context_free(mrb_state * mrb, void * ptr);
static SerialMonitor * get_smon_ptr(mrb_value & argv);

static mrb_value mrb_xlsx_initialize(mrb_state * mrb, mrb_value self);
static mrb_value mrb_xlsx_create(mrb_state * mrb, mrb_value self);
static mrb_value mrb_xlsx_open(mrb_state * mrb, mrb_value self);
static mrb_value mrb_xlsx_worksheet(mrb_state * mrb, mrb_value self);
static mrb_value mrb_xlsx_sheet_names(mrb_state * mrb, mrb_value self);
static mrb_value mrb_xlsx_set_seet_name(mrb_state * mrb, mrb_value self);
static mrb_value mrb_xlsx_set_value(mrb_state * mrb, mrb_value self);
static mrb_value mrb_xlsx_cell(mrb_state * mrb, mrb_value self);
static mrb_value mrb_xlsx_save(mrb_state * mrb, mrb_value self);
static void mrb_xlsx_context_free(mrb_state * mrb, void * ptr);


/* -- static tables -- */
static const char *  SoftwareRevision = "0.14.06";
static const struct mrb_data_type mrb_core_context_type =
{
    "mrb_core_context",         mrb_core_context_free
};
static const struct mrb_data_type mrb_bedit_context_type =
{
    "mrb_bedit_context",        mrb_bedit_context_free
};
static const struct mrb_data_type mrb_cpp_regexp_context_type =
{
    "mrb_cpp_regexp_context",   mrb_regexp_context_free
};
static const struct mrb_data_type mrb_thread_context_type =
{
    "mrb_cpp_thread_context",   mrb_thread_context_free
};
static const struct mrb_data_type mrb_smon_context_type =
{
    "mrb_smon_context",         mrb_smon_context_free
};
static const struct mrb_data_type mrb_xlsx_context_type =
{
    "mrb_open_xlsx_context",    mrb_xlsx_context_free
};


/* -- Application -- */
class Application : public Object
{
protected:
    mrb_state *                             mrb_main;
    std::string                             prog;
    static Application *                    obj;
    boost::program_options::variables_map & opts;
    std::vector<std::string> &              args;
    std::vector<size_t>                     timer;
    unsigned int                            id;
    std::atomic<bool>                       is_gets;
    std::atomic<bool>                       is_cin_open;
    std::list<std::string>                  list_cin;
    std::thread                             cin_th;
    SerialMonitor::Sync                     cin_cync;
    std::map<unsigned int, Object *>        res;
    std::mutex                              mtx_init;
    Core *                                  core;

private:
    void push(Object * obj)
    {
        if(nullptr != obj)
        {
            for(unsigned int id = this->id + 1; id != this->id; id ++)
            {
                if(res.count(id) == 0)
                {
                    this->id = id;
                    obj->setControlID(id, &res);
                    break;
                }
            }
        }
    }
    void exit(int code) { }
    inline void garbage_collect(mrb_state * mrb)
    {
        if((nullptr != mrb_main) && (mrb == mrb_main))
        {
#if 0
            mrb_garbage_collect(mrb_main);
#endif
        }
    }

public:
    static Application * getObject(mrb_state * mrb);
    Application(std::string & program, boost::program_options::variables_map & optmap, std::vector<std::string> & arg)
      : mrb_main(nullptr), prog(program), opts(optmap), args(arg), timer(4), id(1), core(nullptr)
    {
        obj = this;
        timer[0] =   30;
        timer[1] =  300;
        timer[2] =  500;
        timer[3] = 1000;
        is_gets.store(false);
        is_cin_open.store(true);
        core = Core::createObject();
    }
    virtual ~Application(void)
    {
        if( is_gets.load() ) { is_cin_open.store(false); }
        if(nullptr != core)  { delete core; core = nullptr; }
        std::lock_guard<std::mutex> lock(mtx_init);
        for(auto & item : res)
        {
            auto key = item.first;
            delete res[key];
            res[key] = nullptr;
        }
        res.clear();
    }

    mrb_value core_get_args(mrb_state * mrb, mrb_value self)
    {
        mrb_value arry = mrb_ary_new(mrb);
        for(auto & arg : args)
        {
            mrb_ary_push(mrb, arry, mrb_str_new_cstr(mrb, arg.c_str()));
        }
        return arry;
    }

    mrb_value core_get_opts(mrb_state * mrb, mrb_value self)
    {
        mrb_value hash = mrb_hash_new(mrb);
        if(opts.count("comlist"))          { mrb_hash_set(mrb, hash, mrb_str_new_cstr(mrb, "comlist"),          mrb_bool_value(true)); }
        if(opts.count("pipelist"))         { mrb_hash_set(mrb, hash, mrb_str_new_cstr(mrb, "pipelist"),         mrb_bool_value(true)); }
        if(opts.count("gap"))              { mrb_hash_set(mrb, hash, mrb_str_new_cstr(mrb, "gap"),              mrb_int_value(mrb,     opts["gap"].as<unsigned int>()));                      }
        if(opts.count("timer"))            { mrb_hash_set(mrb, hash, mrb_str_new_cstr(mrb, "timer"),            mrb_int_value(mrb,     opts["timer"].as<unsigned int>()));                    }
        if(opts.count("timer2"))           { mrb_hash_set(mrb, hash, mrb_str_new_cstr(mrb, "timer2"),           mrb_int_value(mrb,     opts["timer2"].as<unsigned int>()));                   }
        if(opts.count("timer3"))           { mrb_hash_set(mrb, hash, mrb_str_new_cstr(mrb, "timer3"),           mrb_int_value(mrb,     opts["timer3"].as<unsigned int>()));                   }
        if(opts.count("crc"))              { mrb_hash_set(mrb, hash, mrb_str_new_cstr(mrb, "crc"),              mrb_str_new_cstr(mrb, (opts["crc"].as<std::string>()).c_str()));              }
        if(opts.count("crc8"))             { mrb_hash_set(mrb, hash, mrb_str_new_cstr(mrb, "crc8"),             mrb_str_new_cstr(mrb, (opts["crc8"].as<std::string>()).c_str()));             }
        if(opts.count("sum"))              { mrb_hash_set(mrb, hash, mrb_str_new_cstr(mrb, "sum"),              mrb_str_new_cstr(mrb, (opts["sum"].as<std::string>()).c_str()));              }
        if(opts.count("FLOAT"))            { mrb_hash_set(mrb, hash, mrb_str_new_cstr(mrb, "FLOAT"),            mrb_str_new_cstr(mrb, (opts["FLOAT"].as<std::string>()).c_str()));            }
        if(opts.count("float"))            { mrb_hash_set(mrb, hash, mrb_str_new_cstr(mrb, "float"),            mrb_str_new_cstr(mrb, (opts["float"].as<std::string>()).c_str()));            }
        if(opts.count("oneline"))          { mrb_hash_set(mrb, hash, mrb_str_new_cstr(mrb, "oneline"),          mrb_str_new_cstr(mrb, (opts["oneline"].as<std::string>()).c_str()));          }
        if(opts.count("bin-edit"))         { mrb_hash_set(mrb, hash, mrb_str_new_cstr(mrb, "bin-edit"),         mrb_str_new_cstr(mrb, (opts["bin-edit"].as<std::string>()).c_str()));         }
        if(opts.count("makeQR"))           { mrb_hash_set(mrb, hash, mrb_str_new_cstr(mrb, "makeQR"),           mrb_str_new_cstr(mrb, (opts["makeQR"].as<std::string>()).c_str()));           }
        if(opts.count("read-bin-to-xlsx")) { mrb_hash_set(mrb, hash, mrb_str_new_cstr(mrb, "read-bin-to-xlsx"), mrb_str_new_cstr(mrb, (opts["read-bin-to-xlsx"].as<std::string>()).c_str())); }
        if(opts.count("mruby-script"))     { mrb_hash_set(mrb, hash, mrb_str_new_cstr(mrb, "mruby-script"),     mrb_str_new_cstr(mrb, (opts["mruby-script"].as<std::string>()).c_str()));     }
        if(opts.count("version"))          { mrb_hash_set(mrb, hash, mrb_str_new_cstr(mrb, "version"),          mrb_str_new_cstr(mrb, (opts["version"].as<std::string>()).c_str()));          }
        if(opts.count("help"))             { mrb_hash_set(mrb, hash, mrb_str_new_cstr(mrb, "help"),             mrb_str_new_cstr(mrb, (opts["help"].as<std::string>()).c_str()));             }
        if(opts.count("help-misc"))        { mrb_hash_set(mrb, hash, mrb_str_new_cstr(mrb, "help-misc"),        mrb_str_new_cstr(mrb, (opts["help-misc"].as<std::string>()).c_str()));        }
        return hash;
    }

    mrb_value core_prog(mrb_state * mrb, mrb_value self)
    {
        return mrb_str_new_cstr( mrb, prog.c_str() );
    }
    mrb_value core_gets(mrb_state * mrb, mrb_value self)
    {
        if( (is_cin_open.load()) && (!is_gets.load()) )
        {
            auto watch_cin = [&]()
            {
                cin_th.detach();
                is_gets.store(true);
                while(is_cin_open.load())
                {
                    try
                    {
                        if(std::cin.eof())          { is_cin_open.store(false); break; }
                        else if(std::cin.fail())    { is_cin_open.store(false); break; }
                        else
                        {
                            std::string str("");
                            core->gets(str);
                            if(is_cin_open.load())
                            {
                                std::lock_guard<std::mutex> lock(cin_cync.mtx);
                                list_cin.push_back(str);
                                cin_cync.cond.notify_all();
                            }
                        }
                    } catch(std::exception & exp)   { is_cin_open.store(false); break; }
                }
                is_gets.store(false);
            };
            cin_th= std::thread(watch_cin);
        }
        SerialMonitor * smon = nullptr;
        BinaryControl * bin  = nullptr;
        mrb_value proc; mrb_int argc; mrb_value * argv;
        mrb_get_args(mrb, "&*", &proc, &argv, &argc);
        if(!mrb_nil_p(proc))
        {
            if(2 == argc)
            {
                smon = get_smon_ptr(argv[0]);
                bin  = get_bedit_ptr(argv[1]);
                if((nullptr != smon) && (nullptr != bin))
                    { smon->set_sync(&cin_cync); }
            }
        }
        auto lamda = [&]
        {
            if( !is_cin_open.load() ) { return true; }
            if(0 < list_cin.size())   { return true; }
            if(nullptr != smon)
            {
                if(SerialMonitor::NONE != (smon->refInfo()).state) { return true; }
            }
            return false;
        };
        std::unique_lock<std::mutex> lock(cin_cync.mtx);
        if(0 < list_cin.size())
        {
            auto str = *(list_cin.begin());
            list_cin.pop_front();
            if(nullptr != smon) { smon->set_sync(nullptr); }
            return mrb_str_new_cstr( mrb, str.c_str() );
        }
        while( is_cin_open.load() )
        {
            cin_cync.cond.wait(lock, lamda);
            if(0 < list_cin.size())
            {
                auto str = *(list_cin.begin());
                list_cin.pop_front();
                if(nullptr != smon) { smon->set_sync(nullptr); }
                return mrb_str_new_cstr( mrb, str.c_str() );
            }
            if((nullptr != smon) && (nullptr != bin))
            {
                lock.unlock();
                SerialMonitor::ReciveInfo info = smon->read(*bin);
                lock.lock();
                if(SerialMonitor::NONE != info.state)
                {
                    mrb_value argv[2];
                    argv[0] = mrb_int_value(mrb, info.idx);
                    argv[1] = mrb_int_value(mrb, info.state);
                    mrb_yield_argv(mrb, proc, 2, argv);
                }
            }
        }
        if(nullptr != smon) { smon->set_sync(nullptr); }
        return mrb_nil_value();
    }

    mrb_value bedit_init(mrb_state * mrb, mrb_value self)
    {
        std::lock_guard<std::mutex> lock(mtx_init);
        BinaryControl * bedit = nullptr;
        mrb_int argc; mrb_value * argv;
        mrb_get_args(mrb, "*", &argv, &argc);
        mrb_value item;
        std::list<BinaryControl *> list_bin;
        std::list<BinaryControl *> list_temp;
        switch(argc)
        {
        case 1:
            switch( mrb_type( argv[0] ) )
            {
            case MRB_TT_INTEGER:
                bedit = new BinaryControl( static_cast<int>( mrb_integer( argv[0] ) ) );
                break;
            case MRB_TT_STRING:
                {
                    std::string data(RSTR_PTR(mrb_str_ptr(argv[0])));
                    bedit = new BinaryControl( data );
                }
                break;
            case MRB_TT_OBJECT:
                {
                    BinaryControl * bin_src = get_bedit_ptr( argv[0] );
                    if(nullptr != bin_src) { bedit = new BinaryControl( *bin_src ); }
                }
                break;
            case MRB_TT_ARRAY:
                while( !mrb_nil_p( item = mrb_ary_shift(mrb, argv[0])) )
                {
                    switch(mrb_type(item))
                    {
                    case MRB_TT_STRING:
                        {
                            std::string data(RSTR_PTR(mrb_str_ptr(item)));
                            auto bin = new BinaryControl(data);
                            list_bin.push_back( bin );
                            list_temp.push_back( bin );
                        }
                        break;
                    case MRB_TT_OBJECT:
                        {
                            BinaryControl * bin = get_bedit_ptr(item);
                            list_bin.push_back( bin );
                        }
                        break;
                    default:
                        break;
                    }
                }
                bedit = new BinaryControl(list_bin);
                for(auto & bin: list_temp) { delete bin; }
                list_temp.clear();
                break;
            default:
                break;
            }
            break;
        case 2:
            if(  (MRB_TT_OBJECT  == mrb_type(argv[0]))      // BinEdit object
               &&(MRB_TT_INTEGER == mrb_type(argv[1])))     // copy size
            {
                BinaryControl * bin_src = get_bedit_ptr( argv[0] );
                if(nullptr != bin_src)
                {
                    bedit = new BinaryControl();
                    bedit->clone(0, mrb_integer(argv[1]), *bin_src);
                }
            }
            else if(  (MRB_TT_INTEGER == mrb_type(argv[0]))     // length
                    &&(MRB_TT_INTEGER == mrb_type(argv[1])))    // fill data
            {
                auto len  = static_cast<int>(mrb_integer(argv[0]));
                auto data = mrb_integer(argv[1]);
                bedit = new BinaryControl(len, data);
            }
            else
            {
            }
            break;
        case 3:
            if(  (MRB_TT_OBJECT  == mrb_type(argv[0]))      // BinEdit Object
               &&(MRB_TT_INTEGER == mrb_type(argv[1]))      // copy start addresss
               &&(MRB_TT_INTEGER == mrb_type(argv[2])))     // copy size
            {
                BinaryControl * bin_src = get_bedit_ptr( argv[0] );
                if(nullptr != bin_src)
                {
                    bedit = new BinaryControl();
                    bedit->clone(mrb_integer(argv[1]), mrb_integer(argv[2]), *bin_src);
                }
            }
            break;
        default:
            break;
        }
        if(nullptr == bedit) { bedit = new BinaryControl(); }
        mrb_data_init(self, bedit, &mrb_bedit_context_type);
        push(bedit);
        return self;
    }

    mrb_value cppregexp_init(mrb_state * mrb, mrb_value self)
    {
        std::lock_guard<std::mutex> lock(mtx_init);
        mrb_int argc; mrb_value * argv;
        mrb_get_args(mrb, "*", &argv, &argc);
        CppRegexp * regexp = nullptr;
        std::list<std::string> arg;
        mrb_value item;
        switch(argc)
        {
        case 1:
            switch(mrb_type(argv[0]))
            {
            case MRB_TT_STRING:
                regexp = new CppRegexp( RSTR_PTR(mrb_str_ptr(argv[0])) );
                break;
            case MRB_TT_ARRAY:
                while( !mrb_nil_p( item = mrb_ary_shift(mrb, argv[0])) )
                {
                    if(MRB_TT_STRING == mrb_type(item))
                    {
                        std::string str( RSTR_PTR(mrb_str_ptr(item)) );
                        arg.push_back( str );
                    }
                }
                regexp = new CppRegexp( arg );
                break;
            }
            break;
        default:
            for(auto idx = 0; idx < argc; idx ++)
            {
                if(MRB_TT_STRING == mrb_type(argv[idx]))
                {
                    std::string str( RSTR_PTR(mrb_str_ptr(argv[idx])) );
                    arg.push_back( str );
                }
                regexp = new CppRegexp( arg );
            }
            break;
        }
        if(nullptr == regexp) { regexp = new CppRegexp(); }
        mrb_data_init(self, regexp, &mrb_cpp_regexp_context_type);
        push(regexp);
        return self;
    }

    mrb_value thread_init(mrb_state * mrb, mrb_value self)
    {
        std::lock_guard<std::mutex> lock(mtx_init);
        WorkerThread * th_ctrl = new WorkerThread();
        mrb_data_init(self, th_ctrl, &mrb_thread_context_type);
        th_ctrl->start(mrb, self);
        return self;
    }

    mrb_value smon_init(mrb_state * mrb, mrb_value self)
    {
        std::lock_guard<std::mutex> lock(mtx_init);
        std::list<std::string> ports;
        mrb_int argc; mrb_value * argv;
        mrb_get_args(mrb, "*", &argv, &argc);
        mrb_value item;
        for(auto idx=0; idx < argc; idx++)
        {
            switch(mrb_type(argv[idx]))
            {
            case MRB_TT_STRING:
                {
                    std::string str(RSTR_PTR(mrb_str_ptr(argv[idx])));
                    ports.push_back(str);
                }
                break;
            case MRB_TT_ARRAY:
                while( !mrb_nil_p(item = mrb_ary_shift(mrb, argv[idx])) )
                {
                    if(MRB_TT_STRING == mrb_type(item))
                    {
                        std::string str(RSTR_PTR(mrb_str_ptr(item)));
                        ports.push_back(str);
                    }
                }
                break;
            default:
                break;
            }
        }
        SerialMonitor * smon = new SerialMonitor(self, ports, timer);
        mrb_data_init(self, smon, &mrb_smon_context_type);
        push(smon);
        return self;
    }

    mrb_value xlsx_init(mrb_state * mrb, mrb_value self)
    {
        std::lock_guard<std::mutex> lock(mtx_init);
        OpenXLSXCtrl * xlsx = new OpenXLSXCtrl();
        mrb_data_init(self, xlsx, &mrb_xlsx_context_type);
        push(xlsx);
        return self;
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
            mrb_main = mrb;
            /* Class Core */
            struct RClass * core_class = mrb_define_class(mrb, "Core", mrb->object_class);
            mrb_define_module_function(mrb, core_class, "tick",      mrb_core_tick,             MRB_ARGS_ANY()     );
            mrb_define_module_function(mrb, core_class, "date",      mrb_core_date,             MRB_ARGS_ANY()     );
            mrb_define_module_function(mrb, core_class, "gets",      mrb_core_gets,             MRB_ARGS_ANY()     );
            mrb_define_module_function(mrb, core_class, "exists",    mrb_core_exists,           MRB_ARGS_ARG(1, 1) );
            mrb_define_module_function(mrb, core_class, "timestamp", mrb_core_file_timestamp,   MRB_ARGS_ARG(1, 1) );
            mrb_define_module_function(mrb, core_class, "makeQR",    mrb_core_make_qr,          MRB_ARGS_ARG(1, 1) );
            mrb_define_module_function(mrb, core_class, "args",      mrb_core_get_args,         MRB_ARGS_NONE()    );
            mrb_define_module_function(mrb, core_class, "opts",      mrb_core_get_opts,         MRB_ARGS_NONE()    );
            mrb_define_module_function(mrb, core_class, "prog",      mrb_core_prog,             MRB_ARGS_NONE()    );

            /* Class BinEdit */
            struct RClass * bedit_class = mrb_define_class(mrb, "BinEdit", mrb->object_class);
            mrb_define_method( mrb, bedit_class, "initialize", mrb_bedit_initialize, MRB_ARGS_ANY()     );
            mrb_define_method( mrb, bedit_class, "length",     mrb_bedit_length,     MRB_ARGS_NONE()    );
            mrb_define_method( mrb, bedit_class, "dump",       mrb_bedit_dump,       MRB_ARGS_ANY()     );
            mrb_define_method( mrb, bedit_class, "write",      mrb_bedit_write,      MRB_ARGS_ANY()     );
            mrb_define_method( mrb, bedit_class, "memset",     mrb_bedit_memset,     MRB_ARGS_ANY()     );
            mrb_define_method( mrb, bedit_class, "memcpy",     mrb_bedit_memcpy,     MRB_ARGS_ANY()     );
            mrb_define_method( mrb, bedit_class, "memcmp",     mrb_bedit_memcmp,     MRB_ARGS_ANY()     );
            mrb_define_method( mrb, bedit_class, "get",        mrb_bedit_get,        MRB_ARGS_ANY()     );
            mrb_define_method( mrb, bedit_class, "set",        mrb_bedit_set,        MRB_ARGS_ANY()     );
            mrb_define_method( mrb, bedit_class, "pos",        mrb_bedit_pos,        MRB_ARGS_NONE()    );
            mrb_define_method( mrb, bedit_class, "crc32",      mrb_bedit_crc32,      MRB_ARGS_ARG(1, 1) );
            mrb_define_method( mrb, bedit_class, "crc16",      mrb_bedit_crc16,      MRB_ARGS_ANY()     );
            mrb_define_method( mrb, bedit_class, "crc8",       mrb_bedit_crc8,       MRB_ARGS_ANY()     );
            mrb_define_method( mrb, bedit_class, "sum",        mrb_bedit_sum,        MRB_ARGS_ANY()     );
            mrb_define_method( mrb, bedit_class, "xsum",       mrb_bedit_xsum,       MRB_ARGS_ANY()     );
            mrb_define_method( mrb, bedit_class, "compress",   mrb_bedit_compress,   MRB_ARGS_NONE()    );
            mrb_define_method( mrb, bedit_class, "uncompress", mrb_bedit_uncompress, MRB_ARGS_NONE()    );
            mrb_define_method( mrb, bedit_class, "save",       mrb_bedit_save,       MRB_ARGS_ARG(1, 1) );

            /* Class CppRegexp */
            struct RClass * cppregexp_class = mrb_define_class(mrb, "CppRegexp", mrb->object_class);
            mrb_define_module_function(mrb, cppregexp_class, "reg_match",   mrb_cppregexp_reg_match,   MRB_ARGS_ARG(2, 1) );
            mrb_define_module_function(mrb, cppregexp_class, "reg_replace", mrb_cppregexp_reg_replace, MRB_ARGS_ARG(3, 1) );
            mrb_define_module_function(mrb, cppregexp_class, "reg_split",   mrb_cppregexp_reg_split,   MRB_ARGS_ARG(2, 1) );
            mrb_define_method( mrb, cppregexp_class, "initialize", mrb_cppregexp_initialize, MRB_ARGS_ANY()     );
            mrb_define_method( mrb, cppregexp_class, "length",     mrb_cppregexp_length,     MRB_ARGS_NONE()    );
            mrb_define_method( mrb, cppregexp_class, "match",      mrb_cppregexp_match,      MRB_ARGS_ANY()     );
            mrb_define_method( mrb, cppregexp_class, "grep",       mrb_cppregexp_grep,       MRB_ARGS_ANY()     );
            mrb_define_method( mrb, cppregexp_class, "replace",    mrb_cppregexp_replace,    MRB_ARGS_ANY()     );
            mrb_define_method( mrb, cppregexp_class, "select",     mrb_cppregexp_select,     MRB_ARGS_ARG(1, 1) );
            mrb_define_method( mrb, cppregexp_class, "split",      mrb_cppregexp_split,      MRB_ARGS_ARG(1, 1) );

            /* Class Thread */
            struct RClass * thread_class = mrb_define_class(mrb, "WorkerThread", mrb->object_class);
            mrb_define_const(mrb, thread_class, "STOP",      mrb_fixnum_value(WorkerThread::Stop)     );
            mrb_define_const(mrb, thread_class, "WAKEUP",    mrb_fixnum_value(WorkerThread::Wakeup)   );
            mrb_define_const(mrb, thread_class, "RUN",       mrb_fixnum_value(WorkerThread::Run)      );
            mrb_define_const(mrb, thread_class, "WAIT_JOIN", mrb_fixnum_value(WorkerThread::WaitStop) );
            mrb_define_module_function( mrb, thread_class, "ms_sleep", mrb_thread_ms_sleep, MRB_ARGS_ARG(1, 1) );
            mrb_define_method(mrb, thread_class, "initialize",  mrb_thread_initialize,      MRB_ARGS_NONE()   );
            mrb_define_method(mrb, thread_class, "state",       mrb_thread_state,           MRB_ARGS_ANY()    );
            mrb_define_method(mrb, thread_class, "start",       mrb_thread_start,           MRB_ARGS_ANY()    );
            mrb_define_method(mrb, thread_class, "wait",        mrb_thread_wait,            MRB_ARGS_NONE()   );
            mrb_define_method(mrb, thread_class, "notify",      mrb_thread_notify,          MRB_ARGS_NONE()   );
            mrb_define_method(mrb, thread_class, "synchronize", mrb_thread_sync,            MRB_ARGS_NONE()   );
            mrb_define_method(mrb, thread_class, "join",        mrb_thread_join,            MRB_ARGS_ANY()    );
            mrb_define_method(mrb, thread_class, "stop",        mrb_thread_stop,            MRB_ARGS_NONE()   );
            mrb_define_method(mrb, thread_class, "fifo_push",   mrb_thread_fifo_push,       MRB_ARGS_ANY()    );
            mrb_define_method(mrb, thread_class, "fifo_pop",    mrb_thread_fifo_pop,        MRB_ARGS_ARG(1, 1));
            mrb_define_method(mrb, thread_class, "fifo_len",    mrb_thread_fifo_len,        MRB_ARGS_NONE()   );
            mrb_define_method(mrb, thread_class, "fifo_wait",   mrb_thread_fifo_wait,       MRB_ARGS_NONE()   );

            /* Class Smon */
            struct RClass * smon_class = mrb_define_class(mrb, "Smon", mrb->object_class);
            mrb_define_const(  mrb, smon_class, "GAP",              mrb_fixnum_value(SerialMonitor::GAP)                );
            mrb_define_const(  mrb, smon_class, "TO1",              mrb_fixnum_value(SerialMonitor::TIME_OUT_1)         );
            mrb_define_const(  mrb, smon_class, "TO2",              mrb_fixnum_value(SerialMonitor::TIME_OUT_2)         );
            mrb_define_const(  mrb, smon_class, "TO3",              mrb_fixnum_value(SerialMonitor::TIME_OUT_3)         );
            mrb_define_const(  mrb, smon_class, "CLOSE",            mrb_fixnum_value(SerialMonitor::CLOSE)              );
            mrb_define_const(  mrb, smon_class, "CACHE_FULL",       mrb_fixnum_value(SerialMonitor::CACHE_FULL)         );
            mrb_define_const(  mrb, smon_class, "CTIMER",           mrb_fixnum_value(SerialMonitor::CTIMER)             );
            mrb_define_const(  mrb, smon_class, "UTIMER",           mrb_fixnum_value(SerialMonitor::UTIMER)             );
            mrb_define_const(  mrb, smon_class, "NONE",             mrb_fixnum_value(SerialMonitor::NONE)               );
            mrb_define_module_function(mrb, smon_class, "comlist",  mrb_smon_comlist,       MRB_ARGS_ANY()              );
            mrb_define_module_function(mrb, smon_class, "pipelist", mrb_smon_pipelist,      MRB_ARGS_ANY()              );
            mrb_define_method( mrb, smon_class, "initialize",       mrb_smon_initialize,    MRB_ARGS_REQ( 2 )           );
            mrb_define_method( mrb, smon_class, "send",             mrb_smon_send,          MRB_ARGS_ANY()              );
            mrb_define_method( mrb, smon_class, "read_wait",        mrb_smon_read_wait,     MRB_ARGS_ANY()              );
            mrb_define_method( mrb, smon_class, "close",            mrb_smon_close,         MRB_ARGS_NONE()             );
            mrb_define_method( mrb, smon_class, "timer",            mrb_smon_timer,         MRB_ARGS_ANY()              );

            /* Class OpenXLSX */
            struct RClass * xlsx_class = mrb_define_class(mrb, "OpenXLSX", mrb->object_class);
            mrb_define_method( mrb, xlsx_class, "initialize",       mrb_xlsx_initialize,        MRB_ARGS_NONE()         );
            mrb_define_method( mrb, xlsx_class, "create",           mrb_xlsx_create,            MRB_ARGS_ANY()          );
            mrb_define_method( mrb, xlsx_class, "open",             mrb_xlsx_open,              MRB_ARGS_ANY()          );
            mrb_define_method( mrb, xlsx_class, "sheet",            mrb_xlsx_worksheet,         MRB_ARGS_ARG( 1, 1 )    );
            mrb_define_method( mrb, xlsx_class, "sheet_names",      mrb_xlsx_sheet_names,       MRB_ARGS_NONE()         );
            mrb_define_method( mrb, xlsx_class, "setSheetName",     mrb_xlsx_set_seet_name,     MRB_ARGS_ARG( 1, 1 )    );
            mrb_define_method( mrb, xlsx_class, "set_value",        mrb_xlsx_set_value,         MRB_ARGS_ARG( 2, 1 )    );
            mrb_define_method( mrb, xlsx_class, "cell",             mrb_xlsx_cell,              MRB_ARGS_ARG( 1, 1 )    );
            mrb_define_method( mrb, xlsx_class, "save",             mrb_xlsx_save,              MRB_ARGS_NONE()         );

            /* exec mRuby Script */
            extern const uint8_t default_options[];
            mrb_load_irep(mrb, default_options);
            if(opts.count("mruby-script"))
            {
                std::string mruby_fnames = opts["mruby-script"].as<std::string>();
                std::regex reg(",");
                for( auto && fname : str_split( mruby_fnames, reg ) )
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
Application * Application::getObject(mrb_state * mrb)
{
    if(nullptr != Application::obj)
    {
        Application::obj->garbage_collect(mrb);
    }
    return Application::obj;
}


/* -- functions for mruby interface -- */
static std::string makeQRsvg(const std::string & arg)
{
    const char *text = arg.c_str();
    const qrcodegen::QrCode::Ecc errCorLvl = qrcodegen::QrCode::Ecc::LOW;
    const qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(text, errCorLvl);
    int border = 4;
    if((border * 2) > INT_MAX - qr.getSize())
    {
        throw std::overflow_error("Border too large");
    }
    std::ostringstream sb;
    sb << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    sb << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\"";
    sb <<                                   " \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n";
    sb << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" viewBox=\"0 0 ";
    sb << (qr.getSize() + border * 2) << " " << (qr.getSize() + border * 2) << "\" stroke=\"none\">\n";
    sb << "  <rect width=\"100%\" height=\"100%\" fill=\"#FFFFFF\"/>\n";
    sb << "  <path d=\"";
    for(int y = 0; y < qr.getSize(); y++)
    {
        for(int x = 0; x < qr.getSize(); x++)
        {
            if(qr.getModule(x, y))
            {
                if(x != 0 || y != 0) { sb << " "; }
                sb << "M" << (x + border) << "," << (y + border) << "h1v1h-1z";
            }
        }
    }
    sb << "\" fill=\"#000000\"/>\n";
    sb << "</svg>\n";
    return sb.str();
}

mrb_value mrb_core_tick(mrb_state* mrb, mrb_value self)
{
    static std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
    auto now = std::chrono::system_clock::now();
    auto temp = start;
    start = now;
    mrb_int argc; mrb_value * argv;
    mrb_get_args(mrb, "*", &argv, &argc);
    if(1 == argc)
    {
        switch(mrb_type(argv[0]))
        {
        case MRB_TT_INTEGER:
            if(0 == mrb_integer(argv[0]))
            {
                auto tick = std::chrono::duration_cast<std::chrono::microseconds>(now-temp).count();
                return mrb_int_value(mrb, tick);
            }
            break;
        case MRB_TT_STRING:
            {
                std::string arg(RSTR_PTR(mrb_str_ptr(argv[0])));
                if(arg == "us")
                {
                    auto tick = std::chrono::duration_cast<std::chrono::microseconds>(now-temp).count();
                    return mrb_int_value(mrb, tick);
                }
            }
            break;
        default:
            break;
        }
    }
    auto tick = std::chrono::duration_cast<std::chrono::milliseconds>(now - temp).count();
    return mrb_int_value( mrb, tick);
}

mrb_value mrb_core_date(mrb_state* mrb, mrb_value self)
{
    std::stringstream date;
    auto now = std::chrono::system_clock::now();
    mrb_int argc; mrb_value * argv;
    mrb_get_args(mrb, "*", &argv, &argc);
#if 0
// MXE Build Error
    if((1 == argc) && (MRB_TT_STRING == mrb_type(argv[0])))
    {
        //std::string time_zone("Asia/Tokyo");
        std::string time_zone(RSTR_PTR(mrb_str_ptr(argv[0])));
        auto tz = std::chrono::locate_zone(time_zone);
        auto ltime = tz->to_local(now);
        std::time_t time = std::chrono::system_clock::to_time_t(tz->to_sys(ltime));
        std::tm * lt = std::localtime(&time);
        uint64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        date << lt->tm_year+1900;
        date << "/" << std::setfill('0') << std::right << std::setw(2) << lt->tm_mon + 1;
        date << "/" << std::setfill('0') << std::right << std::setw(2) << lt->tm_mday;
        date << " " << std::setfill('0') << std::right << std::setw(2) << lt->tm_hour;
        date << ":" << std::setfill('0') << std::right << std::setw(2) << lt->tm_min;
        date << ":" << std::setfill('0') << std::right << std::setw(2) << lt->tm_sec;
        date << "." << std::setfill('0') << std::right << std::setw(3) << (ms % 1000);
        return mrb_str_new_cstr(mrb, (date.str()).c_str());
    }
#endif
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm * lt = std::localtime(&time);
    uint64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    date << lt->tm_year+1900;
    date << "/" << std::setfill('0') << std::right << std::setw(2) << lt->tm_mon + 1;
    date << "/" << std::setfill('0') << std::right << std::setw(2) << lt->tm_mday;
    date << " " << std::setfill('0') << std::right << std::setw(2) << lt->tm_hour;
    date << ":" << std::setfill('0') << std::right << std::setw(2) << lt->tm_min;
    date << ":" << std::setfill('0') << std::right << std::setw(2) << lt->tm_sec;
    date << "." << std::setfill('0') << std::right << std::setw(3) << (ms % 1000);
    return mrb_str_new_cstr(mrb, (date.str()).c_str());
}

mrb_value mrb_core_gets(mrb_state* mrb, mrb_value self)
{
    Application * obj = Application::getObject(mrb);
    if(nullptr != obj)
    {
        auto result = obj->core_gets(mrb, self);
        return result;
    }
    return self;
}

mrb_value mrb_core_exists(mrb_state* mrb, mrb_value self)
{
    mrb_int argc; mrb_value * argv;
    mrb_get_args(mrb, "*", &argv, &argc);
    if(1==argc)
    {
        switch(mrb_type(argv[0]))
        {
        case MRB_TT_STRING:
            {
                std::string fname( RSTR_PTR(mrb_str_ptr(argv[0])) );
                return mrb_bool_value( std::filesystem::exists(fname) );
            }
            break;
        case MRB_TT_ARRAY:
            {
                mrb_value item;
                while( !mrb_nil_p( item = mrb_ary_shift(mrb, argv[0])) )
                {
                    if(MRB_TT_STRING == mrb_type(item))
                    {
                        std::string fname( RSTR_PTR(mrb_str_ptr(item)) );
                        if( !std::filesystem::exists(fname))
                        {
                            return mrb_bool_value(false);
                        }
                    }
                    else
                    {
                        return mrb_bool_value(false);
                    }
                }
                return mrb_bool_value(true);
            }
            break;
        default:
            break;
        }
    }
    for(auto idx=0; idx < argc; idx++)
    {
        if(MRB_TT_STRING == mrb_type(argv[idx]))
        {
            std::string fname( RSTR_PTR(mrb_str_ptr(argv[idx])) );
            if( !std::filesystem::exists(fname))
            {
                return mrb_bool_value(false);
            }
        }
        else
        {
            return mrb_bool_value(false);
        }
    }
    return mrb_bool_value(true);
}

mrb_value mrb_core_file_timestamp(mrb_state* mrb, mrb_value self)
{
    mrb_int argc; mrb_value * argv;
    mrb_get_args(mrb, "*", &argv, &argc);
    if(1 == argc)
    {
        switch(mrb_type(argv[0]))
        {
        case MRB_TT_STRING:
            {
                std::string path( RSTR_PTR(mrb_str_ptr(argv[0])) );
                auto ftime = boost::filesystem::last_write_time(path);
                std::ostringstream timestamp;
                timestamp << ctime(&ftime);
                return mrb_str_new_cstr(mrb, (timestamp.str()).c_str());
            }
            break;
        case MRB_TT_ARRAY:
            {
                mrb_value arry = mrb_ary_new(mrb);
                mrb_value item;
                while( !mrb_nil_p( item = mrb_ary_shift(mrb, argv[0])) )
                {
                    if(MRB_TT_STRING == mrb_type(item))
                    {
                        std::string path( RSTR_PTR(mrb_str_ptr(item)) );
                        auto ftime = boost::filesystem::last_write_time(path);
                        std::ostringstream timestamp;
                        timestamp << ctime(&ftime);
                        mrb_ary_push(mrb, arry, mrb_str_new_cstr(mrb, (timestamp.str()).c_str()));
                    }
                    else
                    {
                        mrb_ary_push(mrb, arry, mrb_nil_value());
                    }

                }
                return arry;
            }
            break;
        default:
            break;
        }
    }
    mrb_value arry = mrb_ary_new(mrb);
    for(auto idx=0; idx < argc; idx++)
    {
        if(MRB_TT_STRING == mrb_type(argv[idx]))
        {
            std::string path( RSTR_PTR(mrb_str_ptr(argv[idx])) );
            auto ftime = boost::filesystem::last_write_time(path);
            std::ostringstream timestamp;
            timestamp << ctime(&ftime);
            mrb_ary_push(mrb, arry, mrb_str_new_cstr(mrb, (timestamp.str()).c_str()));
        }
        else
        {
            mrb_ary_push(mrb, arry, mrb_nil_value());
        }
    }
    return arry;
}

mrb_value mrb_core_make_qr(mrb_state* mrb, mrb_value self)
{
    mrb_int argc; mrb_value * argv;
    mrb_get_args(mrb, "*", &argv, &argc);
    if((1 == argc) && (MRB_TT_STRING == mrb_type(argv[0])))
    {
        std::string arg( RSTR_PTR(mrb_str_ptr(argv[0])) );
        if(0 < arg.size())
        {
            auto qr_code = makeQRsvg(arg);
            return mrb_str_new_cstr(mrb, qr_code.c_str());
        }
    }
    return mrb_nil_value();
}

mrb_value mrb_core_get_args(mrb_state * mrb, mrb_value self)
{
    Application * obj = Application::getObject(mrb);
    if(nullptr != obj)
    {
        auto result = obj->core_get_args(mrb, self);
        return result;
    }
    return self;
}

mrb_value mrb_core_get_opts(mrb_state * mrb, mrb_value self)
{
    Application * obj = Application::getObject(mrb);
    if(nullptr != obj)
    {
        auto result = obj->core_get_opts(mrb, self);
        return result;
    }
    return self;
}

mrb_value mrb_core_prog(mrb_state * mrb, mrb_value self)
{
    Application * obj = Application::getObject(mrb);
    if(nullptr != obj)
    {
        auto result = obj->core_prog(mrb, self);
        return result;
    }
    return mrb_nil_value();
}
static void mrb_core_context_free(mrb_state * mrb, void * ptr)
{
}

mrb_value mrb_bedit_initialize(mrb_state * mrb, mrb_value self)
{
    Application * obj = Application::getObject(mrb);
    if(nullptr != obj)
    {
        auto result = obj->bedit_init(mrb, self);
        return result;
    }
    return mrb_nil_value();
}
mrb_value mrb_bedit_length(mrb_state * mrb, mrb_value self)
{
    BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
    if(nullptr != bedit) { return mrb_int_value(mrb, bedit->size()); }
    return mrb_int_value(mrb, 0);
}
mrb_value mrb_bedit_dump(mrb_state * mrb, mrb_value self)
{
    BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
    if(nullptr != bedit)
    {
        mrb_int address = 0;
        mrb_int size    = bedit->size();
        mrb_int argc; mrb_value * argv; mrb_get_args(mrb, "*", &argv, &argc);
        switch(argc)
        {
        case 1:
            if( MRB_TT_INTEGER == mrb_type(argv[0]) )
            {
                size = mrb_integer(argv[0]);
            }
            break;
        case 2:
            if(   (MRB_TT_INTEGER == mrb_type(argv[0]))     // arg 1: address
               && (MRB_TT_INTEGER == mrb_type(argv[0])) )   // arg 2: size
            {
                address = mrb_integer(argv[0]);
                size    = mrb_integer(argv[1]);
            }
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
mrb_value mrb_bedit_write(mrb_state * mrb, mrb_value self)
{
    BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
    if(nullptr != bedit)
    {
        mrb_int address = 0; mrb_int size = 0; char * data_ptr = nullptr;
        mrb_int argc; mrb_value * argv; mrb_get_args(mrb, "*", &argv, &argc);
        switch(argc)
        {
        case 1:
            if(MRB_TT_STRING == mrb_type(argv[0]))
            {
                data_ptr = RSTR_PTR( mrb_str_ptr(argv[0]) );
            }
            break;
        case 2:
            if(  (MRB_TT_INTEGER == mrb_type(argv[0]))
               &&(MRB_TT_STRING  == mrb_type(argv[1])))
            {
                address = mrb_integer(argv[0]);
                data_ptr = RSTR_PTR( mrb_str_ptr(argv[1]) );
            }
            break;
        case 3:
            if(  (MRB_TT_INTEGER == mrb_type(argv[0]))
               &&(MRB_TT_INTEGER == mrb_type(argv[1]))
               &&(MRB_TT_STRING  == mrb_type(argv[2])))
            {
                address = mrb_integer(argv[0]);
                size    = mrb_integer(argv[1]);
                data_ptr = RSTR_PTR( mrb_str_ptr(argv[2]) );
            }
            break;
        default:
            break;
        }
        if(nullptr != data_ptr)
        {
            std::string data(data_ptr);
            auto wsize = bedit->write(address, size, data);
            return mrb_int_value(mrb, wsize);
        }
    }
    return mrb_int_value(mrb, 0);
}
mrb_value mrb_bedit_memset(mrb_state * mrb, mrb_value self)
{
    BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
    if(nullptr != bedit)
    {
        mrb_int size = 0;
        mrb_int argc; mrb_value * argv; mrb_get_args(mrb, "*", &argv, &argc);
        switch(argc)
        {
        case 1:
            if(MRB_TT_INTEGER == mrb_type(argv[0]))         // arg 1: set data
            {
                size = bedit->memset(0, mrb_integer(argv[0]), bedit->size());
                break;
            }
        case 2:
            if(  (MRB_TT_INTEGER == mrb_type(argv[0]))      // arg 1: set data
               &&(MRB_TT_INTEGER == mrb_type(argv[1])))     // arg 2: set size
            {
                size = bedit->memset(0, mrb_integer(argv[0]), mrb_integer(argv[1]));
                break;
            }
        case 3:
            if(  (MRB_TT_INTEGER == mrb_type(argv[0]))      // arg 1: set start address
               &&(MRB_TT_INTEGER == mrb_type(argv[1]))      // arg 2: set data
               &&(MRB_TT_INTEGER == mrb_type(argv[2])))     // arg 3: set size
            {
                size = bedit->memset(mrb_integer(argv[0]), mrb_integer(argv[1]), mrb_integer(argv[2]));
                break;
            }
        default:
            break;
        }
        return mrb_int_value(mrb, size);
    }
    return mrb_int_value(mrb, 0);
}
mrb_value mrb_bedit_memcpy(mrb_state * mrb, mrb_value self)
{
    BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
    if(nullptr != bedit)
    {
        mrb_int argc; mrb_value * argv; mrb_get_args(mrb, "*", &argv, &argc);
        switch(argc)
        {
        case 1:
            if(MRB_TT_OBJECT == mrb_type(argv[0]))          // arg 1: Source BinEdit
            {
                BinaryControl * src = get_bedit_ptr(argv[0]);
                if(nullptr != src)
                {
                    return mrb_int_value(mrb, bedit->memcpy(0, 0, src->size(), *src));
                }
            }
            break;
        case 2:
            if(  (MRB_TT_INTEGER == mrb_type(argv[0]))      // arg 1: dst start address
               &&(MRB_TT_OBJECT  == mrb_type(argv[1])))     // arg 2: Source BinEdit
            {
                BinaryControl * src = get_bedit_ptr(argv[1]);
                if(nullptr != src)
                {
                    return mrb_int_value(mrb, bedit->memcpy(mrb_integer(argv[0]), 0, src->size(), *src));
                }
            }
            break;
        case 3:
            if(  (MRB_TT_INTEGER == mrb_type(argv[0]))      // arg 1: dst start address
               &&(MRB_TT_INTEGER == mrb_type(argv[1]))      // arg 2: copy size
               &&(MRB_TT_OBJECT  == mrb_type(argv[2])))     // arg 3: Source BinEdit
            {
                BinaryControl * src = get_bedit_ptr(argv[2]);
                if(nullptr != src)
                {
                    return mrb_int_value(mrb, bedit->memcpy(mrb_integer(argv[0]), 0, mrb_integer(argv[1]), *src));
                }
            }
            break;
        case 4:
            if(  (MRB_TT_INTEGER == mrb_type(argv[0]))      // arg 1: dest binary start address
               &&(MRB_TT_INTEGER == mrb_type(argv[1]))      // arg 2: source binary start address
               &&(MRB_TT_INTEGER == mrb_type(argv[2]))      // arg 3: copy size
               &&(MRB_TT_OBJECT  == mrb_type(argv[3])))     // arg 4: Source BinEdit
            {
                BinaryControl * src = get_bedit_ptr(argv[3]);
                if(nullptr != src)
                {
                    return mrb_int_value(mrb, bedit->memcpy(mrb_integer(argv[0]), mrb_integer(argv[1]), mrb_integer(argv[2]), *src));
                }
            }
            break;
        default:
            break;
        }
    }
    return mrb_int_value(mrb, 0);
}
mrb_value mrb_bedit_memcmp(mrb_state * mrb, mrb_value self)
{
    BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
    if(nullptr != bedit)
    {
        mrb_int argc; mrb_value * argv; mrb_get_args(mrb, "*", &argv, &argc);
        switch(argc)
        {
        case 1:
            if(MRB_TT_OBJECT == mrb_type(argv[0]))
            {
                BinaryControl * src = get_bedit_ptr(argv[0]);
                if(nullptr != src)
                {
                    return mrb_int_value(mrb, bedit->memcmp(0, 0, src->size(), *src));
                }
            }
            break;
        default:
            break;
        }
    }
    return mrb_int_value(mrb, -2);
}
mrb_value mrb_bedit_get(mrb_state * mrb, mrb_value self)
{
    BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
    if(nullptr != bedit)
    {
        mrb_int address = bedit->get_pos();
        char *  fmt_ptr = nullptr;
        mrb_int argc; mrb_value * argv; mrb_get_args(mrb, "*", &argv, &argc);
        switch(argc)
        {
        case 1:
            if( MRB_TT_STRING == mrb_type(argv[0]) ) { fmt_ptr = RSTR_PTR(mrb_str_ptr(argv[0])); }  // arg 1: format
            break;
        case 2:
            if(   (MRB_TT_INTEGER == mrb_type(argv[0]))     // arg 1: address
               && (MRB_TT_STRING == mrb_type(argv[1])) )    // arg 2: format
            {
                address = mrb_integer(argv[0]);
                fmt_ptr = RSTR_PTR(mrb_str_ptr(argv[1]));
            }
            break;
        default:
            break;
        }
        if(nullptr != fmt_ptr)
        {
            std::string fmt(fmt_ptr);
            mrb_value arry = mrb_ary_new(mrb);
            if( bedit->get(mrb, address, fmt, arry) )
            {
                struct RArray *rarry = mrb_ary_ptr(arry);
                mrb_int len = ARY_LEN(rarry);
                switch(len)
                {
                case 0:
                    break;
                case 1:
                    return mrb_ary_shift(mrb, arry);
                    break;
                default:
                    return arry;
                }
            }
        }
    }
    return mrb_nil_value();
}
mrb_value mrb_bedit_set(mrb_state * mrb, mrb_value self)
{
    BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
    if(nullptr != bedit)
    {
        mrb_int address = bedit->get_pos();
        char *  fmt_ptr = nullptr;
        mrb_value arry;
        mrb_int argc; mrb_value * argv; mrb_get_args(mrb, "*", &argv, &argc);
        switch(argc)
        {
        case 2:
            if(  (MRB_TT_STRING == mrb_type(argv[0]))
               &&(MRB_TT_ARRAY  == mrb_type(argv[1])))
            {
                fmt_ptr = RSTR_PTR(mrb_str_ptr(argv[0]));
                arry = argv[1];
            }
            break;
        case 3:
            if(  (MRB_TT_INTEGER == mrb_type(argv[0]))
               &&(MRB_TT_STRING  == mrb_type(argv[1]))
               &&(MRB_TT_ARRAY   == mrb_type(argv[2])))
            {
                address = mrb_integer(argv[0]);
                fmt_ptr = RSTR_PTR(mrb_str_ptr(argv[1]));
                arry = argv[2];
            }
            break;
        default:
            break;
        }
        if(nullptr!=fmt_ptr)
        {
            std::string fmt(fmt_ptr);
            return mrb_int_value(mrb, bedit->set(mrb, address, fmt, arry));
        }
    }
    return mrb_nil_value();
}
mrb_value mrb_bedit_pos(mrb_state * mrb, mrb_value self)
{
    BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
    if(nullptr != bedit)
    {
        return mrb_int_value(mrb, bedit->ref_pos());
    }
    return mrb_nil_value();
}
mrb_value mrb_bedit_crc32(mrb_state * mrb, mrb_value self)
{
    BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
    if(nullptr != bedit)
    {
        char temp[9] = { 0 };
        sprintf(temp, "%08X", bedit->crc32());
        return mrb_str_new_cstr(mrb, temp);
    }
    return mrb_nil_value();
}
mrb_value mrb_bedit_crc16(mrb_state * mrb, mrb_value self)
{
    BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
    if(nullptr != bedit)
    {
        char temp[5] = { 0 };
        sprintf(temp, "%04X", bedit->modbus_crc16());
        return mrb_str_new_cstr(mrb, temp);
    }
    return mrb_nil_value();
}
mrb_value mrb_bedit_crc8(mrb_state * mrb, mrb_value self)
{
    BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
    if(nullptr != bedit)
    {
        char temp[3] = { 0 };
        sprintf(temp, "%02X", bedit->crc8());
        return mrb_str_new_cstr(mrb, temp);
    }
    return mrb_nil_value();
}
mrb_value mrb_bedit_sum(mrb_state * mrb, mrb_value self)
{
    BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
    if(nullptr != bedit)
    {
        char temp[3] = { 0 };
        sprintf(temp, "%02X", bedit->sum());
        return mrb_str_new_cstr(mrb, temp);
    }
    return mrb_nil_value();
}
mrb_value mrb_bedit_xsum(mrb_state * mrb, mrb_value self)
{
    BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
    if(nullptr != bedit)
    {
        char temp[3] = { 0 };
        sprintf(temp, "%02X", bedit->xsum());
        return mrb_str_new_cstr(mrb, temp);
    }
    return mrb_nil_value();
}
mrb_value mrb_bedit_compress(mrb_state * mrb, mrb_value self)
{
    BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
    if(nullptr != bedit)
    {
        auto size = bedit->compress();
        return mrb_int_value(mrb, size);
    }
    return mrb_int_value(mrb, 0);
}
mrb_value mrb_bedit_uncompress(mrb_state * mrb, mrb_value self)
{
    BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
    if(nullptr != bedit)
    {
        auto size = bedit->uncompress();
        return mrb_int_value(mrb, size);
    }
    return mrb_int_value(mrb, 0);
}
mrb_value mrb_bedit_save(mrb_state * mrb, mrb_value self)
{
    BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
    if(nullptr != bedit)
    {
        mrb_int argc; mrb_value * argv; mrb_get_args(mrb, "*", &argv, &argc);
        if( (1==argc) && (MRB_TT_STRING == mrb_type(argv[0])) )
        {
            std::string fname( RSTR_PTR( mrb_str_ptr( argv[0] ) ) );
            bedit->saveBinaryFile(fname);
        }
    }
    return self;
}

static mrb_value mrb_cppregexp_reg_match(mrb_state* mrb, mrb_value self)
{
    mrb_value proc; mrb_int argc; mrb_value * argv;
    mrb_get_args(mrb, "&*", &proc, &argv, &argc);
    if((0 < argc) && (MRB_TT_STRING == mrb_type(argv[0])))
    {
        std::string str( RSTR_PTR(mrb_str_ptr(argv[0])) );
        for(auto idx = 1; idx < argc; idx ++)
        {
            switch( mrb_type(argv[idx]) )
            {
            case MRB_TT_STRING:
                {
                    std::regex reg( RSTR_PTR(mrb_str_ptr(argv[idx])) );
                    if(std::regex_search(str, reg))
                    {
                        return mrb_bool_value(true);
                    }
                }
                break;
            case MRB_TT_ARRAY:
                {
                    bool result = true;
                    mrb_value item;
                    while( result && !mrb_nil_p( item = mrb_ary_shift(mrb, argv[idx])) )
                    {
                        if(MRB_TT_STRING == mrb_type(item))
                        {
                            std::regex reg( RSTR_PTR(mrb_str_ptr(item)) );
                            if(std::regex_search(str, reg))
                            {
                                continue;
                            }
                        }
                        result = false;
                    }
                    if(result) { return mrb_bool_value(true); }
                }
                break;
            default:
                break;
            }
        }
    }
    return mrb_bool_value(false);
}
static mrb_value mrb_cppregexp_reg_replace(mrb_state* mrb, mrb_value self)
{
    mrb_int argc; mrb_value * argv;
    mrb_get_args(mrb, "*", &argv, &argc);
    std::string result("");
    if(1 < argc)
    {
        switch(mrb_type(argv[0]))
        {
        case MRB_TT_STRING:
            {
                std::string str(RSTR_PTR(mrb_str_ptr(argv[0])));
                switch(mrb_type(argv[1]))
                {
                case MRB_TT_STRING:
                    {
                        std::regex reg(RSTR_PTR(mrb_str_ptr(argv[1])));
                        if((3 == argc) && (MRB_TT_STRING == mrb_type(argv[2])))
                        {
                            std::string rep(RSTR_PTR(mrb_str_ptr(argv[2])));
                            str = std::regex_replace(str, reg, rep);
                        }
                        return mrb_str_new_cstr(mrb, str.c_str());
                    }
                    break;
                case MRB_TT_ARRAY:
                    {
                        for(auto idx=1; idx < argc; idx++)
                        {
                            if(MRB_TT_ARRAY == mrb_type(argv[idx]))
                            {
                                mrb_value arg1 = mrb_ary_shift(mrb, argv[idx]);
                                mrb_value arg2 = mrb_ary_shift(mrb, argv[idx]);
                                if(  (MRB_TT_STRING == mrb_type(arg1))
                                   &&(MRB_TT_STRING == mrb_type(arg2)))
                                {
                                    std::regex  reg(RSTR_PTR(mrb_str_ptr(arg1)));
                                    std::string rep(RSTR_PTR(mrb_str_ptr(arg2)));
                                    str = std::regex_replace(str, reg, rep);
                                }
                            }
                        }
                        return mrb_str_new_cstr(mrb, str.c_str());
                    }
                    break;
                }
            }
            break;
        case MRB_TT_ARRAY:
            {
                auto arry = mrb_ary_new(mrb);
                mrb_value item;
                switch(mrb_type(argv[1]))
                {
                case MRB_TT_STRING:
                    {
                        std::regex reg(RSTR_PTR(mrb_str_ptr(argv[1])));
                        if((3 == argc) && (MRB_TT_STRING == mrb_type(argv[2])))
                        {
                            std::string rep(RSTR_PTR(mrb_str_ptr(argv[2])));
                            while( !mrb_nil_p( item = mrb_ary_shift(mrb, argv[0])) )
                            {
                                std::string str(RSTR_PTR(mrb_str_ptr(item)));
                                if(MRB_TT_STRING == mrb_type(item))
                                {
                                    str = std::regex_replace(str, reg, rep);
                                    mrb_ary_push(mrb, arry , mrb_str_new_cstr(mrb, str.c_str()));
                                }
                            }
                        }
                    }
                    break;
                case MRB_TT_ARRAY:
                    while( !mrb_nil_p( item = mrb_ary_shift(mrb, argv[0])) )
                    {
                        if(MRB_TT_STRING == mrb_type(item))
                        {
                            std::string str(RSTR_PTR(mrb_str_ptr(item)));
                            for(auto idx=1; idx < argc; idx++)
                            {
                                if(MRB_TT_ARRAY == mrb_type(argv[idx]))
                                {
                                    mrb_value arg1 = mrb_ary_shift(mrb, argv[idx]);
                                    mrb_value arg2 = mrb_ary_shift(mrb, argv[idx]);
                                    if(  (MRB_TT_STRING == mrb_type(arg1))
                                       &&(MRB_TT_STRING == mrb_type(arg2)))
                                    {
                                        std::regex  reg(RSTR_PTR(mrb_str_ptr(arg1)));
                                        std::string rep(RSTR_PTR(mrb_str_ptr(arg2)));
                                        str = std::regex_replace(str, reg, rep);
                                    }
                                }
                            }
                            mrb_ary_push(mrb, arry , mrb_str_new_cstr(mrb, str.c_str()));
                        }
                    }
                    break;
                default:
                    while( !mrb_nil_p( item = mrb_ary_shift(mrb, argv[0])) )
                    {
                        if(MRB_TT_STRING == mrb_type(item))
                        {
                            std::string str(RSTR_PTR(mrb_str_ptr(item)));
                            mrb_ary_push(mrb, arry , mrb_str_new_cstr(mrb, str.c_str()));
                        }
                    }
                    break;
                }
                return arry;
            }
            break;
        default:
            break;
        }
    }
    return self;
}
static mrb_value mrb_cppregexp_reg_split(mrb_state* mrb, mrb_value self)
{
    mrb_value proc; mrb_int argc; mrb_value * argv;
    mrb_get_args(mrb, "&*", &proc, &argv, &argc);
    mrb_value arry = mrb_ary_new(mrb);
    if(    (2 == argc)
        && (mrb_type(argv[0]) == MRB_TT_STRING)
        && (mrb_type(argv[1]) == MRB_TT_STRING) )
    {
        std::string str(RSTR_PTR(mrb_str_ptr(argv[0])));
        std::regex  reg(RSTR_PTR(mrb_str_ptr(argv[1])));
        for(auto && str : str_split(str, reg))
        {
            mrb_ary_push(mrb, arry , mrb_str_new_cstr(mrb, str.c_str()));
        }
    }
    return arry;
}
mrb_value mrb_cppregexp_initialize(mrb_state * mrb, mrb_value self)
{
    Application * obj = Application::getObject(mrb);
    if(nullptr != obj)
    {
        auto result = obj->cppregexp_init(mrb, self);
        return result;
    }
    return mrb_nil_value();
}
mrb_value mrb_cppregexp_length(mrb_state * mrb, mrb_value self)
{
    CppRegexp * reg = static_cast<CppRegexp *>(DATA_PTR(self));
    if(nullptr != reg)
    {
        return mrb_int_value(mrb, reg->length());
    }
    return mrb_int_value(mrb, 0);
}
mrb_value mrb_cppregexp_match(mrb_state * mrb, mrb_value self)
{
    mrb_value result = mrb_ary_new(mrb);
    CppRegexp * reg = static_cast<CppRegexp *>(DATA_PTR(self));
    if(nullptr != reg)
    {
        std::list<std::string> text;
        mrb_int argc; mrb_value * argv;
        mrb_get_args(mrb, "*", &argv, &argc);
        switch(argc)
        {
        case 0:
            break;
        case 1:
            switch(mrb_type(argv[0]))
            {
            case MRB_TT_STRING:
                if( reg->match(RSTR_PTR(mrb_str_ptr(argv[0]))) )
                {
                    return mrb_bool_value(true);
                }
                return mrb_bool_value(false);
            case MRB_TT_ARRAY:
                {
                    mrb_value item;
                    while( !mrb_nil_p( item = mrb_ary_shift(mrb, argv[0])) )
                    {
                        if(MRB_TT_STRING == mrb_type(item))
                        {
                            std::string str( RSTR_PTR( mrb_str_ptr( item ) ) );
                            text.push_back( str );
                        }
                    }
                }
                break;
            default:
                break;
            }
            break;
        default:
            for(auto idx = 0; idx < argc; idx ++)
            {
                if( MRB_TT_STRING == mrb_type( argv[idx] ) )
                {
                    std::string str( RSTR_PTR( mrb_str_ptr( argv[idx] ) ) );
                    text.push_back( str );
                }
            }
            break;
        }
        for( auto & item : reg->match( text ) )
        {
            mrb_ary_push(mrb, result, mrb_str_new_cstr(mrb, item.c_str()));
        }
    }
    return result;
}
mrb_value mrb_cppregexp_grep(mrb_state * mrb, mrb_value self)
{
    mrb_value result = mrb_ary_new(mrb);
    CppRegexp * reg = static_cast<CppRegexp *>(DATA_PTR(self));
    std::list<std::string> text;
    if(nullptr != reg)
    {
        mrb_int argc; mrb_value * argv;
        mrb_get_args(mrb, "*", &argv, &argc);
        switch(argc)
        {
        case 0:
            break;
        case 1:
            switch(mrb_type(argv[0]))
            {
            case MRB_TT_STRING:
                {
                    std::string str( RSTR_PTR( mrb_str_ptr( argv[0] ) ) );
                    text.push_back( str );
                }
                break;
            case MRB_TT_ARRAY:
                {
                    mrb_value item;
                    while( !mrb_nil_p( item = mrb_ary_shift(mrb, argv[0])) )
                    {
                        if(MRB_TT_STRING == mrb_type(item))
                        {
                            std::string str( RSTR_PTR( mrb_str_ptr( item ) ) );
                            text.push_back( str );
                        }
                    }
                }
                break;
            default:
                break;
            }
            break;
        default:
            for(auto idx = 0; idx < argc; idx ++)
            {
                if( MRB_TT_STRING == mrb_type( argv[idx] ) )
                {
                    std::string str( RSTR_PTR( mrb_str_ptr( argv[idx] ) ) );
                    text.push_back( str );
                }
            }
            break;
        }
        for( auto & item : reg->grep( text ) )
        {
            mrb_ary_push(mrb, result, mrb_str_new_cstr(mrb, item.c_str()));
        }
    }
    return result;
}
mrb_value mrb_cppregexp_replace(mrb_state * mrb, mrb_value self)
{
    mrb_value result = mrb_ary_new(mrb);
    CppRegexp * reg = static_cast<CppRegexp *>(DATA_PTR(self));
    if(nullptr != reg)
    {
        std::list<std::string> text;
        char * rep = nullptr;
        mrb_int argc; mrb_value * argv;
        mrb_get_args(mrb, "*", &argv, &argc);
        switch(argc)
        {
        case 0:
        case 1:
            break;
        case 2:
            if(  (MRB_TT_STRING == mrb_type(argv[0]))
               &&(MRB_TT_STRING == mrb_type(argv[1])))
            {
                rep = RSTR_PTR(mrb_str_ptr(argv[0]));
                std::string str( RSTR_PTR( mrb_str_ptr( argv[1] ) ) );
                text.push_back(str);
            }
            break;
        default:
            if(MRB_TT_STRING == mrb_type(argv[0]))
            {
                rep = RSTR_PTR(mrb_str_ptr(argv[0]));
                for(auto idx = 1; idx < argc; idx ++)
                {
                    if(MRB_TT_STRING == mrb_type(argv[idx]))
                    {
                        std::string str( RSTR_PTR(mrb_str_ptr(argv[idx])) );
                        text.push_back(str);
                    }
                }
            }
            break;
        }
        reg->replace(text, rep);
        for( auto & item : text)
        {
            mrb_ary_push(mrb, result, mrb_str_new_cstr(mrb, item.c_str()));
        }
    }
    return result;
}
mrb_value mrb_cppregexp_select(mrb_state * mrb, mrb_value self)
{
    CppRegexp * reg = static_cast<CppRegexp *>(DATA_PTR(self));
    if(nullptr != reg)
    {
        mrb_int argc; mrb_value * argv;
        mrb_get_args(mrb, "*", &argv, &argc);
        switch(argc)
        {
        case 1:
            if(MRB_TT_STRING == mrb_type(argv[0]))
            {
                return mrb_int_value(mrb, reg->select( RSTR_PTR(mrb_str_ptr(argv[0])) ));
            }
            break;
        default:
            break;
        }
    }
    return mrb_nil_value();
}
mrb_value mrb_cppregexp_split(mrb_state * mrb, mrb_value self)
{
    CppRegexp * reg = static_cast<CppRegexp *>(DATA_PTR(self));
    if(nullptr != reg)
    {
        mrb_int argc; mrb_value * argv;
        mrb_get_args(mrb, "*", &argv, &argc);
        switch(argc)
        {
        case 1:
            if(MRB_TT_STRING == mrb_type(argv[0]))
            {
                std::string str( RSTR_PTR(mrb_str_ptr( argv[0] ) ) );
                mrb_value arry = mrb_ary_new(mrb);
                for( auto && str : reg->split(str))
                {
                    mrb_ary_push(mrb, arry , mrb_str_new_cstr(mrb, str.c_str()));
                }
                return arry ;
            }
            break;
        default:
            break;
        }
    }
    return mrb_nil_value();
}

mrb_value mrb_thread_initialize(mrb_state * mrb, mrb_value self)
{
    Application * obj = Application::getObject(mrb);
    if(nullptr != obj)
    {
        auto result = obj->thread_init(mrb, self);
        return result;
    }
    return mrb_nil_value();
}

mrb_value mrb_thread_state(mrb_state * mrb, mrb_value self)
{
    WorkerThread * th_ctrl = static_cast<WorkerThread *>(DATA_PTR(self));
    if(nullptr != th_ctrl) { return mrb_fixnum_value(th_ctrl->get_state()); }
    return mrb_nil_value();
}
mrb_value mrb_thread_start(mrb_state * mrb, mrb_value self)
{
    WorkerThread * th_ctrl = static_cast<WorkerThread *>(DATA_PTR(self));
    if(nullptr != th_ctrl ) { th_ctrl->start(mrb, self); }
    return mrb_nil_value();
}
mrb_value mrb_thread_wait(mrb_state * mrb, mrb_value self)
{
    WorkerThread * th_ctrl = static_cast<WorkerThread *>(DATA_PTR(self));
    if(nullptr != th_ctrl) { th_ctrl->wait(mrb); }
    return mrb_nil_value();
}
mrb_value mrb_thread_notify(mrb_state * mrb, mrb_value self)
{
    mrb_value proc;
    mrb_get_args(mrb, "&", &proc);
    if(!mrb_nil_p(proc))
    {
        WorkerThread * th_ctrl = static_cast<WorkerThread * >(DATA_PTR(self));
        if(nullptr != th_ctrl)
        {
            auto result = th_ctrl->notify(mrb, proc);
            return result;
        }
        th_ctrl->notify();
        return mrb_bool_value(true);
    }
    return mrb_nil_value();
}
mrb_value mrb_thread_sync(mrb_state * mrb, mrb_value self)
{
    mrb_value proc;
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
mrb_value mrb_thread_join(mrb_state * mrb, mrb_value self)
{
    WorkerThread * th_ctrl = static_cast<WorkerThread *>(DATA_PTR(self));
    if(nullptr != th_ctrl) { th_ctrl->join(); }
    return mrb_nil_value();
}
mrb_value mrb_thread_stop(mrb_state * mrb, mrb_value self)
{
    WorkerThread * th_ctrl = static_cast<WorkerThread *>(DATA_PTR(self));
    if(nullptr != th_ctrl) { th_ctrl->stop(mrb); }
    return mrb_nil_value();
}
mrb_value mrb_thread_fifo_push(mrb_state * mrb, mrb_value self)
{
    WorkerThread * th_ctrl = static_cast<WorkerThread *>(DATA_PTR(self));
    if(nullptr != th_ctrl) { return th_ctrl->fifo_push(mrb, self); }
    return mrb_nil_value();
}
mrb_value mrb_thread_fifo_pop(mrb_state * mrb, mrb_value self)
{
    WorkerThread * th_ctrl = static_cast<WorkerThread *>(DATA_PTR(self));
    if(nullptr != th_ctrl) { return th_ctrl->fifo_pop(mrb, self); }
    return mrb_nil_value();
}
mrb_value mrb_thread_fifo_len(mrb_state * mrb, mrb_value self)
{
    WorkerThread * th_ctrl = static_cast<WorkerThread *>(DATA_PTR(self));
    if(nullptr != th_ctrl) { return th_ctrl->fifo_len(mrb, self); }
    return mrb_nil_value();
}
mrb_value mrb_thread_fifo_wait(mrb_state * mrb, mrb_value self)
{
    WorkerThread * th_ctrl = static_cast<WorkerThread *>(DATA_PTR(self));
    if(nullptr != th_ctrl) { return th_ctrl->fifo_wait(mrb, self); }
    return mrb_nil_value();
}
static mrb_value mrb_thread_ms_sleep(mrb_state * mrb, mrb_value self)
{
    mrb_int tick = 0;
    mrb_get_args(mrb, "i", &tick);
    std::this_thread::sleep_for(std::chrono::milliseconds(tick));
    return mrb_nil_value();
}

static mrb_value mrb_smon_comlist(mrb_state* mrb, mrb_value self)
{
    mrb_value arry = mrb_ary_new(mrb);
    ComList com;
    std::list<std::string> list = com.ref();
    mrb_int argc; mrb_value * argv;
    mrb_get_args(mrb, "*", &argv, &argc);
    switch(argc)
    {
    case 1:
        switch(mrb_type(argv[0]))
        {
        case MRB_TT_STRING:
            {
                std::regex reg(RSTR_PTR(mrb_str_ptr(argv[0])));
                for(auto & str : list)
                {
                    if( std::regex_search(str, reg) )
                    {
                        mrb_ary_push(mrb, arry , mrb_str_new_cstr(mrb, str.c_str()));
                    }
                }
            }
            break;
        case MRB_TT_ARRAY:
            {
                std::list<std::regex> regs;
                mrb_value item;
                while( !mrb_nil_p( item = mrb_ary_shift(mrb, argv[0])) )
                {
                    if(MRB_TT_STRING == mrb_type(item))
                    {
                        char * c_str = RSTR_PTR(mrb_str_ptr(item));
                        std::string reg_str(c_str);
                        regs.push_back(std::regex(reg_str, std::regex_constants::icase));
                    }
                }
                list.sort();
                for(auto & str : list)
                {
                    bool match = true;
                    for(auto reg : regs) { if( !std::regex_search(str, reg) ) { match = false; break;; } }
                    if( match ) { mrb_ary_push(mrb, arry , mrb_str_new_cstr(mrb, str.c_str())); }
                }
            }
            break;
        default:
            break;
        }
        break;
    case 2:
        if(  (MRB_TT_INTEGER == mrb_type(argv[0]))
           &&(MRB_TT_ARRAY   == mrb_type(argv[1])))
        {
            auto cnt = mrb_integer(argv[0]);
            std::list<std::regex> regs;
            mrb_value item;
            while( !mrb_nil_p( item = mrb_ary_shift(mrb, argv[1])) )
            {
                if(MRB_TT_STRING == mrb_type(item))
                {
                    char * c_str = RSTR_PTR(mrb_str_ptr(item));
                    std::string reg_str(c_str);
                    regs.push_back(std::regex(reg_str, std::regex_constants::icase));
                }
            }
            list.sort();
            for(auto & str : list)
            {
                if( cnt <= 0) break;
                bool match = true;
                for(auto & reg : regs) { if( !std::regex_search(str, reg) ) { match = false; break;; } }
                if( match ) { mrb_ary_push(mrb, arry , mrb_str_new_cstr(mrb, str.c_str())); cnt --; }
            }
        }
        break;
    default:
        for(auto & str : list)
        {
            mrb_ary_push(mrb, arry , mrb_str_new_cstr(mrb, str.c_str()));
        }
        break;
    }
    return arry ;
}
static mrb_value mrb_smon_pipelist(mrb_state* mrb, mrb_value self)
{
    mrb_value arry = mrb_ary_new(mrb);
    PipeList pipe;
    std::list<std::string> list = pipe.ref();
    mrb_int argc; mrb_value * argv;
    mrb_get_args(mrb, "*", &argv, &argc);
    switch(argc)
    {
    case 1:
        switch(mrb_type(argv[0]))
        {
        case MRB_TT_STRING:
            {
                std::regex reg(RSTR_PTR(mrb_str_ptr(argv[0])));
                for(auto & str : list)
                {
                    if( std::regex_search(str, reg) )
                    {
                        mrb_ary_push(mrb, arry , mrb_str_new_cstr(mrb, str.c_str()));
                    }
                }
            }
            break;
        case MRB_TT_ARRAY:
            {
                std::list<std::regex> regs;
                mrb_value item;
                while( !mrb_nil_p( item = mrb_ary_shift(mrb, argv[0])) )
                {
                    if(MRB_TT_STRING == mrb_type(item))
                    {
                        char * c_str = RSTR_PTR(mrb_str_ptr(item));
                        std::string reg_str(c_str);
                        regs.push_back(std::regex(reg_str, std::regex_constants::icase));
                    }
                }
                list.sort();
                for(auto & str : list)
                {
                    bool match = true;
                    for(auto reg : regs) { if( !std::regex_search(str, reg) ) { match = false; break;; } }
                    if( match ) { mrb_ary_push(mrb, arry , mrb_str_new_cstr(mrb, str.c_str())); }
                }
            }
            break;
        default:
            break;
        }
        break;
    case 2:
        if(  (MRB_TT_INTEGER == mrb_type(argv[0]))
           &&(MRB_TT_ARRAY   == mrb_type(argv[1])))
        {
            auto cnt = mrb_integer(argv[0]);
            std::list<std::regex> regs;
            mrb_value item;
            while( !mrb_nil_p( item = mrb_ary_shift(mrb, argv[1])) )
            {
                if(MRB_TT_STRING == mrb_type(item))
                {
                    char * c_str = RSTR_PTR(mrb_str_ptr(item));
                    std::string reg_str(c_str);
                    regs.push_back(std::regex(reg_str, std::regex_constants::icase));
                }
            }
            list.sort();
            for(auto & str : list)
            {
                if( cnt <= 0) break;
                bool match = true;
                for(auto & reg : regs) { if( !std::regex_search(str, reg) ) { match = false; break;; } }
                if( match ) { mrb_ary_push(mrb, arry , mrb_str_new_cstr(mrb, str.c_str())); cnt --; }
            }
        }
        break;
    default:
        for(auto & str : list)
        {
            mrb_ary_push(mrb, arry , mrb_str_new_cstr(mrb, str.c_str()));
        }
        break;
    }
    return arry ;
}
mrb_value mrb_smon_initialize(mrb_state * mrb, mrb_value self)
{
    Application * obj = Application::getObject(mrb);
    if(nullptr != obj)
    {
        auto result = obj->smon_init(mrb, self);
        return result;
    }
    return mrb_nil_value();
}

mrb_value mrb_smon_send(mrb_state * mrb, mrb_value self)
{
    SerialMonitor * smon = static_cast<SerialMonitor *>(DATA_PTR(self));
    if(nullptr != smon)
    {
        BinaryControl * bin = nullptr;
        std::string data("");
        mrb_int idx = 0, timer = 0;
        mrb_int argc; mrb_value * argv;
        mrb_get_args(mrb, "*", &argv, &argc);
        switch(argc)
        {
        case 1:
            if(MRB_TT_STRING == mrb_type(argv[0])) { data = RSTR_PTR(mrb_str_ptr(argv[0])); }
            else                                   { bin  = get_bedit_ptr(argv[0]);         }
            break;
        case 2:
            if(MRB_TT_INTEGER == mrb_type(argv[0]))
            {
                idx = mrb_integer(argv[0]);
                if(MRB_TT_STRING == mrb_type(argv[1])) { data = RSTR_PTR(mrb_str_ptr(argv[1])); }
                else                                   { bin  = get_bedit_ptr(argv[1]);         }
            }
            else if(MRB_TT_INTEGER == mrb_type(argv[1]))
            {
                if(MRB_TT_STRING == mrb_type(argv[0])) { data = RSTR_PTR(mrb_str_ptr(argv[0])); }
                else                                   { bin  = get_bedit_ptr(argv[0]);         }
                timer = mrb_integer(argv[1]);
            } else { }
            break;
        case 3:
            if(  (MRB_TT_INTEGER == mrb_type(argv[0]))
               &&(MRB_TT_INTEGER == mrb_type(argv[2])))
            {
                idx   = mrb_integer(argv[0]);
                if(MRB_TT_STRING == mrb_type(argv[1])) { data = RSTR_PTR(mrb_str_ptr(argv[1])); }
                else                                   { bin  = get_bedit_ptr(argv[1]);         }
                timer = mrb_integer(argv[2]);
            }
            break;
        default:
            break;
        }
        if(nullptr != bin)
        {
            smon->send(idx, *bin, timer);
        }
        else if(0 < data.size())
        {
            BinaryControl bin(data);
            smon->send(idx, bin, timer);
        }
        else
        {
        }
    }
    return self;
}

mrb_value mrb_smon_read_wait(mrb_state * mrb, mrb_value self)
{
    SerialMonitor * smon = static_cast<SerialMonitor *>(DATA_PTR(self));
    if(nullptr != smon)
    {
        mrb_int argc; mrb_value * argv;
        mrb_get_args(mrb, "*", &argv, &argc);
        if(1==argc)
        {
            BinaryControl * bin = get_bedit_ptr( argv[0] );
            if(nullptr != bin)
            {
                auto state = smon->read_wait(*bin);
                return mrb_fixnum_value(state);
            }
        }
    }
    return mrb_nil_value();
}

mrb_value mrb_smon_close(mrb_state * mrb, mrb_value self)
{
    SerialMonitor * smon = static_cast<SerialMonitor *>(DATA_PTR(self));
    if(nullptr != smon)
    {
        smon->close();
        delete smon;
        DATA_PTR(self) = nullptr;
    }
    return self;
}

mrb_value mrb_smon_timer(mrb_state * mrb, mrb_value self)
{
    SerialMonitor * smon = static_cast<SerialMonitor *>(DATA_PTR(self));
    if(nullptr != smon)
    {
        mrb_int argc; mrb_value * argv;
        mrb_get_args(mrb, "*", &argv, &argc);
        if(  (2==argc)
           &&(MRB_TT_INTEGER == mrb_type(argv[0]))
           &&(MRB_TT_INTEGER == mrb_type(argv[1])))
        {
            mrb_int mode  = mrb_integer(argv[0]);
            mrb_int timer = mrb_integer(argv[1]);
            smon->user_timer(mode, timer);
        }
    }
    return self;
}

static SerialMonitor * get_smon_ptr(mrb_value & argv)
{
    const mrb_data_type * src_type = DATA_TYPE(argv);
    if( src_type == &mrb_smon_context_type )
    {
        SerialMonitor * smon = static_cast<SerialMonitor *>(DATA_PTR(argv));
        return smon;
    }
    return nullptr;
}

mrb_value mrb_xlsx_initialize(mrb_state * mrb, mrb_value self)
{
    Application * obj = Application::getObject(mrb);
    if(nullptr != obj)
    {
        auto result = obj->xlsx_init(mrb, self);
        return result;
    }
    return mrb_nil_value();
}
mrb_value mrb_xlsx_create(mrb_state * mrb, mrb_value self)
{
    mrb_value proc; mrb_int argc; mrb_value * argv;
    mrb_get_args(mrb, "&*", &proc, &argv, &argc);
    switch(argc)
    {
    case 1:
        if( !mrb_nil_p(proc) )
        {
            OpenXLSXCtrl * xlsx = static_cast<OpenXLSXCtrl *>(DATA_PTR(self));
            if(nullptr != xlsx)
            {
                std::string fname( RSTR_PTR( mrb_str_ptr( argv[0] ) ) );
                xlsx->create(fname);
                xlsx->workbook();
                mrb_value result = mrb_yield_argv(mrb, proc, 0, nullptr);
                xlsx->save();
                xlsx->close();
                return result;
            }
        }
        break;
    default:
        break;
    }
    return mrb_nil_value();
}
mrb_value mrb_xlsx_open(mrb_state * mrb, mrb_value self)
{
    mrb_value proc; mrb_int argc; mrb_value * argv;
    mrb_get_args(mrb, "&*", &proc, &argv, &argc);
    if((1 == argc) && (!mrb_nil_p(proc)))
    {
        OpenXLSXCtrl * xlsx = static_cast<OpenXLSXCtrl *>(DATA_PTR(self));
        if(nullptr != xlsx)
        {
            struct RString * s = mrb_str_ptr(argv[0]);
            std::string fname(RSTR_PTR(s));
            xlsx->open(fname);
            xlsx->workbook();
            mrb_value ret = mrb_yield_argv(mrb, proc, 0, nullptr);
            xlsx->close();
            return ret;
        }
    }
    return mrb_nil_value();
}
mrb_value mrb_xlsx_worksheet(mrb_state * mrb, mrb_value self)
{
    OpenXLSXCtrl * xlsx = static_cast<OpenXLSXCtrl *>(DATA_PTR(self));
    if(nullptr != xlsx)
    {
        mrb_value proc; mrb_int argc; mrb_value * argv;
        mrb_get_args(mrb, "&*", &proc, &argv, &argc);
        if((1 == argc) && (MRB_TT_STRING == mrb_type(argv[0])) && (!mrb_nil_p(proc)))
        {
            std::string sheet_name( RSTR_PTR(mrb_str_ptr(argv[0])) );
            xlsx->worksheet(sheet_name);
            return mrb_yield_argv(mrb, proc, 0, nullptr);
        }
    }
    return mrb_nil_value();
}
mrb_value mrb_xlsx_sheet_names(mrb_state * mrb, mrb_value self)
{
    mrb_value arry = mrb_ary_new(mrb);
    OpenXLSXCtrl * xlsx = static_cast<OpenXLSXCtrl *>(DATA_PTR(self));
    if(nullptr != xlsx)
    {
        auto list = xlsx->getWorkSheetNames();
        for(auto & name: list)
        {
            mrb_ary_push(mrb, arry, mrb_str_new_cstr(mrb, name.c_str()));
        }
    }
    return arry;
}
mrb_value mrb_xlsx_set_seet_name(mrb_state * mrb, mrb_value self)
{
    OpenXLSXCtrl * xlsx = static_cast<OpenXLSXCtrl *>(DATA_PTR(self));
    if(nullptr != xlsx )
    {
        mrb_int argc; mrb_value * argv;
        mrb_get_args(mrb, "*", &argv, &argc);
        if( (1 == argc) && (MRB_TT_STRING == mrb_type(argv[0])) )
        {
            std::string sheet_name( RSTR_PTR(mrb_str_ptr(argv[0])) );
            xlsx->set_sheet_name(sheet_name);
        }
    }
    return mrb_nil_value();
}
mrb_value mrb_xlsx_set_value(mrb_state * mrb, mrb_value self)
{
    OpenXLSXCtrl * xlsx = static_cast<OpenXLSXCtrl *>(DATA_PTR(self));
    if(nullptr != xlsx)
    {
        mrb_int argc; mrb_value * argv;
        mrb_get_args(mrb, "*", &argv, &argc);
        if( (2 == argc) && (MRB_TT_STRING == mrb_type(argv[0])) )
        {
            std::string cel( RSTR_PTR(mrb_str_ptr(argv[0])) );
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
mrb_value mrb_xlsx_cell(mrb_state * mrb, mrb_value self)
{
    OpenXLSXCtrl * xlsx = static_cast<OpenXLSXCtrl *>(DATA_PTR(self));
    if(nullptr != xlsx )
    {
        mrb_int argc; mrb_value * argv;
        mrb_get_args(mrb, "*", &argv, &argc);
        if( (1 == argc) && (MRB_TT_STRING == mrb_type(argv[0])) )
        {
            std::string cell_name( RSTR_PTR(mrb_str_ptr(argv[0])) );
            OpenXLSXCtrl::Data val;
            std::string str; val.str = &str;
            auto type = xlsx->get_cell(cell_name, val);
            switch(type)
            {
            case OpenXLSXCtrl::Boolean:
                return mrb_bool_value(val.vint != 0);
            case OpenXLSXCtrl::Integer:
                return mrb_int_value( mrb, val.vint );
            case OpenXLSXCtrl::Float:
                return mrb_float_value( mrb, val.vfloat );
            case OpenXLSXCtrl::String:
                return mrb_str_new_cstr( mrb, str.c_str() );
            case OpenXLSXCtrl::Empty:
            case OpenXLSXCtrl::Error:
            default:
                break;
            }
        }
        else
        {
        }
    }
    return mrb_nil_value();
}
mrb_value mrb_xlsx_save(mrb_state * mrb, mrb_value self)
{
    OpenXLSXCtrl * xlsx = static_cast<OpenXLSXCtrl *>(DATA_PTR(self));
    if(nullptr != xlsx ) { xlsx->save(); }
    return self;
}

void mrb_bedit_context_free(mrb_state * mrb, void * ptr)
{
#if 0
printf("%s:%d: %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
    if(nullptr != ptr)
    {
        BinaryControl * bedit = static_cast<BinaryControl *>(ptr);
        delete bedit;
    }
}
static BinaryControl * get_bedit_ptr(mrb_value & argv)
{
    const mrb_data_type * src_type = DATA_TYPE(argv);
    if( src_type == &mrb_bedit_context_type )
    {
        BinaryControl * bin_src = static_cast<BinaryControl *>(DATA_PTR(argv));
        return bin_src;
    }
    return nullptr;
}
void mrb_regexp_context_free(mrb_state * mrb, void * ptr)
{
    if(nullptr != ptr)
    {
        CppRegexp * cppreg = static_cast<CppRegexp *>(ptr);
        delete cppreg;
    }
}
void mrb_thread_context_free(mrb_state * mrb, void * ptr)
{
#if 0
printf("%s:%d: %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
    if(nullptr != ptr)
    {
        WorkerThread * th_ctrl = static_cast<WorkerThread *>(ptr);
        delete th_ctrl;
    }
}

void mrb_smon_context_free(mrb_state * mrb, void * ptr)
{
#if 0
printf("%s:%d: %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
    if(nullptr != ptr)
    {
        SerialMonitor * smon = static_cast<SerialMonitor *>(ptr);
        delete smon;
    }
}
void mrb_xlsx_context_free(mrb_state * mrb, void * ptr)
{
#if 0
printf("%s:%d: %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
    if(nullptr != ptr)
    {
        OpenXLSXCtrl * xlsx = static_cast<OpenXLSXCtrl *>(ptr);
        delete xlsx;
    }
}

/* -------- << class BinaryControl >>-------- */
BinaryControl::BinaryControl(std::string & data)
  : length(0), compress_size(0), pos(0), data(nullptr)
{
    auto reg_file = std::regex("^file:");
    auto reg_comp = std::regex("^compress:");
    auto reg_text = std::regex("^tx:");
    if(std::regex_search(data, reg_file))
    {   /* file */
        auto fname= std::regex_replace(data, reg_file, "");
        loadBinaryFile(fname);
    }
    else if(std::regex_search(data, reg_comp))
    {   /* compress file */
        auto fname= std::regex_replace(data, reg_comp, "");
        loadBinaryFile(fname);
        chg_compress();
    }
    else if(std::regex_search(data, reg_text))
    {   /* text data */
        data = std::regex_replace(data, reg_text, "");
        alloc(data.size());
        std::memcpy( this->data, data.c_str(), length);
    }
    else
    {
        data = std::regex_replace(data, std::regex("[^0-9a-fA-F]"), "");
        alloc(data.size()/2);
        length = this->write(0, static_cast<mrb_int>(length), data);
    }
}

BinaryControl::~BinaryControl(void)
{
    if(nullptr!=data)
    {
        std::free(data);
    }
    length = compress_size = pos = 0;
    data = nullptr;
}

void BinaryControl::chg_compress(void)
{
    if(0 == compress_size)
    {
        compress_size = length;
        length = 0;
    }
}

void BinaryControl::alloc(size_t size)
{
    length        = size;
    compress_size = 0;
    pos           = 0;
    if( nullptr != data ) { std::free(data); data = nullptr;                        }
    if( 0 < size )        { data = static_cast<unsigned char *>(std::malloc(size)); }
}

size_t BinaryControl::resize(size_t size)
{
    if(length < size)
    {
        auto temp = std::realloc(data, size);
        if(nullptr == temp) { return length; }
        data = static_cast<unsigned char *>(temp);
    }
    this->length = size;
    return this->length;
}

void BinaryControl::clone(mrb_int address, mrb_int size, const BinaryControl & src)
{
    if(nullptr != data) { std::free(data); data = nullptr; }
    if(0 == src.compress_size) { length = size; compress_size = 0;    }
    else                       { length = size; compress_size = size; }
    if(src.size() < size) { size = src.size(); }
    data = static_cast<unsigned char *>(std::malloc(size));
    std::memcpy(data, &(src.data[address]), size);
    pos = 0;
}

size_t BinaryControl::loadBinaryFile(std::string & fname)
{
    std::filesystem::path path(fname);
    auto            file_sz = std::filesystem::file_size(path);
    unsigned char * temp    = static_cast<unsigned char *>(std::malloc(file_sz));
    if((0 < file_sz) && (nullptr != temp))
    {
        if(std::filesystem::exists(fname))
        {
            std::ifstream fin(fname, std::ios::binary);
            if(fin.is_open())
            {
                unsigned char * org_data = data;
                fin.read(reinterpret_cast<char *>(temp), file_sz);
                {
                    data          = temp;
                    length        = file_sz;
                    compress_size = 0;
                }
                if(nullptr != org_data) { std::free(org_data); }
                pos = 0;
                return length;
            }
        }
    }
    return 0;
}

void BinaryControl::saveBinaryFile(std::string & fname)
{
    std::ofstream fout(fname, std::ios::binary);
    fout.write(reinterpret_cast<char *>(data), size());
}

uint32_t BinaryControl::compress(void)
{
    if(nullptr != data)
    {
        if(0 == compress_size)
        {
            auto size_max = LZ4_compressBound(length);
            auto cmp_data = std::malloc(size_max);
            if(nullptr != cmp_data)
            {
                compress_size = LZ4_compress_default(reinterpret_cast<const char *>(data), static_cast<char *>(cmp_data), length, size_max);
                if(compress_size < length)
                {
                    std::free(data);
                    data = static_cast<unsigned char *>(cmp_data);
                    pos = 0;
                    return compress_size;
                }
                std::free(cmp_data);
                compress_size = 0;
            }
        }
    }
    return 0;
}

uint32_t BinaryControl::uncompress(void)
{
    if(nullptr != data)
    {
        if(0 < compress_size)
        {
            if(0 < length)
            {
                auto dst = std::malloc(length);
                if(nullptr != dst)
                {
                    auto size = LZ4_decompress_safe(reinterpret_cast<const char *>(data), reinterpret_cast<char *>(dst), compress_size, length);
                    if(0 < size)
                    {
                        std::free(data);
                        data = static_cast<unsigned char *>(dst);
                        compress_size = 0;
                        length = size;
                        pos = 0;
                        return size;
                    }
                    std::free(dst);
                }
            }
            else
            {
                size_t buff_sz = 256;
                char * buff = static_cast<char *>(std::malloc(buff_sz));
                LZ4_streamDecode_t lz4s = { { 0 } };
                for(auto cnt=0; cnt < 32; cnt ++)
                {
                    auto sz = LZ4_decompress_safe(reinterpret_cast<const char *>(data), buff, compress_size, buff_sz);
                    if(0 < sz)
                    {
                        length = sz;
                        compress_size = 0;
                        std::free(data);
                        data = reinterpret_cast<unsigned char *>(buff);
                        break;
                    }
                    buff_sz <<= 1;
                    auto temp = std::realloc(buff, buff_sz);
                    if(nullptr == temp) { break; }
                    buff = static_cast<char *>(temp);
                }
            }
        }
    }
    return 0;
}

uint32_t BinaryControl::memset(mrb_int address, mrb_int set_data, mrb_int sz)
{
    uint32_t len = 0;
    auto size = this->size();
    if(address < size)
    {
        len = address - size;
        if(sz < len) { len = sz; }
        std::memset(&(data[address]), static_cast<uint8_t>(set_data), len);
    }
    return len;
}

uint32_t BinaryControl::memcpy(mrb_int address_dst, mrb_int address_src, mrb_int len, BinaryControl & src)
{
    auto dst_len = size()     - address_dst;
    auto src_len = src.size() - address_src;
    if(dst_len < len) { len = dst_len; }
    if(src_len < len) { len = src_len; }
    if(0 < len)
    {
        std::memcpy(&(data[address_dst]), &(src.data[address_src]), len);
    }
    return len;
}

int32_t BinaryControl::memcmp(mrb_int address_dst, mrb_int address_src, mrb_int len, BinaryControl & src)
{
    auto dst_len = size()     - address_dst;
    auto src_len = src.size() - address_src;
    if(dst_len < len) { len = dst_len; }
    if(src_len < len) { len = src_len; }
    if(0 < len)
    {
        return std::memcmp(&(data[address_dst]), &(src.data[address_src]), len);
    }
    return -2;
}

uint32_t BinaryControl::write(mrb_int address, mrb_int size, std::string & data)
{
    size_t cnt = 0;
    auto length = this->size();
    if(address < length)
    {
        length -= address;
        if(length < size) { size = length; }
        if(0 < size)
        {
            const unsigned char * str = reinterpret_cast<const unsigned char *>(data.c_str());
            unsigned char val = 0;
            auto str_len = data.size();
            for(auto idx = 0, len = 0; (cnt < size) && (idx < str_len); idx ++)
            {
                unsigned char tmp = toValue(str[idx]);
                if(tmp <= 0x0F)
                {
                    val = (val << 4) | tmp;
                    len ++;
                    if(0 == (len & 1))
                    {
                        this->data[address + cnt] = val;
                        cnt ++;
                    }
                }
            }
        }
    }
    return cnt;
}

uint32_t BinaryControl::write_big(mrb_int address, mrb_int size, std::string & src)
{
    size_t cnt = 0;
    auto length = this->size();
    if(address < length)
    {
        length -= address;
        if(length < size) { size = length; }
        if(0 < size)
        {
            unsigned char * data = static_cast<unsigned char *>(std::malloc(size));
            const unsigned char * str = reinterpret_cast<const unsigned char *>(src.c_str());
            unsigned char val = 0;
            auto str_len = src.size();
            for(auto idx = 0, len = 0; (cnt < size) && (idx < str_len); idx ++)
            {
                unsigned char tmp = toValue(str[idx]);
                if(tmp <= 0x0F)
                {
                    val = (val << 4) | tmp;
                    len ++;
                    if(0 == (len & 1))
                    {
                        data[cnt] = val;
                        cnt ++;
                    }
                }
            }
            for(size_t idx = 0, top = cnt - 1; idx < cnt; idx ++)
            {
                this->data[address + (top - idx)] = data[idx];
            }
            std::free(data);
        }
    }
    return cnt;
}

uint32_t BinaryControl::dump(mrb_int address, mrb_int size, std::string & output)
{
    uint32_t cnt = 0;
    auto length = this->size();
    if(address < length)
    {
        length -= address;
        if(length < size) { size = length; }
        if(0 < size)
        {
            for( ; (cnt < size); cnt ++)
            {
                char temp[4] = { 0 };
                sprintf(temp, "%02X", data[address + cnt]);
                output += temp;
            }
        }
    }
    return cnt;
}

uint32_t BinaryControl::dump_big(mrb_int address, mrb_int size, std::string & output)
{
    uint32_t cnt = 0;
    auto length = this->size();
    if(address < length)
    {
        length -= address;
        if(length < size) { size = length; }
        if(0 < size)
        {
            auto top = size - 1;
            for( ; (cnt < size); cnt ++)
            {
                char temp[4] = { 0 };
                sprintf(temp, "%02X", data[address + (top - cnt)]);
                output += temp;
            }
        }
    }
    return cnt;
}

bool BinaryControl::get(mrb_state * mrb, uint32_t address, std::string & format_, mrb_value & array)
{
    auto state = ' ';
    std::string num;
    auto format = format_ + ' ';
    for(auto t: format)
    {
        auto length = this->size();
        if(('0' <= t) && (t <= '9'))
        {
            num += t;
        }
        else
        {
            switch(state)
            {
            case 'a':
                {
                    size_t size = stoi(num);
                    if(address < length)
                    {
                        length -= address;
                        if(length < size) { size = length; }
                        if(0 < size)
                        {
                            char str[size + 1];
                            str[size] = '\0';
                            std::memcpy(str, &(data[address]), size);
                            mrb_ary_push(mrb, array, mrb_str_new_cstr(mrb, str));
                            address += size;
                        }
                    }
                }
                break;
            case 'A':
                {
                    size_t size = stoi(num);
                    if(address < length)
                    {
                        length -= address;
                        if(length < size) { size = length; }
                        if(0 < size)
                        {
                            char str[size + 1];
                            str[size] = '\0';
                            for(size_t idx = 0, top = size - 1; idx < size; idx ++)
                            {
                                str[top - idx] = data[address + idx];
                            }
                            mrb_ary_push(mrb, array, mrb_str_new_cstr(mrb, str));
                            address += size;
                        }
                    }
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
            case 'H':
                {
                    mrb_int addr = address;
                    mrb_int size = static_cast<unsigned int>(stoi(num));
                    std::string hex;
                    dump_big(addr, size, hex);
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
            case 'w': {  Word temp; std::memcpy(&(temp.buff[0]), &(data[address]), 2); mrb_ary_push(mrb, array, mrb_int_value(mrb, temp.uint16)); address += 2; } break;
            case 's': {  Word temp; std::memcpy(&(temp.buff[0]), &(data[address]), 2); mrb_ary_push(mrb, array, mrb_int_value(mrb, temp.int16));  address += 2; } break;
            case 'd': { DWord temp; std::memcpy(&(temp.buff[0]), &(data[address]), 4); mrb_ary_push(mrb, array, mrb_int_value(mrb, temp.uint32));  address += 4; } break;
            case 'i': { DWord temp; std::memcpy(&(temp.buff[0]), &(data[address]), 4); mrb_ary_push(mrb, array, mrb_int_value(mrb, temp.int32));   address += 4; } break;
            case 'f': { DWord temp; std::memcpy(&(temp.buff[0]), &(data[address]), 4); mrb_ary_push(mrb, array, mrb_float_value(mrb, temp.value)); address += 4; } break;

            case 'W': { Word temp;  for(auto idx = 0; idx < 2; idx ++) { temp.buff[idx] = data[address + (1-idx)]; } mrb_ary_push(mrb, array, mrb_int_value(mrb, temp.uint16));  address += 2; } break;
            case 'S': { Word temp;  for(auto idx = 0; idx < 2; idx ++) { temp.buff[idx] = data[address + (1-idx)]; } mrb_ary_push(mrb, array, mrb_int_value(mrb, temp.int16));   address += 2; } break;
            case 'D': { DWord temp; for(auto idx = 0; idx < 4; idx ++) { temp.buff[idx] = data[address + (3-idx)]; } mrb_ary_push(mrb, array, mrb_int_value(mrb, temp.uint32));  address += 4; } break;
            case 'I': { DWord temp; for(auto idx = 0; idx < 4; idx ++) { temp.buff[idx] = data[address + (3-idx)]; } mrb_ary_push(mrb, array, mrb_int_value(mrb, temp.int32));   address += 4; } break;
            case 'F': { DWord temp; for(auto idx = 0; idx < 4; idx ++) { temp.buff[idx] = data[address + (3-idx)]; } mrb_ary_push(mrb, array, mrb_float_value(mrb, temp.value)); address += 4; } break;

            case 'A': state = t; break;
            case 'a': state = t; break;
            case 'H': state = t; break;
            case 'h': state = t; break;
            default:
                break;
            }
        }
    }
    pos = address;
    return true;
}

mrb_int BinaryControl::set(mrb_state * mrb, uint32_t address, std::string & format_, mrb_value & array)
{
    mrb_int size = 0;
    auto format = format_ + ' ';
    for(auto t: format)
    {
        auto length = this->size();
        switch(t)
        {
        case 'c': { DWord temp; temp.val    = static_cast< int8_t >(mrb_integer(mrb_ary_shift(mrb, array))); data[address] = temp.data; address += 1; size += 1; } break;
        case 'b': { DWord temp; temp.data   = static_cast<uint8_t >(mrb_integer(mrb_ary_shift(mrb, array))); data[address] = temp.data; address += 1; size += 1; } break;
        case 's': { DWord temp; temp.int16  = static_cast< int16_t>(mrb_integer(mrb_ary_shift(mrb, array))); std::memcpy(&(data[address]), &(temp.buff[0]), 2); address += 2; size += 2; } break;
        case 'w': { DWord temp; temp.uint16 = static_cast<uint16_t>(mrb_integer(mrb_ary_shift(mrb, array))); std::memcpy(&(data[address]), &(temp.buff[0]), 2); address += 2; size += 2; } break;
        case 'i': { DWord temp; temp.int32  = static_cast< int32_t>(mrb_integer(mrb_ary_shift(mrb, array))); std::memcpy(&(data[address]), &(temp.buff[0]), 4); address += 4; size += 4; } break;
        case 'd': { DWord temp; temp.uint32 = static_cast<uint32_t>(mrb_integer(mrb_ary_shift(mrb, array))); std::memcpy(&(data[address]), &(temp.buff[0]), 4); address += 4; size += 4; } break;
        case 'f': { DWord temp; temp.value  = static_cast<float>(mrb_float(mrb_ary_shift(mrb, array)));      std::memcpy(&(data[address]), &(temp.buff[0]), 4); address += 4; size += 4; } break;

        case 'S': { DWord temp; temp.int16  = static_cast< int16_t>(mrb_integer(mrb_ary_shift(mrb, array))); for(auto idx = 0; idx < 2; idx ++) { data[address + (1-idx)] = temp.buff[idx]; } address += 2; size += 2; } break;
        case 'W': { DWord temp; temp.uint16 = static_cast<uint16_t>(mrb_integer(mrb_ary_shift(mrb, array))); for(auto idx = 0; idx < 2; idx ++) { data[address + (1-idx)] = temp.buff[idx]; } address += 2; size += 2; } break;
        case 'I': { DWord temp; temp.int32  = static_cast< int32_t>(mrb_integer(mrb_ary_shift(mrb, array))); for(auto idx = 0; idx < 4; idx ++) { data[address + (3-idx)] = temp.buff[idx]; } address += 4; size += 4; } break;
        case 'D': { DWord temp; temp.uint32 = static_cast<uint32_t>(mrb_integer(mrb_ary_shift(mrb, array))); for(auto idx = 0; idx < 4; idx ++) { data[address + (3-idx)] = temp.buff[idx]; } address += 4; size += 4; } break;
        case 'F': { DWord temp; temp.value  = static_cast<float>(mrb_float(mrb_ary_shift(mrb, array)));      for(auto idx = 0; idx < 4; idx ++) { data[address + (3-idx)] = temp.buff[idx]; } address += 4; size += 4; } break;

        case 'a':
            {
                auto item = mrb_ary_shift(mrb, array);
                if(address < length)
                {
                    length -= address;
                    char * msg = RSTR_PTR(mrb_str_ptr(item));
                    size_t msg_sz = strlen(msg);
                    if(length < msg_sz) { msg_sz = length; }
                    std::memcpy(&(data[address]), msg, msg_sz);
                    address += msg_sz;
                    size += msg_sz;
                }
            }
            break;
        case 'A':
            {
                auto item = mrb_ary_shift(mrb, array);
                if(address < length)
                {
                    char * msg = RSTR_PTR(mrb_str_ptr(item));
                    size_t msg_sz = strlen(msg);
                    for(size_t idx = 0, top = msg_sz - 1; idx < msg_sz; idx ++)
                    {
                        data[address + idx] = msg[top - idx];
                    }
                    address += msg_sz;
                    size += msg_sz;
                }
            }
            break;
        case 'H':
            {
                auto item = mrb_ary_shift(mrb, array);
                char * msg = RSTR_PTR(mrb_str_ptr(item));
                std::string data(msg);
                auto w_size = write_big(address, (data.size() / 2), data);
                address += w_size;
                size += w_size;
            }
            break;
        case 'h':
            {
                auto item = mrb_ary_shift(mrb, array);
                char * msg = RSTR_PTR(mrb_str_ptr(item));
                std::string data(msg);
                auto w_size = write(address, (data.size() / 2), data);
                address += w_size;
                size += w_size;
            }
            break;
        default:
            break;
        }
    }
    pos = address;
    return size;
}

unsigned short BinaryControl::modbus_crc16(void)
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
    MyEntity::CalcCRC16 crc(modbusCRC);
    for(size_t idx = 0; idx < length; idx ++)
    {
        crc << data[idx];
    }
    return *crc;
}

unsigned char BinaryControl::crc8(void)
{
    static const unsigned char crctab8[16] = { 0x00,0x9B,0xAD,0x36,0xC1,0x5A,0x6C,0xF7,0x19,0x82,0xB4,0x2F,0xD8,0x43,0x75,0xEE };
    unsigned char crc = 0;
    unsigned char high = 0;
    for(unsigned int idx = 0; idx < length; idx ++)
    {
        unsigned char data = this->data[idx];
        high = crc >> 4;
        crc <<= 4;
        crc ^= crctab8[ high ^ (data >> 4) ];

        high = crc >> 4;
        crc <<= 4;
        crc ^= crctab8[ high ^ (data & 0x0f) ];
    }
    return crc;
}

uint32_t BinaryControl::crc32(void)
{
    static const uint32_t tbl_crc32[256] =
    {
        0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
        0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
        0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
        0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
        0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433, 0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
        0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
        0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
        0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f, 0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
        0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
        0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
        0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b, 0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
        0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
        0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
        0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777, 0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
        0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
        0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
    };
    uint32_t crc = 0xFFFFFFFF;
    for(unsigned int idx = 0; idx < length; idx ++)
    {
        crc = tbl_crc32[(crc ^ this->data[idx]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

unsigned char BinaryControl::sum(void)
{
    unsigned char sum = 0;
    for(unsigned int idx = 0; idx < length; idx ++)
    {
        sum += this->data[idx];
    }
    sum = ~sum;
    sum += 1;
    return sum;
}

unsigned char BinaryControl::xsum(void)
{
    unsigned char sum = 0;
    for(unsigned int idx = 0; idx < length; idx ++)
    {
        sum ^= this->data[idx];
    }
    return sum;
}

/* -------- << class CppRegexp >>-------- */
CppRegexp::~CppRegexp(void)
{
}
bool CppRegexp::match(const std::string & str)
{
    for(auto & reg : regs)
    {
        if( std::regex_search(str, reg) )
        {
            return true;
        }
    }
    return false;
}
std::list<std::string> CppRegexp::match(std::list<std::string> & text)
{
    std::list<std::string> result;
    for(auto & str : text)
    {
        if( match( str ) )
        {
            result.push_back( str );
        }
    }
    return result;
}
std::list<std::string> CppRegexp::grep(std::list<std::string> & text)
{
    std::list<std::string> result;
    if(0 < regs.size())
    {
        for(auto & str : text)
        {
            bool match = true;
            for(auto & reg : regs)
            {
                if( !std::regex_search(str, reg) )
                {
                    match = false;
                    break;
                }
            }
            if( match )
            {
                result.push_back( str );
            }
        }
    }
    return result;
}
void CppRegexp::replace(std::list<std::string> & text, const char * rep)
{
    if( rep != nullptr )
    {
        for(auto & str : text)
        {
            for(auto & reg : regs)
            {
                str = std::regex_replace(str, reg, rep);
            }
        }
    }
}
unsigned int CppRegexp::select(const char * str)
{
    if(0 < regs.size())
    {
        unsigned int idx = 0;
        for(auto & reg : regs)
        {
            if( std::regex_search(str, reg) )
            {
                break;
            }
            idx ++;
        }
        return idx;
    }
    return 1;
}
std::list<std::string> CppRegexp::split(std::string & org)
{
    std::list<std::string> result;
    result.push_back( org );
    for(auto & reg : regs)
    {
        std::list<std::string> temp;
        for(auto str : result)
        {
            for(auto & item : str_split(str, reg))
            {
                temp.push_back(item);
            }
        }
        result.clear();
        result = temp;
    }
    return result;
}

/* -------- << class WorkerThread >>-------- */
WorkerThread::~WorkerThread(void)
{
    std::lock_guard<std::mutex> lock(mtx);
    if(nullptr != this->mrb)
    {
        mrb_close(this->mrb);
        this->mrb = nullptr;
    }
}
bool WorkerThread::start(mrb_state * mrb, mrb_value self)
{
    bool result = false;
    std::unique_lock<std::mutex> lock(mtx);
    if(nullptr == this->mrb)
    {
        mrb_int argc; mrb_value * argv;
        mrb_get_args(mrb, "&*", &proc, &argv, &argc);
        if(!mrb_nil_p(proc))
        {
            this->loop_cnt = 0;
            if((1==argc) && (MRB_TT_INTEGER == mrb_type(argv[0])))
            {
                loop_cnt = mrb_integer(argv[0]);
            }
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
            result = true;
        }
    }
    return result;
}
void WorkerThread::run_context(size_t id)
{
    std::unique_lock<std::mutex> lock(mtx);
    this->mrb = mrb_open();
    if(nullptr != this->mrb)
    {
        state = Run;
        cond.notify_all();

        size_t cnt = 0;
        while((Run == state) &&((0==loop_cnt) || (cnt < loop_cnt)))
        {
            auto lamda = [this]
            {
                if(Run      == state) { return true; }
                if(WaitStop == state) { return true; }
                return false;
            };
            mrb_value argv[1];
            argv[0] = mrb_int_value(mrb, cnt);
            cond.wait(lock, lamda);
            lock.unlock();
            mrb_yield_argv(this->mrb, proc, 1, argv);
            lock.lock();
            cnt ++;
        }
        mrb_close(this->mrb);
        this->mrb = nullptr;
    }
    state = Stop;
    cond.notify_all();
    th_ctrl.detach();
}
void WorkerThread::join(void)
{
    auto lamda = [this]
    {
        if(Stop == state) { return true; }
        return false;
    };
    std::unique_lock<std::mutex> lock(mtx);
    cond.wait(lock, lamda);
}
void WorkerThread::wait(mrb_state * mrb)
{
    auto lamda = [this, &mrb]
    {
        if(Run      == state) { return true; }
        if(WaitStop == state) { return true; }
        if(Stop     == state) { return true; }
        return false;
    };
    std::unique_lock<std::mutex> lock(mtx);
    state = Wait;
    cond.wait(lock, lamda);
    mrb_value proc; mrb_get_args(this->mrb, "&", &proc);
    if(!mrb_nil_p(proc))
    {
        mrb_yield_argv(mrb, proc, 0, NULL);
    }
}
void WorkerThread::stop(mrb_state * mrb)
{
    std::lock_guard<std::mutex> lock(mtx);
    switch(state)
    {
    case Wakeup:
    case Run:
    case Wait:
        state = WaitStop;
        break;
    case WaitStop:
        break;
    case Stop:
    default:
        state = Stop;
        break;
    }
    cond.notify_all();
}
mrb_value WorkerThread::fifo_push(mrb_state * mrb, mrb_value self)
{
    FifoItem item; item.len = 0; item.data = nullptr;
    mrb_int argc; mrb_value * argv;
    mrb_get_args(mrb, "*", &argv, &argc);
    switch(argc)
    {
    case 1:
        if(MRB_TT_STRING == mrb_type(argv[0]))
        {
            std::string str(RSTR_PTR(mrb_str_ptr(argv[0])));
            item.str  = str;
            std::lock_guard<std::mutex> lock(mtx_fifo);
            fifo.push_back(item);
            cond_fifo.notify_all();
            return mrb_bool_value(true);
        }
        else
        {
            BinaryControl * bin = get_bedit_ptr(argv[0]);
            if(nullptr != bin)
            {
                item.data = bin->ptr();
                item.len  = bin->size();
                bin->attach(nullptr, 0);
                std::lock_guard<std::mutex> lock(mtx_fifo);
                fifo.push_back(item);
                cond_fifo.notify_all();
                return mrb_bool_value(true);
            }
        }
        break;
    case 2:
        if(MRB_TT_STRING == mrb_type(argv[0]))
        {
            BinaryControl * bin = get_bedit_ptr(argv[1]);
            if(nullptr != bin)
            {
                std::string str(RSTR_PTR(mrb_str_ptr(argv[0])));
                item.str = str;
                bin->attach(item.data, item.len);
                std::lock_guard<std::mutex> lock(mtx_fifo);
                fifo.push_back(item);
                cond_fifo.notify_all();
                return mrb_bool_value(true);
            }
        }
        break;
    default:
        break;
    }
    return mrb_bool_value(false);
}
mrb_value WorkerThread::fifo_pop(mrb_state * mrb, mrb_value self)
{
    mrb_int argc; mrb_value * argv;
    mrb_get_args(mrb, "*", &argv, &argc);
    if(1==argc)
    {
        BinaryControl * bin = get_bedit_ptr(argv[0]);
        if(nullptr != bin)
        {
            std::lock_guard<std::mutex> lock(mtx_fifo);
            cond_fifo.notify_all();
            if(0<fifo.size())
            {
                auto item = *(fifo.begin());
                fifo.pop_front();
                if(0 < item.len)
                {
                    bin->attach(item.data, item.len);
                }
                if(0 < item.str.size())
                {
                    return mrb_str_new_cstr(mrb, item.str.c_str());
                }
                cond_fifo.notify_all();
            }
        }
    }
    return mrb_str_new_cstr(mrb, "");
}
mrb_value WorkerThread::fifo_len(mrb_state * mrb, mrb_value self)
{
    std::lock_guard<std::mutex> lock(mtx_fifo);
    auto len = fifo.size();
    return mrb_int_value(mrb, len);
}
mrb_value WorkerThread::fifo_wait(mrb_state * mrb, mrb_value self)
{
    auto lamda = [&]
    {
        if(0<fifo.size()) { return true; }
        return false;
    };
    std::unique_lock<std::mutex> lock(mtx_fifo);
    cond_fifo.wait(lock, lamda);
    auto len = fifo.size();
    return mrb_int_value(mrb, len);
}


/* -------- << class SerialMonitor >>-------- */
SerialMonitor::SerialMonitor(mrb_value & self, std::list<std::string> & ports, std::vector<size_t> & timer_default)
  : com(ports.size()), oneshot(ports.size()), cache(ports.size()), sync(nullptr), cache_size(1024), rcv_enable(true)
{
    for(size_t idx=0; idx < cache.size(); idx++)
    {
        cache[idx] = {idx, NONE, 0, nullptr };
    }
    SerialControl::Profile info = { 1200, SerialControl::odd, SerialControl::one, true };
    std::vector<std::regex> regs;
    std::vector<std::function<void(std::string&)>> act;
    regs.push_back(std::regex("one",  std::regex::icase));
    regs.push_back(std::regex("two",  std::regex::icase));
    regs.push_back(std::regex("none", std::regex::icase));
    regs.push_back(std::regex("even", std::regex::icase));
    regs.push_back(std::regex("odd",  std::regex::icase));
    regs.push_back(std::regex("GAP=[0-9]+", std::regex::icase));
    regs.push_back(std::regex("TO1=[0-9]+", std::regex::icase));
    regs.push_back(std::regex("TO2=[0-9]+", std::regex::icase));
    regs.push_back(std::regex("TO3=[0-9]+", std::regex::icase));
    regs.push_back(std::regex("rts",  std::regex::icase));
    regs.push_back(std::regex("[0-9]+", std::regex::icase));
    act.push_back( [&](std::string & arg) { info.stop = SerialControl::one;    } );
    act.push_back( [&](std::string & arg) { info.stop = SerialControl::two;    } );
    act.push_back( [&](std::string & arg) { info.parity = SerialControl::none; } );
    act.push_back( [&](std::string & arg) { info.parity = SerialControl::even; } );
    act.push_back( [&](std::string & arg) { info.parity = SerialControl::odd;  } );
    act.push_back( [&](std::string & arg) { timer[0] = stoi(std::regex_replace(arg, std::regex("GAP=([0-9]+)", std::regex::icase), "$1")); } );
    act.push_back( [&](std::string & arg) { timer[1] = stoi(std::regex_replace(arg, std::regex("TO1=([0-9]+)", std::regex::icase), "$1")); } );
    act.push_back( [&](std::string & arg) { timer[2] = stoi(std::regex_replace(arg, std::regex("TO2=([0-9]+)", std::regex::icase), "$1")); } );
    act.push_back( [&](std::string & arg) { timer[3] = stoi(std::regex_replace(arg, std::regex("TO3=([0-9]+)", std::regex::icase), "$1")); } );
    act.push_back( [&](std::string & arg) { info.rtsctrl = (std::regex_match(arg, std::regex("rts=false", std::regex::icase)) ? false : true); } );
    act.push_back( [&](std::string & arg) { info.baud = stoi(arg); } );
    std::regex reg(",");
    std::unique_lock<std::mutex> lock(mtx);
    size_t idx = 0;
    for(auto & port : ports)
    {
        timer = timer_default;
        auto args = str_split( port, reg);
        auto name = *(args.begin());
        args.pop_front();
        for(auto arg : args)
        {
            for(size_t idx=0, max=regs.size(); idx < max; idx++)
            {
                if(std::regex_match(arg, regs[idx]))
                {
                    (act[idx])(arg);
                    break;
                }
            }
        }
        com[idx] = SerialControl::createObject(name.c_str(), info);
        cache[idx].buff  = static_cast<unsigned char *>(std::malloc(cache_size));
        cache[idx].cnt   = 0;
        cache[idx].state = WAKEUP;
        std::thread th(&SerialMonitor::reciver, this, idx);
        th.detach();
        auto lamda = [&]
        {
            if(cache[idx].state != WAKEUP)
            {
                return true;
            }
            return false;
        };
        cond.wait(lock, lamda);
        idx ++;
    }
}
SerialMonitor::~SerialMonitor(void)
{
    this->close();
}
void SerialMonitor::close(void)
{
    bool is_open = false;
    std::unique_lock<std::mutex> lock(mtx);
    rcv_enable = false;
    try
    {
        for(auto idx=0; idx < com.size(); idx ++)
        {
            if(nullptr != com[idx])
            {
                com[idx]->close();
                is_open = true;
            }
        }
        if(is_open)
        {
            auto lamda = [this]
            {
                for(auto & cache : this->cache)
                {
                    if(cache.state != CLOSE)
                    {
                        return false;
                    }
                }
                return true;
            };
            cond.wait(lock, lamda);
            for(auto idx=0; idx < com.size(); idx ++)
            {
                delete com[idx];
                com[idx] = nullptr;
            }
            for(auto idx=0; idx < cache.size(); idx ++)
            {
                if(nullptr != cache[idx].buff)
                {
                    std::free(cache[idx].buff);
                    cache[idx].buff = nullptr;
                }
                cache[idx].cnt   = 0;
            }
            for(auto & item : rcv_cache)
            {
                std::free(item.buff);
                item.buff = nullptr;
            }
            rcv_cache.clear();
        }
    } catch(...) { }
}
void SerialMonitor::send(size_t idx, BinaryControl & bin, uint32_t timer)
{
    if( nullptr != com[idx])
    {
        if(0 < bin.size())
        {
            try
            {
                std::lock_guard<std::mutex> lock(mtx);
                com[idx]->send(bin.ptr(), bin.size());
                std::this_thread::sleep_for(std::chrono::milliseconds(timer));
                if(nullptr != oneshot[idx])
                {
                    oneshot[idx]->restart();
                }
            } catch(...) { }
        }
    }
}
SerialMonitor::ReciveInfo SerialMonitor::read(BinaryControl & bin)
{
    ReciveInfo info = { 0, NONE, 0, nullptr };
    std::lock_guard<std::mutex> lock(mtx);
    if(0 < rcv_cache.size())
    {
        info = *(rcv_cache.begin());
        rcv_cache.pop_front();
        if(0 < info.cnt)
        {
            auto data = bin.ptr();
            if(nullptr != data) { std::free(data); }
            bin.attach(info.buff, info.cnt);
        }
        else
        {
            bin.resize(0);
        }
    }
    return info;
}
SerialMonitor::State SerialMonitor::read_wait(BinaryControl & bin)
{
    ReciveInfo rcv_info = { 0, NONE, 0, nullptr };
    auto lamda = [&]
    {
        for(size_t idx = 0; idx < cache.size(); idx ++)
        {
            if(cache[idx].state == CLOSE)
            {
                return true;
            }
        }
        if(0 < rcv_cache.size())
        {
            return true;
        }
        return false;
    };
    std::unique_lock<std::mutex> lock(mtx);
    cond.wait(lock, lamda);
    rcv_info = *(rcv_cache.begin());
    rcv_cache.pop_front();
    if(0 < rcv_info.cnt)
    {
        auto data = bin.ptr();
        if(nullptr != data) { std::free(data); }
        bin.attach(rcv_info.buff, rcv_info.cnt);
    }
    return rcv_info.state;
}
void SerialMonitor::reciver(size_t idx)
{
    std::vector<size_t> timer;
    {
        std::lock_guard<std::mutex> lock(mtx);
        timer = this->timer;
        cache[idx].state = NONE;
        cond.notify_all();
    }
    auto user_timer = [&](size_t t_idx)
    {
        std::lock_guard<std::mutex> lock(mtx);

        if(CLOSE != cache[idx].state)
        {
            if( !com[idx]->rts_status() )
            {
                auto update_cache = [&](size_t idx)
                {
                    static const enum State evt_state[] = { GAP, TIME_OUT_1, TIME_OUT_2, TIME_OUT_3 };
                    cache[idx].state = evt_state[t_idx];
                    rcv_cache.push_back(cache[idx]);
                    cache[idx].cnt  = 0;
                    cache[idx].buff = static_cast<unsigned char *>(std::malloc(cache_size));
                };
                if(nullptr != sync)
                {
                    std::lock_guard<std::mutex> lock(sync->mtx);
                    update_cache(idx);
                    sync->cond.notify_all();
                }
                else
                {
                    update_cache(idx);
                }
                cond.notify_all();
            }
        }
    };
    OneShotTimer timeout_evter(timer, user_timer);
    {
        std::lock_guard<std::mutex> lock(mtx);
        oneshot[idx] = &timeout_evter;
    }
    while((rcv_enable) && (nullptr != com[idx]))
    {
        unsigned char data;
        size_t len;
        try { len = com[idx]->read(&(data), 1); } catch(...) { break; }
        if(0 == len) { break; }
        std::lock_guard<std::mutex> lock(mtx);
        if(rcv_enable)
        {
            timeout_evter.restart();
            cache[idx].buff[cache[idx].cnt] = data;
            cache[idx].cnt += len;
            if(cache_size <= cache[idx].cnt)
            {
                if(nullptr != sync)
                {
                    std::lock_guard<std::mutex> lock(sync->mtx);
                    cache[idx].state = CACHE_FULL;
                    rcv_cache.push_back(cache[idx]);
                    sync->cond.notify_all();
                }
                else
                {
                    cache[idx].state = CACHE_FULL;
                    rcv_cache.push_back(cache[idx]);
                }
                cache[idx].buff  = static_cast<unsigned char *>(std::malloc(cache_size));
                cache[idx].cnt   = 0;
                cond.notify_all();
            }
        }
    }
    std::lock_guard<std::mutex> lock(mtx);
    rcv_enable = false;
    cache[idx].state = CLOSE;
    cache[idx].cnt   = 0;
    rcv_cache.push_back(cache[idx]);
    cache[idx].buff  = static_cast<unsigned char *>(std::malloc(cache_size));
    cache[idx].cnt   = 0;
    cond.notify_all();
}

void SerialMonitor::user_timer(int mode, unsigned int tick)
{
    auto act_cyclic = [this]()
    {
        std::lock_guard<std::mutex> lock(mtx);
        ReciveInfo info = { 0, CTIMER, 0, nullptr };
        if(nullptr != sync)
        {
            std::lock_guard<std::mutex> lock(sync->mtx);
            rcv_cache.push_back(info);
            sync->cond.notify_all();
        }
        else
        {
            rcv_cache.push_back(info);
        }
        cond.notify_all();
    };
    auto act_oneshot = [this]()
    {
        std::lock_guard<std::mutex> lock(mtx);
        ReciveInfo info = { 0, UTIMER, 0, nullptr };
        if(nullptr != sync)
        {
            std::lock_guard<std::mutex> lock(sync->mtx);
            rcv_cache.push_back(info);
            sync->cond.notify_all();
        }
        else
        {
            rcv_cache.push_back(info);
        }
        cond.notify_all();
    };
    switch(mode)
    {
    case CTIMER:
        cyclic.start(tick, act_cyclic);
        break;
    case UTIMER:
        utimer.start(tick, act_oneshot);
        break;
    default:
        break;
    }
}

/* -------- << class OpenXLSXCtrl >>-------- */
OpenXLSXCtrl::~OpenXLSXCtrl(void)
{
}
void OpenXLSXCtrl::worksheet(std::string & sheet_name)
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
OpenXLSXCtrl::Type OpenXLSXCtrl::get_cell(std::string & cell, OpenXLSXCtrl::Data & val) const
{
    switch( (sheet.cell(OpenXLSX::XLCellReference(cell)).value()).type() )
    {
    case OpenXLSX::XLValueType::Empty:
        return Empty;
    case OpenXLSX::XLValueType::Boolean:
        if( (sheet.cell(OpenXLSX::XLCellReference(cell)).value()).get<bool>() ) { val.vint = 1; }
        else                                                                    { val.vint = 0; }
        return Boolean;
    case OpenXLSX::XLValueType::Integer:
        val.vint = static_cast<int>((sheet.cell(OpenXLSX::XLCellReference(cell)).value()).get<int64_t>());
        return Integer;
    case OpenXLSX::XLValueType::Float:
        val.vfloat = static_cast<float>((sheet.cell(OpenXLSX::XLCellReference(cell)).value()).get<double>());
        return Float;
    case OpenXLSX::XLValueType::String:
        *(val.str) = ((sheet.cell(OpenXLSX::XLCellReference(cell)).value()).get<std::string>());
        return String;
    case OpenXLSX::XLValueType::Error:
    default:
        break;
    }
    return Error;
}


/* -------- << Program Main >>-------- */
extern "C"
{
extern const char    help_msg[];
extern unsigned int  help_size;
}
int main(int argc, char * argv[])
{
    try
    {
        boost::program_options::options_description desc("smon.exe [Options]");
        desc.add_options()
            ("comlist",                                                         "print com port list"                             )
            ("pipelist",                                                        "print pipe name list"                            )
            ("gap,g",           boost::program_options::value<unsigned int>(),  "time out tick. Default   30 ( 30 [ms])"          )
            ("timer,t",         boost::program_options::value<unsigned int>(),  "time out tick. Default  300 (300 [ms])"          )
            ("timer2",          boost::program_options::value<unsigned int>(),  "time out tick. Default  500 (500 [ms])"          )
            ("timer3",          boost::program_options::value<unsigned int>(),  "time out tick. Default 1000 (  1 [s])"           )
            ("crc,c",           boost::program_options::value<std::string>(),   "calclate modbus RTU CRC"                         )
            ("crc8",            boost::program_options::value<std::string>(),   "calclate CRC8"                                   )
            ("sum,s",           boost::program_options::value<std::string>(),   "calclate checksum of XOR"                        )
            ("FLOAT,F",         boost::program_options::value<std::string>(),   "hex to float value"                              )
            ("float,f",         boost::program_options::value<std::string>(),   "litle endian hex to float value"                 )
            ("oneline,1",                                                       "1 line command"                                  )
            ("bin-edit,2",                                                      "binary command editor"                           )
            ("makeQR,3",        boost::program_options::value<std::string>(),   "make QR code of svg"                             )
            ("read-bin-to-xlsx",                                                "read binary to xlsx file"                        )
            ("mruby-script,m",  boost::program_options::value<std::string>(),   "execute mruby script"                            )
            ("version,v",                                                       "print version"                                   )
            ("help,h",                                                          "help"                                            )
            ("help-misc",                                                       "display of exsample and commet, ext class ...etc");
        boost::program_options::variables_map argmap;

        auto const parsing_result = parse_command_line( argc, argv, desc );
        store( parsing_result, argmap );
        notify( argmap );

        if(argmap.count("version"))
        {
            std::cout << "smon Revision " << SoftwareRevision << std::endl;
            std::cout << "    mruby Revision 3.3.0" << std::endl;
            std::cout << "    OpenXLSX Revision 0.4.1" << std::endl;
            std::cout << "    QR-Code-generator Revision 1.8.0" << std::endl;
            return 0;
        }
        if(argmap.count("help"))
        {
            std::cout << "smon Revision " << SoftwareRevision << std::endl;
            std::cout << std::endl;
            std::cout << desc << std::endl;
            return 0;
        }
        if(argmap.count("help-misc"))
        {
            std::cout << "smon Revision " << SoftwareRevision << std::endl;
            std::cout << std::endl;
            std::cout << desc << std::endl;
            std::cout << help_msg << std::endl;
#if 0
            printf("debug: %d\n", help_size);
#endif
            return 0;
        }
        std::vector<std::string> arg;
        for(auto const& str : collect_unrecognized(parsing_result.options, boost::program_options::include_positional))
        {
            arg.push_back(str);
        }
        std::string prog(argv[0]);
        Application app(prog, argmap, arg);
        app.main();
    }
    catch(std::exception & exp) { std::cerr << "exeption: " << exp.what() << std::endl; }
    catch(...)                  { std::cerr << "unknown exeption" << std::endl;         }
    return 0;
}
