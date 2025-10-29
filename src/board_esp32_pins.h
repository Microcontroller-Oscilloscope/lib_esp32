/*
	board_esp32_pins.h - pin configuration for Espressif ESP32
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
 *                             +------------+
 *                             |            |
 *                          +--+            +--+
 *                   3V3   1|                  |38  GND
 *                    EN   2|                  |37  (D23, COPI0)
 *        (I36, A1_0) VP   3|                  |36  (D22, SCL0)
 *        (I39, A1_3) VN   4|                  |35  (D1, TX0)
 *           (I34, A1_6)   5|                  |34  (D3, RX0)
 *           (I35, A1_7)   6|                  |33  (D21, SDA0)
 *    (D32, A1_4, XTALP)   7|                  |32  GND
 *    (D33, A1_5, XTALN)   8|                  |31  (D19, CIPO0)
 *     (D25, A2_8, DAC1)   9|                  |30  (D18, SCK0)
 *     (D26, A2_9, DAC2)  10|                  |29  (D5, CS0)
 *           (D27, A2_7)  11|                  |28  (D17, TX2)
 *     (D14, A2_6, SCK1)  12|                  |27  (D16, RX2)
 *    (D12, A2_5, CIPO1)  13|                  |26  (D4, A2_0)
 *                   GND  14|                  |25  (D0, A2_1)
 *    (D13, A2_4, COPI1)  15|                  |24  (D2, A2_2)
 *             (D9, RX1)  16|                  |23  (D15, A2_3, CS1)
 *            (D10, TX1)  17|                  |22  (D8)
 *             (D11) CMD  18|                  |21  (D7)
 *                    5V  19|      +----+      |20  CLK (D6)
 *                          +------|    |------+
 *                                 +----+
 * 
 * All digital IO pins support PWM
 * I pins are input only
 * Pins 16-18, 20-22 are tied to board flash and may cause issues
 * Pins D0,D2,D4,D5,D12,D15 are strapping pins used to boot/flash
 * A2 cant be used while wifi or bluetooth are enabled
 * D0,D3,D5,D6-11,D14,D15 outputs HIGH and/or PWM when booting
 */

#ifdef PLATFORMIO
	#include <pins_arduino.h>
#endif

// digital
#ifndef D0
	#define D0 0
#endif
#ifndef D1
	#define D1 1
#endif
#ifndef D2
	#define D2 2
#endif
#ifndef D3
	#define D3 3
#endif
#ifndef D4
	#define D4 4
#endif
#ifndef D5
	#define D5 5
#endif
#ifndef D6
	#define D6 6
#endif
#ifndef D7
	#define D7 7
#endif
#ifndef D8
	#define D8 8
#endif
#ifndef D9
	#define D9 9
#endif
#ifndef D10
	#define D10 10
#endif
#ifndef D11
	#define D11 11
#endif
#ifndef D12
	#define D12 12
#endif
#ifndef D13
	#define D13 13
#endif
#ifndef D14
	#define D14 14
#endif
#ifndef D15
	#define D15 15
#endif
#ifndef D16
	#define D16 16
#endif
#ifndef D17
	#define D17 17
#endif
#ifndef D18
	#define D18 18
#endif
#ifndef D19
	#define D19 19
#endif
#ifndef D21
	#define D21 21
#endif
#ifndef D22
	#define D22 22
#endif
#ifndef D23
	#define D23 23
#endif
#ifndef D25
	#define D25 25
#endif
#ifndef D26
	#define D26 26
#endif
#ifndef D27
	#define D27 27
#endif
#ifndef D32
	#define D32 32
#endif
#ifndef D33
	#define D33 33
#endif
#ifndef D34
	#define D34 34
#endif
#ifndef D35
	#define D35 35
#endif
#ifndef D36
	#define D36 36
#endif
#ifndef D39
	#define D39 39
#endif

// serial
#ifndef RX0
	#define RX0 D3
#endif
#ifndef TX0
	#define TX0 D1
#endif
#ifndef RX1
	#define RX1 D9
#endif
#ifndef TX1
	#define TX1 D10
#endif
#ifndef RX2
	#define RX2 D16
#endif
#ifndef TX2
	#define TX2 D17
#endif
#ifndef SERIAL_COUNT
	#define SERIAL_COUNT 3
#endif

// SPI
#ifndef CS0
	#define CS0 D5
#endif
#ifndef COPI0
	#define COPI0 D23
#endif
#ifndef CIPO0
	#define CIPO0 D19
#endif
#ifndef SCK0
	#define SCK0 D18
#endif
#ifndef CS1
	#define CS1 D15
#endif
#ifndef COPI1
	#define COPI1 D13
#endif
#ifndef CIPO1
	#define CIPO1 D12
#endif
#ifndef SCK1
	#define SCK1 D14
#endif
#ifndef SPI_COUNT
	#define SPI_COUNT 2
#endif

// I2C (second I2C can be assigned to any pin)
#ifndef SDA0
	#define SDA0 D21
#endif
#ifndef SCL0
	#define SCL0 D22
#endif
#ifndef I2C_COUNT
	#define I2C_COUNT 2
#endif

// analog (excluding ADC2 used for WiFi/Bluetooth)
#ifndef ADC0
	#define ADC0 D36
#endif
#ifndef ADC1
	#define ADC1 D39
#endif
#ifndef ADC2
	#define ADC2 D32
#endif
#ifndef ADC3
	#define ADC3 D33
#endif
#ifndef ADC4
	#define ADC4 D34
#endif
#ifndef ADC5
	#define ADC5 D35
#endif
#ifndef ADC_COUNT
	#define ADC_COUNT 6
#endif

// DAC
#ifndef DAC0
	#define DAC0 D25
#endif
#ifndef DAC1
	#define DAC1 D26
#endif
#ifndef DAC_COUNT
	#define DAC_COUNT 2
#endif

// PWM (all PWM can be assigned to any pin except D34-D39)
#ifndef PWM_COUNT
	#define PWM_COUNT 16
#endif

// LED
#ifndef LED_COUNT
	#define LED_COUNT 0
#endif