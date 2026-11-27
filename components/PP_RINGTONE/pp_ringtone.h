/****************************************************************************
 * Copyright (C) 2025 by Paweł Smarkucki                                    *
 *                                                                          *
 *   This file is part of NIXIE B16.                                        *
 *                                                                          *
 *   NIXIE B16 is free software: you can redistribute it, modify it,        *
 *   sell it and do whatever you want under no terms or conditions.         *
 *                                                                          *
 *   NIXIE B16 is distributed in the hope that it will be useful,           *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                   *
 ****************************************************************************/

/* Headers */

#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "esp_log.h"
#include "esp_gatts_api.h"

#include "freertos/FreeRTOS.h"

#include "pp_global.h"
#include "pp_object_transfer_defs.h"
#include "pp_object_manager.h"
#include "pp_alarm.h"

typedef enum {
    IDLE_RINGTONE,
    DATA_RINGTONE,
    VERIFY_RINGTONE
}ringtone_sm_t;

/* Macros */
#define RINGTONE_NEW_CODE       0xAA
#define RINGTONE_DELETE_CODE    0xBB
#define RINGTONE_VERIFY_CODE    0xCC
#define RINGTONE_PLAY_CODE      0xDD
#define RINGTONE_STOP_CODE      0xEE
#define RINGTONE_DATA_CODE      0xFF

#define RINGTONE_CODE_SIZE          1
#define RINGTONE_NAME_LEN_SIZE      1
#define RIGNTONE_DATA_LEN_SIZE      4
#define RIGNTONE_ID_SIZE            6
#define RIGNTONE_DEFAULT_SIZE       1

/* RINGTONE IDLE macros */
#define RINGTONE_CODE_INDEX         0

/* RINGTONE IDLE -> RINGTONE_NEW_CODE macros */
#define RINGTONE_NEW_NAME_LEN_INDEX 1
#define RINGTONE_NEW_NAME_INDEX     2

/* RINGTONE IDLE -> RINGTONE_DELETE_CODE macros */
#define RINGTONE_DEL_ID_INDEX       1

/* RINGTONE IDLE -> RINGTONE_DELETE_CODE macros */
#define RINGTONE_PLAY_DEF_INDEX     1
#define RINGTONE_PLAY_ID_INDEX      2

/* RINGTONE DATA macros */
#define RINGTONE_DATA_ID_SIZE       4
#define RINGTONE_DATA_LEN_SIZE      2

#define RINGTONE_DATA_ID_INDEX      1
#define RINGTONE_DATA_LEN_INDEX     5
#define RINGTONE_DATA_INDEX         7

/* RINGTONE VERIFY macros */
#define RINGTONE_VER_CODE_INDEX 0

/* Functions */

/** @brief pp_ringtone_action: Ringtone action
 *
 * @return
 */
otp_rsp_status_t pp_ringtone_action(uint8_t *data, uint16_t len);

