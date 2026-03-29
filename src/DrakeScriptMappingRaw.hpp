#pragma once
#include <inttypes.h>
#include "DrakeScriptMappingInterface.hpp"

class DrakeScriptMappingRaw : public DrakeScriptMappingInterface
{
	public:
		
		// Установить указатель на глобальный массив данным со скриптами.
		void SetScriptsArray(uint8_t *array, uint32_t length)
		{
			_scripts_array = array;
			_scripts_array_length = length;

			return;
		}
		
		// Добавить в карту скриптов новый скрипт
		// Скопировать код скрипта нужно самостоятельно
		void AddScriptMap(uint16_t id, uint32_t start_idx, uint16_t length)
		{
			if(id >= _max_scripts_count) return;

			_scripts_map[id] = {start_idx, length, 1};

			return;
		}
		
		// Получить указатель на начало скрипта и его длину в байтах
		virtual bool GetScriptPtr(uint16_t id, uint8_t *&array_ptr, uint16_t &length) override
		{
			if(id >= _max_scripts_count) return false;
			auto &obj = _scripts_map[id];
			if(obj.mode <= 0) return false;
			
			array_ptr = &_scripts_array[obj.start_idx];
			length = obj.length;
			
			return true;
		}
		
	private:
		
		uint8_t *_scripts_array = nullptr;
		uint32_t _scripts_array_length = 0;
};
