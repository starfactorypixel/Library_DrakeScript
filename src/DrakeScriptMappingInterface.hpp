#pragma once
#include <inttypes.h>

class DrakeScriptMappingInterface
{
	public:
		
		enum mode_t : uint8_t
		{
			MODE_DISABLED,
			MODE_ENABLED,
		};
		
		virtual bool GetScriptPtr(uint16_t id, uint8_t *&array_ptr, uint16_t &length) = 0;
		
		void EnableScript(uint16_t id)
		{
			auto &obj = _scripts_map[id];
			obj.mode = MODE_ENABLED;

			return;
		}
		
		void DisableScript(uint16_t id)
		{
			auto &obj = _scripts_map[id];
			obj.mode = MODE_DISABLED;
			
			return;
		}
		
	protected:
		
		static constexpr uint16_t _max_scripts_count = 2048;
		
		void Init()
		{
			memset(_scripts_map, 0x00, sizeof(_scripts_map));
			
			return;
		}
		
		struct script_map_t
		{
			uint32_t start_idx;			// Индекс начала скрипта
			uint16_t length;			// Длина скрипта
			mode_t mode;				// Режим работы скрипта
		} _scripts_map[_max_scripts_count];
};
