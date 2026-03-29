#pragma once
#include <inttypes.h>

class DrakeScriptMappingInterface
{
	public:
		
		virtual bool GetScriptPtr(uint16_t id, uint8_t *&array_ptr, uint16_t &length) = 0;
		
	protected:
		
		static constexpr uint16_t _max_scripts_count = 2048;
		
		struct script_map_t
		{
			uint32_t start_idx;			// Индекс начала скрипта
			uint16_t length;			// Длина скрипта
			int8_t mode;				// Режим работы скрипта: 0 - нету, -1 - отключён пользователем, -2 - отключён по ошибке, 1 - активен.
			//uint8_t _aligning;		// 
		} _scripts_map[_max_scripts_count];

};
