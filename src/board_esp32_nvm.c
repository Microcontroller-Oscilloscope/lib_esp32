/*
	board_esp32_nvm.c - nvm configuration for Espressif ESP32
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

#include <board.h>
#include <nvm/nvm.h>
#include <comm/hard_serial/hard_serial.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <string.h>

#define CHAR_KEY_SIZE NVM_MAX_SIZE_BYTES + 1U
#define OSC_NAME_SPACE "Osc"

bool nvmBegan = false;
bool threadLock = false;
nvs_handle_t handler;

#define THREAD_LOCK() \
	if (threadLock) { \
		return false; \
	} \
	threadLock = true;

#define THREAD_UNLOCK() threadLock = false;

enum NVMStartCode nvmInit(nvm_size_t setNVMSize) {

	if (nvmBegan) {
		return NVM_STARTED;
	}

	if (setNVMSize == (nvm_size_t)DEFAULT_NVM_SIZE) {
		return NVM_INVALID_SIZE;
	}

	THREAD_LOCK();

	if (nvs_flash_init() != ESP_OK) {
		THREAD_UNLOCK();
		return NVM_FAILED;
	}

	if (nvs_open(OSC_NAME_SPACE, NVS_READWRITE, &handler) != ESP_OK) {
		THREAD_UNLOCK();
		return NVM_FAILED;
	}

	nvmBegan = true;
	THREAD_UNLOCK();

	return NVM_OK;
}

bool nvmMaxSize(nvm_size_t *size) {
	if (nvmBegan) {
		*size = NVM_MAX_SIZE;
		return true;
	}

	*size = DEFAULT_NVM_SIZE;
	return false;
}

/**
 * Stops and closes NVM
 * 
 * @warning INTENDED TO USE AFTER 'nvmSetDefaults'
 * @warning NOT INTENDED FOR EXTERNAL USE
 * 
 * @note requires 'nvmInit' after calling
 * 
 * @return if stop was successful
 */
bool nvmStop(void) {
	if (!nvmBegan) {
		return false;
	}

	nvs_flash_deinit();
	nvmBegan = false;

	return true;
}

/**
 * Clears all nvm data
 * 
 * @warning WILL CLEAR NVM DATA
 * @warning NOT INTENDED FOR EXTERNAL USE
 * 
 * @return if clear was successful
 */
bool nvmClear(void) {

	if (nvs_flash_erase() != ESP_OK) {
		return false;
	}

	return true;
}

enum NVMDefaultCode nvmSetDefaults(void) {

	THREAD_LOCK();

	// ensures NVM_SIZE isn't too big for microcontroller
	nvm_size_t nvmMaxValue;
	if (nvmMaxSize(&nvmMaxValue)) {
		if (NVM_SIZE > nvmMaxValue) {
			THREAD_UNLOCK();
			return NVM_DEFAULT_SIZE_TOO_BIG;
		}
	}
	else {
		THREAD_UNLOCK();
		// if nvm not started or unable to get size
		return NVM_DEFAULT_FAIL_MAX_SIZE;
	}

	// ensures clear works
	if (!nvmClear()) {
		THREAD_UNLOCK();
		return NVM_DEFAULT_FAIL_CLEAR;
	}

	// stops nvm
	if (!nvmStop()) {
		THREAD_UNLOCK();
		return NVM_DEFAULT_FAIL_STOP;
	}

	THREAD_UNLOCK();

	// restarts nvm for operations
	enum NVMStartCode startCode = nvmInit(NVM_SIZE);
	if (startCode != NVM_OK) {
		return NVM_DEFAULT_FAIL_INIT;
	}

	// writes critical values
	enum NVMDefaultCode code = nvmSetCritDefaults(nvmMaxValue);
	if (code != NVM_DEFAULT_OK) {
		return code;
	}

	code = nvmSetEnvDefaults();

	//writes platform values
	return code;
}

/**
 * Converts integer key to char array key
 * 
 * @param key integer key
 * @param keyStr char array to store key
 */
void keyToChar(nvm_size_t key, char* keyStr) {
	for (uint8_t i = 0U; i < NVM_MAX_SIZE_BYTES; i++) {
		uint8_t subKey = (key >> (8*i));
		keyStr[i] = subKey;
	}
	keyStr[NVM_MAX_SIZE_BYTES] = END_OF_CHAR;
}

