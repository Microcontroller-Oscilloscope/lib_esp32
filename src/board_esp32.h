/*
	board_esp32.h - configuration flags for Espressif ESP32
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

#ifndef BOARD_ESP32_H
#define BOARD_ESP32_H

#include "../board_generic.h"

#ifdef ESP32
	#define ESP32DEVC
	#define BOARD_FOUND
#endif

#ifdef ESP32DEVC

	#if defined(CONFIG_ESPTOOLPY_MONITOR_BAUD) && !defined(PLATFORMIO)
		#ifdef BAUD_RATE
			#undef BAUD_RATE
		#endif
		#define BAUD_RATE CONFIG_ESPTOOLPY_MONITOR_BAUD
	#endif

	#include "board_esp32_pins.h"

	#ifndef NUM_IO_PINS
		#define NUM_IO_PINS 32 // number pins available to controller
	#endif

	#include <esp_attr.h>

	#ifdef PLATFORMIO
		#include <esp32-hal-gpio.h>
	#endif

	/****************************
	 * Type Defines
	****************************/

	typedef const char memCharString;

	#ifdef PLATFORMIO
		#include <pgmspace.h>
		#define PROG_FLASH PROGMEM // storage specifier for flash space
	#endif

	/****************************
	 * Board Overrides
	****************************/

	#ifndef NVM_SIZE
		#define NVM_SIZE FLASH_NVM_SIZE // size in bytes of NVM
	#endif

	#ifndef CORE_COUNT
		#define CORE_COUNT 2 // amount of CPU cores available to board
	#endif

	#include <hal/gpio_types.h>

	#ifndef EXTERNAL_LED_PIN
		#define EXTERNAL_STATUS_LED_PIN 23 // pin for external status LED
	#endif

	#ifndef SERIAL_PRINTF
		#define SERIAL_PRINTF // uses printf as serial
	#endif

	#ifndef IO_INTERNAL
		#define IO_INTERNAL // uses internal IO functions to set pins
	#endif

	#ifndef DELAY_INTERNAL
		#define DELAY_INTERNAL // uses internal delay functions
	#endif

	#ifndef NVM_INTERNAL
		#define NVM_INTERNAL // uses internal nvm functions
	#endif

	/****************************
	 * NVM Config
	****************************/

	#ifndef __NVM_BEGIN__
		#define __NVM_BEGIN__ // Calls begin function for NVM
	#endif
	#ifndef __NVM_BEGIN_RETURN__
		#define __NVM_BEGIN_RETURN__ // Checks return parameter of nvm begin
	#endif

	/****************************
	 * Timer Config
	 * 
	 * Only 4 hardware timers available
	****************************/

	#define FREQ_MAX 5000000 // max frequency user set timer can be

	#ifndef NUM_TIMERS
		#define NUM_TIMERS 4 // amount of hardware timers to use
	#endif

	typedef bool hard_timer_return_t; // return type of timer function
	typedef void* hard_timer_param_t; // parameter type of timer function

	/**
	 * Sets function to run in RAM if possible
	 * 
	 * @param function function name
	 * 
	 * @note {return_type} RUN_IN_RAM({function_name}) {function_name} ({params}) {{content}}
	 */
	#define RUN_IN_RAM(function) IRAM_ATTR

	/**
	 * Returns from timer function
	 * 
	 * APB_CLK = 80,000,000Hz
	 * 
	 * freq = desired frequency (Hz)
	 * 
	 * freq = APB_CLK / (scalar * timerTicks)
	 * 
	 * @note hard_timer_return_t RUN_IN_RAM({function_name}) {function_name}(hard_timer_param_t emptyParams) {
	 * @note 	{contents}
	 * @note 	HARD_TIMER_END();
	 * @note }
	 * 
	 * @warning emptyParams doesn't include any user input parameters
	 * @warning 64-bit counter
	 * @warning 16-bit scalar
	 */
	#define HARD_TIMER_END()

	/****************************
	 * Test Timer Config
	****************************/

	#define TEST_FAST_FREQ 290000 // target frequency

#endif
#endif