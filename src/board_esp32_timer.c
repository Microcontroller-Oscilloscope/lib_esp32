/*
	board_esp32_timer.c - timer configuration for Espressif ESP32
	Copyright (C) 2025 Camren Chraplak

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/**
 * APB_CLK = 80,000,000Hz
 * 
 * freq = desired frequency (Hz)
 * 
 * freq = APB_CLK / (scalar * timerTicks)
 * 
 * 64-bit counter
 * 16-bit scalar
 */

#include <osc_common/common_timer.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>

#define TIMER_COUNT_ZERO 0U // value for setting timer tick count to 0
#define SCALAR_MAX UINT16_MAX // max value for timer scalar

typedef uint16_t prescalar_t; // pre scalar type
typedef uint64_t timertick_t; // timer tick type

#if ESP_IDF_VERSION_MAJOR == 4

#include <driver/timer.h>
#include <esp_intr_alloc.h>

typedef struct hw_timer_s {
	uint8_t group; // timer group
	uint8_t num; // timer number
} hard_timer_group_t;

// stores timer groups and numbers
static hard_timer_group_t timerGroups[4] = {
	{.group=0, .num=0}, // timer0
	{.group=1, .num=0}, // timer1
	{.group=0, .num=1}, // timer2
	{.group=1, .num=1}, // timer3
};

// hardware timer pointers
hard_timer_group_t *timers[] = {
	#if NUM_TIMERS >= 1
		NULL,
	#endif
	#if NUM_TIMERS >= 2
		NULL,
	#endif
	#if NUM_TIMERS >= 3
		NULL,
	#endif
	#if NUM_TIMERS >= 4
		NULL,
	#endif
};

typedef hard_timer_group_t** timer_ptr_t;

#elif ESP_IDF_VERSION_MAJOR == 5

#include <soc/clk_tree_defs.h>
#include <driver/gptimer_types.h>
#include <hal/timer_types.h>
#include <driver/gptimer.h>

#if NUM_TIMERS >= 1
	gptimer_handle_t handler0 = NULL;
#endif
#if NUM_TIMERS >= 2
	gptimer_handle_t handler1 = NULL;
#endif
#if NUM_TIMERS >= 3
	gptimer_handle_t handler2 = NULL;
#endif
#if NUM_TIMERS >= 4
	gptimer_handle_t handler3 = NULL;
#endif

gptimer_handle_t *timers[] = {
	#if NUM_TIMERS >= 1
		&handler0,
	#endif
	#if NUM_TIMERS >= 2
		&handler1,
	#endif
	#if NUM_TIMERS >= 3
		&handler2,
	#endif
	#if NUM_TIMERS >= 4
		&handler3,
	#endif
};

typedef gptimer_handle_t** timer_ptr_t;

// gptimer requires this as minimum freq
#define HARD_TIMER_FREQ_MIN 1221

#endif

uint8_t claimed = 0U; // stores whether timers were claimed or not

/**
 * Scales input priority
 * 
 * @param priority of type timer_priority_t
 * 
 * @note function can only run up to priority 'ESP_INTR_FLAG_LEVEL3' since functions are in c
 * 
 * @return priority flag for 'intr_alloc_flags' when calling 'timer_isr_callback_add'
 */
int setPriority(timer_priority_t priority) {
	#ifdef PLATFORMIO
		// use input priority for PlatformIO
		int adjustment = priority / (UINT8_MAX / 3);
		#if ESP_IDF_VERSION_MAJOR == 4
			return (1 << adjustment);
		#elif ESP_IDF_VERSION_MAJOR == 5
			return adjustment;
		#else
			return 0;
		#endif
	#else
		// use default priority with esp-idf
		return 0;
	#endif
}

/**
 * Gets timer based on desired timer
 * 
 * @param timer timer to select
 * 
 * @return pointer to timer selected
 */
timer_ptr_t getTimer(hard_timer_t timer) {

	if (timer == HARD_TIMER_INVALID) {
		return NULL;
	}
	return &timers[timer];
}

/**
 * Gets next unstarted and unclaimed timer
 * 
 * @return available timer
 */
hard_timer_t getNextTimer(void) {
	for (uint8_t i = 0; i < NUM_TIMERS; i++) {
		if (!hardTimerStarted(i) && !hardTimerClaimed(i)) {
			return (hard_timer_t)i;
		}
	}
	return HARD_TIMER_INVALID;
}

/**
 * Sets timer claimed state
 * 
 * @param timer timer to set
 * @param state whether or not timer is claimed
 */
