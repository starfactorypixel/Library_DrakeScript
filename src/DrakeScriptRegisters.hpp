#pragma once
#include <inttypes.h>
#include <string.h>

class DrakeScriptRegisters
{
	public:
		
		typedef int32_t reg_type_t;
		enum reg_idx_t : uint8_t
		{
			REG_DUMMY = 0,
			
			REG_TRIG_ID = 1,
			REG_NOW_ID = 2,
			
			REG_A = 3,
			REG_B = 4,
			REG_C = 5,
			REG_D = 6,
			REG_E = 7,
			REG_F = 8,
			REG_G = 9,
			REG_H = 10,
			REG_I = 11,
			REG_J = 12,
			REG_K = 13,
			REG_L = 14,
			REG_M = 15,
		};
		
		DrakeScriptRegisters() : _registers{}
		{}
		
		inline void RegisterAllClear()
		{
			memset(_registers, 0x00, sizeof(_registers));
			
			return;
		}
		
		inline reg_type_t RegisterGet(reg_idx_t idx)
		{
			if(idx >= (sizeof(_registers) / sizeof(reg_type_t)))
				return _registers[0];
			
			return _registers[idx];
		}
		
		inline void RegisterSet(reg_idx_t idx, reg_type_t value)
		{
			if(idx >= (sizeof(_registers) / sizeof(reg_type_t)))
				return;
			
			_registers[idx] = value;
			return;
		}
		
		inline reg_type_t &Register(reg_idx_t idx)
		{
			if(idx >= (sizeof(_registers) / sizeof(reg_type_t)))
				return _registers[0];
			
			return _registers[idx];
		}
		
	private:
		
		reg_type_t _registers[1 + 2 + 13];
};
