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
#include "Timer.hpp"
#include "SerialControl.hpp"
#include <OpenXLSX.hpp>
#include <boost/thread.hpp>
#include <boost/program_options.hpp>
#include <sstream>
#include <string>
#include <list>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <regex>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <mruby.h>
#include <mruby/proc.h>
#include <mruby/data.h>
#include <mruby/compile.h>
#include <mruby/string.h>
#include <mruby/hash.h>
#include <mruby/variable.h>


namespace MyApplications
{
    /**
     * Serial Monitor Main Class
     */
    class Application : MyApplications::SerialSignal
    {
    private:
        enum
        {
            bank_size = 1024,   //!< buffer size
            bank_num  = 32      //!< buffer count
        };
        enum State
        {
            BEGIN = 0,
            RECVING, 
            TO_GAP,
            TO_1,
            TO_2,
            TO_3
        };

        bool                                active;     /*!< effective serial monitor   */
        enum State                          state;      /*!< communication status       */
        unsigned char                       evt;
        unsigned int                        w_bank;
        unsigned int                        r_bank;
        bool                                is_mruby;
        std::string                         mruby_fname;
        unsigned char                       rdata[bank_num][bank_size];
        unsigned int                        ridx[bank_num];
        unsigned int                        timer[4];

        std::mutex                          mtx;        /*!< Event Mutex                */
        std::condition_variable             cond;       /*!< Event Condition Variable   */
        MyApplications::SerialControl *            serial;
        MyApplications::SerialControl::Profile     profile;
        std::mutex                          msg_mtx;
        std::condition_variable             msg_cond;
        std::list<std::string>              msg;
    public:
        Application(void);
        virtual ~Application(void);
        static Application & ref(void);

        std::string msg_wait(void);
        void rcvIntarval(unsigned int tick);
        MyApplications::SerialControl::Profile & refProfile(void);
        void setTimeOut(unsigned int type, unsigned int tick);
        int checkOptions(boost::program_options::variables_map & argmap);

        void read_stdin(size_t id);
        void recive(size_t id);
        void eventer(size_t id);
        void msg_send(std::string & str);
        void mruby_exec(size_t id);
        int main(char * com_name);
    };
};

using namespace MyApplications;
using namespace MyEntity;
using namespace OpenXLSX;
using namespace boost;
using namespace std;

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
static mrb_value smon_initialize(mrb_state * mrb, mrb_value self)
{
//    char * str;
//    mrb_get_args(mrb, "z", &str);
    return self;
}
static mrb_value smon_crc(mrb_state * mrb, mrb_value self)
{
    char result[5] = {0};
    char * str;
    mrb_get_args(mrb, "z", &str);
    string data(str);
    if((data.size()%2) == 0)
    {
        CalcCRC16 crc(modbusCRC);
        unsigned int size = 0;
        for(unsigned int idx=0, max=data.size();idx<max; idx += 2)
        {
            stringstream ss;
            ss << hex << data.substr(idx, 2);
            int val;
            ss >> val;
            crc << val;
            size ++;
        }
        sprintf(result, "%04x", *crc);
    }
    return mrb_str_new_cstr(mrb, result);
}
static mrb_value smon_wait(mrb_state * mrb, mrb_value self)
{
    Application & app = Application::ref();
    string result(app.msg_wait());
    return mrb_str_new_cstr(mrb, result.c_str());
}
static mrb_value smon_send(mrb_state * mrb, mrb_value self)
{
    char * str;
    mrb_get_args(mrb, "z", &str);
    string data(str);
    Application & app = Application::ref();
    app.msg_send(data);
    return self;
}
static mrb_value smon_regex_matches(mrb_state * mrb, mrb_value self)
{
    char * reg_cstr;
    char * cstr;
    mrb_get_args(mrb, "zz", &reg_cstr, &cstr);
    std::regex reg(reg_cstr);
    if(std::regex_search(cstr, reg))
    {
        return mrb_true_value();
    }
    return mrb_false_value();
}
static mrb_value regex_initialize(mrb_state * mrb, mrb_value self)
{
    char * reg_cstr;
    mrb_get_args(mrb, "z", &reg_cstr);
    return self;
}
static mrb_value regex_regex_matches(mrb_state * mrb, mrb_value self)
{
    char * cstr;
    mrb_get_args(mrb, "z", &cstr);
    return mrb_bool_value(false);
}
static bool prnModbusCRC(string & data)
{
    if((data.size()%2) == 0)
    {
        CalcCRC16 crc(modbusCRC);
        unsigned int size = 0;
        for(unsigned int idx=0, max=data.size();idx<max; idx += 2)
        {
            stringstream ss;
            ss << hex << data.substr(idx, 2);
            int val;
            ss >> val;
            crc << val;
            size ++;
        }
        printf("%s: %04x (%d [byte])\n", data.c_str(), *crc, size);
        return true;
    }
    cout << data << endl;
    return false;
}

