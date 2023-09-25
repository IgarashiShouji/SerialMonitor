/**
 * Print COM list on microsoft windows
 *
 * @file    ComList.cpp
 * @brief   get COM list on Microsoft Windows
 * @author  Shouji, Igarashi
 *
 * (c) 2019 Shouji, Igarashi.
 *
 * @see string
 * @see vector
 */

#include "ComList.hpp"
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

/**
 * constractor on ComList: create COM list
 */
ComList::ComList(void)
{
    HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_COMPORT, 0, 0, DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);
    if(0 != hDevInfo)
    {
        iconv_t icd = iconv_open("UTF-8", "cp932");
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
vector<string> & ComList::ref(void)
{
    return list;
}
