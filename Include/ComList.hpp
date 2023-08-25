/**
 * Print COM list on microsoft windows
 *
 * @file    ComList.hpp
 * @brief   get COM list on Microsoft Windows
 * @author  Shouji, Igarashi
 *
 * (c) 2019 Shouji, Igarashi.
 *
 * @see string
 * @see vector
 */

#ifndef __COM_LIST_HPP__
#define __COM_LIST_HPP__

#include <string>
#include <vector>

/**
 * get COM list on Microsoft Windows
 */
class ComList
{
private:
    std::vector<std::string> list;
public:
    ComList(void);
    virtual ~ComList(void);
    std::vector<std::string> & ref();
};


#endif
