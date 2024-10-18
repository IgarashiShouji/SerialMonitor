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
#include "ComList.hpp"
#include "PipeList.hpp"
#include <iconv.h>
#include <string>
#include <list>
#include <thread>
#include <chrono>
#include <stdio.h>
#include <regex>
#include <boost/asio.hpp>

#include <unistd.h>


using namespace MyEntity;
using namespace boost::asio;
using namespace std;


/**
 * convert to UTF8 string from CP932
 *
 * @param   dst
 * @param   icd
 * @param   src
 * @param   size
 */
static void toUTF8(string & dst, iconv_t & icd, char * src, size_t size)
{
    size_t sz = size;
    while(0 < sz)
    {
        char utf8[size*2];
        char * p_dst = utf8;
        size_t sz_utf8 = sizeof(utf8);
        iconv(icd, &src, &sz, &p_dst, &sz_utf8);
        *p_dst = '\0';
        dst += utf8;
    }
}


/* --------------------------------------------------------------------------------<< Serial Control >>-------------------------------------------------------------------------------- */
class RtsContorl
{
protected:
    int     fd;
    bool    ctrl;
    bool    rts;
public:
    RtsContorl(int _fd, bool _ctrl) : fd(_fd), ctrl(_ctrl), rts(false) { }
    virtual ~RtsContorl(void)                                          { }
    virtual void set(void)              { if(ctrl) { int data = TIOCM_RTS; ioctl(fd, TIOCMBIS, &data); } rts = true;   }
    virtual void clear(void)            { if(ctrl) { int data = TIOCM_RTS; ioctl(fd, TIOCMBIC, &data); } rts = false;  }
    virtual bool status(void) const     { if(ctrl) { return rts; } return false; }
};

class SerialControlBoost : public SerialControl
{
protected:
    boost::asio::io_service     io;
    boost::asio::serial_port    port;
    unsigned int                _baudrate;
    unsigned char               bit_num;
    RtsContorl                  rts;
    int                         fd_pipe[2];

public:
    SerialControlBoost(const char * name, unsigned int bd, Parity pt, StopBit st, bool rts_ctrl);
    virtual ~SerialControlBoost(void);
    virtual std::size_t read(unsigned char * data, std::size_t size);
    virtual std::size_t send(unsigned char * data, std::size_t size);
    virtual bool rts_status(void) const;
    virtual void setRTS(void);
    virtual void clearRTS(void);
    virtual void close(void);
};

SerialControlBoost::SerialControlBoost(const char * name, unsigned int bd, Parity pt, StopBit st, bool rts_ctrl)
  : port(io, name), _baudrate(bd), bit_num(1 + 8 + st), rts(port.native_handle(), rts_ctrl)
{
    auto result =  pipe(fd_pipe);
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

static void fdclose(int fd)
{
    close(fd);
}
SerialControlBoost::~SerialControlBoost(void)
{
    close();
    fdclose(fd_pipe[0]);
    fdclose(fd_pipe[1]);
}

void SerialControlBoost::close(void)
{
    static const uint8_t cmd = 1;
    auto w_size = write(fd_pipe[1], &cmd, sizeof(cmd));
    port.close();
}

std::size_t SerialControlBoost::read(unsigned char * data, std::size_t size)
{
    std::size_t len = 0;
    try
    {
        if( port.is_open() )
        {
            auto fd = port.native_handle();
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(fd, &fds);
            FD_SET(fd_pipe[0], &fds);
            auto max =fd; 
            if( max < fd_pipe[0] ) { max = fd_pipe[0]; }
            auto result = select( max+1, &fds, NULL, NULL, NULL );
            if( FD_ISSET(fd_pipe[0], &fds) )    { return 0; }
            if( FD_ISSET(fd, &fds) )            { len = port.read_some(buffer(data, size)); }
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

bool SerialControlBoost::rts_status(void) const { return rts.status(); }
void SerialControlBoost::setRTS(void)           { rts.set();   }
void SerialControlBoost::clearRTS(void)         { rts.clear(); }

SerialControl * SerialControl::createObject(const string & name, unsigned int baud, Parity pt, StopBit st, bool rts)
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

    ConstArray<const char *> arry(list, __ArrayCount(list));
    ConstCString target(baud.c_str(), baud.size());
    size_t idx = getIndexArray<const char *>(arry, target);
    if(idx < __ArrayCount(list))
    {
        info = prof[idx];
        return true;
    }
    return false;
}


/* --------------------------------------------------------------------------------<< ComList >>-------------------------------------------------------------------------------- */
/**
 * constractor on ComList: create COM list
 */
ComList::ComList(void)
  : list(0)
{
    auto fp = popen("/bin/ls -l /sys/class/tty/*/device/driver | grep serial |  /bin/awk '{print $9}' | /bin/awk -F'/' '{print $5}'", "r");
    if( nullptr != fp)
    {
        char temp[1024] = {0};
        for(auto msg = fgets(temp, sizeof(temp), fp); (nullptr != msg) ; msg = fgets(temp, sizeof(temp), fp) )
        {
            string dev_name_(msg);
            auto dev_name = std::regex_replace(dev_name_, std::regex("\\n"), "");
            if( std::regex_search(dev_name, std::regex("USB")))
            {
                std::string info("");
                char cmd[1024];
                sprintf(cmd, "/bin/udevadm info /dev/%s | /bin/grep ID_SERIAL | /bin/sed -e 's/^.*ID_SERIAL.*=//'", dev_name.c_str());
                auto fp2 = popen(cmd, "r");
                if(nullptr != fp2)
                {
                    char temp[1024] = {0};
                    auto msg = fgets(temp, sizeof(temp), fp2);
                    if(nullptr != msg)
                    {
                        info = ": ";
                        info += msg;
                        info = std::regex_replace(info, std::regex("\\n"), "");
                    }
                    pclose(fp2);
                }
                dev_name = "/dev/" + dev_name + info;
            }
            else
            {
                dev_name = "/dev/" + dev_name;
            }
            list.push_back(dev_name);
        }
        pclose(fp);
    }
}

/**
 * destractor on comlist
 */
ComList::~ComList(void)
{
}

/**
 * reference of COM list
 */
vector<string> & ComList::ref(void)
{
    return list;
}



/* --------------------------------------------------------------------------------<< PipeList >>-------------------------------------------------------------------------------- */
/**
 * constractor on PipeList: create pipe name list
 */
PipeList::PipeList(void)
{
}

/**
 * destractor on PipeList
 */
PipeList::~PipeList(void)
{
}

/**
 * reference of COM list
 */
std::vector<string> & PipeList::ref(void)
{
    return list;
}
