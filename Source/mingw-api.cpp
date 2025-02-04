/**
 * microsoft windows api adapter
 *
 * @file    mingw-api.cpp
 * @brief   this file is adapter of Microsoft Windows API
 * @author  Shouji, Igarashi
 *
 * (c) 2024 Shouji, Igarashi.
 *
 * @see string
 * @see vector
 */
#include "Entity.hpp"
#include "SerialControl.hpp"
#include "ComList.hpp"
#include "PipeList.hpp"

#include <chrono>
#include <list>
#include <string>
#include <thread>

#include <windows.h>
#include <hidsdi.h>
#include <iconv.h>
#include <initguid.h>
#include <setupapi.h>


using namespace std;


/* --------------------------------------------------------------------------------<< Local Logic >>-------------------------------------------------------------------------------- */
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

/* --------------------------------------------------------------------------------<< ComList >>-------------------------------------------------------------------------------- */
/**
 * constractor on ComList: create COM list
 */
ComList::ComList(void)
{
    HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_COMPORT, 0, 0, DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);
    if(0 != hDevInfo)
    {
        //iconv_t icd = iconv_open("UTF-8", "cp932");
        iconv_t icd = iconv_open("cp932", "cp932");
        SP_DEVINFO_DATA Data={ sizeof(SP_DEVINFO_DATA) };
        Data.cbSize = sizeof(Data);
        for(int cnt=0; SetupDiEnumDeviceInfo(hDevInfo, cnt, &Data); cnt++)
        {
            string str;
            HKEY key=SetupDiOpenDevRegKey(hDevInfo, &Data, DICS_FLAG_GLOBAL, 0, DIREG_DEV,KEY_QUERY_VALUE);
            if(key)
            {
                char name[1024];
                DWORD type=0;
                DWORD size=sizeof(name);
                RegQueryValueEx(key, "PortName", NULL, &type, (LPBYTE)name, &size);
                toUTF8(str, icd, name, size);
            }
            DWORD  size = 0;
            LPTSTR buf=NULL;
            DWORD  dataT;
            while(!SetupDiGetDeviceRegistryProperty(hDevInfo, &Data, SPDRP_DEVICEDESC, &dataT, (PBYTE)buf, size, &size))
            {
                if(GetLastError()!=ERROR_INSUFFICIENT_BUFFER)
                {
                    break;
                }
                if(buf)
                {
                    LocalFree(buf);
                }
                buf=(LPTSTR)LocalAlloc(LPTR, size*2);
            }
            str += ": ";
            toUTF8(str, icd, buf, size);
            if(buf)
            {
                LocalFree(buf);
            }
            list.push_back(str);
        }
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }
}

/**
 * destractor on ComList
 */
ComList::~ComList(void)
{
}

/**
 * reference of COM list
 */
std::list<std::string> & ComList::ref(void)
{
    return list;
}


/* --------------------------------------------------------------------------------<< PipeList >>-------------------------------------------------------------------------------- */
/**
 * constractor on PipeList: create pipe name list
 */
PipeList::PipeList(void)
{
    WIN32_FIND_DATAA fd{};
    auto hFind = FindFirstFileA(R"(\\.\pipe\*.*)", &fd);
    if(hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            list.push_back(std::string(fd.cFileName));
        } while (FindNextFile(hFind, &fd));
        FindClose(hFind);
    }
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
std::list<std::string> & PipeList::ref(void)
{
    return list;
}


/* --------------------------------------------------------------------------------<< CoreWin >>-------------------------------------------------------------------------------- */
class CoreWin : public Core
{
protected:
    HANDLE hStdin;

public:
    CoreWin(void);
    virtual ~CoreWin(void);
    virtual void gets(std::string & str);
};

CoreWin::CoreWin(void)
{
    hStdin= GetStdHandle(STD_INPUT_HANDLE);
    if(INVALID_HANDLE_VALUE == hStdin)
    {
        printf("%s:%d: %s: ErrorCode(%d)\n", __FILE__, __LINE__, __FUNCTION__, GetLastError());
    }
}

CoreWin::~CoreWin(void)
{
}