#define SET_NVS(key, setter, value) \
	if (!nvmBegan) { \
		return false; \
	} \
	THREAD_LOCK(); \
	char keyStr[CHAR_KEY_SIZE]; \
	keyToChar(key, keyStr); \
	if (setter(handler, keyStr, value) != ESP_OK) { \
		THREAD_UNLOCK(); \
		return false; \
	} \
	if (nvs_commit(handler) != ESP_OK) { \
		THREAD_UNLOCK(); \
		return false; \
	} \
	THREAD_UNLOCK(); \
	return true;

#define GET_NVS(key, getter, value, canDefault, defaultValue) \
	if (!nvmBegan) { \
		return false; \
	} \
	THREAD_LOCK(); \
	char keyStr[CHAR_KEY_SIZE]; \
	keyToChar(key, keyStr); \
	if (getter(handler, keyStr, value) != ESP_OK) { \
		THREAD_UNLOCK(); \
		return false; \
	} \
	if (!canDefault && *value == defaultValue) { \
		THREAD_UNLOCK(); \
		return false; \
	} \
	THREAD_UNLOCK(); \
	return true;

bool nvmWriteCharArray(nvm_size_t key, char* value, uint8_t maxLength) {
	if (!nvmBegan) {
		return false;
	}

	uint8_t valueLen = charArraySize(value);
	if (valueLen == 0) {
		return false;
	}
	else if (valueLen > maxLength) {
		return false;
	}
	else if (valueLen == CHAR_LEN_ERROR) {
		return false;
	}

	THREAD_LOCK();

	char keyStr[CHAR_KEY_SIZE];
	keyToChar(key, keyStr);
	
	if (nvs_set_str(handler, keyStr, value) != ESP_OK) {
		THREAD_UNLOCK();
		return false;
	}
	if (nvs_commit(handler) != ESP_OK) {
		THREAD_UNLOCK();
		return false;
	}

	THREAD_UNLOCK();

	return true;
}

bool nvmGetCharArray(nvm_size_t key, char* value, uint8_t maxLength) {
	if (!nvmBegan) {
		return false;
	}
	if (value == NULL) {
		return false;
	}
	if (maxLength == 0U) {
		return false;
	}

	THREAD_LOCK();
	
	char keyStr[CHAR_KEY_SIZE];
	keyToChar(key, keyStr);

	size_t strSize = 0;
	if (nvs_get_str(handler, keyStr, NULL, &strSize) != ESP_OK) {
		THREAD_UNLOCK();
		return false;
	}
	if (strSize > maxLength) {
		THREAD_UNLOCK();
		return false;
	}
	if (nvs_get_str(handler, keyStr, value, &strSize) != ESP_OK) {
		THREAD_UNLOCK();
		return false;
	}

	THREAD_UNLOCK();

	return true;
}

bool nvmWriteBool(nvm_size_t key, bool value) {
	SET_NVS(key, nvs_set_u8, value);
}

bool nvmWriteI8(nvm_size_t key, int8_t value) {
	SET_NVS(key, nvs_set_i8, value);
}

bool nvmWriteUI8(nvm_size_t key, uint8_t value) {
	SET_NVS(key, nvs_set_u8, value);
}

bool nvmWriteI16(nvm_size_t key, int16_t value) {
	SET_NVS(key, nvs_set_i16, value);
}

bool nvmWriteUI16(nvm_size_t key, uint16_t value) {
	SET_NVS(key, nvs_set_u16, value);
}

bool nvmWriteI32(nvm_size_t key, int32_t value) {
	SET_NVS(key, nvs_set_i32, value);
}

bool nvmWriteUI32(nvm_size_t key, uint32_t value) {
	SET_NVS(key, nvs_set_u32, value);
}

bool nvmWriteI64(nvm_size_t key, int64_t value) {
	SET_NVS(key, nvs_set_i64, value);
}

bool nvmWriteUI64(nvm_size_t key, uint64_t value) {
	SET_NVS(key, nvs_set_u64, value);
}

