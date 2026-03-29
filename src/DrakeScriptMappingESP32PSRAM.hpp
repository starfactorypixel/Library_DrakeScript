#pragma once
#include <inttypes.h>
#include <string.h>
#include <esp_heap_caps.h>
#include "DrakeScriptMappingInterface.hpp"

template <uint32_t _psram_size> 
class DrakeScriptMappingESP32PSRAM : public DrakeScriptMappingInterface
{
	public:
		
		bool Init()
		{
			_psram_data = (uint8_t *)heap_caps_malloc(_psram_size, MALLOC_CAP_SPIRAM);

			return (_psram_data != nullptr);
		}
		
		// Скопировать тело скрипта в PSRAM блок
		bool AddScript(uint16_t id, const uint8_t *array, uint16_t length)
		{
			if(id >= _max_scripts_count) return false;
			if(_psram_data_offset + length > _psram_size) return false;

			memcpy(&_psram_data[_psram_data_offset], array, length);
			_scripts_map[id] = {_psram_data_offset, length, 1};
			_psram_data_offset += length;

			return true;
		}

		// Получить указатель на начало скрипта и его длину в байтах
		virtual bool GetScriptPtr(uint16_t id, uint8_t *&array_ptr, uint16_t &length) override
		{
			if(id >= _max_scripts_count) return false;
			auto &obj = _scripts_map[id];
			if(obj.mode <= 0) return false;
			
			array_ptr = &_psram_data[obj.start_idx];
			length = obj.length;
			
			return true;
		}

	private:
		
		uint8_t *_psram_data = nullptr;
		uint32_t _psram_data_offset = 0;
};