static bool prnCheckSum(string & data)
{
    if((data.size()%2) == 0)
    {
        unsigned char sum = 0;
        for(unsigned int idx=0, max=data.size();idx<max; idx += 2)
        {
            stringstream ss;
            ss << hex << data.substr(idx, 2);
            int val;
            ss >> val;
            sum ^= static_cast<unsigned char>(val);
        }
        printf("%s: %02x\n", data.c_str(), sum);
        return true;
    }
    cout << data << endl;
    return false;
}

/**
 * print float type numeric from hex data.(big endian)
 *
 * data  hex string
 * return true(success) / flase(failure of output)
 */
static bool prnFloat(string & data)
{
    if(data.size() == 8)
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
            stringstream ss;
            ss << hex << data.substr(idx, 2);
            int val;
            ss >> val;
            fval.dword <<= 8;
            fval.dword |= static_cast<unsigned long>(val);
        }
        printf("%s: %f(%e)\n", data.c_str(), fval.f, fval.f);
        return true;
    }
    cout << data << endl;
    return false;
}

/**
 * print float type numeric from hex data.(little edian)
 *
 * data  hex string
 * return true(success) / flase(failure of output)
 */
static bool prnFloatl(string & org_data)
{
    string data("");
    if(org_data.size() == 8)
    {
        data  = org_data.substr(6, 2);
        data += org_data.substr(4, 2);
        data += org_data.substr(2, 2);
        data += org_data.substr(0, 2);
    }
    return prnFloat(data);
}

/**
 * constracter
 *
 */
Application::Application(void)
  : active(true), state(BEGIN), evt(0), w_bank(0), r_bank(0), is_mruby(false)
{
    for(auto & idx: ridx)
    {
        idx = 0;
    }
    timer[0] = 3;
    timer[1] = 30;
    timer[2] = 50;
    timer[3] = 100;
}

/**
 * destractor
 */
Application::~Application(void)
{
    if(nullptr != serial)
    {
        delete serial;
    }
    serial = nullptr;
}

/**
 * get Application Object
 */
Application & Application ::ref(void)
{
    static Application * app = nullptr;
    if(nullptr == app)
    {
        app = new Application();
    }
    return (*app);
}

string Application::msg_wait(void)
{
    string str("");
    std::unique_lock<std::mutex> lock(msg_mtx);
    while(0 == msg.size())
    {
        msg_cond.wait(lock);
    }
    str = msg.front();
    msg.pop_front();
    return str;
}

void Application::msg_send(std::string & str)
{
    if((0==(str.size() % 2)) && (0<(str.size() / 2)))
    {
        unsigned int len = 0;
        unsigned char data[256];
        unsigned int max = str.size() / 2;
        if(sizeof(data)<max)
        {
            max = sizeof(data);
        }
        for(unsigned int idx = 0; len < max; len ++)
        {
            unsigned char val = (toValue(str.at(idx ++)) << 4);
            val |= toValue(str.at(idx ++));
            data[len] = val;
        }
        serial->send(data, len);
    }
}

void Application::rcvIntarval(unsigned int tick)
{
    if(active)
    {
        unsigned char evt = 0x40;
        for( auto to: timer)
        {
            if(tick==to)
            {
                std::lock_guard<std::mutex> lock(mtx);
                this->evt = evt;
                cond.notify_one();
            }
            evt >>= 1;
        }
    }
}