bool nvmWriteFloat(nvm_size_t key, float value) {

	/*if (sizeof(float) == sizeof(uint16_t)) {
		uint16_t newVal;
		for (uint8_t i = 0; i < sizeof(float); i++) {
			uint8_t *newValPtr = (uint8_t *)(&newVal) + i;
			uint8_t *valuePtr = (uint8_t *)(&value) + i;
			memcpy(newValPtr, valuePtr, sizeof(uint8_t));
		}
		return nvmWriteUI16(key, newVal);
	}
	else if (sizeof(float) == sizeof(uint32_t)) {
		uint32_t newVal;
		for (uint8_t i = 0; i < sizeof(float); i++) {
			uint8_t *newValPtr = (uint8_t *)(&newVal) + i;
			uint8_t *valuePtr = (uint8_t *)(&value) + i;
			memcpy(newValPtr, valuePtr, sizeof(uint8_t));
		}
		return nvmWriteUI32(key, newVal);
	}
	else if (sizeof(float) == sizeof(uint64_t)) {
		uint64_t newVal;
		for (uint8_t i = 0; i < sizeof(float); i++) {
			uint8_t *newValPtr = (uint8_t *)(&newVal) + i;
			uint8_t *valuePtr = (uint8_t *)(&value) + i;
			memcpy(newValPtr, valuePtr, sizeof(uint8_t));
		}
		return nvmWriteUI64(key, newVal);
	}*/

	if (sizeof(float) == sizeof(uint32_t)) {
		uint32_t newVal;
		memcpy(&newVal, &value, sizeof(float));
		return nvmWriteUI32(key, newVal);
	}
	else if (sizeof(float) == sizeof(uint64_t)) {
		uint64_t newVal;
		memcpy(&newVal, &value, sizeof(float));
		return nvmWriteUI64(key, newVal);
	}

	return false;
}

bool nvmWriteDouble(nvm_size_t key, double value) {

	/*if (sizeof(double) == sizeof(uint16_t)) {
		uint16_t newVal;
		for (uint8_t i = 0; i < sizeof(double); i++) {
			uint8_t *newValPtr = (uint8_t *)(&newVal) + i;
			uint8_t *valuePtr = (uint8_t *)(&value) + i;
			memcpy(newValPtr, valuePtr, sizeof(uint8_t));
		}
		return nvmWriteUI16(key, newVal);
	}
	else if (sizeof(double) == sizeof(uint32_t)) {
		uint32_t newVal;
		for (uint8_t i = 0; i < sizeof(double); i++) {
			uint8_t *newValPtr = (uint8_t *)(&newVal) + i;
			uint8_t *valuePtr = (uint8_t *)(&value) + i;
			memcpy(newValPtr, valuePtr, sizeof(uint8_t));
		}
		return nvmWriteUI32(key, newVal);
	}
	else if (sizeof(double) == sizeof(uint64_t)) {
		uint64_t newVal;
		for (uint8_t i = 0; i < sizeof(double); i++) {
			uint8_t *newValPtr = (uint8_t *)(&newVal) + i;
			uint8_t *valuePtr = (uint8_t *)(&value) + i;
			memcpy(newValPtr, valuePtr, sizeof(uint8_t));
		}
		return nvmWriteUI64(key, newVal);
	}*/

	if (sizeof(double) == sizeof(uint32_t)) {
		uint32_t newVal;
		memcpy(&newVal, &value, sizeof(double));
		return nvmWriteUI32(key, newVal);
	}
	else if (sizeof(double) == sizeof(uint64_t)) {
		uint64_t newVal;
		memcpy(&newVal, &value, sizeof(double));
		return nvmWriteUI64(key, newVal);
	}

	return false;
}

bool nvmGetBool(nvm_size_t key, bool *value, bool canDefault) {
	GET_NVS(key, nvs_get_u8, (uint8_t*)value, canDefault, DEFAULT_BOOL);
}

bool nvmGetI8(nvm_size_t key, int8_t *value, bool canDefault) {
	GET_NVS(key, nvs_get_i8, value, canDefault, (int8_t)DEFAULT_INT);
}

bool nvmGetUI8(nvm_size_t key, uint8_t *value, bool canDefault) {
	GET_NVS(key, nvs_get_u8, value, canDefault, (uint8_t)DEFAULT_INT);
}

bool nvmGetI16(nvm_size_t key, int16_t *value, bool canDefault) {
	GET_NVS(key, nvs_get_i16, value, canDefault, (int16_t)DEFAULT_INT);
}

bool nvmGetUI16(nvm_size_t key, uint16_t *value, bool canDefault) {
	GET_NVS(key, nvs_get_u16, value, canDefault, (uint16_t)DEFAULT_INT);
}

bool nvmGetI32(nvm_size_t key, int32_t *value, bool canDefault) {
	GET_NVS(key, nvs_get_i32, value, canDefault, (int32_t)DEFAULT_INT);
}

bool nvmGetUI32(nvm_size_t key, uint32_t *value, bool canDefault) {
	GET_NVS(key, nvs_get_u32, value, canDefault, (uint32_t)DEFAULT_INT);
}

bool nvmGetI64(nvm_size_t key, int64_t *value, bool canDefault) {
	GET_NVS(key, nvs_get_i64, value, canDefault, (int64_t)DEFAULT_INT);
}

