#pragma once
#include <inttypes.h>
#include <string.h>
#include "DrakeScriptMappingInterface.hpp"
#include "DrakeScriptRegisters.hpp"
#include "DrakeScriptOperators.hpp"

using namespace DrakeScript;

class DrakeScriptCore
{
	static constexpr uint16_t _max_iter_count = 1024;
	
	using opcode_func_t = void (*)(DrakeScriptRegisters &registers, const uint8_t *bytes, uint16_t &offset);

	enum ctrl_t : uint8_t
	{
		CTRL_NORMAL = 0,			// Нормальный режим
		CTRL_EXIT,					// Выход из скрипта
		CTRL_JUMP_SCRIPT,			// Переход на новый скрипт. Новый id скрипта записывается в _trigger.id
	};
	
	public:
		
		DrakeScriptCore(DrakeScriptMappingInterface &map) : _mapping(map), _trigger{}, _custom_opcodes(nullptr)
		{}
		
		void SetCustomOpcodeFunc(opcode_func_t func)
		{
			_custom_opcodes = func;
			
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
			_trigger.trig_id = id;
			_trigger.trig_len = length;
			_trigger.trig_data = data;
			_trigger.ctrl = CTRL_NORMAL;
			_trigger.now_id = id;
			_trigger.iter = 0;

			_RunScript();
			
			return;
		}
		
	private:
		
		void _RunScript()
		{
			uint8_t *script_ptr = nullptr;
			uint16_t script_length = 0;
			if(_mapping.GetScriptPtr(_trigger.trig_id, script_ptr, script_length) == false) return;

			uint16_t offset = 0;
			while(offset < script_length)
			{
				uint8_t *pointer = &script_ptr[offset];
				_RunOpcode(pointer, offset);
				
				if(++_trigger.iter >= _max_iter_count)
				{
					_mapping.DisableScript(_trigger.trig_id, _mapping.STATE_DISABLED_LOOP);
					return;
				}
				
				switch(_trigger.ctrl)
				{
					case CTRL_EXIT:
					{
						return;
					};
					case CTRL_JUMP_SCRIPT:
					{
						if(_mapping.GetScriptPtr(_trigger.now_id, script_ptr, script_length) == false) return;
						offset = 0;
						
						break;
					};
				}
				_trigger.ctrl = CTRL_NORMAL;
			}
			
			return;
		}
		
