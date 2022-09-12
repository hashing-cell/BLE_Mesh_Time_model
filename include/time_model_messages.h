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
#ifndef TIME_MODEL_MESSAGES_H
#define TIME_MODEL_MESSAGES_H
#include <stdint.h>

/**
 * @file time_model_messages.h
 * @author Jefferson Zhai ()
 * @brief Time Model Messages definitions
 * @version 0.1
 * 
 * @copyright Copyright (c) 2022
 * 
 * @details Implemented according to the Bluetooth Mesh Model
 * Specifications https://www.bluetooth.com/specifications/specs/mesh-model-1-0-1/ to
 * the best of my ability
 * 
 */

/** Size of the Time Set message payload according to Section 5.2.1.2 */
#define TIME_SET_LEN 10

/** Minimum and maximum sizes of the Time Status message payload according to Section 5.2.1.3 */
#define TIME_STATUS_MINLEN 5
#define TIME_STATUS_MAXLEN 10

/** Size of the Time Zone Set message payload according to Section 5.2.1.5 */
#define TIME_ZONE_SET_LEN 6

/** Size of the Time Zone Status message payload according to Section 5.2.1.6 */
#define TIME_ZONE_STATUS_LEN 7

/** Size of the TAI Delta Set message payload according to Section 5.2.1.8 */
#define TAI_UTC_DELTA_SET_LEN 7

/** Size of the TAI Delta Status message payload according to Section 5.2.1.9 */
#define TAI_UTC_DELTA_STATUS_LEN 9

/** Size of the Time Role Set message payload according to Section 5.2.1.11 */
#define TIME_ROLE_SET_LEN 1

/** Size of the Time Role Status message payload according to Section 5.2.1.12 */
#define TIME_ROLE_STATUS_LEN 1

/** Time model opcodes, Section 7.1 */
typedef enum {
    TIME_OPCODE_GET = 0x8237,
    TIME_OPCODE_SET = 0x5C,
    TIME_OPCODE_STATUS = 0x5D,
    TIME_OPCODE_ROLE_GET = 0x8238,
    TIME_OPCODE_ROLE_SET = 0x8239,
    TIME_OPCODE_ROLE_STATUS = 0x823A,
    TIME_OPCODE_ZONE_GET = 0x823B,
    TIME_OPCODE_ZONE_SET = 0x823C,
    TIME_OPCODE_ZONE_STATUS = 0x823D,
    TIME_OPCODE_TAI_UTC_DELTA_GET = 0x823E,
    TIME_OPCODE_TAI_UTC_DELTA_SET = 0x823F,
    TIME_OPCODE_TAI_UTC_DELTA_STATUS = 0x8240
} time_model_opcode_t;

/** Message packet definition for the Time Set Message, Section 5.2.1.2, Table 5.15 */
typedef struct __attribute((packed)) {
    uint64_t tai_seconds : 40;
    uint8_t subsecond;
    uint8_t uncertainty;
    uint8_t time_authority : 1;
    uint16_t tai_utc_delta : 15;
    uint8_t time_zone_offset;
} time_set_msg_pkt_t;

/** Message packet definition for the Time Status Message, Section 5.2.1.3, Table 5.16 */
typedef struct __attribute((packed)) {
    uint64_t tai_seconds : 40;
    uint8_t subsecond;
    uint8_t uncertainty;
    uint8_t time_authority : 1;
    uint16_t tai_utc_delta : 15;
    uint8_t time_zone_offset;
} time_status_msg_pkt_t;

/** Message packet definition for the Time Zone Set Message, Section 5.2.1.5, Table 5.17 */
typedef struct __attribute((packed)) {
    uint8_t time_zone_offset_new;
    uint64_t time_zone_change : 40;
} time_zone_set_msg_pkt_t;

/** Message packet definition for the Time Zone Status Message, Section 5.2.1.6, Table 5.18 */
typedef struct __attribute((packed)) {
    uint8_t time_zone_offset_current;
    uint8_t time_zone_offset_new;
    uint64_t time_zone_change : 40;
} time_zone_status_msg_pkt_t;

/** Message packet definition for the TAI Delta Set Message, Section 5.2.1.8, Table 5.19 */
typedef struct __attribute((packed)) {
    uint16_t tai_utc_delta_new : 15;
    uint8_t padding : 1;
    uint64_t tai_utc_delta_change : 40;
} tai_utc_delta_set_msg_pkt_t;

/** Message packet definition for the TAI Delta Status Message, Section 5.2.1.9, Table 5.20 */
typedef struct __attribute((packed)) {
    uint16_t tai_utc_delta_current : 15;
    uint8_t padding1 : 1;
    uint16_t tai_utc_delta_new : 15;
    uint8_t padding2 : 1;
    uint64_t tai_utc_delta_change : 40;
} tai_utc_delta_status_msg_pkt_t;

/** Message packet defintion for the Time Role Set message, Section 5.2.1.11, Table 5.21 */
typedef struct __attribute((packed)) {
    uint8_t time_role;
} time_role_set_msg_pkt_t;

/** Message packet defintion for the Time Role Status message, Section 5.2.1.12, Table 5.22 */
typedef struct __attribute((packed)) {
    uint8_t time_role;
} time_role_status_msg_pkt_t;

/**
 * @brief Utility function to encode a Time Zone Offset parameter to 
 * the appropriate format for the message
 * 
 * @param[in] arg   Time Zone Offset parameter to be encoded into message form
 * 
 * @return uint8_t Encoded Time Zone Offset parameter for the actual message payload
 */
static inline uint8_t time_zone_offset_encode(int16_t arg) {
    return (uint8_t) (arg + TIME_ZONE_STATE_OFFSET_FACTOR);
}

/**
 * @brief Utility function to decode a Time Zone Offset from the actual
 * message payload to its actual value
 * 
 * @param[in] arg   Time Zone Offset value from message payload to be decoded
 * 
 * @return uint16_t Decoded Time Zone Offset value
 */
static inline int16_t time_zone_offset_decode(uint8_t msg_param) {
    return (int16_t) (msg_param - TIME_ZONE_STATE_OFFSET_FACTOR);
}


/**
 * @brief Utility function to encode a TAI Delta parameter to 
 * the appropriate format for the message
 * 
 * @param[in] arg   TAI Delta parameter to be encoded into message form
 * 
 * @return uint16_t  Encoded TAI Delta parameter for the actual message payload
 */
static inline uint16_t tai_utc_delta_encode(int32_t arg) {
    return (uint16_t) (arg + TAI_DELTA_STATE_OFFSET_FACTOR);
}

/**
 * @brief Utility function to decode a TAI Delta from the actual
 * message payload to its actual value
 * 
 * @param[in] arg   TAI Delta value from message payload to be decoded
 * 
 * @return uint32_t Decoded TAI Delta value
 */
static inline int32_t tai_utc_delta_decode(uint16_t msg_param) {
    return (int32_t) (msg_param - TAI_DELTA_STATE_OFFSET_FACTOR);
}


#endif
