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
#include <thread>
#include <chrono>
#include <string>
#include <list>
#include <windows.h>
#include <setupapi.h>
#include <initguid.h>
#include <hidsdi.h>
#include <iconv.h>

using namespace std;

/* --------------------------------------------------------------------------------<< Serial Control >>-------------------------------------------------------------------------------- */
class SerialControlWinAPI : public SerialControl
{
protected:
    HANDLE handle;
    bool    ctrl;
    bool    rts;
    static const DWORD clear_RTS = 4;
    static const DWORD set_RTS   = 3;

public:
    SerialControlWinAPI(const char * name, unsigned int bd, Parity pt, StopBit st, bool rts);
    virtual ~SerialControlWinAPI(void);
    virtual std::size_t read(unsigned char * data, std::size_t size);
    virtual std::size_t send(unsigned char * data, std::size_t size);
    virtual bool rts_status(void) const;
    virtual void setRTS(void);
    virtual void clearRTS(void);
    virtual void close(void);
};

SerialControlWinAPI::SerialControlWinAPI(const char * name, unsigned int bd, Parity pt, StopBit st, bool rts_ctrl)
  : handle(INVALID_HANDLE_VALUE), ctrl(rts_ctrl), rts(false)
{
    std::string sname("\\\\.\\");
    sname += name;

    handle = CreateFile(sname.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if(INVALID_HANDLE_VALUE != handle)
    {
        DCB params = { 0 };
        params.DCBlength = sizeof(params);
        if(GetCommState(handle, &params))
        {
            params.BaudRate = bd;
            params.ByteSize = 8;
            switch(pt)
            {
                case none: params.Parity = NOPARITY;   break;
                case odd:  params.Parity = ODDPARITY;  break;
                case even: params.Parity = EVENPARITY; break;
                default: break;
            }
            switch(st)
            {
                case one: params.StopBits = ONESTOPBIT;  break;
                case two: params.StopBits = TWOSTOPBITS; break;
                default: break;
            }
            if(SetCommState(handle, &params))
            {
                COMMTIMEOUTS timeouts = {0};
                timeouts.ReadIntervalTimeout        = 50;
                timeouts.ReadTotalTimeoutConstant   = 50;
                timeouts.ReadTotalTimeoutMultiplier = 10;
                timeouts.WriteTotalTimeoutConstant  = 50;
                timeouts.WriteTotalTimeoutMultiplier = 10;
                if(SetCommTimeouts(handle, &timeouts))
                {
                    return;
                }
            }
        }
        close();
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
    if(INVALID_HANDLE_VALUE != handle)
    {
        while(rd_size <= size)
        {
            DWORD bytesReadSerial = 0;
            if(ReadFile(handle, &(data[rd_size]), (size - rd_size), &bytesReadSerial, nullptr))
            {
                rd_size += bytesReadSerial;
printf("ReadFile 1: %d(%d)/%d\n", rd_size, bytesReadSerial, size);
            }
            else
            {
                if(GetLastError() != ERROR_IO_PENDING)
                {
                    close();
                    rd_size = 0;
                }
            }
        }
    }
    return rd_size;
}
std::size_t SerialControlWinAPI::send(unsigned char * data, std::size_t size)
{
    size_t wr_size = 0;
    if((INVALID_HANDLE_VALUE != handle) && (0 < size))
    {
        while(wr_size < size)
        {
            DWORD bytesWriteSerial = 0;
            if(WriteFile(handle, &(data[wr_size]), (size - wr_size), &bytesWriteSerial, nullptr))
            {
                wr_size += bytesWriteSerial;
//printf("WriteFile 1: %d(%d)/%d\n", wr_size, bytesWriteSerial, size);
            }
            else
            {
                close();
                wr_size = 0;
                break;
            }
        }
    }
    return wr_size;
}
bool SerialControlWinAPI::rts_status(void) const
{
    if(INVALID_HANDLE_VALUE != handle)
    {
        if(ctrl)
        {
            return rts;
        }
    }
    return false;
}
void SerialControlWinAPI::setRTS(void)
{
    if(INVALID_HANDLE_VALUE != handle)
    {
        if(ctrl)
        {
            EscapeCommFunction(handle, set_RTS);
        }
        rts = true;
    }
}
void SerialControlWinAPI::clearRTS(void)
{
    if(INVALID_HANDLE_VALUE != handle)
    {
        if(ctrl)
        {
            EscapeCommFunction(handle, clear_RTS);
        }
        rts = false;
    }
}
void SerialControlWinAPI::close(void)
{
    if(INVALID_HANDLE_VALUE != handle)
    {
        CloseHandle(handle);
        handle = INVALID_HANDLE_VALUE;
    }
}

#if 0
SerialControl * SerialControl::createObject(const string & name, unsigned int baud, Parity pt, StopBit st, bool rts)
{
    SerialControl * com = new SerialControlWinAPI(name.c_str(), baud, pt, st, rts);
    return com;
}
#endif

#if 0
void test(void)
{
//#include <windows.h>
//#include <iostream>
//#include <string>

    // シリアルポートを開く
    const char* serialPortName = "COM1"; // 使用するシリアルポート
    HANDLE hSerial = CreateFileA(
        serialPortName,
        GENERIC_READ | GENERIC_WRITE,
        0,    // 他のプロセスと共有しない
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED, // 非同期I/O
        nullptr
    );

    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open serial port: " << GetLastError() << std::endl;
        return;
    }

    // シリアルポートの設定
    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Failed to get serial parameters: " << GetLastError() << std::endl;
        CloseHandle(hSerial);
        return;
    }

    dcbSerialParams.BaudRate = CBR_9600;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Failed to set serial parameters: " << GetLastError() << std::endl;
        CloseHandle(hSerial);
        return;
    }

    // 標準入力を非同期モードに設定
    HANDLE hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    if (hStdInput == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to get standard input handle: " << GetLastError() << std::endl;
        CloseHandle(hSerial);
        return;
    }

    // オーバーラップ構造を初期化
    OVERLAPPED olSerial = { 0 };
    olSerial.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    OVERLAPPED olInput = { 0 };
    olInput.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    if (!olSerial.hEvent || !olInput.hEvent) {
        std::cerr << "Failed to create event objects: " << GetLastError() << std::endl;
        CloseHandle(hSerial);
        return;
    }

    // メインループ
    while (true) {
        char serialBuffer[256];
        char inputBuffer[256];

        // シリアルポートの読み取りを非同期で開始
        DWORD bytesReadSerial = 0;
        if (!ReadFile(hSerial, serialBuffer, sizeof(serialBuffer) - 1, &bytesReadSerial, &olSerial)) {
            if (GetLastError() != ERROR_IO_PENDING) {
                std::cerr << "Serial read error: " << GetLastError() << std::endl;
                break;
            }
        }

        // 標準入力の読み取りを非同期で開始
        DWORD bytesReadInput = 0;
        if (!ReadFile(hStdInput, inputBuffer, sizeof(inputBuffer) - 1, &bytesReadInput, &olInput)) {
            if (GetLastError() != ERROR_IO_PENDING) {
                std::cerr << "Input read error: " << GetLastError() << std::endl;
                break;
            }
        }

        // イベントの待機
        HANDLE events[] = { olSerial.hEvent, olInput.hEvent };
        DWORD waitResult = WaitForMultipleObjects(2, events, FALSE, INFINITE);

        switch (waitResult) {
        case WAIT_OBJECT_0: // シリアルポートからのデータ
            if (GetOverlappedResult(hSerial, &olSerial, &bytesReadSerial, TRUE)) {
                serialBuffer[bytesReadSerial] = '\0';
                std::cout << "Serial Input: " << serialBuffer << std::endl;
            }
            break;

        case WAIT_OBJECT_0 + 1: // 標準入力からのデータ
            if (GetOverlappedResult(hStdInput, &olInput, &bytesReadInput, TRUE)) {
                inputBuffer[bytesReadInput] = '\0';
                std::cout << "Standard Input: " << inputBuffer << std::endl;
            }
            break;

        default:
            std::cerr << "WaitForMultipleObjects error: " << GetLastError() << std::endl;
            break;
        }
    }

    // リソースの解放
    CloseHandle(hSerial);
    CloseHandle(olSerial.hEvent);
    CloseHandle(olInput.hEvent);
}
#endif




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
