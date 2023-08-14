#pragma once

#include <chrono>

#include "nearby_platform_audio.h"
#include "nearby_platform_battery.h"
#include "nearby_platform_bt.h"
#include "nearby_platform_ble.h"
#include "nearby_platform_os.h"
#include "nearby_platform_persistence.h"
#include "nearby_platform_se.h"
#include "nearby_platform_trace.h"
#include "nearby_fp_client.h"

#include <esp_bt.h>
#include <esp_gap_bt_api.h>
#include <esp_gap_ble_api.h>
#include <esp_gatts_api.h>
#include <esp_bt_defs.h>
#include <esp_bt_device.h>
#include <esp_bt_main.h>
#include <esp_gatt_common_api.h>
#include <esp_random.h>
#include <nvs_flash.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "logger.hpp"
#include "task.hpp"
#include "timer.hpp"