void setTimerClaimed(hard_timer_t timer, bool state) {

	if (timer == HARD_TIMER_INVALID) {
		return;
	}
	if (state) {
		claimed |= (1 << (timer));
	}
	else {
		claimed &= (~(1 << (timer)));
	}
}

hard_timer_t claimTimer(struct hardTimerPriority *priority) {
	hard_timer_t timer = getNextTimer();
	if (timer != HARD_TIMER_INVALID) {
		setTimerClaimed(timer, true);
	}
	return timer;
}

bool unclaimTimer(hard_timer_t timer) {

	if (hardTimerClaimed(timer)) {
		setTimerClaimed(timer, false);
		return true;
	}
	return false;
}

bool hardTimerClaimed(hard_timer_t timer) {
	if (timer == HARD_TIMER_INVALID) {
		return false;
	}
	return !!(claimed & (1 << timer));
}

/**
 * Gets hard timer stats for target frequency
 * 
 * @param freq pointer to desired frequency in Hz
 * @param timer pointer to timer ID
 * @param scalar pointer to scalar value
 * @param timerTicks pointer to desired tick count
 * 
 * @return result of getting timer stats
 * 
 * @note freq value is changed to actual freq if values are slightly off
 */
enum HardTimerStatusReturn getHardTimerStats(freq_t *freq, hard_timer_t *timer, prescalar_t *scalar, timertick_t *timerTicks) {

	enum HardTimerStatusReturn status = HARD_TIMER_OK;

	// freq doesn't divide evenly into APB_CLK
	if (APB_CLK_FREQ % *freq != 0) {
		status = HARD_TIMER_SLIGHTLY_OFF;
	}

	// scalar * timerTicks = APB_CLK / freq
	freq_t target = APB_CLK_FREQ / *freq;

	if (target <= SCALAR_MAX) {
		// scalar within max value
		*scalar = (prescalar_t)target;
		*timerTicks = 1;
	}
	else {
		// scalar not within max value
		*scalar = 1;
		*timerTicks = (timertick_t)target;

		while (*timerTicks % 2 == 0 && *scalar * 2 <= SCALAR_MAX) {
			*timerTicks /= 2;
			*scalar *= 2;
		}
	}

	*freq = APB_CLK_FREQ / (*scalar * *timerTicks);

	if ((!hardTimerClaimed(*timer) && hardTimerStarted(*timer)) || *timer == HARD_TIMER_INVALID) {
		*timer = getNextTimer();
	}

	if (*timer == HARD_TIMER_INVALID) {
		return HARD_TIMER_FAIL;
	}

	return status;
}

bool hardTimerStarted(hard_timer_t timer) {

	timer_ptr_t timerPtr = getTimer(timer);

	if (timerPtr == NULL) {
		return false;
	}

	#if ESP_IDF_VERSION_MAJOR == 4
		if (*timerPtr != NULL) {
			return true;
		}
	#elif ESP_IDF_VERSION_MAJOR == 5
		if (**timerPtr != NULL) {
			return true;
		}
	#endif

	return false;
}

bool cancelHardTimer(hard_timer_t timer) {
	
	if (hardTimerStarted(timer)) {

		timer_ptr_t timerPtr = getTimer(timer);

		#if ESP_IDF_VERSION_MAJOR == 4
			// cancels timer
			timer_set_alarm((*timerPtr) -> group, (*timerPtr) -> num, false);
			timer_pause((*timerPtr) -> group, (*timerPtr) -> num);
			timer_set_counter_value((*timerPtr) -> group, (*timerPtr) -> num, TIMER_COUNT_ZERO);

			// deconstructs timer
			timer_isr_callback_remove((*timerPtr) -> group, (*timerPtr) -> num);
			timer_deinit((*timerPtr) -> group, (*timerPtr) -> num);

			*timerPtr = NULL;
			return true;
		#elif ESP_IDF_VERSION_MAJOR == 5

			gptimer_stop(**timerPtr);
			gptimer_disable(**timerPtr);
			gptimer_del_timer(**timerPtr);

			**timerPtr = NULL;
			return true;
		#endif
	}

	return false;
}

#if ESP_IDF_VERSION_MAJOR == 4

#define TIMER_CALLBACK_PROTOTYPE(name, callback) \
	static bool name(void *params) { \
		((void(*)())callback)(params); \
		return false; \
	}

#elif ESP_IDF_VERSION_MAJOR == 5

#define TIMER_CALLBACK_PROTOTYPE(name, callback) \
	static bool name(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *params) { \
		((void(*)())callback)(params); \
		return false; \
	}

#endif

#if NUM_TIMERS >= 1
	hard_timer_function_ptr_t timerFunc0;
	TIMER_CALLBACK_PROTOTYPE(timerCallback0, timerFunc0)
