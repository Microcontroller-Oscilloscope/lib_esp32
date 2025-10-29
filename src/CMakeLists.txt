# CMakeLsts.txt - cmake for ESP32 Core files
# Copyright (C) 2025 Camren Chraplak

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

idf_component_register(
    SRCS "board_esp32_nvm.c" "board_esp32_serial.c" "board_esp32_delay.c" "board_esp32_io.c" "board_esp32_thread.c" "board_esp32_timer.c"
    INCLUDE_DIRS ""
)