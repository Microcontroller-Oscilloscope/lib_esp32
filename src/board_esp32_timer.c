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

#include <hard_timer.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <freertos/timers.h>
#include <driver/timer.h>
#include <esp_intr_alloc.h>

#define TIMER_COUNT_ZERO 0U // value for setting timer tick count to 0
#define SCALAR_MAX UINT16_MAX // max value for timer scalar

typedef uint16_t prescalar_t; // pre scalar type
typedef uint64_t timertick_t; // timer tick type

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

uint8_t claimed = 0U; // stores whether timers were claimed or not

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
	return (1 << (priority / (UINT8_MAX / 3)));
}

/**
 * Gets timer based on desired timer
 * 
 * @param timer timer to select
 * 
 * @return pointer to timer selected
 */
hard_timer_group_t** getTimer(hard_timer_t timer) {

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
	return !!(claimed & (1 << (timerGroups[timer].group + timerGroups[timer].num * 2)));
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
	hard_timer_group_t** timerPtr = getTimer(timer);

	if (timerPtr == NULL) {
		return false;
	}
	if (*timerPtr != NULL) {
		return true;
	}
	return false;
}

bool cancelHardTimer(hard_timer_t timer) {
	
	if (hardTimerStarted(timer)) {

		hard_timer_group_t** timerPtr = getTimer(timer);

		// cancels timer
		timer_set_alarm((*timerPtr) -> group, (*timerPtr) -> num, false);
		timer_pause((*timerPtr) -> group, (*timerPtr) -> num);
		timer_set_counter_value((*timerPtr) -> group, (*timerPtr) -> num, TIMER_COUNT_ZERO);

		// deconstructs timer
		timer_isr_callback_remove((*timerPtr) -> group, (*timerPtr) -> num);
		timer_deinit((*timerPtr) -> group, (*timerPtr) -> num);
		*timerPtr = NULL;

		return true;
	}

	return false;
}

bool setHardTimer(hard_timer_t *timer, freq_t *freq, hard_timer_function_ptr_t function, timer_priority_t priority) {
	
	if (function == NULL || freq == NULL || timer == NULL) {
		return false;
	}
	if (*freq == (freq_t)0 || *freq > FREQ_MAX) {
		return false;
	}

	prescalar_t scalar;
	timertick_t timerTicks;

	if (getHardTimerStats(freq, timer, &scalar, &timerTicks) == HARD_TIMER_FAIL) {
		return false;
	}

	if (!hardTimerStarted(*timer)) {

		hard_timer_group_t** timerPtr = getTimer(*timer);

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
		timer_isr_callback_add((*timerPtr) -> group, (*timerPtr) -> num, function, NULL, setPriority(priority));

		// run timer
		timer_set_alarm_value((*timerPtr) -> group, (*timerPtr) -> num, timerTicks);
		timer_set_auto_reload((*timerPtr) -> group, (*timerPtr) -> num, true);
		timer_set_alarm((*timerPtr) -> group, (*timerPtr) -> num, true);
		timer_start((*timerPtr) -> group, (*timerPtr) -> num);
		return true;
	}

	return false;
}