void CoreWin::gets(std::string & str)
{
    DWORD waitResult = WaitForSingleObject(hStdin, INFINITE);
    if(WAIT_OBJECT_0 == waitResult)
    {
        std::getline(std::cin, str);
    } else { printf("%s:%d: %s: result(0x%x), ErrorCode(%d)\n", __FILE__, __LINE__, __FUNCTION__, waitResult, GetLastError()); }
}

Core * Core::createObject(void)
{
    static Core * core = nullptr;
    if(nullptr == core)
    {
        core = new CoreWin();
    }
    return core;
}


#if 0
/* --------------------------------------------------------------------------------<< Serial Control >>-------------------------------------------------------------------------------- */
class SerialControlWinAPI : public SerialControl
{
protected:
    HANDLE handle;
    struct Profile profile;
    bool rts;
    unsigned char bit_num;

    enum RtsCtrl
    {
        set_RTS   = 3,
        clear_RTS = 4,
    };
    OVERLAPPED sendOverlapped, recieveOverlapped;

public:
    SerialControlWinAPI(const char * name, SerialControl::Profile & profile);
    virtual ~SerialControlWinAPI(void);
    virtual std::size_t read(unsigned char * data, std::size_t size);
    virtual std::size_t send(unsigned char * data, std::size_t size);
    virtual bool rts_status(void) const;
    virtual void setRTS(bool rts);
    virtual void close(void);
};

SerialControlWinAPI::SerialControlWinAPI(const char * name, SerialControl::Profile & prof)
  : handle(INVALID_HANDLE_VALUE), profile(prof), rts(true), bit_num(1 + 8)
{
    std::string sname("\\\\.\\");
    sname += name;

    bit_num += profile.stop;
    bit_num += (profile.parity != none ? 1 : 0);
    handle = CreateFile(sname.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    if(INVALID_HANDLE_VALUE != handle)
    {
        DCB params = { 0 };
        params.DCBlength = sizeof(params);
        if(GetCommState(handle, &params))
        {
            params.BaudRate = profile.baud;
            params.ByteSize = 8;
            switch(profile.parity)
            {
                case none: params.Parity = NOPARITY;   break;
                case odd:  params.Parity = ODDPARITY;  break;
                case even: params.Parity = EVENPARITY; break;
                default: break;
            }
            switch(profile.stop)
            {
                case one: params.StopBits = ONESTOPBIT;  break;
                case two: params.StopBits = TWOSTOPBITS; break;
                default: break;
            }
            if(profile.rtsctrl) { params.fRtsControl = RTS_CONTROL_ENABLE; }
            else                { params.fRtsControl = RTS_CONTROL_DISABLE; }
            params.fOutxCtsFlow = FALSE;
            params.EvtChar = '\0';
            if(SetCommState(handle, &params))
            {
#if 0
                COMMTIMEOUTS timeouts = {0};
                timeouts.ReadIntervalTimeout        = 50;
                timeouts.ReadTotalTimeoutConstant   = 50;
                timeouts.ReadTotalTimeoutMultiplier = 10;
                timeouts.WriteTotalTimeoutConstant  = 50;
                timeouts.WriteTotalTimeoutMultiplier = 10;
                if(SetCommTimeouts(handles[0], &timeouts))
#endif
                {
                    ZeroMemory(&sendOverlapped,    sizeof(OVERLAPPED));
                    ZeroMemory(&recieveOverlapped, sizeof(OVERLAPPED));
                    recieveOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
                    if(recieveOverlapped.hEvent == NULL)
                    {
                        DWORD err = GetLastError();
                        printf("CreateEvent failed: %d\n", err);
                        return;
                    }
                    //PurgeComm(handle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
                    //SetCommMask(handle, EV_RXFLAG|EV_RXCHAR|EV_ERR);
                    setRTS(false);
                    return;
                }
            }
        }
        printf("%s:%d: %s: ErrorCode(%d)\n", __FILE__, __LINE__, __FUNCTION__, GetLastError());
        CloseHandle(handle);
        handle = INVALID_HANDLE_VALUE;
    }
}

SerialControlWinAPI::~SerialControlWinAPI(void)
{
    close();
}

std::size_t SerialControlWinAPI::read(unsigned char * data, std::size_t size)
{
    size_t rd_size = 0;
    try
    {
        if(INVALID_HANDLE_VALUE != handle)
        {
            while(rd_size < size)
            {
                ZeroMemory(&recieveOverlapped, sizeof(OVERLAPPED));
                recieveOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
                if(NULL != recieveOverlapped.hEvent)
                {
                    DWORD event;
                    if(WaitCommEvent(handle, &event, &recieveOverlapped))
                    {
                        if((EV_RXFLAG == event) || (EV_RXCHAR == event))
                        {
                        }
                    }
                }
                else
                {
                    DWORD err = GetLastError();
                    printf("CreateEvent failed: %d\n", err);
                    return rd_size;
                }

                        if(WAIT_OBJECT_0 == WaitForSingleObject(handle, INFINITE))
                        {
                        DWORD rd_len = 0;
                        //if(ReadFile(handle, &(data[rd_size]), (size - rd_size), &rd_len , nullptr))
                        if(ReadFile(handle, &(data[rd_size]), (size - rd_size), &rd_len , &recieveOverlapped))
                        {
                            rd_size += rd_len;
                        }
                        else
                        {
                            if(GetLastError() != ERROR_IO_PENDING)
                            {
                                printf("%s:%d: %s: ErrorCode(%d)\n", __FILE__, __LINE__, __FUNCTION__, GetLastError());
                                close();
                                return 0;
                            }
                        }
                        }
                    }
                    else
                    {
                        printf("%s:%d: %s: ErrorCode(%d)\n", __FILE__, __LINE__, __FUNCTION__, GetLastError());
                        if(INVALID_HANDLE_VALUE != handle)
                        {
                            PurgeComm(handle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
                        }
                        return 0;
                    }

                }
                else
                {
                    auto code = GetLastError();
                    if(ERROR_IO_PENDING == code)
                    {
                        printf("%s:%d: %s: ErrorCode(%d)\n", __FILE__, __LINE__, __FUNCTION__, code);
                    } else { printf("%s:%d: %s: ErrorCode(%d)\n", __FILE__, __LINE__, __FUNCTION__, code); }
                }
            }
        }
    } catch(...) { }
    return rd_size;
}

