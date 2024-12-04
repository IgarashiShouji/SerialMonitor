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

/* -- static const & functions -- */
static const char *  SoftwareRevision = "0.13.8";
std::chrono::system_clock::time_point start;

static unsigned char toValue(unsigned char data)
{
    unsigned char val=0xff;
         if(('0' <= data) && (data<='9')) { val = data - '0';      }
    else if(('a' <= data) && (data<='f')) { val = 10 + data - 'a'; }
    else if(('A' <= data) && (data<='F')) { val = 10 + data - 'A'; }
    else                                  { }
    return val;
}

class BinaryControl
{
protected:
    size_t length;
    size_t compress_size;
    size_t pos;
    unsigned char * data;
    std::mutex mtx;

public:
    BinaryControl(void) : length(0), compress_size(0), pos(0), data(nullptr) { }
    BinaryControl(size_t size_) : length(0), compress_size(0), pos(0), data(nullptr) { alloc(size_); }
    BinaryControl(std::string & src)
      : length(0), compress_size(0), pos(0), data(nullptr)
    {
        auto size = src.size();
        alloc(size);
        length = this->write(0, static_cast<mrb_int>(size), src);
    }
    virtual ~BinaryControl(void)
    {
        std::lock_guard<std::mutex> lock(mtx);
        if(nullptr!=data)
        {
            std::free(data);
        }
    }
    void alloc(size_t size)
    {
        std::lock_guard<std::mutex> lock(mtx);
        length        = size;
        compress_size = 0;
        pos           = 0;
        if( nullptr != data ) { std::free(data); data = nullptr;                        }
        if( 0 < size )        { data = static_cast<unsigned char *>(std::malloc(size)); }
    }
    size_t resize(size_t size)
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
    unsigned char * ptr(void)
    {
        return data;
    }
    size_t size(void) const
    {
        if(0 < compress_size) { return compress_size; }
        return length;
    }
    size_t get_pos(void)
    {
        if(pos < size()) { return pos; }
        return 0;
    }
    void clone(mrb_int address, mrb_int size, const BinaryControl & src)
    {
        if(nullptr != data) { std::free(data); data = nullptr; }
        if(0 == src.compress_size) { length = size; compress_size = 0;    }
        else                       { length = size; compress_size = size; }
        if(src.size() < size) { size = src.size(); }
        data = static_cast<unsigned char *>(std::malloc(size));
        std::memcpy(data, &(src.data[address]), size);
        pos = 0;
    }
    size_t loadBinaryFile(std::string & fname)
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
                        std::lock_guard<std::mutex> lock(mtx);
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
    void saveBinaryFile(std::string & fname)
    {
        std::lock_guard<std::mutex> lock(mtx);
        std::ofstream fout(fname, std::ios::binary);
        fout.write(reinterpret_cast<char *>(data), size());
    }
    void chg_compress(void)
    {
        if(0 == compress_size)
        {
            compress_size = length;
            length = 0;
        }
    }
    uint32_t compress(void)
    {
        if(nullptr != data)
        {
            if(0 == compress_size)
            {
                auto size_max = LZ4_compressBound(length);
                auto cmp_data = std::malloc(size_max);
                if(nullptr != cmp_data)
                {
                    std::lock_guard<std::mutex> lock(mtx);
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
    uint32_t uncompress(void)
    {
        if(nullptr != data)
        {
            if(0 < compress_size)
            {
                std::lock_guard<std::mutex> lock(mtx);
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
    uint32_t memset(mrb_int address, mrb_int set_data, mrb_int sz)
    {
        uint32_t len = 0;
        auto size = this->size();
        if(address < size)
        {
            len = address - size;
            if(sz < len) { len = sz; }
            std::lock_guard<std::mutex> lock(mtx);
            std::memset(&(data[address]), static_cast<uint8_t>(set_data), len);
        }
        return len;
    }
    uint32_t memcpy(mrb_int address_dst, mrb_int address_src, mrb_int len, BinaryControl & src)
    {
        auto dst_len = size()     - address_dst;
        auto src_len = src.size() - address_src;
        if(dst_len < len) { len = dst_len; }
        if(src_len < len) { len = src_len; }
        if(0 < len)
        {
            std::lock_guard<std::mutex> lock(mtx);
            std::memcpy(&(data[address_dst]), &(src.data[address_src]), len);
        }
        return len;
    }
    int32_t memcmp(mrb_int address_dst, mrb_int address_src, mrb_int len, BinaryControl & src)
    {
        auto dst_len = size()     - address_dst;
        auto src_len = src.size() - address_src;
        if(dst_len < len) { len = dst_len; }
        if(src_len < len) { len = src_len; }
        if(0 < len)
        {
            std::lock_guard<std::mutex> lock(mtx);
            return std::memcmp(&(data[address_dst]), &(src.data[address_src]), len);
        }
        return -2;
    }
    uint32_t write(mrb_int address, mrb_int size, std::string & src)
    {
        size_t cnt = 0;
        auto length = this->size();
        if(address < length)
        {
            length -= address;
            if(length < size) { size = length; }
            if(0 < size)
            {
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
                             data[address + cnt] = val;
                             cnt ++;
                         }
                     }
                 }
            }
        }
        return cnt;
    }
    uint32_t write_big(mrb_int address, mrb_int size, std::string & src)
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
    uint32_t dump(mrb_int address, mrb_int size, std::string & output)
    {
        uint32_t cnt = 0;
        auto length = this->size();
        if(address < length)
        {
            length -= address;
            if(length < size) { size = length; }
            if(0 < size)
            {
                std::lock_guard<std::mutex> lock(mtx);
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
    uint32_t dump_big(mrb_int address, mrb_int size, std::string & output)
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
                std::lock_guard<std::mutex> lock(mtx);
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
    bool get(mrb_state * mrb, uint32_t address, std::string & format_, mrb_value & array)
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
    mrb_int set(mrb_state * mrb, uint32_t address, std::string & format_, mrb_value & array)
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
    inline size_t ref_pos(void) const
    {
        return pos;
    }
};


/* class Options */
static mrb_value mrb_opt_initialize(mrb_state * mrb, mrb_value self);
static mrb_value mrb_opt_get(mrb_state * mrb, mrb_value self);
static mrb_value mrb_opt_size(mrb_state * mrb, mrb_value self);

/* class Core */
static auto str_split = [](std::string & src, std::regex & reg)
{
    std::list<std::string> result;
    std::copy( std::sregex_token_iterator{src.begin(), src.end(), reg, -1}, std::sregex_token_iterator{}, std::back_inserter(result) );
    return result;
};
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
    static const unsigned char crctab8[16] = { 0x00,0x9B,0xAD,0x36,0xC1,0x5A,0x6C,0xF7,0x19,0x82,0xB4,0x2F,0xD8,0x43,0x75,0xEE };
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
    mrb_value arry = mrb_ary_new(mrb);
    ComList com;
    std::list<std::string> list = com.ref();
    mrb_int argc;
    mrb_value * argv;
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
                    if( std::regex_search(str, reg) ) { mrb_ary_push(mrb, arry , mrb_str_new_cstr(mrb, str.c_str())); }
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
static mrb_value mrb_core_pipelist(mrb_state* mrb, mrb_value self)
{
    mrb_value arry = mrb_ary_new(mrb);
    PipeList pipe;
    std::list<std::string> list = pipe.ref();
    mrb_int argc;
    mrb_value * argv;
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
                    if( std::regex_search(str, reg) ) { mrb_ary_push(mrb, arry , mrb_str_new_cstr(mrb, str.c_str())); }
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
    sb << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n";
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
static mrb_value mrb_core_make_qr(mrb_state* mrb, mrb_value self)
{
    char * mruby_arg;
    mrb_get_args(mrb, "z", &mruby_arg);
    std::string arg(mruby_arg);
    if(0 < arg.size())
    {
        auto qr_code = makeQRsvg(arg);
        return mrb_str_new_cstr(mrb, qr_code.c_str());
    }
    return mrb_nil_value();
}

static mrb_value mrb_core_tick(mrb_state* mrb, mrb_value self)
{
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
            if(0 == mrb_integer(argv[0])) { return mrb_int_value(mrb, std::chrono::duration_cast<std::chrono::microseconds>(now - temp).count()); }
            break;
        case MRB_TT_STRING:
            {
                std::string arg(RSTR_PTR(mrb_str_ptr(argv[0])));
                if(arg == "us") { return mrb_int_value(mrb, std::chrono::duration_cast<std::chrono::microseconds>(now - temp).count()); }
            }
            break;
        default:
            break;
        }
    }
    return mrb_int_value( mrb, std::chrono::duration_cast<std::chrono::milliseconds>(now - temp).count());
}


/* class CppRegexp */
static mrb_value mrb_cppregexp_initialize(mrb_state * mrb, mrb_value self);
static mrb_value mrb_cppregexp_length(mrb_state * mrb, mrb_value self);
static mrb_value mrb_cppregexp_match(mrb_state * mrb, mrb_value self);
static mrb_value mrb_cppregexp_grep(mrb_state * mrb, mrb_value self);
static mrb_value mrb_cppregexp_replace(mrb_state * mrb, mrb_value self);
static mrb_value mrb_cppregexp_select(mrb_state * mrb, mrb_value self);
static mrb_value mrb_cppregexp_split(mrb_state * mrb, mrb_value self);
static mrb_value mrb_core_reg_match(mrb_state* mrb, mrb_value self)
{
    mrb_value proc; mrb_int argc; mrb_value * argv;
    mrb_get_args(mrb, "&*", &proc, &argv, &argc);
    if(   (2 == argc)
       && (mrb_type(argv[0]) == MRB_TT_STRING)
       && (mrb_type(argv[1]) == MRB_TT_STRING) )
    {
        char * str = RSTR_PTR(mrb_str_ptr(argv[0]));
        char * reg = RSTR_PTR(mrb_str_ptr(argv[1]));
        if( std::regex_search(str, std::regex(reg)) ) { return mrb_bool_value(true); }
    }
    return mrb_bool_value(false);
}
static mrb_value mrb_core_reg_replace(mrb_state* mrb, mrb_value self)
{
    mrb_value proc; mrb_int argc; mrb_value * argv;
    mrb_get_args(mrb, "&*", &proc, &argv, &argc);
    std::string result("");
    switch(argc)
    {
    case 1:
    case 2:
        if(mrb_type(argv[0]) == MRB_TT_STRING)
        {
            result = RSTR_PTR(mrb_str_ptr(argv[0]));
        }
        break;
    case 3:
        if(    (mrb_type(argv[0]) == MRB_TT_STRING)
            && (mrb_type(argv[1]) == MRB_TT_STRING)
            && (mrb_type(argv[2]) == MRB_TT_STRING) )
        {
            char * str = RSTR_PTR(mrb_str_ptr(argv[0]));
            char * reg = RSTR_PTR(mrb_str_ptr(argv[1]));
            char * rep = RSTR_PTR(mrb_str_ptr(argv[2]));
            result = std::regex_replace(str, std::regex(reg), rep);
        }
        break;
    }
    return mrb_str_new_cstr( mrb, result.c_str() );
}
static mrb_value mrb_core_split(mrb_state* mrb, mrb_value self)
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
static void mrb_regexp_context_free(mrb_state * mrb, void * ptr);

/* class thread */
static mrb_value mrb_thread_initialize(mrb_state * mrb, mrb_value self);
static mrb_value mrb_thread_run(mrb_state * mrb, mrb_value self);
static mrb_value mrb_thread_join(mrb_state * mrb, mrb_value self);
static mrb_value mrb_thread_is_run(mrb_state * mrb, mrb_value self);
static mrb_value mrb_thread_state(mrb_state * mrb, mrb_value self);
static mrb_value mrb_thread_sync(mrb_state * mrb, mrb_value self);
static mrb_value mrb_thread_wait(mrb_state * mrb, mrb_value self);
static mrb_value mrb_thread_notify(mrb_state * mrb, mrb_value self);
static mrb_value mrb_thread_stop(mrb_state * mrb, mrb_value self);
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
static mrb_value mrb_xlsx_save(mrb_state * mrb, mrb_value self);
static void mrb_xlsx_context_free(mrb_state * mrb, void * ptr);

/* class BinEdit */
static mrb_value mrb_bedit_initialize(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_length(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_save(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_memset(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_memcpy(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_memcmp(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_write(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_dump(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_get(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_set(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_pos(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_compress(mrb_state * mrb, mrb_value self);
static mrb_value mrb_bedit_uncompress(mrb_state * mrb, mrb_value self);
static void mrb_bedit_context_free(mrb_state * mrb, void * ptr);
static const struct mrb_data_type mrb_bedit_context_type = { "mrb_open_bedit_context", mrb_bedit_context_free, };


/* class */
class CppRegexp
{
private:
    std::vector<std::regex> regs;
public:
    virtual ~CppRegexp(void) {}
    CppRegexp(const char * str)
    {
        std::regex reg(str);
        regs.push_back(reg);
    }
    CppRegexp(const std::list<std::string> & arg)
    {
        for(auto & str : arg)
        {
            std::regex reg(str);
            regs.push_back(reg);
        }
    }
    unsigned int length(void) { return regs.size(); }
    bool match(const std::string & str)
    {
        for( auto & reg : regs )
        {
            if( std::regex_search(str, reg) ) { return true; }
        }
        return false;
    }
    std::list<std::string> match(std::list<std::string> & text)
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
    std::list<std::string> grep( std::list<std::string> & text )
    {
        std::list<std::string> result;
        for(auto & str : text)
        {
            bool match = true;
            for( auto & reg : regs )
            {
                if( ! std::regex_search(str, reg) )
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
        return result;
    }
    void replace( std::list<std::string> & text, const char * rep)
    {
        if(rep != nullptr)
        {
            for(auto & str : text)
            {
                for( auto & reg : regs )
                {
                    str = std::regex_replace(str, reg, rep);
                }
            }
        }
    }
    unsigned int select(const char * str)
    {
        unsigned int idx = 0;
        for( auto & reg : regs )
        {
            if( std::regex_search(str, reg) ) { break; }
            idx ++;
        }
        return idx;
    }
    std::list<std::string> split(std::string & org)
    {
        std::list<std::string> result;
        result.push_back( org );
        for( auto & reg : regs )
        {
            std::list<std::string> temp;
            for( auto str : result )
            {
                for(auto & item : str_split(str, reg)) { temp.push_back(item); }
            }
            result.clear();
            result = temp;
        }
        return result;
    }
};

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
        lock.unlock();
        return result;
    }
    void run_context(size_t id)
    {
        {
            std::lock_guard<std::mutex> lock(mtx);
            this->mrb = mrb_open();
            if(nullptr != this->mrb)
            {
                state = Run;
                cond.notify_all();
            }
        }
        while(Run == state)
        {
            mrb_yield_argv(mrb, proc, 0, NULL);
        }
        std::lock_guard<std::mutex> lock(mtx);
        if(nullptr != this->mrb)
        {
            mrb_close(this->mrb);
            this->mrb = nullptr;
        }
        state = Stop;
    }
    void join(void)
    {
        bool check = false;
        {
            std::lock_guard<std::mutex> lock(mtx);
            if(state != Stop ) { check = true; }
        }
        if(check) { th_ctrl.join(); }
    }
    enum Status get_state(void) const { return state; }
    void wait(mrb_state * mrb)
    {
        auto lamda = [this, &mrb]
        {
            switch(state)
            {
            case Run:
                return true;
                break;
            case Stop:
                return true;
                break;
            default:
                break;
            }
            return false;
        };
        std::unique_lock<std::mutex> lock(mtx);
        state = Wait;
        cond.wait(lock, lamda);
        lock.unlock();
    }
    void notify(mrb_state * mrb)
    {
        std::lock_guard<std::mutex> lock(mtx);
        state = Run;
        cond.notify_all();
    }
    mrb_value notify(mrb_state * mrb, mrb_value & proc)
    {
        std::lock_guard<std::mutex> lock(mtx);
        auto result = mrb_yield_argv(mrb, proc, 0, NULL);
        state = Run;
        cond.notify_all();
        return result;
    }
    mrb_value sync(mrb_state * mrb, mrb_value & proc)
    {
        std::lock_guard<std::mutex> lock(mtx);
        auto result = mrb_yield_argv(mrb, proc, 0, NULL);
        return result;
    }
    void stop(mrb_state * mrb)
    {
        std::lock_guard<std::mutex> lock(mtx);
        switch(state)
        {
        case Wakeup:
        case Run:
        case Wait:
            state = WaitJoin;
            break;
        case WaitJoin:
            break;
        case Stop:
        default:
            state = Stop;
            break;
        }
        cond.notify_all();
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
        unsigned char * prev;
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
    {   /* str_split */
        std::vector<std::string> args;
        std::regex reg(",");
        for( auto && item : str_split( arg, reg) )
        {
            args.push_back(item);
        }
        std::string name(args[0]);
        std::string baud(def_boud);
        SerialControl::Profile info;
        SerialControl::hasBaudRate(baud, info);
        if(1 < args.size())
        {
            for(auto idx = 1; idx < args.size(); idx ++)
            {
                auto & arg = args[idx];
                if      ( std::regex_match(arg, std::regex("one")))        { info.stop = SerialControl::one; }
                else if ( std::regex_match(arg, std::regex("ONE")))        { info.stop = SerialControl::one; }
                else if ( std::regex_match(arg, std::regex("two")))        { info.stop = SerialControl::two; }
                else if ( std::regex_match(arg, std::regex("TWO")))        { info.stop = SerialControl::two; }
                else if ( std::regex_match(arg, std::regex("none")))       { info.parity = SerialControl::none; }
                else if ( std::regex_match(arg, std::regex("NONE")))       { info.parity = SerialControl::none; }
                else if ( std::regex_match(arg, std::regex("even")))       { info.parity = SerialControl::even; }
                else if ( std::regex_match(arg, std::regex("EVEN")))       { info.parity = SerialControl::even; }
                else if ( std::regex_match(arg, std::regex("odd")))        { info.parity = SerialControl::odd; }
                else if ( std::regex_match(arg, std::regex("ODD")))        { info.parity = SerialControl::odd; }
                else if ( std::regex_match(arg, std::regex("GAP=[0-9]+"))) { timer[0] = stoi(std::regex_replace(arg, std::regex("GAP=([0-9]+)"), "$1")); }
                else if ( std::regex_match(arg, std::regex("TO1=[0-9]+"))) { timer[1] = stoi(std::regex_replace(arg, std::regex("TO1=([0-9]+)"), "$1")); }
                else if ( std::regex_match(arg, std::regex("TO2=[0-9]+"))) { timer[2] = stoi(std::regex_replace(arg, std::regex("TO2=([0-9]+)"), "$1")); }
                else if ( std::regex_match(arg, std::regex("TO3=[0-9]+"))) { timer[3] = stoi(std::regex_replace(arg, std::regex("TO3=([0-9]+)"), "$1")); }
                else
                {
                    if( !SerialControl::hasBaudRate(arg, info) )
                    {
                        info.baud = stoi(arg);
                    }
                }
            }
        }
        com = SerialControl::createObject(name.c_str(), info.baud, info.parity, info.stop, rts_ctrl);
        cache.buff  = new unsigned char [cache_size];
        cache.cnt   = 0;
        cache.state = NONE;
        std::thread temp(&SerialMonitor::recive, this, 0);
        th_recive.swap(temp);
    }
    virtual ~SerialMonitor(void)
    {
        this->close();
    }
    void close(void)
    {
        std::unique_lock<std::mutex> lock(mtx);
        rcv_enable = false;
        if(nullptr != com)
        {
            try { com->close(); } catch(...) { }
            auto lamda = [this]
            {
                if(cache.state == CLOSE)
                {
                    return true;
                }
                return false;
            };
            cond.wait(lock, lamda);
            th_recive.detach();
            delete com;
            com = nullptr;
        }
        if(nullptr != cache.buff)
        {
            delete [] cache.buff;
            cache.buff = nullptr;
        }
        cache.cnt   = 0;
        for(auto & item : rcv_cache)
        {
            delete [] item.buff;
            item.buff = nullptr;
        }
        rcv_cache.clear();
        res_list.erase(this);
    }
    void send(BinaryControl & bin, unsigned int timer)
    {
        if( nullptr != com)
        {
            if(0 < bin.size())
            {
                try { com->send(bin.ptr(), bin.size()); } catch(...) { }
                std::this_thread::sleep_for(std::chrono::milliseconds(timer));
                if( nullptr != tevter)
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    tevter->restart();
                }
            }
        }
    }
    void send(std::string data, unsigned int timer)
    {
        if( nullptr != com)
        {
            BinaryControl bin(data);
            if(0 < bin.size())
            {
                try { com->send(bin.ptr(), bin.size()); } catch(...) { }
                std::this_thread::sleep_for(std::chrono::milliseconds(timer));
                if( nullptr != tevter)
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    tevter->restart();
                }
            }
        }
    }
    SerialMonitor::State recive_wait(std::string & data)
    {
        ReciveInfo rcv_info = { NONE, 0, nullptr, nullptr };
        auto lamda = [this, &rcv_info]
        {
            if(cache.state == CLOSE)
            {
                this->close();
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
            size_t len;
            try { len = com->read(&(data), 1); } catch(...) { break; }
            if(0 == len) { break; }
            std::lock_guard<std::mutex> lock(mtx);
            if(rcv_enable)
            {
                 timeout_evter.restart();
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
        std::lock_guard<std::mutex> lock(mtx);
        if(CLOSE != cache.state)
        {
            if(idx < __ArrayCount(evt_state))
            {
                if( !com->rts_status() )
                {
                    cache.state = evt_state[idx];
                    rcv_cache.push_back(cache);
                    cache.buff = new unsigned char [cache_size];
                    cache.cnt  = 0;
                    cond.notify_all();
                }
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
    OpenXLSX::XLDocument  doc;
    OpenXLSX::XLWorkbook  book;
    OpenXLSX::XLWorksheet sheet;

public:
    OpenXLSXCtrl(void)          { }
    virtual ~OpenXLSXCtrl(void) { }
//  void create(const std::string & fname)           { doc.create(fname, true);     }
    void create(const std::string & fname)           { doc.create(fname);           }
    void open(const std::string & fname)             { doc.open(fname);             }
    void workbook(void)                              { book = doc.workbook();       }
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
    void set_cell_value(char * cell_name, char * val) { sheet.cell(OpenXLSX::XLCellReference(cell_name)).value() = val; }
    void set_cell_value(char * cell_name, float val)
    {
        DWord temp = { .value = val };
        if((temp.uint32 & 0x7fffff) != 0x7fffff) { sheet.cell(OpenXLSX::XLCellReference(cell_name)).value() = val; }
        else                                     { sheet.cell(OpenXLSX::XLCellReference(cell_name)).value() = "NaN"; }
    }
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
    void save(void)  { doc.save();  }
    void close(void) { doc.close(); }
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

    mrb_value cppregexp_init(mrb_state * mrb, mrb_value self)
    {
        static const struct mrb_data_type mrb_cpp_regexp_context_type =
        {
            "mrb_cpp_regexp_context", mrb_regexp_context_free,
        };
        mrb_value proc; mrb_int argc; mrb_value * argv;
        mrb_get_args(mrb, "&*", &proc, &argv, &argc);
        CppRegexp * regexp = nullptr;
        std::list<std::string> arg;
        mrb_value item;
        switch(argc)
        {
        case 1:
            switch(mrb_type(argv[0]))
            {
            case MRB_TT_STRING:
                regexp = new CppRegexp( RSTR_PTR( mrb_str_ptr( argv[0] ) ) );
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
        mrb_data_init(self, regexp, &mrb_cpp_regexp_context_type);
        return self;
    }
    mrb_value cppregexp_length(mrb_state * mrb, mrb_value self)
    {
        CppRegexp * reg = static_cast<CppRegexp *>(DATA_PTR(self));
        if(nullptr != reg)
        {
            return mrb_int_value(mrb, reg->length());
        }
        return mrb_int_value(mrb, 0);
    }
    mrb_value cppregexp_match(mrb_state * mrb, mrb_value self)
    {
        mrb_value result = mrb_ary_new(mrb);
        CppRegexp * reg = static_cast<CppRegexp *>(DATA_PTR(self));
        std::list<std::string> text;
        if(nullptr != reg)
        {
            mrb_value proc; mrb_int argc; mrb_value * argv;
            mrb_get_args(mrb, "&*", &proc, &argv, &argc);
            mrb_value item;
            switch(argc)
            {
            case 0:
                break;
            case 1:
                switch(mrb_type(argv[0]))
                {
                case MRB_TT_STRING:
                    if(reg->match( RSTR_PTR( mrb_str_ptr( argv[0] ) ) ))
                    {
                        return mrb_bool_value(true);
                    }
                    return mrb_bool_value(false);
                case MRB_TT_ARRAY:
                    while( !mrb_nil_p( item = mrb_ary_shift(mrb, argv[0])) )
                    {
                        if(MRB_TT_STRING == mrb_type(item))
                        {
                            std::string str( RSTR_PTR( mrb_str_ptr( item ) ) );
                            text.push_back( str );
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
    mrb_value cppregexp_grep(mrb_state * mrb, mrb_value self)
    {
        mrb_value result = mrb_ary_new(mrb);
        CppRegexp * reg = static_cast<CppRegexp *>(DATA_PTR(self));
        std::list<std::string> text;
        if(nullptr != reg)
        {
            mrb_value proc; mrb_int argc; mrb_value * argv;
            mrb_get_args(mrb, "&*", &proc, &argv, &argc);
            mrb_value item;
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
                    while( !mrb_nil_p( item = mrb_ary_shift(mrb, argv[0])) )
                    {
                        if(MRB_TT_STRING == mrb_type(item))
                        {
                            std::string str( RSTR_PTR( mrb_str_ptr( item ) ) );
                            text.push_back( str );
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
    mrb_value cppregexp_replace(mrb_state * mrb, mrb_value self)
    {
        mrb_value result = mrb_ary_new(mrb);
        CppRegexp * reg = static_cast<CppRegexp *>(DATA_PTR(self));
        if(nullptr != reg)
        {
            mrb_value proc; mrb_int argc; mrb_value * argv;
            mrb_get_args(mrb, "&*", &proc, &argv, &argc);
            std::list<std::string> text;
            char * rep = nullptr;
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
    mrb_value cppregexp_select(mrb_state * mrb, mrb_value self)
    {
        CppRegexp * reg = static_cast<CppRegexp *>(DATA_PTR(self));
        if(nullptr != reg)
        {
            mrb_value proc; mrb_int argc; mrb_value * argv;
            mrb_get_args(mrb, "&*", &proc, &argv, &argc);
            switch(argc)
            {
            case 1:
                if(MRB_TT_STRING == mrb_type(argv[0]))
                {
                    return mrb_int_value( mrb, reg->select( RSTR_PTR( mrb_str_ptr( argv[0] ) ) ) );
                }
                break;
            default:
                break;
            }
        }
        return mrb_nil_value();
    }
    mrb_value cppregexp_split(mrb_state * mrb, mrb_value self)
    {
        CppRegexp * reg = static_cast<CppRegexp *>(DATA_PTR(self));
        if(nullptr != reg)
        {
            mrb_value proc; mrb_int argc; mrb_value * argv;
            mrb_get_args(mrb, "&*", &proc, &argv, &argc);
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

    mrb_value thread_init(mrb_state * mrb, mrb_value self)
    {
        static const struct mrb_data_type mrb_thread_context_type =
        {
            "mrb_cpp_thread_context", mrb_thread_context_free,
        };
        mrb_value proc; mrb_int argc; mrb_value * argv;
        mrb_get_args(mrb, "&*", &proc, &argv, &argc);
        WorkerThread * th_ctrl = new WorkerThread();
        mrb_data_init(self, th_ctrl, &mrb_thread_context_type);
        return self;
    }
    mrb_value thread_run(mrb_state * mrb, mrb_value self)
    {
        mrb_value proc;
        mrb_get_args(mrb, "&", &proc);
        if (!mrb_nil_p(proc))
        {
            WorkerThread * th_ctrl = static_cast<WorkerThread *>(DATA_PTR(self));
            if(nullptr != th_ctrl )
            {
                th_ctrl->run(mrb, self);
            }
        }
        return mrb_nil_value();
    }
    mrb_value thread_join(mrb_state * mrb, mrb_value self)
    {
        WorkerThread * th_ctrl = static_cast<WorkerThread *>(DATA_PTR(self));
        if(nullptr != th_ctrl)
        {
            th_ctrl->join();
        }
        return mrb_nil_value();
    }
    mrb_value thread_state(mrb_state * mrb, mrb_value self)
    {
        WorkerThread * th_ctrl = static_cast<WorkerThread *>(DATA_PTR(self));
        if(nullptr != th_ctrl)
        {
            return mrb_fixnum_value(th_ctrl->get_state());
        }
        return mrb_nil_value();
    }
    mrb_value thread_wait(mrb_state * mrb, mrb_value self)
    {
        WorkerThread * th_ctrl = static_cast<WorkerThread *>(DATA_PTR(self));
        if(nullptr != th_ctrl)
        {
            th_ctrl->wait(mrb);
        }
        return mrb_nil_value();
    }
    mrb_value thread_notiry(mrb_state * mrb, mrb_value self)
    {
        mrb_value proc = mrb_nil_value();
        mrb_get_args(mrb, "&", &proc);
        if (!mrb_nil_p(proc))
        {
            WorkerThread * th_ctrl = static_cast<WorkerThread * >(DATA_PTR(self));
            if(nullptr != th_ctrl)
            {
                auto result = th_ctrl->notify(mrb, proc);
                return result;
            }
        }
        return mrb_nil_value();
    }
    mrb_value thread_stop(mrb_state * mrb, mrb_value self)
    {
        WorkerThread * th_ctrl = static_cast<WorkerThread *>(DATA_PTR(self));
        if(nullptr != th_ctrl)
        {
            th_ctrl->stop(mrb);
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
        SerialMonitor * smon = static_cast<SerialMonitor *>(DATA_PTR(self));
        if(nullptr != smon)
        {
            mrb_value proc = mrb_nil_value();
            mrb_get_args(mrb, "&", &proc);
            if (!mrb_nil_p(proc))
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
                return mrb_yield_argv(mrb, proc, 2, &(argv[0]));
            }
        }
        return mrb_nil_value();
    }
    mrb_value smon_send(mrb_state * mrb, mrb_value self)
    {
        SerialMonitor * smon = static_cast<SerialMonitor *>(DATA_PTR(self)); if(nullptr != smon)
        {
            char *      msg   = nullptr;
            mrb_int     timer = 0;
            mrb_int     argc;
            mrb_value * argv;
            mrb_get_args(mrb, "*", &argv, &argc); switch(argc)
            {
            case 1:
                if(MRB_TT_STRING == mrb_type(argv[0]))
                {
                    struct RString * str = mrb_str_ptr(argv[0]); msg = RSTR_PTR(str);
                }
                else
                {
                    const mrb_data_type * src_type = DATA_TYPE(argv[0]);
                    if( src_type == &mrb_bedit_context_type )
                    {
                        BinaryControl * bin = static_cast<BinaryControl *>(DATA_PTR(argv[0]));
                        smon->send(*bin, timer);
                    }
                }
                break;
            case 2:
                if(MRB_TT_INTEGER == mrb_type(argv[1]))
                {
                    timer = mrb_integer(argv[1]);
                    if(MRB_TT_STRING  == mrb_type(argv[0]))
                    {
                        struct RString * str = mrb_str_ptr(argv[0]);
                        msg = RSTR_PTR(str);
                    }
                    else
                    {
                        const mrb_data_type * src_type = DATA_TYPE(argv[0]);
                        if( src_type == &mrb_bedit_context_type )
                        {
                            BinaryControl * bin = static_cast<BinaryControl *>(DATA_PTR(argv[0]));
                            smon->send(*bin, timer);
                        }
                    }
                }
                break;
            default:
                break;
            }
            if(nullptr != msg)
            {
                smon->send(msg, timer);
            }
        }
        return self;
    }
    mrb_value smon_close(mrb_state * mrb, mrb_value self)
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
                    mrb_value ret = mrb_yield_argv(mrb, proc, 0, nullptr);
                    xlsx->save();
                    xlsx->close();
                    return ret;
                }
            }
            break;
        default:
            break;
        }
        return mrb_nil_value();
    }
    mrb_value xlsx_open(mrb_state * mrb, mrb_value self)
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
    mrb_value xlsx_worksheet(mrb_state * mrb, mrb_value self)
    {
        mrb_value proc; mrb_int argc; mrb_value * argv;
        mrb_get_args(mrb, "&*", &proc, &argv, &argc);
        if((1 == argc) && (!mrb_nil_p(proc)))
        {
            OpenXLSXCtrl * xlsx = static_cast<OpenXLSXCtrl *>(DATA_PTR(self));
            if(nullptr != xlsx)
            {
                struct RString * sheet_name = mrb_str_ptr(argv[0]);
                xlsx->worksheet(RSTR_PTR(sheet_name ));
                return mrb_yield_argv(mrb, proc, 0, nullptr);
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
    mrb_value xlsx_save(mrb_state * mrb, mrb_value self)
    {
        OpenXLSXCtrl * xlsx = static_cast<OpenXLSXCtrl *>(DATA_PTR(self));
        if(nullptr != xlsx )
        {
            xlsx->save();
        }
        return self;
    }

    BinaryControl * get_bedit_ptr(mrb_value & argv)
    {
        const mrb_data_type * src_type = DATA_TYPE(argv);
        if( src_type == &mrb_bedit_context_type )
        {
            BinaryControl * bin_src = static_cast<BinaryControl *>(DATA_PTR(argv));
            return bin_src;
        }
        return nullptr;
    }
    mrb_value bedit_init(mrb_state * mrb, mrb_value self)
    {
        BinaryControl * bedit = new BinaryControl();
        mrb_int argc;
        mrb_value * argv;
        mrb_get_args(mrb, "*", &argv, &argc);
        switch(argc)
        {
        case 1:
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
                    auto reg_file = std::regex("^file:");
                    auto reg_comp  = std::regex("^compress:");
                    if(std::regex_search(data, reg_file))
                    {
                        auto fname= std::regex_replace(data, reg_file, "");
                        auto sz = bedit->loadBinaryFile(fname);
                    }
                    else if(std::regex_search(data, reg_comp))
                    {
                        auto fname= std::regex_replace(data, reg_comp, "");
                        auto sz = bedit->loadBinaryFile(fname);
                        bedit->chg_compress();
                    }
                    else
                    {
                        mrb_int max = (data.size() / 2);
                        bedit->alloc(max);
                        auto len = bedit->write(0, max, data);
                        if(0 == len) { bedit->alloc(0);    }
                        else         { bedit->resize(len); }
                    }
                }
                break;
            case MRB_TT_OBJECT:
                {
                    BinaryControl * bin_src = get_bedit_ptr(argv[0]);
                    if(nullptr != bin_src) { bedit->clone(0, bin_src->size(), *bin_src); }
                }
                break;
            default:
                break;
            }
            break;
        case 2:
            if(  (MRB_TT_OBJECT  == mrb_type(argv[0]))
               &&(MRB_TT_INTEGER == mrb_type(argv[1])))
            {
                const mrb_data_type * src_type = DATA_TYPE(argv[0]);
                if( src_type == &mrb_bedit_context_type )
                {
                    BinaryControl * bin_src = static_cast<BinaryControl *>(DATA_PTR(argv[0]));
                    if(nullptr != bin_src)
                    {
                        bedit->clone(0, mrb_integer(argv[1]), *bin_src);
                    }
                }
            }
            else if(  (MRB_TT_INTEGER == mrb_type(argv[0]))
                    &&(MRB_TT_INTEGER == mrb_type(argv[1])))
            {
                bedit->alloc(static_cast<int>(mrb_integer(argv[0])));
                bedit->memset(0, mrb_integer(argv[1]), bedit->size());
            }
            else
            {
            }
            break;
        case 3:
            if(  (MRB_TT_OBJECT  == mrb_type(argv[0]))
               &&(MRB_TT_INTEGER == mrb_type(argv[1]))
               &&(MRB_TT_INTEGER == mrb_type(argv[2])))
            {
                const mrb_data_type * src_type = DATA_TYPE(argv[0]);
                if( src_type == &mrb_bedit_context_type )
                {
                    BinaryControl * bin_src = static_cast<BinaryControl *>(DATA_PTR(argv[0]));
                    if(nullptr != bin_src)
                    {
                        bedit->clone(mrb_integer(argv[1]), mrb_integer(argv[2]), *bin_src);
                    }
                }
            }
            break;
        default:
            break;
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
    mrb_value bedit_memset(mrb_state * mrb, mrb_value self)
    {
        BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
        if(nullptr != bedit)
        {
            mrb_int size = 0;
            mrb_int argc;
            mrb_value * argv;
            mrb_get_args(mrb, "*", &argv, &argc);
            switch(argc)
            {
            case 1: if(MRB_TT_INTEGER == mrb_type(argv[0]))     { size = bedit->memset(                   0, mrb_integer(argv[0]),        bedit->size()); break; }
            case 2: if(  (MRB_TT_INTEGER == mrb_type(argv[0]))
                       &&(MRB_TT_INTEGER == mrb_type(argv[1]))) { size = bedit->memset(                   0, mrb_integer(argv[0]), mrb_integer(argv[1])); break; }
            case 3: if(  (MRB_TT_INTEGER == mrb_type(argv[0]))
                       &&(MRB_TT_INTEGER == mrb_type(argv[1]))
                       &&(MRB_TT_INTEGER == mrb_type(argv[2]))) { size = bedit->memset(mrb_integer(argv[0]), mrb_integer(argv[1]), mrb_integer(argv[2])); break; }
            default: { break; }
            }
            return mrb_int_value(mrb, size);
        }
        return mrb_int_value(mrb, 0);
    }
    mrb_value bedit_memcpy(mrb_state * mrb, mrb_value self)
    {
        BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
        if(nullptr != bedit)
        {
            mrb_int argc;
            mrb_value * argv;
            mrb_get_args(mrb, "*", &argv, &argc);
            switch(argc)
            {
            case 1:
                if(MRB_TT_OBJECT == mrb_type(argv[0]))
                {
                    BinaryControl * src = get_bedit_ptr(argv[0]);
                    if(nullptr != src) { return mrb_int_value(mrb, bedit->memcpy(0, 0, src->size(), *src)); }
                }
                break;
            case 2:
                if(  (MRB_TT_INTEGER == mrb_type(argv[0]))
                   &&(MRB_TT_OBJECT  == mrb_type(argv[1])))
                {
                    BinaryControl * src = get_bedit_ptr(argv[1]);
                    if(nullptr != src) { return mrb_int_value(mrb, bedit->memcpy(mrb_integer(argv[0]), 0, src->size(), *src)); }
                }
                break;
            case 3:
                if(  (MRB_TT_INTEGER == mrb_type(argv[0]))
                   &&(MRB_TT_INTEGER == mrb_type(argv[1]))
                   &&(MRB_TT_OBJECT  == mrb_type(argv[2])))
                {
                    BinaryControl * src = get_bedit_ptr(argv[2]);
                    if(nullptr != src) { return mrb_int_value(mrb, bedit->memcpy(mrb_integer(argv[0]), 0, mrb_integer(argv[1]), *src)); }
                }
                break;
            case 4:
                if(  (MRB_TT_INTEGER == mrb_type(argv[0]))
                   &&(MRB_TT_INTEGER == mrb_type(argv[1]))
                   &&(MRB_TT_INTEGER == mrb_type(argv[2]))
                   &&(MRB_TT_OBJECT  == mrb_type(argv[3])))
                {
                    BinaryControl * src = get_bedit_ptr(argv[3]);
                    if(nullptr != src) { return mrb_int_value(mrb, bedit->memcpy(mrb_integer(argv[0]), mrb_integer(argv[1]), mrb_integer(argv[2]), *src)); }
                }
                break;
            default:
                break;
            }
        }
        return mrb_int_value(mrb, 0);
    }
    mrb_value bedit_cmp(mrb_state * mrb, mrb_value self)
    {
        BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
        if(nullptr != bedit)
        {
            mrb_int argc;
            mrb_value * argv;
            mrb_get_args(mrb, "*", &argv, &argc);
            switch(argc)
            {
            case 1:
                if(MRB_TT_OBJECT == mrb_type(argv[0]))
                {
                    BinaryControl * src = get_bedit_ptr(argv[0]);
                    if(nullptr != src) { return mrb_int_value(mrb, bedit->memcmp(0, 0, src->size(), *src)); }
                }
                break;
            default:
                break;
            }
        }
        return mrb_int_value(mrb, -2);
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
                auto wsize = bedit->write(address, size, data);
                return mrb_int_value(mrb, wsize);
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
    mrb_value bedit_get(mrb_state * mrb, mrb_value self)
    {
        BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
        if(nullptr != bedit)
        {
            mrb_int address = bedit->get_pos();
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
    mrb_value bedit_set(mrb_state * mrb, mrb_value self)
    {
        BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
        if(nullptr != bedit)
        {
            mrb_int address = bedit->get_pos();
            char *  fmt_ptr = nullptr;
            mrb_value arry;
            mrb_int argc;
            mrb_value * argv;
            mrb_get_args(mrb, "*", &argv, &argc);
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
    mrb_value bedit_pos(mrb_state * mrb, mrb_value self)
    {
        BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
        if(nullptr != bedit)
        {
            return mrb_int_value(mrb, bedit->ref_pos());
        }
        return mrb_nil_value();
    }

    mrb_value bedit_compress(mrb_state * mrb, mrb_value self)
    {
        BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
        if(nullptr != bedit)
        {
            auto size = bedit->compress();
            return mrb_int_value(mrb, size);
        }
        return mrb_int_value(mrb, 0);
    }
    mrb_value bedit_uncompress(mrb_state * mrb, mrb_value self)
    {
        BinaryControl * bedit = static_cast<BinaryControl *>(DATA_PTR(self));
        if(nullptr != bedit)
        {
            auto size = bedit->uncompress();
            return mrb_int_value(mrb, size);
        }
        return mrb_int_value(mrb, 0);
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
            struct RClass * core_class = mrb_define_class_under( mrb, mrb->kernel_module, "Core", mrb->object_class );
            mrb_define_module_function(mrb, core_class, "crc16",        mrb_core_crc16,         MRB_ARGS_ARG( 1, 1 )    );
            mrb_define_module_function(mrb, core_class, "crc8",         mrb_core_crc8,          MRB_ARGS_ARG( 1, 1 )    );
            mrb_define_module_function(mrb, core_class, "sum",          mrb_core_sum,           MRB_ARGS_ARG( 1, 1 )    );
            mrb_define_module_function(mrb, core_class, "to_hex",       mrb_core_to_hex,        MRB_ARGS_ARG( 2, 1 )    );
            mrb_define_module_function(mrb, core_class, "gets",         mrb_core_gets,          MRB_ARGS_ANY()          );
            mrb_define_module_function(mrb, core_class, "exists",       mrb_core_exists,        MRB_ARGS_ARG( 1, 1 )    );
            mrb_define_module_function(mrb, core_class, "timestamp",    mrb_core_file_timestamp,MRB_ARGS_ARG( 1, 1 )    );
            mrb_define_module_function(mrb, core_class, "makeQR",       mrb_core_make_qr,       MRB_ARGS_ARG( 1, 1 )    );
            mrb_define_module_function(mrb, core_class, "tick",         mrb_core_tick,          MRB_ARGS_ARG( 1, 1 )    );

            /* Class options */
            struct RClass * opt_class = mrb_define_class_under( mrb, mrb->kernel_module, "Args", mrb->object_class );
            mrb_define_method( mrb, opt_class, "initialize",            mrb_opt_initialize,     MRB_ARGS_ANY()          );
            mrb_define_method( mrb, opt_class, "size",                  mrb_opt_size,           MRB_ARGS_NONE()         );
            mrb_define_method( mrb, opt_class, "[]",                    mrb_opt_get,            MRB_ARGS_ARG( 1, 1 )    );

            /* Class CppRegexp */
            struct RClass * cppregexp_class = mrb_define_class_under( mrb, mrb->kernel_module, "CppRegexp", mrb->object_class );
            mrb_define_module_function(mrb, cppregexp_class, "reg_match",    mrb_core_reg_match,       MRB_ARGS_ARG( 2, 1 )  );
            mrb_define_module_function(mrb, cppregexp_class, "reg_replace",  mrb_core_reg_replace,     MRB_ARGS_ARG( 3, 1 )  );
            mrb_define_module_function(mrb, cppregexp_class, "reg_split",    mrb_core_split,           MRB_ARGS_ARG( 2, 1 )  );
            mrb_define_method( mrb, cppregexp_class,    "initialize",   mrb_cppregexp_initialize, MRB_ARGS_ANY()        );
            mrb_define_method( mrb, cppregexp_class,    "length",       mrb_cppregexp_length,     MRB_ARGS_NONE()       );
            mrb_define_method( mrb, cppregexp_class,    "match",        mrb_cppregexp_match,      MRB_ARGS_ANY()        );
            mrb_define_method( mrb, cppregexp_class,    "grep",         mrb_cppregexp_grep,       MRB_ARGS_ANY()        );
            mrb_define_method( mrb, cppregexp_class,    "replace",      mrb_cppregexp_replace,    MRB_ARGS_ANY()        );
            mrb_define_method( mrb, cppregexp_class,    "select",       mrb_cppregexp_select,     MRB_ARGS_ANY()        );
            mrb_define_method( mrb, cppregexp_class,    "split",        mrb_cppregexp_split,      MRB_ARGS_ARG( 1, 1 )  );

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
            mrb_define_method( mrb, thread_class, "stop",               mrb_thread_stop,        MRB_ARGS_NONE()         );

            /* Class BinEdit */
            struct RClass * bedit_class = mrb_define_class_under( mrb, mrb->kernel_module, "BinEdit", mrb->object_class );
            mrb_define_method( mrb, bedit_class, "initialize",      mrb_bedit_initialize,       MRB_ARGS_ANY()          );
            mrb_define_method( mrb, bedit_class, "length",          mrb_bedit_length,           MRB_ARGS_ARG( 1, 1 )    );
            mrb_define_method( mrb, bedit_class, "save",            mrb_bedit_save,             MRB_ARGS_ARG( 1, 1 )    );
            mrb_define_method( mrb, bedit_class, "compress",        mrb_bedit_compress,         MRB_ARGS_NONE()         );
            mrb_define_method( mrb, bedit_class, "uncompress",      mrb_bedit_uncompress,       MRB_ARGS_NONE()         );
            mrb_define_method( mrb, bedit_class, "memset",          mrb_bedit_memset,           MRB_ARGS_ARG( 3, 1 )    );
            mrb_define_method( mrb, bedit_class, "memcpy",          mrb_bedit_memcpy,           MRB_ARGS_ARG( 3, 1 )    );
            mrb_define_method( mrb, bedit_class, "memcmp",          mrb_bedit_memcmp,           MRB_ARGS_ARG( 3, 1 )    );
            mrb_define_method( mrb, bedit_class, "write",           mrb_bedit_write,            MRB_ARGS_ARG( 2, 1 )    );
            mrb_define_method( mrb, bedit_class, "dump",            mrb_bedit_dump,             MRB_ARGS_ARG( 2, 1 )    );
            mrb_define_method( mrb, bedit_class, "get",             mrb_bedit_get,              MRB_ARGS_ARG( 2, 1 )    );
            mrb_define_method( mrb, bedit_class, "set",             mrb_bedit_set,              MRB_ARGS_ARG( 3, 1 )    );
            mrb_define_method( mrb, bedit_class, "pos",             mrb_bedit_pos,              MRB_ARGS_NONE()         );

            /* Class Smon */
            struct RClass * smon_class = mrb_define_class_under( mrb, mrb->kernel_module, "Smon", mrb->object_class     );
            mrb_define_const(  mrb, smon_class, "GAP",              mrb_fixnum_value(SerialMonitor::GAP)                );
            mrb_define_const(  mrb, smon_class, "TO1",              mrb_fixnum_value(SerialMonitor::TIME_OUT_1)         );
            mrb_define_const(  mrb, smon_class, "TO2",              mrb_fixnum_value(SerialMonitor::TIME_OUT_2)         );
            mrb_define_const(  mrb, smon_class, "TO3",              mrb_fixnum_value(SerialMonitor::TIME_OUT_3)         );
            mrb_define_const(  mrb, smon_class, "CLOSE",            mrb_fixnum_value(SerialMonitor::CLOSE)              );
            mrb_define_const(  mrb, smon_class, "CACHE_FULL",       mrb_fixnum_value(SerialMonitor::CACHE_FULL)         );
            mrb_define_const(  mrb, smon_class, "NONE",             mrb_fixnum_value(SerialMonitor::NONE)               );
            mrb_define_method( mrb, smon_class, "initialize",       mrb_smon_initialize,    MRB_ARGS_REQ( 2 )           );
            mrb_define_method( mrb, smon_class, "wait",             mrb_smon_wait,          MRB_ARGS_ARG( 2, 1 )        );
            mrb_define_method( mrb, smon_class, "send",             mrb_smon_send,          MRB_ARGS_ARG( 2, 1 )        );
            mrb_define_method( mrb, smon_class, "close",            mrb_smon_close,         MRB_ARGS_NONE()             );
            mrb_define_module_function(mrb, smon_class, "comlist",  mrb_core_comlist,       MRB_ARGS_ANY()              );
            mrb_define_module_function(mrb, smon_class, "pipelist", mrb_core_pipelist,      MRB_ARGS_ANY()              );

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
Application * Application::getObject(void)
{
    return Application::obj;
}

mrb_value mrb_core_gets(mrb_state* mrb, mrb_value self)             { auto result = (Application::getObject())->core_gets(mrb, self);                               return result; }
mrb_value mrb_opt_initialize(mrb_state * mrb, mrb_value self)       { auto result = (Application::getObject())->opt_init(mrb, self);                                return result; }
mrb_value mrb_opt_size(mrb_state * mrb, mrb_value self)             { auto result = (Application::getObject())->opt_size(mrb, self);                                return result; }
mrb_value mrb_opt_get(mrb_state * mrb, mrb_value self)              { auto result = (Application::getObject())->opt_get(mrb, self);                                 return result; }

mrb_value mrb_cppregexp_initialize(mrb_state * mrb, mrb_value self) { auto result = (Application::getObject())->cppregexp_init(mrb, self);                          return result; }
mrb_value mrb_cppregexp_length(mrb_state * mrb, mrb_value self)     { auto result = (Application::getObject())->cppregexp_length(mrb, self);                        return result; }
mrb_value mrb_cppregexp_match(mrb_state * mrb, mrb_value self)      { auto result = (Application::getObject())->cppregexp_match(mrb, self);                         return result; }
mrb_value mrb_cppregexp_grep(mrb_state * mrb, mrb_value self)       { auto result = (Application::getObject())->cppregexp_grep(mrb, self);                          return result; }
mrb_value mrb_cppregexp_replace(mrb_state * mrb, mrb_value self)    { auto result = (Application::getObject())->cppregexp_replace(mrb, self);                       return result; }
mrb_value mrb_cppregexp_select(mrb_state * mrb, mrb_value self)     { auto result = (Application::getObject())->cppregexp_select(mrb, self);                        return result; }
mrb_value mrb_cppregexp_split(mrb_state * mrb, mrb_value self)      { auto result = (Application::getObject())->cppregexp_split(mrb, self);                         return result; }

mrb_value mrb_thread_initialize(mrb_state * mrb, mrb_value self)    { auto result = (Application::getObject())->thread_init(mrb, self);                             return result; }
mrb_value mrb_thread_run(mrb_state * mrb, mrb_value self)           { auto result = (Application::getObject())->thread_run(mrb, self);                              return result; }
mrb_value mrb_thread_state(mrb_state * mrb, mrb_value self)         { auto result = (Application::getObject())->thread_state(mrb, self);                            return result; }
mrb_value mrb_thread_join(mrb_state * mrb, mrb_value self)          { auto result = (Application::getObject())->thread_join(mrb, self);  mrb_garbage_collect(mrb);  return result; }
mrb_value mrb_thread_wait(mrb_state * mrb, mrb_value self)          { auto result = (Application::getObject())->thread_wait(mrb, self);  mrb_garbage_collect(mrb);  return result; }
mrb_value mrb_thread_notify(mrb_state * mrb, mrb_value self)        { auto result = (Application::getObject())->thread_notiry(mrb, self);                           return result; }
mrb_value mrb_thread_stop(mrb_state * mrb, mrb_value self)          { auto result = (Application::getObject())->thread_stop(mrb, self);                             return result; }
mrb_value mrb_thread_sync(mrb_state * mrb, mrb_value self)          { auto result = (Application::getObject())->thread_sync(mrb, self);                             return result; }

mrb_value mrb_smon_initialize(mrb_state * mrb, mrb_value self)      { auto result = (Application::getObject())->smon_init(mrb, self);                               return result; }
mrb_value mrb_smon_send(mrb_state * mrb, mrb_value self)            { auto result = (Application::getObject())->smon_send(mrb, self);                               return result; }
mrb_value mrb_smon_wait(mrb_state * mrb, mrb_value self)            { auto result = (Application::getObject())->smon_wait(mrb, self);    mrb_garbage_collect(mrb);  return result; }
mrb_value mrb_smon_close(mrb_state * mrb, mrb_value self)           { auto result = (Application::getObject())->smon_close(mrb, self);   mrb_garbage_collect(mrb);  return result; }

mrb_value mrb_xlsx_initialize(mrb_state * mrb, mrb_value self)      { auto result = (Application::getObject())->xlsx_init(mrb, self);                               return result; }
mrb_value mrb_xlsx_create(mrb_state * mrb, mrb_value self)          { auto result = (Application::getObject())->xlsx_create(mrb, self);  mrb_garbage_collect(mrb);  return result; }
mrb_value mrb_xlsx_open(mrb_state * mrb, mrb_value self)            { auto result = (Application::getObject())->xlsx_open(mrb, self);    mrb_garbage_collect(mrb);  return result; }
mrb_value mrb_xlsx_worksheet(mrb_state * mrb, mrb_value self)       { auto result = (Application::getObject())->xlsx_worksheet(mrb, self);                          return result; }
mrb_value mrb_xlsx_sheet_names(mrb_state * mrb, mrb_value self)     { auto result = (Application::getObject())->xlsx_work_sheet_names(mrb, self);                   return result; }
mrb_value mrb_xlsx_set_seet_name(mrb_state * mrb, mrb_value self)   { auto result = (Application::getObject())->xlsx_set_sheet_name(mrb, self);                     return result; }
mrb_value mrb_xlsx_set_value(mrb_state * mrb, mrb_value self)       { auto result = (Application::getObject())->xlsx_set_value(mrb, self);                          return result; }
mrb_value mrb_xlsx_cell(mrb_state * mrb, mrb_value self)            { auto result = (Application::getObject())->xlsx_cell(mrb, self);                               return result; }
mrb_value mrb_xlsx_save(mrb_state * mrb, mrb_value self)            { auto result = (Application::getObject())->xlsx_save(mrb, self);                               return result; }

mrb_value mrb_bedit_initialize(mrb_state * mrb, mrb_value self)     { auto result = (Application::getObject())->bedit_init(mrb, self);   mrb_garbage_collect(mrb);  return result; }
mrb_value mrb_bedit_length(mrb_state * mrb, mrb_value self)         { auto result = (Application::getObject())->bedit_length(mrb, self);                            return result; }
mrb_value mrb_bedit_save(mrb_state * mrb, mrb_value self)           { auto result = (Application::getObject())->bedit_save(mrb, self);                              return result; }
mrb_value mrb_bedit_compress(mrb_state * mrb, mrb_value self)       { auto result = (Application::getObject())->bedit_compress(mrb, self);                          return result; }
mrb_value mrb_bedit_uncompress(mrb_state * mrb, mrb_value self)     { auto result = (Application::getObject())->bedit_uncompress(mrb, self);                        return result; }
mrb_value mrb_bedit_write(mrb_state * mrb, mrb_value self)          { auto result = (Application::getObject())->bedit_write(mrb, self);                             return result; }
mrb_value mrb_bedit_memset(mrb_state * mrb, mrb_value self)         { auto result = (Application::getObject())->bedit_memset(mrb, self);                            return result; }
mrb_value mrb_bedit_memcpy(mrb_state * mrb, mrb_value self)         { auto result = (Application::getObject())->bedit_memcpy(mrb, self);                            return result; }
mrb_value mrb_bedit_memcmp(mrb_state * mrb, mrb_value self)         { auto result = (Application::getObject())->bedit_cmp(mrb, self);                               return result; }
mrb_value mrb_bedit_dump(mrb_state * mrb, mrb_value self)           { auto result = (Application::getObject())->bedit_dump(mrb, self);   mrb_garbage_collect(mrb);  return result; }
mrb_value mrb_bedit_get(mrb_state * mrb, mrb_value self)            { auto result = (Application::getObject())->bedit_get(mrb, self);    mrb_garbage_collect(mrb);  return result; }
mrb_value mrb_bedit_set(mrb_state * mrb, mrb_value self)            { auto result = (Application::getObject())->bedit_set(mrb, self);    mrb_garbage_collect(mrb);  return result; }
mrb_value mrb_bedit_pos(mrb_state * mrb, mrb_value self)            { auto result = (Application::getObject())->bedit_pos(mrb, self);                               return result; }

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
            ("baud,b",          boost::program_options::value<std::string>(),   "baud rate      Default 1200O1 ex) -b 9600E1"     )
            ("gap,g",           boost::program_options::value<unsigned int>(),  "time out tick. Default   30 ( 30 [ms])"          )
            ("timer,t",         boost::program_options::value<unsigned int>(),  "time out tick. Default  300 (300 [ms])"          )
            ("timer2",          boost::program_options::value<unsigned int>(),  "time out tick. Default  500 (500 [ms])"          )
            ("timer3",          boost::program_options::value<unsigned int>(),  "time out tick. Default 1000 (  1 [s])"           )
            ("no-rts",                                                          "no control RTS signal"                           )
            ("crc,c",           boost::program_options::value<std::string>(),   "calclate modbus RTU CRC"                         )
            ("crc8",            boost::program_options::value<std::string>(),   "calclate CRC8"                                   )
            ("sum,s",           boost::program_options::value<std::string>(),   "calclate checksum of XOR"                        )
            ("FLOAT,F",         boost::program_options::value<std::string>(),   "hex to float value"                              )
            ("float,f",         boost::program_options::value<std::string>(),   "litle endian hex to float value"                 )
            ("makeQR",          boost::program_options::value<std::string>(),   "make QR code of svg"                             )
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
            std::cout << "smon Revision " << SoftwareRevision << " , mruby Revision 3.3.0" << std::endl;
            std::cout << std::endl;
            std::cout << desc << std::endl;
            return 0;
        }
        if(argmap.count("help-misc"))
        {
            std::cout << "smon Revision " << SoftwareRevision << " , mruby Revision 3.3.0" << std::endl;
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
        Application app( argmap, arg );
        start = std::chrono::system_clock::now();
        app.main();
    }
    catch(std::exception & exp) { std::cerr << "exeption: " << exp.what() << std::endl; }
    catch(...)                  { std::cerr << "unknown exeption" << std::endl;         }
    return 0;
}
