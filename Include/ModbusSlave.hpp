/**
 * MODBUS protocol slave controler class.
 *
 * @file ModbusSlave.hpp
 * @brief  Modbus Protocol Class
 * @author Shouji, Igarashi
 *
 * (c) 2018 Shouji, Igarashi.
 */

#ifndef __ModbusSlave_hpp__
#define __ModbusSlave_hpp__

#include "Entity.hpp"

namespace MyEntity
{
    class ModbusReciver
    {
    public:
        virtual void resetStatus(void) = 0;
        virtual bool hasFunctionCode(unsigned char fc) const = 0;
        virtual bool setData(unsigned char data) = 0;
        virtual bool isComplete(void) const = 0;
        virtual void frameComp(unsigned short calc_crc, unsigned short rcv_crc) = 0;
        virtual unsigned short makeResponse(unsigned char frame[], unsigned short size ) const = 0;
    };

    class UnsupportFunctionReciver : public ModbusReciver
    {
    public:
        inline UnsupportFunctionReciver(void);
        inline ~UnsupportFunctionReciver(void);
        virtual void resetStatus(void);
        virtual bool hasFunctionCode(unsigned char fc) const;
        virtual bool setData(unsigned char data);
        virtual bool isComplete(void) const;
        virtual void frameComp(unsigned short calc_crc, unsigned short rcv_crc);
        virtual unsigned short makeResponse(unsigned char frame[], unsigned short size) const;
    };

    class ModbusFunction : public ModbusReciver
    {
    public:
        typedef enum
        {
            ReadInputRegister           = 0x04,
            ReadHoldingRegisters        = 0x03,
            WriteSingleRegister         = 0x06,
            WriteMultipleRegisters      = 0x10,
            MaskWriteRegister           = 0x16,
            ReadWriteMultiPleRegisters  = 0x17
        } FunctionCode;

        typedef struct
        {
            unsigned short address;
            unsigned short size;
            unsigned short * reg;
        } Registers;

    protected:
        ConstArray<const Registers> & reg;
        inline ModbusFunction(ConstArray<const Registers> & _reg);
    public:
        virtual ~ModbusFunction(void);
        virtual void resetStatus(void);
        virtual bool setData(unsigned char data);
        virtual bool isComplete(void) const;
        virtual void frameComp(unsigned short calc_crc, unsigned short rcv_crc);
        virtual unsigned short makeResponse(unsigned char frame[], unsigned short size) const;
    protected:
        bool setDataToReadFunction(unsigned char frame[], unsigned short size, unsigned char data);
    };

    class InputRegister : public ModbusFunction
    {
    private:
        unsigned short start;
        unsigned short quantity;
        unsigned short state;
    public:
        inline InputRegister(ConstArray<const Registers> & reg);
        virtual ~InputRegister(void);
        virtual void resetStatus(void);
        virtual bool hasFunctionCode(unsigned char fc) const;
        virtual bool setData(unsigned char data);
        virtual bool isComplete(void) const;
        virtual void frameComp(unsigned short calc_crc, unsigned short rcv_crc);
        virtual unsigned short makeResponse(unsigned char frame[], unsigned short size) const;
    };

    class ModbusListiner
    {
    public:
        typedef enum
        {
            RCV_OK,
            RCV_NOK,
            UNSUPPORT_FC
        } CompResult;
        virtual void rcvSlaveAddress(unsigned char addr);
        virtual void rcvFunctionCode(unsigned char fc);
        virtual void rcvData(void);
        virtual void frameComp(unsigned short calc_crc, unsigned short rcv_crc);
        virtual void rcvComp(ModbusListiner::CompResult result);
    };

    class ModbusTimeOut
    {
    public:
        typedef enum
        {
            Octed1_5,
            Octed3_5
        } TimeOutType;
        virtual void timeout(ModbusTimeOut::TimeOutType type) = 0;
    };