		/*
			Запуск выполнения текущего опкода с параметрами
			 - `const uint8_t *bytes` - указатель на начало опкода с параметрами;
			 - `uint16_t &offset` - смещение для расчёта следующего опкода, байт;
		*/
		void _RunOpcode(const uint8_t *bytes, uint16_t &offset)
		{
			auto opcode = (const opcode_idx_t)bytes[0];
			
			switch(opcode)
			{
				case OP_ScriptInit:
				{
					auto obj = (const ScriptInit_t *) bytes;
					
					_registers.RegisterAllClear();
					_registers.Register(_registers.REG_TRIG_ID) = _trigger.trig_id;
					_registers.Register(_registers.REG_NOW_ID) = _trigger.now_id;
					
					offset += sizeof(*obj);
					break;
				}
				case OP_TriggerParseReg:
				{
					auto obj = (const TriggerParseReg_t *) bytes;

					_registers.Register(obj->reg1) = read_i32_fast(&_trigger.trig_data[obj->offset], obj->type);
					
					offset += sizeof(*obj);
					break;
				}
				case OP_SetScriptArgVal:
				{
					auto obj = (const SetScriptArgVal_t *) bytes;
					
					_SetScriptArg(obj->script_id, obj->type, obj->offset, obj->value);
					
					offset += sizeof(*obj);
					break;
				}
				case OP_SetScriptArgReg:
				{
					auto obj = (const SetScriptArgReg_t *) bytes;
					
					_SetScriptArg(obj->script_id, obj->type, obj->offset, _registers.Register(obj->reg1));
					
					offset += sizeof(*obj);
					break;
				}
				case OP_GetScriptArgReg:
				{
					auto obj = (const GetScriptArgReg_t *) bytes;
					
					_GetScriptArg(obj->script_id, obj->type, obj->offset, _registers.Register(obj->reg1));
					
					offset += sizeof(*obj);
					break;
				}
				
				case OP_IfRegValEqu:
				{
					auto obj = (const IfRegValEqu_t *) bytes;
					
					offset += sizeof(*obj);
					if(!(_registers.Register(obj->reg1) == obj->value))
						offset = obj->to_addr;
					
					break;
				}
				case OP_IfRegRegEqu:
				{
					auto obj = (const IfRegRegEqu_t *) bytes;
					
					offset += sizeof(*obj);
					if(!(_registers.Register(obj->reg1) == _registers.Register(obj->reg2)))
						offset = obj->to_addr;
					
					break;
				}
				case OP_IfRegValNeq:
				{
					auto obj = (const IfRegValNeq_t *) bytes;
					
					offset += sizeof(*obj);
					if(!(_registers.Register(obj->reg1) != obj->value))
						offset = obj->to_addr;
					
					break;
				}
				case OP_IfRegRegNeq:
				{
					auto obj = (const IfRegRegNeq_t *) bytes;
					
					offset += sizeof(*obj);
					if(!(_registers.Register(obj->reg1) != _registers.Register(obj->reg2)))
						offset = obj->to_addr;
					
					break;
				}
				case OP_IfRegValLss:
				{
					auto obj = (const IfRegValLss_t *) bytes;
					
					offset += sizeof(*obj);
					if(!(_registers.Register(obj->reg1) < obj->value))
						offset = obj->to_addr;
					
					break;
				}
				case OP_IfRegRegLss:
				{
					auto obj = (const IfRegRegLss_t *) bytes;
					
					offset += sizeof(*obj);
					if(!(_registers.Register(obj->reg1) < _registers.Register(obj->reg2)))
						offset = obj->to_addr;
					
					break;
				}
				case OP_IfRegValLeq:
				{
					auto obj = (const IfRegValLeq_t *) bytes;
					
					offset += sizeof(*obj);
					if(!(_registers.Register(obj->reg1) <= obj->value))
						offset = obj->to_addr;
					
					break;
				}
				case OP_IfRegRegLeq:
				{
					auto obj = (const IfRegRegLeq_t *) bytes;
					
					offset += sizeof(*obj);
					if(!(_registers.Register(obj->reg1) <= _registers.Register(obj->reg2)))
						offset = obj->to_addr;
					
					break;
				}
				case OP_IfRegValGtr:
				{
					auto obj = (const IfRegValGtr_t *) bytes;
					
					offset += sizeof(*obj);
					if(!(_registers.Register(obj->reg1) > obj->value))
						offset = obj->to_addr;
					
					break;
				}
				case OP_IfRegRegGtr:
				{
					auto obj = (const IfRegRegGtr_t *) bytes;
					
					offset += sizeof(*obj);
					if(!(_registers.Register(obj->reg1) > _registers.Register(obj->reg2)))
						offset = obj->to_addr;
					
					break;
				}
				case OP_IfRegValGeq:
				{
					auto obj = (const IfRegValGeq_t *) bytes;
					
					offset += sizeof(*obj);
					if(!(_registers.Register(obj->reg1) >= obj->value))
						offset = obj->to_addr;
					
					break;
				}
				case OP_IfRegRegGeq:
				{
					auto obj = (const IfRegRegGeq_t *) bytes;
					
					offset += sizeof(*obj);
					if(!(_registers.Register(obj->reg1) >= _registers.Register(obj->reg2)))
						offset = obj->to_addr;
					
					break;
				}
				
				case OP_SetRegVal:
				{
					auto obj = (const SetRegVal_t *) bytes;
					
					_registers.Register(obj->reg1) = obj->value;
					
					offset += sizeof(*obj);
					break;
				}
				case OP_SetRegReg:
				{
					auto obj = (const SetRegReg_t *) bytes;
					
					_registers.Register(obj->reg1) = _registers.Register(obj->reg2);
					
					offset += sizeof(*obj);
					break;
				}
				
				case OP_IncReg:
				{
					auto obj = (const IncReg_t *) bytes;

					_registers.Register(obj->reg1) += 1;
					
					offset += sizeof(*obj);
					break;
				}
				case OP_DecReg:
				{
					auto obj = (const DecReg_t *) bytes;
					
					_registers.Register(obj->reg1) -= 1;
					
					offset += sizeof(*obj);
					break;
				}
				case OP_NegReg:
				{
					auto obj = (const NegReg_t *) bytes;
					
					_registers.Register(obj->reg1) = -_registers.Register(obj->reg1);
					
					offset += sizeof(*obj);
					break;
				}
				case OP_AddRegVal:
				{
					auto obj = (const AddRegVal_t *) bytes;
					
					_registers.Register(obj->reg1) += obj->value;
					
					offset += sizeof(*obj);
					break;
				}
				case OP_AddRegReg:
				{
					auto obj = (const AddRegReg_t *) bytes;
					
					_registers.Register(obj->reg1) += _registers.Register(obj->reg2);
					
					offset += sizeof(*obj);
					break;
				}
				case OP_SubRegVal:
				{
					auto obj = (const SubRegVal_t *) bytes;
					
					_registers.Register(obj->reg1) -= obj->value;
					
					offset += sizeof(*obj);
					break;
				}
				case OP_SubRegReg:
				{
					auto obj = (const SubRegReg_t *) bytes;
					
					_registers.Register(obj->reg1) -= _registers.Register(obj->reg2);
					
					offset += sizeof(*obj);
					break;
				}
				case OP_MulRegVal:
				{
					auto obj = (const MulRegVal_t *) bytes;
					
					_registers.Register(obj->reg1) *= obj->value;
					
					offset += sizeof(*obj);
					break;
				}
				case OP_MulRegReg:
				{
					auto obj = (const MulRegReg_t *) bytes;
					
					_registers.Register(obj->reg1) *= _registers.Register(obj->reg2);
					
					offset += sizeof(*obj);
					break;
				}
				case OP_DivRegVal:
				{
					auto obj = (const DivRegVal_t *) bytes;
					
					reg_type_t val = obj->value;
					if(val != 0)
						_registers.Register(obj->reg1) /= val;
					
					offset += sizeof(*obj);
					break;
				}
				case OP_DivRegReg:
				{
					auto obj = (const DivRegReg_t *) bytes;
					
					reg_type_t val = _registers.Register(obj->reg2);
					if(val != 0)
						_registers.Register(obj->reg1) /= val;
					
					offset += sizeof(*obj);
					break;
				}
				case OP_ModRegVal:
				{
					auto obj = (const ModRegVal_t *) bytes;
					
					reg_type_t val = obj->value;
					if(val != 0)
						_registers.Register(obj->reg1) %= val;
					
					offset += sizeof(*obj);
					break;
				}
				case OP_ModRegReg:
				{
					auto obj = (const ModRegReg_t *) bytes;
					
					reg_type_t val = _registers.Register(obj->reg2);
					if(val != 0)
						_registers.Register(obj->reg1) %= val;
					
					offset += sizeof(*obj);
					break;
				}
				
				case OP_NotReg:
				{
					auto obj = (const NotReg_t *) bytes;
					
					_registers.Register(obj->reg1) = ~_registers.Register(obj->reg1);
					
					offset += sizeof(*obj);
					break;
				}
				case OP_AndRegVal:
				{
					auto obj = (const AndRegVal_t *) bytes;
					
					_registers.Register(obj->reg1) &= obj->value;
					
					offset += sizeof(*obj);
					break;
				}
				case OP_AndRegReg:
				{
					auto obj = (const AndRegReg_t *) bytes;
					
					_registers.Register(obj->reg1) &= _registers.Register(obj->reg2);
					
					offset += sizeof(*obj);
					break;
				}
				case OP_OrRegVal:
				{
					auto obj = (const OrRegVal_t *) bytes;
					
					_registers.Register(obj->reg1) |= obj->value;
					
					offset += sizeof(*obj);
					break;
				}
				case OP_OrRegReg:
				{
					auto obj = (const OrRegReg_t *) bytes;
					
					_registers.Register(obj->reg1) |= _registers.Register(obj->reg2);
					
					offset += sizeof(*obj);
					break;
				}
				case OP_XorRegVal:
				{
					auto obj = (const XorRegVal_t *) bytes;
					
					_registers.Register(obj->reg1) ^= obj->value;
					
					offset += sizeof(*obj);
					break;
				}
				case OP_XorRegReg:
				{
					auto obj = (const XorRegReg_t *) bytes;
					
					_registers.Register(obj->reg1) ^= _registers.Register(obj->reg2);
					
					offset += sizeof(*obj);
					break;
				}
				
				case OP_ShiftLeftReg:
				{
					auto obj = (const ShiftLeftReg_t *) bytes;
					
					_registers.Register(obj->reg1) <<= obj->count;
					
					offset += sizeof(*obj);
					break;
				}
				case OP_ShiftRightReg:
				{
					auto obj = (const ShiftRightReg_t *) bytes;
					
					_registers.Register(obj->reg1) >>= obj->count;
					
					offset += sizeof(*obj);
					break;
				}
				
				case OP_Goto:
				{
					auto obj = (const Goto_t *) bytes;
					
					offset = obj->to_addr;
					break;
				}
				case OP_Run:
				{
					auto obj = (const Run_t *) bytes;
					
					_trigger.now_id = obj->script_id;
					_trigger.ctrl = CTRL_JUMP_SCRIPT;
					
					offset += sizeof(*obj);
					break;
				}
				case OP_Exit:
				case 0x00:
				case 0xFF:
				{
					auto obj = (const Exit_t *) bytes;
					
					_trigger.ctrl = CTRL_EXIT;
					
					offset += sizeof(*obj);
					break;
				}
				
				default:
				{
					bool handled = false;
					
					if(_custom_opcodes != nullptr)
					{
						uint16_t old_offset = offset;
						_custom_opcodes(_registers, bytes, offset);
						handled = (offset != old_offset);
					}
					
					if(handled == false)
					{
						_mapping.DisableScript(_trigger.now_id, _mapping.STATE_DISABLED_UNKOP);
						_trigger.ctrl = CTRL_EXIT;
					}
					
					break;
				}
			}
			
			return;
		}
		
