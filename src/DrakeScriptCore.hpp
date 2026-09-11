#pragma once
#include <inttypes.h>
#include "DrakeScriptMappingInterface.hpp"
#include "DrakeScriptRegisters.hpp"
#include "DrakeScriptOperators.hpp"

using namespace DrakeScript;

class DrakeScriptCore
{
	static constexpr uint8_t MAX_CUSTOM_OPCODE = 16;
	
	using opcode_func_t = void (*)(DrakeScriptRegisters &registers, const uint8_t *bytes, uint16_t &offset);
	
	enum ctrl_t : uint8_t
	{
		CTRL_NORMAL = 0,			// Нормальный режим
		CTRL_EXIT,					// Выход из скрипта
		CTRL_JUMP_SCRIPT,			// Переход на новый скрипт. Новый id скрипта записываетися в _trigger.id
	};
	
	public:
		
		DrakeScriptCore(DrakeScriptMappingInterface &map) : _mapping(map), _trigger{}, _custom_opcode{}
		{}
		
		void RegCustomOpcode(opcode_idx_t opcode, opcode_func_t func)
		{
			for(auto &obj : _custom_opcode)
			{
				if(obj.opcode != 0x00) continue;
				
				obj = {opcode, func};
				break;
			}
			
			return;
		}
		
		void DelCustomOpcode(opcode_idx_t opcode)
		{
			for(auto &obj : _custom_opcode)
			{
				if(obj.opcode != opcode) continue;
				
				obj = {(opcode_idx_t)0x00, nullptr};
				break;
			}
			
			return;
		}

		/*
			Триггер запуска скрипта
			 - `uint16_t id` - ID скрипта, например CAN ID;
			 - `const uint8_t *data` - Данные передаваемые в скрипт для парсинга, например данные CAN;
			 - `uint8_t length` - Длина данных;
		*/
		void Trigger(uint16_t id, const uint8_t *data, uint8_t length)
		{
			_trigger.id = id;
			_trigger.ctrl = CTRL_NORMAL;
			_trigger.length = length;
			_trigger.data = data;
			
			_RunScript();
			
			return;
		}
		
	private:
		
		void _RunScript()
		{
			uint8_t *script_ptr = nullptr;
			uint16_t script_length = 0;
			if(_mapping.GetScriptPtr(_trigger.id, script_ptr, script_length) == false) return;

			uint16_t offset = 0;
			while(offset < script_length)
			{
				uint8_t *pointer = &script_ptr[offset];
				_RunOpcode(pointer, offset);
				
				switch(_trigger.ctrl)
				{
					case CTRL_EXIT:
					{
						return;
					};
					case CTRL_JUMP_SCRIPT:
					{
						if(_mapping.GetScriptPtr(_trigger.id, script_ptr, script_length) == false) return;
						offset = 0;
						
						break;
					};
				}
			}
			
			return;
		}
		
