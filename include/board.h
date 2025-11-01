/*
	board.h - configuration flags for Espressif ESP32
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

#ifndef BOARD_H
#define BOARD_H

/****************************
 * NVM Config
****************************/

#ifndef NVM_SIZE
	#define NVM_SIZE 4096 // size in bytes of NVM
#endif

/****************************
 * Test Timer Config
****************************/

#define TEST_FAST_FREQ 290000 // target frequency

#endif