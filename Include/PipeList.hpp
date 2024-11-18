/**
 * Print pipe name list on microsoft windows
 *
 * @file    PipeList.hpp
 * @brief   get COM list on Microsoft Windows
 * @author  Shouji, Igarashi
 *
 * (c) 2019 Shouji, Igarashi.
 *
 * @see string
 * @see vector
 */

#ifndef __PIPE_LIST_HPP__
#define __PIPE_LIST_HPP__

#include <string>
#include <vector>

/**
 * get pipe name list on Microsoft Windows
 */
class PipeList
{
private:
    std::list<std::string> list;
public:
    PipeList(void);
    virtual ~PipeList(void);
    std::list<std::string> & ref();
};


#endif
