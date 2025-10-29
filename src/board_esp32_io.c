/*
	board_esp32_io.c - IO configuration for Espressif ESP32
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

#if defined(ESP32DEVC) && defined(IO_INTERNAL)

#include "../../board_common.h"

#include <driver/gpio.h>

bool initBoard() {
	return true;
}

void hardPinMode(pin_t pin, enum pinModeState mode) {
	if (mode == PIN_MODE_DISABLED) {
		gpio_set_direction(pin, GPIO_MODE_DISABLE);
		gpio_set_pull_mode(pin, GPIO_PULLUP_DISABLE);
	}
	else if (mode == PIN_MODE_OUTPUT) {
		gpio_set_direction(pin, GPIO_MODE_OUTPUT);
		gpio_set_pull_mode(pin, GPIO_PULLUP_DISABLE);
	}
	else if (mode == PIN_MODE_INPUT) {
		gpio_set_direction(pin, GPIO_MODE_INPUT);
		gpio_set_pull_mode(pin, GPIO_PULLUP_DISABLE);
	}
	else if (mode == PIN_MODE_INPUT_PULL_UP) {
		gpio_set_direction(pin, GPIO_MODE_INPUT);
		gpio_set_pull_mode(pin, GPIO_PULLUP_ENABLE);
	}
}

void hardDigitalWrite(pin_t pin, enum digitalState value) {
	gpio_set_level(pin, value);
}

#endif