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
#ifndef TIME_MODEL_SETUP_SERVER_H
#define TIME_MODEL_SETUP_SERVER_H

#include <stdint.h>
#include <stdbool.h>

#include "time_model_common.h"
#include "nrf_mesh.h"

/**
 * @file time_model_setup_server.h
 * @author Jefferson Zhai ()
 * @brief Time Server Model and Time Setup Server Model Interface
 * @version 0.1
 * 
 * @copyright Copyright (c) 2022
 * 
 */


/**
 *  
 * @details Whether or not to use the included app timer library of the mesh SDK
 * If this setting is on, then make sure that the app_timer is initialized beforehand
 * 
 * NOTE: Currently, using this option is only supported when there is only 1 instance
 * of the time server/time setup server pair on the node, as only 1 app_timer instance is defined.
 * 
 * If for some reason you need multiple time servers/timer setup servers on the same node (this
 * case which I think is extremely rare), it is highly recommended to use your own timekeeping 
 * implementation to update the time server state instead
 */
#ifndef TIME_MODEL_USE_APP_TIMER
#define TIME_MODEL_USE_APP_TIMER 0
#endif

/**
 * @details Time status message TTL - This should be set to 0 in most cases
 * 
 * Unsolicited Time Status messages are supposed to be sent with a TTL of 0 according to the 
 * specification. However, this poses a drawback as it requires manual configuartion of TIME RELAY nodes
 * ontop of deciding which device is a general bluetooth mesh relay node. 
 * 
 * Setting this define above zero will set the TTL of Time Status message to the number specified in order
 * to trade time accuracy and being slightly non-complaint to the spec, for increased convenience.
*/
#ifndef TIME_STATUS_MSG_TTL
#define TIME_STATUS_MSG_TTL 0
#endif

/** Time server model ID according Section 7.3 */
#define TIME_SERVER_MODEL_ID 0x1200

/** Time setup server model ID according Section 7.3 */
#define TIME_SETUP_SERVER_MODEL_ID 0x1201

/**
 * Example settings for the Time Server model and Time Setup Server model
 * 
 * Element number of time setup server should always be set by user thus
 * has no default defined here
*/
#define TIME_SERVER_DEFAULT_SETTINGS { \
    .settings.force_segmented = false, \
    .settings.transmic_size = NRF_MESH_TRANSMIC_SIZE_SMALL, \
    .settings.publish_upon_state_change = true \
}

#define TIME_SETUP_SERVER_DEFAULT_SETTINGS { \
    .settings.force_segmented = false, \
    .settings.transmic_size = NRF_MESH_TRANSMIC_SIZE_SMALL, \
    .settings.publish_upon_state_change = true, \
    .time_server = TIME_SERVER_DEFAULT_SETTINGS \
}

/* Forward declaration */
typedef struct __time_server_t time_server_t;

/* Forward declaration */
typedef struct __time_setup_server_t time_setup_server_t;

/********************************************************************* 
    TIME SERVER AND SETUP SERVER STATE DEFINITION
**********************************************************************/

/** State definition */
typedef struct {
    uint64_t tai_seconds : 40;
    uint8_t subsecond;
    uint8_t uncertainty;
    bool time_authority;
    int16_t time_zone_offset_current; //allowed range -64 to +191 inclusive, in 15 second minutes intervals
    int16_t time_zone_offset_new; //allowed range -64 to +191 inclusive, in 15 second minutes intervals
    uint64_t time_zone_change : 40;
    int32_t tai_utc_delta_current; //allowed range -255 to +32512 inclusive 
    int32_t tai_utc_delta_new; //allowed range -255 to +32512 inclusive
    uint64_t tai_utc_delta_change : 40;
    time_role_t time_role;
} time_server_state_t;


/********************************************************************* 
    FUNCTIONS REQUIRED FOR USER TO CALL IF 
    TIME_MODEL_USE_APP_TIMER IS NOT USED
**********************************************************************/
/**
 * For implementations not using the build-in APP_TIMER in the model, the responsibility of updating
 * the time_server_state is on the user. If doing so, then note that largest gap to update 
 * the current tai_time is 1 second, otherwise the behaviour of the 
 * time server updating its time_zone / tai_utc_delta at the corresponding 
 * time_zone_change / tai_utc_delta_change time may fail, unless the code in the function
 * "static void current_time_check_time_changes()" is modified accordingly the user's time state update interval
 */