std::size_t SerialControlWinAPI::send(unsigned char * data, std::size_t size)
{
    size_t wr_size = 0;
    try
    {
        if(INVALID_HANDLE_VALUE != handle)
        {
            setRTS(true);
            while(wr_size < size)
            {
                    ZeroMemory(&sendOverlapped,    sizeof(OVERLAPPED));
                DWORD wlen = 0;
                //if(WriteFile(handle, &(data[wr_size]), (size - wr_size), &wlen, &sendOverlapped))
                if(WriteFile(handle, &(data[wr_size]), (size - wr_size), &wlen, nullptr))
                {
//                    unsigned int send_time = (1000 * bit_num * (wlen+1)) / profile.baud;
//                    std::this_thread::sleep_for(std::chrono::milliseconds(send_time));
                    wr_size += wlen;
                }
                else
                {
                    close();
                    return 0;
                }
            }
            setRTS(false);
        }
    } catch(...) { }
    return wr_size;
}

bool SerialControlWinAPI::rts_status(void) const
{
    return rts;
}

void SerialControlWinAPI::setRTS(bool rts)
{
    if(profile.rtsctrl)
    {
        if(rts)
        {
            if(!this->rts)
            {   /* RTS: false -> true */
                EscapeCommFunction(handle, set_RTS);
            }
        }
        else
        {
            if(this->rts)
            {   /* RTS: true -> false */
                EscapeCommFunction(handle, clear_RTS);
            }
        }
    }
    this->rts = rts;
}

void SerialControlWinAPI::close(void)
{
printf("%s:%d: %s: ErrorCode(%d)\n", __FILE__, __LINE__, __FUNCTION__, GetLastError());
    if(INVALID_HANDLE_VALUE != handle)
    {
printf("%s:%d: %s: ErrorCode(%d)\n", __FILE__, __LINE__, __FUNCTION__, GetLastError());
        SetCommMask(handle, EV_ERR);
        PurgeComm(handle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
        CloseHandle(handle);
    }
}

SerialControl * SerialControl::createObject(const std::string & name, SerialControl::Profile & profile)
{
    return new SerialControlWinAPI(name.c_str(), profile);
}
#endif
