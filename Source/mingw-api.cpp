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
std::list<string> & ComList::ref(void)
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
std::vector<string> & PipeList::ref(void)
{
    return list;
}