/**
 * @brief Update the time state from elsewhere in the program, usually from RTC
 * 
 */
void time_state_update_time(time_server_t * p_server, uint64_t tai_seconds, uint8_t subsecond);

/**
 * @brief Update the TAI Delta state from elsewhere in the program, if available
 * 
 */
void time_state_update_time_delta(time_server_t * p_server, uint64_t delta_tai_seconds, uint8_t delta_subsecond);

/**
 * @brief Update the uncertainty from elsewhere in the program, if available
 * 
 */
void time_state_update_uncertainty(time_server_t * p_server, uint8_t uncertainty);


/********************************************************************* 
    LOCAL MESSAGE ACTION FUNCTIONS
**********************************************************************/

/**
 * @brief Function that updates the Time State as if the model received a Time Set message, but locally
 * 
 * @param[in]   p_server    Time Server model context pointer
 * @param[in]   time_params Params for updating the Time State
 * 
 * @retval NRF_SUCCESS              The model is initialized successfully.
 * @retval NRF_ERROR_NULL           NULL pointer given to function.
 * @retval NRF_ERROR_INVALID_PARAM  Invalid parameters supplied
 */
uint32_t time_server_state_set_time(time_server_t * p_server, time_set_params_t * time_params);

/**
 * @brief Function that updates the Time State as if the model received a Time Zone Set message, but locally
 * 
 * @param[in]   p_server            Time Server model context pointer
 * @param[in]   time_zone_params    Params for updating the Time State
 * 
 * @retval NRF_SUCCESS              The model is initialized successfully.
 * @retval NRF_ERROR_NULL           NULL pointer given to function.
 * @retval NRF_ERROR_INVALID_PARAM  Invalid parameters supplied
 */
uint32_t time_server_state_set_time_zone_offset(time_server_t * p_server, time_zone_set_params_t * time_zone_params);


/**
 * @brief Function that updates the Time State as if the model received a TAI Delta Set message, but locally
 * 
 * @param[in]   p_server                Time Server model context pointer
 * @param[in]   tai_utc_delta_params    Params for updating the Time State
 * 
 * @retval NRF_SUCCESS              The model is initialized successfully.
 * @retval NRF_ERROR_NULL           NULL pointer given to function.
 * @retval NRF_ERROR_INVALID_PARAM  Invalid parameters supplied
 */
uint32_t time_server_state_set_tai_utc_delta(time_server_t * p_server, tai_utc_delta_set_params_t * tai_utc_delta_params);


/**
 * @brief Function that updates the Time State as if the model received a Time Role Set message, but locally
 * 
 * @param[in]   p_s_server          Time Setup Server model context pointer
 * @param[in]   time_role_params    Params for updating the Time State
 * 
 * @retval NRF_SUCCESS              The model is initialized successfully.
 * @retval NRF_ERROR_NULL           NULL pointer given to function.
 * @retval NRF_ERROR_INVALID_PARAM  Invalid parameters supplied
 */
uint32_t time_setup_server_state_set_time_role(time_setup_server_t * p_s_server, time_role_set_params_t * time_role_params);

/********************************************************************* 
    TIME SERVER DEFINES
**********************************************************************/

typedef void (*time_get_cb_t)(time_server_t * p_self,
                              const access_message_rx_meta_t * p_meta);

typedef void (*time_status_cb_server_t)(const time_server_t * p_self,
					                    const access_message_rx_meta_t * p_meta,
					                    const time_status_params_t * p_in);

typedef void (*time_zone_get_cb_t)(time_server_t * p_self,
				                   const access_message_rx_meta_t * p_meta);

typedef void (*tai_utc_delta_get_cb_t)(time_server_t * p_self,
				                       const access_message_rx_meta_t * p_meta);

/** Time Server callback struct */
typedef struct {
    time_get_cb_t time_get_cb;
    time_status_cb_server_t time_status_cb;
    time_zone_get_cb_t time_zone_get_cb;
    tai_utc_delta_get_cb_t tai_utc_delta_get_cb;
} time_server_callbacks_t;

