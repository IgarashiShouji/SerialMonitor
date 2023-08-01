#ifndef RtsContorl_hpp
#define RtsContorl_hpp

class RtsContorl
{
public:
    virtual void set(void) = 0;
    virtual void clear(void) = 0;
    virtual bool status(void) const = 0;
};


#endif
