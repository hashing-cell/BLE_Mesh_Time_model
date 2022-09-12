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
#ifndef TIME_MODEL_CLIENT_H
#define TIME_MODEL_CLIENT_H

#include <stdint.h>
#include "access.h"
#include "access_reliable.h"
#include "time_model_common.h" 
#include "time_model_messages.h"

/**
 * @file time_model_client.h
 * @author Jefferson Zhai ()
 * @brief Time Client Model Interface
 * @version 0.1
 * 
 * @copyright Copyright (c) 2022
 * 
 * @details Implemented according to the Bluetooth Mesh Model
 * Specifications https://www.bluetooth.com/specifications/specs/mesh-model-1-0-1/ to
 * the best of my ability
 * 
 * Note for this version of the time client, only Get Time and Time Status are supported,
 * as since the other messages are optional as long as it support a given set of messages.
 * 
 * Implementing the other messages should be fairly trivial however.
 * 
 */

/** Time Client model ID according Section 7.3 */
#define TIME_CLIENT_MODEL_ID 0x1202

#define TIME_CLIENT_DEFAULT_SETTINGS { \
    .settings.timeout = 0, \
    .settings.force_segmented = false, \
    .settings.transmic_size = NRF_MESH_TRANSMIC_SIZE_SMALL, \
}

/* Forward declaration */
typedef struct __time_client_t time_client_t;

typedef void (*time_status_cb_client_t)(const time_client_t * p_self,
					const access_message_rx_meta_t * p_meta,
					const time_status_params_t * p_in);

typedef struct {
    time_status_cb_client_t time_status_cb;
    /** Callback to call after the acknowledged transaction has ended. */
    access_reliable_cb_t ack_transaction_status_cb;
    /** callback called at the end of the each period for the publishing */
    access_publish_timeout_cb_t periodic_publish_cb;
} time_client_callbacks_t;

/**
 * User provided settings and callbacks for the model instance
 */
typedef struct {
    /** Reliable message timeout in microseconds. If this value is set to zero, during model
     * initialization this value will be updated to the value specified by
     * by @ref MODEL_ACKNOWLEDGED_TRANSACTION_TIMEOUT. */
    uint32_t timeout;
    /** If server should force outgoing messages as segmented messages. */
    bool force_segmented;
    /** TransMIC size used by the outgoing server messages. */
    nrf_mesh_transmic_size_t transmic_size;
} time_client_settings_t;

/** Model struct definition */
struct __time_client_t {
    /** Model handle assigned to this instance */
    access_model_handle_t model_handle;
    /** Acknowledged message context variable */
    access_reliable_t access_message;

    /** Model settings and callbacks for this instance */
    time_client_settings_t settings;
};

/**
 * Initializes the Time client.
 *
 * @note This function should only be called _once_.
 * @note The client handles the model allocation and adding.
 *
 * @param[in]     p_client          Client model context pointer.
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
uint32_t time_client_init(time_client_t * p_client, uint8_t element_index);

/**
 * Sets the callbacks of the model for the main application to receive events from the model
 * 
 * @param[in]   p_callbacks     Pointer to a struct containing callbacks for the main application
 * 
 */
void time_client_set_callbacks(time_client_callbacks_t * p_callbacks);

/**
 * Publishes a Time Get message
 * 
 * @note As per the Bluetooth Mesh specification, it is expected to receive
 * a Time Status message from nodes that receive this message that support the 
 * Time Server model
 *  
 * 
 * @param[in]   p_client    Client model context pointer
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
uint32_t time_client_time_get(time_client_t * p_client);
#endif
