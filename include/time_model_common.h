/**
 * MIT License
 * Copyright (c) [2022] [Jefferson Zhai]
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/
#ifndef TIME_MODEL_COMMON_H
#define TIME_MODEL_COMMON_H

#include <stdint.h>
#include <stdbool.h>

#include "model_common.h"

/**
 * @file time_model_common.h
 * @author Jefferson Zhai ()
 * @brief Time Model Common defines
 * @version 0.1
 * 
 * @copyright Copyright (c) 2022
 * 
 * @details Implemented according to the Bluetooth Mesh Model
 * Specifications https://www.bluetooth.com/specifications/specs/mesh-model-1-0-1/ to
 * the best of my ability
 * 
 */

/** Set to your own company ID */
#define TIME_COMPANY_ID (0xFFFF)

/** According to Section 5.1.1.1, 0x0000000000 represents an unknown TAI time */
#define TAI_TIME_UNKNOWN (0x0000000000)

/** In the message payload, the parameter for TAI time is 40 bits*/
#define TAI_TIME_MAX_VAL (1099511627775)

/** According to Section 5.1.1.7, 0x0000000000 represents an unknown TAI of Zone Change*/
#define TIME_ZONE_CHANGE_UNKNOWN (0x0000000000)

/** Section 5.1.1.5 and 5.1.1.6 state that the time zone offset is in 15-minute increments */
#define TIME_ZONE_CHANGE_INCREMENTS (15)

/** Convert an actual number to the equivalent value represented by the Time zone change number, see Section 5.1.1.5 */
#define TIME_ZONE_STATE_OFFSET_FACTOR (0x40)

/** Minimum Time zone change value according to Section 5.1.1.5 */
#define TIME_ZONE_CHANGE_MIN_VAL (-64)

/** Maximum Time zone change value according to Section 5.1.1.5 */
#define TIME_ZONE_CHANGE_MAX_VAL (192)

/** According to Section 5.1.1.10, 0x0000000000 represents an unknown TAI of Delta Change*/
#define TAI_DELTA_CHANGE_UNKNOWN (0x0000000000)

/** Convert an actual number to the equivalent value represented by the TAI Delta Change change number, see Section 5.1.1.8 */
#define TAI_DELTA_STATE_OFFSET_FACTOR (0x00FF)

/** Minimum TAI Delta Change value according to Section 5.1.1.8 */
#define TAI_DELTA_CHANGE_MIN_VAL (-255)

/** Minimum TAI Delta Change value according to Section 5.1.1.8 */
#define TAI_DELTA_CHANGE_MAX_VAL (32512)

/**
 * @brief Possible Time roles, Section 5.1.2
 * 
 */
typedef enum {
    TIME_ROLE_NONE = 0x00,
    TIME_ROLE_AUTHORITY = 0x01,
    TIME_ROLE_RELAY = 0x02,
    TIME_ROLE_CLIENT = 0x03
} time_role_t;

/** Parameters for the Time Set message */
typedef struct {
    uint64_t tai_seconds; //allowed range 0 to 1,099,511,627,775 inclusive
    uint8_t subsecond;
    uint8_t uncertainty;
    bool time_authority;
    int32_t tai_utc_delta; //allowed range -255 to +32512 inclusive
    uint8_t time_zone_offset; //allowed range -64 to +191 inclusive
} time_set_params_t;

/** Parameters for the Time Status message */
typedef struct {
    uint64_t tai_seconds; //allowed range 0 to 1,099,511,627,775 inclusive
    uint8_t subsecond;
    uint8_t uncertainty;
    bool time_authority;
    int32_t tai_utc_delta; //allowed range -255 to +32512 inclusive
    uint8_t time_zone_offset; //allowed range -64 to +191 inclusive
} time_status_params_t;

/** Parameters for the Time Zone Set message */
typedef struct {
    int16_t time_zone_offset_new; //allowed range -64 to +191 inclusive
    uint64_t time_zone_change; //allowed range 0 to 1,099,511,627,775 inclusive
} time_zone_set_params_t;

/** Parameters for the Time Zone Status message */
typedef struct {
    int16_t time_zone_offset_current; //allowed range -64 to +191 inclusive
    int16_t time_zone_offset_new; //allowed range -64 to +191 inclusive
    uint64_t time_zone_change; //allowed range 0 to 1,099,511,627,775 inclusive
} time_zone_status_params_t;

/** Parameters for the TAI Delta Set message */
typedef struct {
    int32_t tai_utc_delta_new; //allowed range -255 to +32512 inclusive 
    uint64_t tai_utc_delta_change; //allowed range 0 to 1,099,511,627,775 inclusive
} tai_utc_delta_set_params_t;

/** Parameters for the TAI Delta Status message */
typedef struct {
    int32_t tai_utc_delta_current; //allowed range -255 to +32512 inclusive 
    int32_t tai_utc_delta_new; //allowed range -255 to +32512 inclusive 
    uint64_t tai_utc_delta_change; //allowed range 0 to 1,099,511,627,775 inclusive
} tai_utc_delta_status_params_t;

/** Parameters for the Time Role Set message */
typedef struct {
    time_role_t time_role;
} time_role_set_params_t;

/** Parameters for the TIme Role Status message */
typedef struct {
    time_role_t time_role;
} time_role_status_params_t;

/**
 * @brief Utility function to validate a TAI time parameter
 * 
 * @param[in] arg   Is a TAI time parameter to be validated
 * 
 * 
 * @return true     TAI time parameter is valid
 * @return false    TAI time parameter is invalid
 */
static inline bool validate_tai_time_arg(uint64_t arg) {
    return (arg <= TAI_TIME_MAX_VAL);
}

/**
 * @brief Utility function to validate a Time zone offset parameter
 * 
 * @param[in] arg   Is a Time zone offset parameter to be validated
 * 
 * 
 * @return true     Time zone offset parameter is valid
 * @return false    Time zone offset parameter is invalid
 */
static inline bool validate_time_zone_offset_arg(int16_t arg) {
    return (arg >= TIME_ZONE_CHANGE_MIN_VAL && arg <= TIME_ZONE_CHANGE_MAX_VAL);
}

/**
 * @brief Utility function to validate a TAI Delta parameter
 * 
 * @param[in] arg   Is a TAI Delta parameter to be validated
 * 
 * 
 * @return true     TAI Delta parameter is valid
 * @return false    TAI Delta parameter is invalid
 */
static inline bool validate_tai_utc_delta_arg(int32_t arg) {
    return (arg >= TAI_DELTA_CHANGE_MIN_VAL && arg <= TAI_DELTA_CHANGE_MAX_VAL);
}

#endif