/**
 * User provided settings and callbacks for the model instance
 */
typedef struct {
    /** If server should force outgoing messages as segmented messages. */
    bool force_segmented;
    /** TransMIC size used by the outgoing server messages. */
    nrf_mesh_transmic_size_t transmic_size;
    /** 
     * Whether the Model should automatically publish by itself everytime its state changes as
     * a result of local action
    */
    bool publish_upon_state_change;
} time_server_settings_t;

/** Model struct definition */
struct __time_server_t {
    /** Model handle assigned to this instance. */
    access_model_handle_t model_handle;

    /** Settings and callbacks for this instance. */
    time_server_settings_t settings;

    /** Time server state */
    time_server_state_t server_state;
    
    /** State handle for this instance and the coexisting time_setup_server instance */
    uint8_t flash_state_handle;
};

/**
 * Sets the callbacks of the model for the main application to receive events from the model
 * 
 * @param[in]   p_callbacks     Pointer to a struct containing callbacks for the main application
 * 
 */
void time_server_set_callbacks(time_server_callbacks_t * p_callbacks);

/**
 * Publishes a Time Status message
 * 
 * @param[in]   p_server    Server model context pointer
 * 
 * @retval NRF_SUCCESS              The message is handed over to the mesh stack for transmission.
 * @retval NRF_ERROR_NULL           NULL pointer given to function.
 * @retval NRF_ERROR_BUSY           The model is busy publishing another message.
 * @retval NRF_ERROR_NO_MEM         No memory available to send the message at this point.
 * @retval NRF_ERROR_NOT_FOUND      The model is not initialized.
 * @retval NRF_ERROR_INVALID_PARAM  Incorrect transition parameters,
 *                                  the model not bound to application key,
 *                                  or publish address not set.
 * @retval NRF_ERROR_FORBIDDEN      Failed to allocate a sequence number from network.
 */ 
uint32_t time_server_time_status_publish(const time_server_t * p_server);


/**
 * Publishes a Time Zone Status message
 * 
 * @param[in]   p_server    Server model context pointer
 * 
 * @retval NRF_SUCCESS              The message is handed over to the mesh stack for transmission.
 * @retval NRF_ERROR_NULL           NULL pointer given to function.
 * @retval NRF_ERROR_BUSY           The model is busy publishing another message.
 * @retval NRF_ERROR_NO_MEM         No memory available to send the message at this point.
 * @retval NRF_ERROR_NOT_FOUND      The model is not initialized.
 * @retval NRF_ERROR_INVALID_PARAM  Incorrect transition parameters,
 *                                  the model not bound to application key,
 *                                  or publish address not set.
 * @retval NRF_ERROR_FORBIDDEN      Failed to allocate a sequence number from network.
 */ 
uint32_t time_server_time_zone_status_publish(const time_server_t * p_server);

/**
 * Publishes a TAI Delta Status message
 * 
 * @param[in]   p_server    Server model context pointer
 * 
 * @retval NRF_SUCCESS              The message is handed over to the mesh stack for transmission.
 * @retval NRF_ERROR_NULL           NULL pointer given to function.
 * @retval NRF_ERROR_BUSY           The model is busy publishing another message.
 * @retval NRF_ERROR_NO_MEM         No memory available to send the message at this point.
 * @retval NRF_ERROR_NOT_FOUND      The model is not initialized.
 * @retval NRF_ERROR_INVALID_PARAM  Incorrect transition parameters,
 *                                  the model not bound to application key,
 *                                  or publish address not set.
 * @retval NRF_ERROR_FORBIDDEN      Failed to allocate a sequence number from network.
 */ 
uint32_t time_server_tai_utc_delta_status_publish(const time_server_t * p_server);

/********************************************************************* 
    TIME SETUP SERVER DEFINES
**********************************************************************/

typedef void (*time_set_cb_t)(const time_setup_server_t * p_s_self,
                              const access_message_rx_meta_t * p_meta,
                              const time_set_params_t * p_in);

typedef void (*time_zone_set_cb_t)(const time_setup_server_t * p_s_self,
				                   const access_message_rx_meta_t * p_meta,
				                   const time_zone_set_params_t * p_in);