bool nvmGetUI64(nvm_size_t key, uint64_t *value, bool canDefault) {
	GET_NVS(key, nvs_get_u64, value, canDefault, (uint64_t)DEFAULT_INT);
}

bool nvmGetFloat(nvm_size_t key, float *value, bool canDefault) {

	/*if (sizeof(float) == sizeof(uint16_t)) {
		uint16_t newVal;
		bool result = nvmGetUI16(key, &newVal, true);
		if (!result) {
			return false;
		}
		for (uint8_t i = 0; i < sizeof(float); i++) {
			uint8_t *newValPtr = (uint8_t *)(&newVal) + i;
			uint8_t *valuePtr = (uint8_t *)(value) + i;
			memcpy(valuePtr, newValPtr, sizeof(uint8_t));
		}
		return true;
	}
	else if (sizeof(float) == sizeof(uint32_t)) {
		uint32_t newVal;
		bool result = nvmGetUI32(key, &newVal, true);
		if (!result) {
			return false;
		}
		for (uint8_t i = 0; i < sizeof(float); i++) {
			uint8_t *newValPtr = (uint8_t *)(&newVal) + i;
			uint8_t *valuePtr = (uint8_t *)(value) + i;
			memcpy(valuePtr, newValPtr, sizeof(uint8_t));
		}
		return true;
	}
	else if (sizeof(float) == sizeof(uint64_t)) {
		uint64_t newVal;
		bool result = nvmGetUI64(key, &newVal, true);
		if (!result) {
			return false;
		}
		for (uint8_t i = 0; i < sizeof(float); i++) {
			uint8_t *newValPtr = (uint8_t *)(&newVal) + i;
			uint8_t *valuePtr = (uint8_t *)(value) + i;
			memcpy(valuePtr, newValPtr, sizeof(uint8_t));
		}
		return true;
	}*/

	if (sizeof(float) == sizeof(uint32_t)) {
		uint32_t newVal;
		bool result = nvmGetUI32(key, &newVal, true);
		if (!result) {
			return false;
		}
		memcpy(value, &newVal, sizeof(float));
		return true;
	}
	else if (sizeof(float) == sizeof(uint64_t)) {
		uint64_t newVal;
		bool result = nvmGetUI64(key, &newVal, true);
		if (!result) {
			return false;
		}
		memcpy(value, &newVal, sizeof(float));
		return true;
	}

	return false;
}

bool nvmGetDouble(nvm_size_t key, double *value, bool canDefault) {

	/*if (sizeof(double) == sizeof(uint16_t)) {
		uint16_t newVal;
		bool result = nvmGetUI16(key, &newVal, true);
		if (!result) {
			return false;
		}
		for (uint8_t i = 0; i < sizeof(double); i++) {
			uint8_t *newValPtr = (uint8_t *)(&newVal) + i;
			uint8_t *valuePtr = (uint8_t *)(value) + i;
			memcpy(valuePtr, newValPtr, sizeof(uint8_t));
		}
		return true;
	}
	else if (sizeof(double) == sizeof(uint32_t)) {
		uint32_t newVal;
		bool result = nvmGetUI32(key, &newVal, true);
		if (!result) {
			return false;
		}
		for (uint8_t i = 0; i < sizeof(double); i++) {
			uint8_t *newValPtr = (uint8_t *)(&newVal) + i;
			uint8_t *valuePtr = (uint8_t *)(value) + i;
			memcpy(valuePtr, newValPtr, sizeof(uint8_t));
		}
		return true;
	}
	else if (sizeof(double) == sizeof(uint64_t)) {
		uint64_t newVal;
		bool result = nvmGetUI64(key, &newVal, true);
		if (!result) {
			return false;
		}
		for (uint8_t i = 0; i < sizeof(double); i++) {
			uint8_t *newValPtr = (uint8_t *)(&newVal) + i;
			uint8_t *valuePtr = (uint8_t *)(value) + i;
			memcpy(valuePtr, newValPtr, sizeof(uint8_t));
		}
		return true;
	}*/

	if (sizeof(double) == sizeof(uint32_t)) {
		uint32_t newVal;
		bool result = nvmGetUI32(key, &newVal, true);
		if (!result) {
			return false;
		}
		memcpy(value, &newVal, sizeof(double));
		return true;
	}
	else if (sizeof(double) == sizeof(uint64_t)) {
		uint64_t newVal;
		bool result = nvmGetUI64(key, &newVal, true);
		if (!result) {
			return false;
		}
		memcpy(value, &newVal, sizeof(double));
		return true;
	}

	return false;
}