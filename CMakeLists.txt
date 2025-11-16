# CMakeLsts.txt - cmake for ESP32 lib_esp32 files
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
    SRC_DIRS "src"
    REQUIRES driver nvs_flash esp_timer esp_rom unity Core
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-expansion-to-defined)