SerialControl::Profile & Application::refProfile(void)
{
    return profile;
}
void Application::setTimeOut(unsigned int type, unsigned int tick)
{
    if(type<(sizeof(timer)/sizeof(timer[0])))
    {
        timer[type] = tick;
    }
}

int Application::checkOptions(boost::program_options::variables_map & argmap)
{
    if(argmap.count("baud"))
    {
        string baud = argmap["baud"].as<string>();
        SerialControl::Profile & pro = refProfile();
        if(!SerialControl::hasBaudRate(baud, pro))
        {
            cout << baud << endl;
            return -1;
        }
    }
    else
    {
        string baud = "B1200O1";
        SerialControl::Profile & pro = refProfile();
        SerialControl::hasBaudRate(baud, pro);
    }
    if(argmap.count("timer3"))
    {
        unsigned int val = argmap["timer3"].as<unsigned int>();
        timer[3] = val;
    }
    if(argmap.count("timer2"))
    {
        unsigned int val = argmap["timer2"].as<unsigned int>();
        timer[2] = val;
    }
    if(argmap.count("timer"))
    {
        unsigned int val = argmap["timer"].as<unsigned int>();
        timer[1] = val;
    }
    if(argmap.count("gap"))
    {
        unsigned int val = argmap["gap"].as<unsigned int>();
        timer[0] = val;
    }
    for(unsigned int idx=0; idx<(sizeof(timer)/sizeof(timer[0])-1); idx++)
    {
        if(timer[idx] >= timer[idx+1])
        {
            return -1;
        }
    }
    /* check mruby Script */
    if(argmap.count("mruby-script"))
    {
        mruby_fname = argmap["mruby-script"].as<string>();
        is_mruby = true;
        return 0;
    }
    return 0;
}

void Application::read_stdin(size_t id)
{
    std::string str;
    while(std::getline(std::cin, str))
    {
        if(str == "exit")
        {
            std::lock_guard<std::mutex> lock(mtx);
            evt = 0x80;
            cond.notify_one();
            break;
        }
        msg_send(str);
    }
}

/**
 * data reciver on serial port
 */
void Application::recive(size_t id)
{
    serial->clearRTS();
    unsigned char test[1024];
    memset(&(test[0]), 0, sizeof(test));
    while(active)
    {
        size_t len = serial->read(test, sizeof(test));
        std::lock_guard<std::mutex> lock(mtx);
        if(0 < len)
        {
            state=RECVING;
        }
        for(size_t cnt = 0; cnt < len; cnt ++)
        {
            rdata[w_bank][ridx[w_bank]++] = test[cnt];
            if(bank_size <= ridx[w_bank])
            {
                w_bank ++;
                w_bank &= 0x0000000F;
            }
        }
    }
    active = false;
}

/**
 * execute of mruby
 */
