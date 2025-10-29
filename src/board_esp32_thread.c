/*
	board_esp32_thread.c - thread configuration for Espressif ESP32
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

#include "../board.h"

#if defined(ESP32DEVC)

#include "../../board_common.h"

#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#include <freertos/task.h>
#include <esp_system.h>

static portMUX_TYPE threadSpinLock = portMUX_INITIALIZER_UNLOCKED;

bool startThreadSafety(void) {

	if (threadSpinLock.count == 0) {
		taskENTER_CRITICAL(&threadSpinLock);
		return true;
	}

	return false;
}

bool endThreadSafety(void) {

	if (threadSpinLock.count == 1) {
		taskEXIT_CRITICAL(&threadSpinLock);
		return true;
	}

	return false;
}

#endif