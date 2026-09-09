# DrakeScript

```cpp
#include <DrakeScriptCore.hpp>
#include <DrakeScriptMappingESP32PSRAM.hpp>

DrakeScriptMappingESP32PSRAM<(1024 * 1024)> ScriptMapObj;
DrakeScriptCore ScriptObj(ScriptMapObj);

void Setup()
{
	// Инициалиризуем PSRAM
	ScriptMapObj.Init();

	// Копируем скрипты в PSRAM и добавляем их в карту скриптов
	ScriptMapObj.AddScript(id1, data1, length1);
	ScriptMapObj.AddScript(id2, data2, length2);

	// Добавляем доп. опкоды.
	ScriptObj.RegCustomOpcode((opcode_idx_t)0xA0, TestOpcode);
	ScriptObj.RegCustomOpcode((opcode_idx_t)0xA1, TestOpcode);
	
	return;
}

void Logic()
{
	// Выполняем скрипты для id, передавая trigger data в качестве параметров
	ScriptLogic::ScriptObj.Trigger(id, obj.data, obj.length);

	return;
}

struct __attribute__((packed)) MyOpcodeOne_t
{
	uint8_t opcode;
	reg_idx_t reg1;
	reg_idx_t reg2;
	uint16_t to_addr;
};

struct __attribute__((packed)) MyOpcodeTwo_t
{
	uint8_t opcode;
	reg_idx_t reg1;
	reg_idx_t reg2;
	uint16_t to_addr;
};

void TestOpcode(DrakeScriptRegisters &registers, const uint8_t *bytes, uint16_t &offset)
{
	opcode_idx_t opcode = (opcode_idx_t)bytes[0];
	
	switch(opcode)
	{
		case 0xA0:
		{
			MyOpcodeOne_t *obj = (MyOpcodeOne_t *) bytes;

			// Логика опкода

			offset += sizeof(*obj);
			break;
		}
		case 0xA1:
		{
			MyOpcodeTwo_t *obj = (MyOpcodeTwo_t *) bytes;

			// Логика опкода

			offset += sizeof(*obj);
			break;
		}
	}
	
	return;
}
```
