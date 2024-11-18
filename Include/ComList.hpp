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
#include <list>

/**
 * get COM list on Microsoft Windows
 */
class ComList
{
private:
    std::list<std::string> list;
public:
    ComList(void);
    virtual ~ComList(void);
    std::list<std::string> & ref();
};


#endif