		void _SetScriptArg(uint16_t id, var_type_t type, uint8_t offset, reg_type_t value)
		{
			// Если указанный id == 0xFFFF то саморедактирование
			if(id == 0xFFFF) id = _trigger.now_id;

			// Если указанный id == 0xFFFE то редактирование скрипта входа
			if(id == 0xFFFE) id = _trigger.trig_id;

			uint8_t *script_ptr = nullptr;
			uint16_t script_length = 0;
			if(_mapping.GetScriptPtr(id, script_ptr, script_length) == false) return;
			if(script_length < sizeof(ScriptInit_t)) return;
			
			auto obj = (ScriptInit_t *) script_ptr;
			
			uint32_t dummy; uint8_t len = write_i32_fast((uint8_t *)&dummy, 0, type);
			
			if(offset + len > sizeof(obj->data)) return;
			
			memcpy(obj->data + offset, &value, len);
			
			return;
		}
		
		void _GetScriptArg(uint16_t id, var_type_t type, uint8_t offset, reg_type_t &value)
		{
			// Если указанный id == 0xFFFF то саморедактирование
			if(id == 0xFFFF) id = _trigger.now_id;

			// Если указанный id == 0xFFFE то редактирование скрипта входа
			if(id == 0xFFFE) id = _trigger.trig_id;

			uint8_t *script_ptr = nullptr;
			uint16_t script_length = 0;
			if(_mapping.GetScriptPtr(id, script_ptr, script_length) == false) return;
			if(script_length < sizeof(ScriptInit_t)) return;
			
			auto obj = (ScriptInit_t *) script_ptr;
			
			uint32_t dummy; uint8_t len = write_i32_fast((uint8_t *)&dummy, 0, type);
			
			if(offset + len > sizeof(obj->data)) return;
			
			value = 0;
			memcpy(&value, obj->data + offset, len);
			
			return;
		}

		DrakeScriptMappingInterface &_mapping;
		DrakeScriptRegisters _registers;
		
		struct trigger_t
		{
			uint16_t trig_id;			// ID скрипта при запуске
			uint8_t trig_len;			// Длина данных триггера при запуске
			const uint8_t *trig_data;	// Данные триггера при запуске

			ctrl_t ctrl;				// Расширенное управление
			uint16_t now_id;			// ID текущего скрипта
			uint16_t iter;				// Кол-во итераций скрипта
		} _trigger;

		opcode_func_t _custom_opcodes;
};