#endif
#if NUM_TIMERS >= 2
	hard_timer_function_ptr_t timerFunc1;
	TIMER_CALLBACK_PROTOTYPE(timerCallback1, timerFunc1)
#endif
#if NUM_TIMERS >= 3
	hard_timer_function_ptr_t timerFunc2;
	TIMER_CALLBACK_PROTOTYPE(timerCallback2, timerFunc2)
#endif
#if NUM_TIMERS >= 4
	hard_timer_function_ptr_t timerFunc3;
	TIMER_CALLBACK_PROTOTYPE(timerCallback3, timerFunc3)
#endif

bool setHardTimer(hard_timer_t *timer, freq_t *freq, hard_timer_function_ptr_t function, void* params, timer_priority_t priority) {
	
	if (function == NULL || freq == NULL || timer == NULL) {
		return false;
	}
	if (*freq == (freq_t)0 || *freq > HARD_TIMER_FREQ_MAX) {
		return false;
	}

	prescalar_t scalar;
	timertick_t timerTicks;

	if (getHardTimerStats(freq, timer, &scalar, &timerTicks) == HARD_TIMER_FAIL) {
		return false;
	}

	if (!hardTimerStarted(*timer)) {

		timer_ptr_t timerPtr = getTimer(*timer);

		#if ESP_IDF_VERSION_MAJOR == 4
			timer_isr_t callback;
		#elif ESP_IDF_VERSION_MAJOR == 5
			gptimer_alarm_cb_t callback;
		#endif

		#if NUM_TIMERS >= 1
			if (*timer == HARD_TIMER0) {
				callback = timerCallback0;
				timerFunc0 = function;
			}
		#endif
		#if NUM_TIMERS >= 2
			if (*timer == HARD_TIMER1) {
				callback = timerCallback1;
				timerFunc1 = function;
			}
		#endif
		#if NUM_TIMERS >= 3
			if (*timer == HARD_TIMER2) {
				callback = timerCallback2;
				timerFunc2 = function;
			}
		#endif
		#if NUM_TIMERS >= 4
			if (*timer == HARD_TIMER3) {
				callback = timerCallback3;
				timerFunc3 = function;
			}
		#endif
		
		#if ESP_IDF_VERSION_MAJOR == 4
			// init timer
			timer_config_t config = {
				.divider = scalar,
				.counter_dir = true,
				.counter_en = TIMER_PAUSE,
				.alarm_en = TIMER_ALARM_DIS,
				.auto_reload = false,
			};
			*timerPtr = &timerGroups[*timer];
			
			timer_init((*timerPtr) -> group, (*timerPtr) -> num, &config);
			timer_set_counter_value((*timerPtr) -> group, (*timerPtr) -> num, TIMER_COUNT_ZERO);
			timer_start((*timerPtr) -> group, (*timerPtr) -> num);
			timer_isr_callback_add((*timerPtr) -> group, (*timerPtr) -> num, callback, params, setPriority(priority));

			// run timer
			timer_set_alarm_value((*timerPtr) -> group, (*timerPtr) -> num, timerTicks);
			timer_set_auto_reload((*timerPtr) -> group, (*timerPtr) -> num, true);
			timer_set_alarm((*timerPtr) -> group, (*timerPtr) -> num, true);
			timer_start((*timerPtr) -> group, (*timerPtr) -> num);

			return true;

		#elif ESP_IDF_VERSION_MAJOR == 5

			uint64_t count = 1;
			freq_t tempFreq = *freq;

			// adjusts frequency to be above min
			while (tempFreq < HARD_TIMER_FREQ_MIN) {
				tempFreq *= 2;
				count *= 2;
			}

			// timer config
			gptimer_config_t config = {
				.clk_src = GPTIMER_CLK_SRC_DEFAULT,
				.direction = GPTIMER_COUNT_UP,
				.resolution_hz = tempFreq,
				.intr_priority = setPriority(priority),
			};

			// function config
			gptimer_alarm_config_t configAlarm = {
				.reload_count = 0,
				.alarm_count = count,
				.flags.auto_reload_on_alarm = true,
			};

			// callback config
			gptimer_event_callbacks_t configCallback = {
				.on_alarm = callback,
			};

			// creates new timer
			gptimer_new_timer(&config, *timerPtr);

			// sets up callback function
			gptimer_set_alarm_action(**timerPtr, &configAlarm);
			gptimer_register_event_callbacks(**timerPtr, &configCallback, params);

			// starts timer
			gptimer_enable(**timerPtr);
			gptimer_start(**timerPtr);

			return true;

		#endif
	}

	return false;
}