#pragma once
#include <inttypes.h>

class DrakeScriptMappingInterface
{
	public:
		
		enum state_t : int8_t
		{
			STATE_DISABLED_LOOP = -2,		// Отключен по причине зацикливания
			STATE_DISABLED = -1,			// Отключен
			STATE_NONE = 0,					// Не загружен
			STATE_ENABLED = 1,				// Включён
		};
		
		virtual bool GetScriptPtr(uint16_t id, uint8_t *&array_ptr, uint16_t &length, bool always = false) = 0;
		
		void EnableScript(uint16_t id)
		{
			if(id >= _max_scripts_count) return;
			
			auto &obj = _scripts_map[id];
			obj.state = STATE_ENABLED;

			return;
		}
		
		void DisableScript(uint16_t id, state_t state = STATE_DISABLED)
		{
			if(id >= _max_scripts_count) return;
			
			auto &obj = _scripts_map[id];
			obj.state = state;

			return;
		}
		
		bool IsLoaded(uint16_t id)
		{
			if(id >= _max_scripts_count) return false;
			
			auto &obj = _scripts_map[id];
			return (obj.state != STATE_NONE && obj.length > 0);
		}

		state_t GetState(uint16_t id)
		{
			if(id >= _max_scripts_count) return STATE_NONE;
			
			auto &obj = _scripts_map[id];
			return obj.state;
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
			state_t state;				// Статус работы скрипта
		} _scripts_map[_max_scripts_count];
};
