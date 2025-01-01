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
#include <boost/asio.hpp>
#include <thread>
#include <chrono>
#include <stdio.h>
#include <windows.h>
#include <iostream>

using namespace MyEntity;
using namespace boost::asio;
using namespace std;

class RtsContorl
{
protected:
    static const DWORD clear_RTS = 4;
    static const DWORD set_RTS   = 3;
    HANDLE  commDevice;
    bool    ctrl;
    bool    rts;
public:
    RtsContorl(HANDLE handle, bool _ctrl) : commDevice(handle), ctrl(_ctrl), rts(false) { }
    virtual ~RtsContorl(void)                                                           { }
    virtual void set(void)          { if(ctrl) { EscapeCommFunction( commDevice, set_RTS );   } rts = true;  }
    virtual void clear(void)        { if(ctrl) { EscapeCommFunction( commDevice, clear_RTS ); } rts = false; }
    virtual bool status(void) const     { if(ctrl) { return rts; } return false; }
};

class SerialControlBoost : public SerialControl
{
protected:
    boost::asio::io_service     io;
    boost::asio::serial_port    port;
    unsigned int                    _baudrate;
    unsigned char               bit_num;
    RtsContorl                  rts;

public:
    SerialControlBoost(const char * name, unsigned int bd, Parity pt, StopBit st, bool rts);
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

void SerialControlBoost::close(void)
{
    port.close();
}
SerialControlBoost::~SerialControlBoost(void)
{
    close();
}

std::size_t SerialControlBoost::read(unsigned char * data, std::size_t size)
{
    std::size_t len = 0;
    try
    {
        if( port.is_open() )
        {
            len = port.read_some(buffer(data, size));
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

#if 0
SerialControl * SerialControl::createObject(const string & name, unsigned int baud, Parity pt, StopBit st, bool rts)
{
    SerialControl * com = new SerialControlBoost(name.c_str(), baud, pt, st, rts);
    return com;
}
#endif



class SerialControlWinAPI : public SerialControl
{
protected:
    HANDLE handle;
    OVERLAPPED olSerial;
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

SerialControl * SerialControl::createObject(const string & name, unsigned int baud, Parity pt, StopBit st, bool rts)
{
    SerialControl * com = new SerialControlWinAPI(name.c_str(), baud, pt, st, rts);
    return com;
}

SerialControlWinAPI::SerialControlWinAPI(const char * name, unsigned int bd, Parity pt, StopBit st, bool rts_ctrl)
  : handle(INVALID_HANDLE_VALUE), ctrl(rts_ctrl), rts(false)
{
    handle = CreateFileA(name, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
    if(INVALID_HANDLE_VALUE != handle)
    {
        DCB dcbSerialParams = { 0 };
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
        if(GetCommState(handle, &dcbSerialParams))
        {
            dcbSerialParams.BaudRate = bd;
            dcbSerialParams.ByteSize = 8;
            switch(pt)
            {
                case none: dcbSerialParams.Parity = NOPARITY;   break;
                case odd:  dcbSerialParams.Parity = ODDPARITY;  break;
                case even: dcbSerialParams.Parity = EVENPARITY; break;
                default: break;
            }
            switch(st)
            {
                case one: dcbSerialParams.StopBits = ONESTOPBIT;  break;
                case two: dcbSerialParams.StopBits = TWOSTOPBITS; break;
                default: break;
            }
            if(SetCommState(handle, &dcbSerialParams))
            {
                olSerial = { 0 };
                olSerial.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
                if(olSerial.hEvent)
                {
                    return;
                }
            }
        }
        close();
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
        DWORD bytesReadSerial = 0;
        if(ReadFile(handle, data, size, &bytesReadSerial, &olSerial))
        {
            HANDLE events[] = { olSerial.hEvent };
            DWORD waitResult = WaitForMultipleObjects(1, events, FALSE, INFINITE);
            switch(waitResult)
            {
            case WAIT_OBJECT_0: // シリアルポートからのデータ
                if(GetOverlappedResult(handle, &olSerial, &bytesReadSerial, TRUE))
                {
                    rd_size = bytesReadSerial;
                }
                break;
            default:
                break;
            }
        }
        else
        {
            if(GetLastError() != ERROR_IO_PENDING) { }
        }
    }
    return rd_size;
}
std::size_t SerialControlWinAPI::send(unsigned char * data, std::size_t size)
{
    size_t wr_size = 0;
    if(INVALID_HANDLE_VALUE != handle)
    {
        DWORD bytesWriteSerial = 0;
        if(WriteFile(handle, data, size, &bytesWriteSerial, &olSerial))
        {
            wr_size = bytesWriteSerial;
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
    }
}

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