		/*
			Запуск выполнения текущего опкода с параметрами
			 - `const uint8_t *bytes` - указатель на начало опкода с парметрами;
			 - `uint16_t &offset` - смещение для расчёта следующего опкода, байт;
		*/
		void _RunOpcode(const uint8_t *bytes, uint16_t &offset)
		{
			opcode_idx_t opcode = (opcode_idx_t)bytes[0];
			
			switch(opcode)
			{
				case OP_ScriptInit:
				{
					ScriptInit_t *obj = (ScriptInit_t *) bytes;
					
					_registers.RegisterAllClear();
					_registers.Register(_registers.REG_SCRIPT_ID) = _trigger.id;
					
					if(obj->mode == 0)
					{
						_registers.Register(_registers.REG_DATA1) = obj->data[0];
						_registers.Register(_registers.REG_DATA2) = obj->data[1];
						_registers.Register(_registers.REG_DATA3) = obj->data[2];
						_registers.Register(_registers.REG_DATA4) = obj->data[3];
					}
					else
					{
						int32_t value = obj->data[0] | (obj->data[1] << 8) | (obj->data[2] << 16) | (obj->data[3] << 24);
						_registers.Register(_registers.REG_DATA1) = value;
					}
					
					offset += sizeof(*obj);
					break;
				}
				case OP_TriggerParseReg:
				{
					TriggerParseReg_t *obj = (TriggerParseReg_t *) bytes;

					_registers.Register(obj->reg1) = read_i32_fast(&_trigger.data[obj->offset], obj->type);
					
					offset += sizeof(*obj);
					break;
				}
				case OP_SetScriptArgVal:
				{
					SetScriptArgVal_t *obj = (SetScriptArgVal_t *) bytes;
					
					_SetScriptArg(obj->script_id, 0, obj->data);
					
					offset += sizeof(*obj);
					break;
				}
				case OP_SetScriptArgReg8:
				{
					SetScriptArgReg8_t *obj = (SetScriptArgReg8_t *) bytes;
					
					uint8_t data[4] = 
					{
						(uint8_t)(_registers.Register(obj->reg1)), 
						(uint8_t)(_registers.Register(obj->reg2)), 
						(uint8_t)(_registers.Register(obj->reg3)), 
						(uint8_t)(_registers.Register(obj->reg4)) 
					};
					_SetScriptArg(obj->script_id, 0, data);
					
					offset += sizeof(*obj);
					break;
				}
				case OP_SetScriptArgReg32:
				{
					SetScriptArgReg32_t *obj = (SetScriptArgReg32_t *) bytes;
					
					uint8_t data[4] = 
					{
						(uint8_t)(_registers.Register(obj->reg1)), 
						(uint8_t)(_registers.Register(obj->reg1) >> 8), 
						(uint8_t)(_registers.Register(obj->reg1) >> 16), 
						(uint8_t)(_registers.Register(obj->reg1) >> 24) 
					};
					_SetScriptArg(obj->script_id, 1, data);
					
					offset += sizeof(*obj);
					break;
				}
				
				case OP_IfRegValEqu:
				{
					IfRegValEqu_t *obj = (IfRegValEqu_t *) bytes;
					
					offset += sizeof(*obj);
					if(!(_registers.Register(obj->reg1) == obj->value))
						offset = obj->to_addr;
					
					break;
				}
				case OP_IfRegValNeq:
				{
					IfRegValNeq_t *obj = (IfRegValNeq_t *) bytes;
					
					offset += sizeof(*obj);
					if(!(_registers.Register(obj->reg1) != obj->value))
						offset = obj->to_addr;
					
					break;
				}
				case OP_IfRegValLss:
				{
					IfRegValLss_t *obj = (IfRegValLss_t *) bytes;
					
					offset += sizeof(*obj);
					if(!(_registers.Register(obj->reg1) < obj->value))
						offset = obj->to_addr;
					
					break;
				}
				case OP_IfRegValLeq:
				{
					IfRegValLeq_t *obj = (IfRegValLeq_t *) bytes;
					
					offset += sizeof(*obj);
					if(!(_registers.Register(obj->reg1) <= obj->value))
						offset = obj->to_addr;
					
					break;
				}
				case OP_IfRegValGtr:
				{
					IfRegValGtr_t *obj = (IfRegValGtr_t *) bytes;
					
					offset += sizeof(*obj);
					if(!(_registers.Register(obj->reg1) > obj->value))
						offset = obj->to_addr;
					
					break;
				}
				case OP_IfRegValGeq:
				{
					IfRegValGeq_t *obj = (IfRegValGeq_t *) bytes;
					
					offset += sizeof(*obj);
					if(!(_registers.Register(obj->reg1) >= obj->value))
						offset = obj->to_addr;
					
					break;
				}
				case OP_IfRegRegEqu:
				{
					IfRegRegEqu_t *obj = (IfRegRegEqu_t *) bytes;
					
					offset += sizeof(*obj);
					if(!(_registers.Register(obj->reg1) == _registers.Register(obj->reg2)))
						offset = obj->to_addr;
					
					break;
				}
				case OP_IfRegRegNeq:
				{
					IfRegRegNeq_t *obj = (IfRegRegNeq_t *) bytes;
					
					offset += sizeof(*obj);
					if(!(_registers.Register(obj->reg1) != _registers.Register(obj->reg2)))
						offset = obj->to_addr;
					
					break;
				}
				case OP_IfRegRegLss:
				{
					IfRegRegLss_t *obj = (IfRegRegLss_t *) bytes;
					
					offset += sizeof(*obj);
					if(!(_registers.Register(obj->reg1) < _registers.Register(obj->reg2)))
						offset = obj->to_addr;
					
					break;
				}
				case OP_IfRegRegLeq:
				{
					IfRegRegLeq_t *obj = (IfRegRegLeq_t *) bytes;
					
					offset += sizeof(*obj);
					if(!(_registers.Register(obj->reg1) <= _registers.Register(obj->reg2)))
						offset = obj->to_addr;
					
					break;
				}
				case OP_IfRegRegGtr:
				{
					IfRegRegGtr_t *obj = (IfRegRegGtr_t *) bytes;
					
					offset += sizeof(*obj);
					if(!(_registers.Register(obj->reg1) > _registers.Register(obj->reg2)))
						offset = obj->to_addr;
					
					break;
				}
				case OP_IfRegRegGeq:
				{
					IfRegRegGeq_t *obj = (IfRegRegGeq_t *) bytes;
					
					offset += sizeof(*obj);
					if(!(_registers.Register(obj->reg1) >= _registers.Register(obj->reg2)))
						offset = obj->to_addr;
					
					break;
				}
				case OP_SetRegVal:
				{
					SetRegVal_t *obj = (SetRegVal_t *) bytes;
					
					_registers.Register(obj->reg1) = obj->value;
					
					offset += sizeof(*obj);
					break;
				}
				case OP_SetRegReg:
				{
					SetRegReg_t *obj = (SetRegReg_t *) bytes;
					
					_registers.Register(obj->reg1) = _registers.RegisterGet(obj->reg2);
					
					offset += sizeof(*obj);
					break;
				}
				case OP_IncReg:
				{
					IncReg_t *obj = (IncReg_t *) bytes;

					_registers.Register(obj->reg1) += 1;
					
					offset += sizeof(*obj);
					break;
				}
				case OP_DecReg:
				{
					DecReg_t *obj = (DecReg_t *) bytes;
					
					_registers.Register(obj->reg1) -= 1;
					
					offset += sizeof(*obj);
					break;
				}
				case OP_NotReg:
				{
					NotReg_t *obj = (NotReg_t *) bytes;
					
					_registers.Register(obj->reg1) = ~_registers.Register(obj->reg1);
					
					offset += sizeof(*obj);
					break;
				}
				case OP_ShiftLeftReg:
				{
					ShiftLeftReg_t *obj = (ShiftLeftReg_t *) bytes;
					
					_registers.Register(obj->reg1) <<= obj->count;
					
					offset += sizeof(*obj);
					break;
				}
				case OP_ShiftRightReg:
				{
					ShiftRightReg_t *obj = (ShiftRightReg_t *) bytes;
					
					_registers.Register(obj->reg1) >>= obj->count;
					
					offset += sizeof(*obj);
					break;
				}
				case OP_AndRegVal:
				{
					AndRegVal_t *obj = (AndRegVal_t *) bytes;
					
					_registers.Register(obj->reg1) &= obj->value;
					
					offset += sizeof(*obj);
					break;
				}
				case OP_AndRegReg:
				{
					AndRegReg_t *obj = (AndRegReg_t *) bytes;
					
					_registers.Register(obj->reg1) &= _registers.Register(obj->reg2);
					
					offset += sizeof(*obj);
					break;
				}
				case OP_OrRegVal:
				{
					OrRegVal_t *obj = (OrRegVal_t *) bytes;
					
					_registers.Register(obj->reg1) |= obj->value;
					
					offset += sizeof(*obj);
					break;
				}
				case OP_OrRegReg:
				{
					OrRegReg_t *obj = (OrRegReg_t *) bytes;
					
					_registers.Register(obj->reg1) |= _registers.Register(obj->reg2);
					
					offset += sizeof(*obj);
					break;
				}
				case OP_AddRegVal:
				{
					AddRegVal_t *obj = (AddRegVal_t *) bytes;
					
					_registers.Register(obj->reg1) += obj->value;
					
					offset += sizeof(*obj);
					break;
				}
				case OP_SubRegVal:
				{
					SubRegVal_t *obj = (SubRegVal_t *) bytes;
					
					_registers.Register(obj->reg1) -= obj->value;
					
					offset += sizeof(*obj);
					break;
				}
				case OP_MulRegVal:
				{
					MulRegVal_t *obj = (MulRegVal_t *) bytes;
					
					_registers.Register(obj->reg1) *= obj->value;
					
					offset += sizeof(*obj);
					break;
				}
				case OP_DivRegVal:
				{
					DivRegVal_t *obj = (DivRegVal_t *) bytes;
					
					_registers.Register(obj->reg1) /= obj->value;
					
					offset += sizeof(*obj);
					break;
				}
				case OP_AddRegReg:
				{
					AddRegReg_t *obj = (AddRegReg_t *) bytes;
					
					_registers.Register(obj->reg1) += _registers.Register(obj->reg2);
					
					offset += sizeof(*obj);
					break;
				}
				case OP_SubRegReg:
				{
					SubRegReg_t *obj = (SubRegReg_t *) bytes;
					
					_registers.Register(obj->reg1) -= _registers.Register(obj->reg2);
					
					offset += sizeof(*obj);
					break;
				}
				case OP_MulRegReg:
				{
					MulRegReg_t *obj = (MulRegReg_t *) bytes;
					
					_registers.Register(obj->reg1) *= _registers.Register(obj->reg2);
					
					offset += sizeof(*obj);
					break;
				}
				case OP_DivRegReg:
				{
					DivRegReg_t *obj = (DivRegReg_t *) bytes;
					
					_registers.Register(obj->reg1) /= _registers.Register(obj->reg2);
					
					offset += sizeof(*obj);
					break;
				}
				case OP_Goto:
				{
					Goto_t *obj = (Goto_t *) bytes;
					
					offset = obj->to_addr;
					break;
				}
				case OP_Exit:
				case 0x00:
				case 0xFF:
				{
					Exit_t *obj = (Exit_t *) bytes;
					
					_trigger.ctrl = CTRL_EXIT;
					
					offset += sizeof(*obj);
					break;
				}
				case OP_Run:
				{
					Run_t *obj = (Run_t *) bytes;
					
					_trigger.id = obj->script_id;
					_trigger.ctrl = CTRL_JUMP_SCRIPT;
					
					offset += sizeof(*obj);
					break;
				}


				case OP_XorRegVal:
				{
					XorRegVal_t *obj = (XorRegVal_t *) bytes;
					
					_registers.Register(obj->reg1) ^= obj->value;
					
					offset += sizeof(*obj);
					break;
				}
				case OP_XorRegReg:
				{
					XorRegReg_t *obj = (XorRegReg_t *) bytes;
					
					_registers.Register(obj->reg1) ^= _registers.Register(obj->reg2);
					
					offset += sizeof(*obj);
					break;
				}

				case OP_ModRegVal:
				{
					ModRegVal_t *obj = (ModRegVal_t *) bytes;
					
					_registers.Register(obj->reg1) %= obj->value;
					
					offset += sizeof(*obj);
					break;
				}
				case OP_ModRegReg:
				{
					ModRegReg_t *obj = (ModRegReg_t *) bytes;
					
					_registers.Register(obj->reg1) %= _registers.Register(obj->reg2);
					
					offset += sizeof(*obj);
					break;
				}
				case OP_NegReg:
				{
					NegReg_t *obj = (NegReg_t *) bytes;
					
					_registers.Register(obj->reg1) = -_registers.Register(obj->reg1);
					
					offset += sizeof(*obj);
					break;
				}









				default:
				{
					for(auto &obj : _custom_opcode)
					{
						if(obj.opcode == opcode)
						{
							uint16_t old_offset = offset;
							obj.func(_registers, bytes, offset);
							if(old_offset == offset)
							{
								_trigger.ctrl = CTRL_EXIT;
							}
							
							break;
						}
					}
					break;
				}
			}
			
			return;
		}
		
		void _SetScriptArg(uint16_t id, uint8_t mode, uint8_t *data)
		{
			// Если указанный id == 0xFFFF то саморедактирование
			if(id == 0xFFFF) id = _trigger.id;
			
			uint8_t *script_ptr = nullptr;
			uint16_t script_length = 0;
			if(_mapping.GetScriptPtr(id, script_ptr, script_length) == false)
				return;
			
			ScriptInit_t *obj = (ScriptInit_t *) script_ptr;
			obj->mode = mode;
			memcpy(obj->data, data, sizeof(obj->data));
			
			return;
		}
		
		DrakeScriptMappingInterface &_mapping;
		DrakeScriptRegisters _registers;
		
		struct trigger_t
		{
			uint16_t id;			// ID выполняемого скрипта
			ctrl_t ctrl;			// Расширенное управление
			uint8_t length;			// Длина данных триггера
			const uint8_t *data;	// Данные триггера
		} _trigger;
		
		struct custom_opcode_t
		{
			opcode_idx_t opcode;
			opcode_func_t func;
		} _custom_opcode[MAX_CUSTOM_OPCODE];
};