typedef void (*tai_utc_delta_set_cb_t)(const time_setup_server_t * p_s_self,
				                       const access_message_rx_meta_t * p_meta,
				                       const tai_utc_delta_set_params_t * p_in);
				       
typedef void (*time_role_get_cb_t)(time_setup_server_t * p_s_self,
				                   const access_message_rx_meta_t * p_meta);
				       
typedef void (*time_role_set_cb_t)(const time_setup_server_t * p_s_self,
				                   const access_message_rx_meta_t * p_meta,
				                   const time_role_set_params_t * p_in);
/**
 * Time setup server callback list.
 */
typedef struct {
    time_set_cb_t time_set_cb;
    time_zone_set_cb_t time_zone_set_cb;
    tai_utc_delta_set_cb_t tai_utc_delta_set_cb;
    time_role_get_cb_t time_role_get_cb;
    time_role_set_cb_t time_role_set_cb;
} time_setup_server_callbacks_t;

/**
 * User provided settings and callbacks for the model instance
 */
typedef struct {
    /** Element Index. */
    uint8_t element_index;
    /** If server should force outgoing messages as segmented messages.
     *  See @ref mesh_model_force_segmented. */
    bool force_segmented;
    /** TransMIC size used by the outgoing server messages.
     * See @ref nrf_mesh_transmic_size_t and @ref mesh_model_large_mic. */
    nrf_mesh_transmic_size_t transmic_size;
    /** 
     * Whether the Model should automatically publish by itself everytime its state changes as
     * a result of local action
    */
    bool publish_upon_state_change;
} time_setup_server_settings_t;

/** Model struct definition */
struct __time_setup_server_t {
    /** Model handle assigned to this instance */
    access_model_handle_t model_handle;
    
    /** Parent model context for the time server, since both models must coexist */
    time_server_t time_server;

    /** Model settings and callbacks for this instance */
    time_setup_server_settings_t settings;
};

/**
 * Initializes Time Setup server and the Time Server
 * 
 * @note This function should only be called _once_.
 * 
 * @param[in]     p_s_server        Time Setup Server model context pointer.
 * @param[in]     element_index     Element index to add the model
 * 
 *
 * @retval NRF_SUCCESS                  The model is initialized successfully.
 * @retval NRF_ERROR_NULL               NULL pointer given to function.
 * @retval NRF_ERROR_NO_MEM             @ref ACCESS_MODEL_COUNT number of models already allocated
 *                                      or no more subscription lists available in memory pool
 *                                      (see @ref ACCESS_SUBSCRIPTION_LIST_COUNT).
 * @retval NRF_ERROR_FORBIDDEN          Multiple model instances per element are not allowed
 *                                      or changes to device composition are not allowed.
 *                                      Adding a new model after device is provisioned is not allowed.
 * @retval NRF_ERROR_NOT_FOUND          Invalid access element index.
*/
uint32_t time_setup_server_init(time_setup_server_t * p_s_server, uint8_t element_index);

/**
 * Sets the callbacks of the model for the main application to receive events from the model
 * 
 * @param[in]   p_callbacks     Pointer to a struct containing callbacks for the main application
 * 
 */
void time_setup_server_set_callbacks(time_setup_server_callbacks_t * p_callbacks);

/**
 * Publishes a Time Role Status message
 * 
 * @param[in]   p_s_server   Server model context pointer
 * 
 * @retval NRF_SUCCESS              The message is handed over to the mesh stack for transmission.
 * @retval NRF_ERROR_NULL           NULL pointer given to function.
 * @retval NRF_ERROR_BUSY           The model is busy publishing another message.
 * @retval NRF_ERROR_NO_MEM         No memory available to send the message at this point.
 * @retval NRF_ERROR_NOT_FOUND      The model is not initialized.
 * @retval NRF_ERROR_INVALID_PARAM  Incorrect transition parameters,
 *                                  the model not bound to application key,
 *                                  or publish address not set.
 * @retval NRF_ERROR_FORBIDDEN      Failed to allocate a sequence number from network.
 */ 
uint32_t time_setup_server_time_role_status_publish(const time_setup_server_t * p_s_server);


#endif