void Application::mruby_exec(size_t id)
{
    mrb_state * mrb = mrb_open();
    struct RClass * regex_class = mrb_define_class_under(mrb, mrb->kernel_module, "regex", mrb->object_class);
    mrb_define_method(mrb, regex_class, "initialize",    regex_initialize, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, regex_class, "reg_matches",   regex_regex_matches, MRB_ARGS_REQ(2));

    struct RClass * smon_class = mrb_define_class_under(mrb, mrb->kernel_module, "Smon", mrb->object_class);
    mrb_define_method(mrb, smon_class, "initialize",    smon_initialize, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, smon_class, "crc",           smon_crc, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, smon_class, "wait",          smon_wait, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, smon_class, "send",          smon_send, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, smon_class, "reg_matches",   smon_regex_matches, MRB_ARGS_REQ(3));

    if(is_mruby)
    {
        ifstream fin(mruby_fname);
        if(fin.is_open())
        {
            string code((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());
            mrb_load_string(mrb, code.c_str());
            mrb_close(mrb);
        }
        else
        {
            is_mruby = false;
        }
    }
    if(!is_mruby)
    {
        static const char code[] = "mon = Smon.new()\n while( 1 )\n msg = mon.wait()\n case msg\n when \"exit\" then\n exit 0;\n else\n print msg, \"\\n\"\n end\n STDOUT.flush\n end";
        mrb_load_string(mrb, code);
        mrb_close(mrb);
    }
    std::lock_guard<std::mutex> lock(mtx);
    evt = 0x80;
    cond.notify_one();
}

/**
 * event watcher
 */
void Application::eventer(size_t id)
{
    std::thread th_mruby(&Application::mruby_exec, this, 0);
    std::thread th_recive(&Application::recive, this, 0);
    std::thread th_stdin(&Application::read_stdin, this, 0);

    while(active)
    {
        unsigned char event_all = 0;
        {
            std::unique_lock<std::mutex> lock(mtx);
            cond.wait(lock);
            event_all = this->evt;
        }
        for(unsigned char mask = 0x80; 0 != mask; mask >>= 1)
        {
            unsigned char evt = event_all & mask;
            if(0 != evt)
            {
                char data[64];
                switch(evt)
                {
                case 0x80:          /* exit event */
                    serial->close();
                    {
                        string str("exit");
                        std::lock_guard<std::mutex> lock(msg_mtx);
                        msg.push_back(str);
                        msg_cond.notify_one();
                    }
                    active = false;
                    break;
                case 0x40:      /* GAP timer event */
                    w_bank ++;
                    w_bank &= 0x0000000F;
                    {
                        std::lock_guard<std::mutex> lock(msg_mtx);
                        while(w_bank != r_bank)
                        {
                            string str;
                            for(unsigned int idx=0, max=ridx[r_bank]; idx<max; idx++)
                            {
                                sprintf(data, "%02X", rdata[r_bank][idx]);
                                str += data;
                            }
                            ridx[r_bank]=0;
                            r_bank ++;
                            r_bank &= 0x0000000F;
                            if(0 < str.size())
                            {
                                msg.push_back(str);
                            }
                        }
                        state=TO_GAP;
                        string msg_gap("GAP:");
                        msg.push_back(msg_gap);
                        msg_cond.notify_one();
                    }
                    break;
                case 0x20:      /* Time Out 1   */
                    {
                        sprintf(data, "TO1:%d0ms", timer[1]);
                        string str(data);
                        std::lock_guard<std::mutex> lock(msg_mtx);
                        if(0 < str.size())
                        {
                            msg.push_back(str);
                        }
                        msg_cond.notify_one();
                    }
                    state=TO_1;
                    break;
                case 0x10:      /* Time Out 2   */
                    {
                        sprintf(data, "TO2:%d0ms", timer[2]);
                        string str(data);
                        std::lock_guard<std::mutex> lock(msg_mtx);
                        if(0 < str.size())
                        {
                            msg.push_back(str);
                        }
                        msg_cond.notify_one();
                    }
                    state=TO_2;
                    break;
                case 0x08:      /* Time out 3   */
                    {
                        sprintf(data, "TO3:%d0ms", timer[3]);
                        string str(data);
                        std::lock_guard<std::mutex> lock(msg_mtx);
                        if(0 < str.size())
                        {
                            msg.push_back(str);
                        }
                        msg_cond.notify_one();
                    }
                    state=TO_3;
                    break;
                default:
                    break;
                }
            }
        }
        cout.flush();
    }
    active = false;
    th_mruby.join();
    th_recive.detach();
    th_stdin.detach();
//    th_recive.join();
//    th_stdin.join();
}

int Application::main(char * com_name)
{
    serial = new SerialControl(com_name, *this, profile.baud, profile.parity, profile.stop);
    TimerThread timer;
    timer.cyclic(*serial, 10);
    std::thread th_eventer(&Application::eventer, this, 0);
    cout.flush();
    th_eventer.join();
    timer.stop();
    return 0;
}

void test_OpenXlsx(void)
{
    cout << "********************************************************************************\n";
    cout << "DEMO PROGRAM #04: Unicode\n";
    cout << "********************************************************************************\n";

    XLDocument doc1;
    doc1.create("./Demo04.xlsx");
    auto wks1 = doc1.workbook().worksheet("Sheet1");
    wks1.setName("test sheet");

    wks1.cell(XLCellReference("A3")).value() = "Hello World";
    doc1.save();
    doc1.saveAs("./スプレッドシート.xlsx");
    doc1.close();

    doc1.open("./スプレッドシート.xlsx");
    wks1 = doc1.workbook().worksheet("test sheet");
    cout << "Cell A1 (Korean)  : " << wks1.cell(XLCellReference("A1")).value().get<std::string>() << std::endl;
    cout << "Cell A2 (Chinese) : " << wks1.cell(XLCellReference("A2")).value().get<std::string>() << std::endl;
    cout << "Cell A3 (Japanese): " << wks1.cell(XLCellReference("A3")).value().get<std::string>() << std::endl;
    cout << "Cell A4 (Hindi)   : " << wks1.cell(XLCellReference("A4")).value().get<std::string>() << std::endl;
    cout << "Cell A5 (Russian) : " << wks1.cell(XLCellReference("A5")).value().get<std::string>() << std::endl;
    cout << "Cell A6 (Greek)   : " << wks1.cell(XLCellReference("A6")).value().get<std::string>() << std::endl;
    doc1.close();
}

int main(int argc, char *argv[])
{
    using namespace boost::program_options;
    int result = 0;
    Application & app = Application::ref();
    try
    {
        options_description desc("smon.exe [Device File] [Options]\n  smon Software revision 0.1.0\n");
        desc.add_options()
            ("baud,b",   value<string>(),       "baud rate ex) -b B9600E1")
            ("gap,g",    value<unsigned int>(), "time out tick. Default 3   ( 30 [ms])")
            ("timer,t",  value<unsigned int>(), "time out tick. Default 30  (300 [ms])")
            ("timer2",   value<unsigned int>(), "time out tick. Default 50  (500 [ms])")
            ("timer3",   value<unsigned int>(), "time out tick. Default 100 (  1 [s])")
            ("no-rts",                          "no control RTS signal")
            ("crc,c",    value<string>(),       "calclate modbus RTU CRC")
            ("sum,s",    value<string>(),       "calclate checksum of XOR")
            ("float,f",  value<string>(),       "hex to float value")
            ("floatl,F", value<string>(),       "litle endian hex to float value")
            ("mruby-script,m", value<string>(), "execute mruby script")
            ("help,h",                          "help");
        variables_map argmap;
        store(parse_command_line(argc, argv, desc), argmap);
        notify(argmap);
        if(argmap.count("help"))
        {
            cout << desc << endl;
            return 0;
        }
        if(argmap.count("crc"))
        {
            string data = argmap["crc"].as<string>();
            bool result = prnModbusCRC(data);
            if(result)
            {
                return 0;
            }
            return -1;
        }
        if(argmap.count("sum"))
        {
            string data = argmap["sum"].as<string>();
            bool result = prnCheckSum(data);
            if(result)
            {
                return 0;
            }
            return -1;
        }
        if(argmap.count("float"))
        {
            string data = argmap["float"].as<string>();
            bool result = prnFloat(data);
            if(result)
            {
                return 0;
            }
            return -1;
        }
        if(argmap.count("floatl"))
        {
            string data = argmap["floatl"].as<string>();
            bool result = prnFloatl(data);
            if(result)
            {
                return 0;
            }
            return -1;
        }
        Application & app = Application::ref();
        int result = app.checkOptions(argmap);
        if(result < 0)
        {
            printf("Timer Setting Error!\n");
            return result;
        }
        try
        {
            result = app.main(argv[1]);
        }
        catch(const std::exception & exp)
        {
            printf("Open Error\n");
            std::cout << exp.what() << std::endl;
        }
    }
    catch(const boost::program_options::error_with_option_name & exp)
    {
        cout << exp.what() << endl;
    }
    catch(const std::exception & exp)
    {
        printf("Open Error\n");
        std::cout << exp.what() << std::endl;
    }
    catch(...)
    {
        printf("Other Error\n");
    }
    return result;
}