    class ModbusTimmer
    {
    public:
        virtual void start( void ) = 0;
        virtual void stop( void ) = 0;
        inline void restart( void );
    };

    class ModbusRtuReciver : public ModbusReciver, ModbusTimeOut
    {
    public:
        typedef enum
        {
            WAITING_ADDR,
            WAITING_FC,
            WAITING_DATA,
            WAITING_CRC_HI,
            WAITING_CRC_LO,
            WAITING_COMP,
            UNSUPPORT_FC_CRC_HI,
            UNSUPPORT_FC_CRC_LO,
            UNSUPPORT_FC,
            UNSUPPORT_FC_TO,
            RCV_OK,
            RCV_NOK
        } State;
    private:
        ModbusListiner &                            listiner;
        ConstArray<ModbusFunction * const> & recivers;
        ModbusTimmer *                              timer;
        unsigned char                               slave_addr;

        ModbusReciver *                 rcv;
        State                           status;
        UnsupportFunctionReciver        unssppurt_fc;
        CalcCRC16                       calc_crc;
        unsigned short                  rcv_crc;

    public:
        inline ModbusRtuReciver(ConstArray<ModbusFunction * const> & _recivers, ModbusListiner & _listiner);
        virtual ~ModbusRtuReciver(void);

        virtual void resetStatus(void);
        virtual bool hasFunctionCode(unsigned char fc) const;
        virtual bool setData(unsigned char data);
        virtual bool isComplete(void) const;
        virtual void frameComp(unsigned short calc_crc, unsigned short rcv_crc);
        virtual unsigned short makeResponse(unsigned char frame[], unsigned short size) const;

        virtual void timeout(ModbusTimeOut::TimeOutType type);

        inline void setTimerControl(ModbusTimmer * timer);
        inline void setSlaveAddress(unsigned char slave_addr);
        inline unsigned short getCalcCRC(void) const;
        inline unsigned short getReciveCRC(void) const;
        inline bool checkCRC(void) const;
        inline void setReciveError(void);
    };


    // -----<< UnsupportFunctionReciver >>-----
    inline UnsupportFunctionReciver::UnsupportFunctionReciver(void)
    {
    }

    inline UnsupportFunctionReciver::~UnsupportFunctionReciver(void)
    {
    }

    // -----<< ModbusFunction >>-----
    inline ModbusFunction::ModbusFunction(ConstArray<const Registers> & _reg)
        : reg(_reg)
    {
    }

    // -----<< InputRegister >>-----
    inline InputRegister::InputRegister(ConstArray<const Registers> & _reg)
      : ModbusFunction(_reg)
    {
    }

    // -----<< ModbusTimmer >>-----
    inline void ModbusTimmer::restart( void )
    {
        stop();
        start();
    }

    // -----<< ModbusRtuReciver >>-----
    inline ModbusRtuReciver::ModbusRtuReciver(ConstArray<ModbusFunction * const> & _recivers, ModbusListiner & _listiner)
        : recivers(_recivers), listiner(_listiner), timer(nullptr), slave_addr(0x01), rcv(&unssppurt_fc)
    {
        resetStatus();
    }

    inline void ModbusRtuReciver::setTimerControl(ModbusTimmer * timer)
    {
        this->timer = timer;
    }

    inline void ModbusRtuReciver::setSlaveAddress(unsigned char slave_addr)
    {
        this->slave_addr = slave_addr;
    }

    inline unsigned short ModbusRtuReciver::getCalcCRC(void) const
    {
        //unsigned short crc = *calc_crc;
        unsigned short crc = 0xffff;
        return crc;
    }

    inline unsigned short ModbusRtuReciver::getReciveCRC(void) const
    {
        return rcv_crc;
    }

    inline bool ModbusRtuReciver::checkCRC(void) const
    {
        if(getCalcCRC()!=rcv_crc)
        {
            return false;
        }
        return true;
    }

    inline void ModbusRtuReciver::setReciveError(void)
    {
        status = RCV_NOK;
    }
};


#endif
