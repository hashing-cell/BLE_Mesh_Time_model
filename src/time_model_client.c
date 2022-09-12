#include "time_model_client.h"
#include "time_model_common.h"

#include <stdint.h>
#include <string.h>

#include "access.h"
#include "access_config.h"

static time_client_callbacks_t time_client_callbacks = {0};

static void handle_time_status(access_model_handle_t model_handle, 
                               const access_message_rx_t * p_rx_msg, 
                               void * p_args) {
    time_client_t * p_client = (time_client_t *) p_args;
    
    time_status_msg_pkt_t * p_msg_in = (time_status_msg_pkt_t *) p_rx_msg->p_data;
	
    if (time_client_callbacks.time_status_cb != NULL) {
	time_status_params_t in_data;
	in_data.tai_seconds = p_msg_in->tai_seconds;
	in_data.subsecond = p_msg_in->subsecond;
	in_data.uncertainty = p_msg_in->uncertainty;
	in_data.time_zone_offset = time_zone_offset_decode(p_msg_in->time_zone_offset);
	in_data.tai_utc_delta = tai_utc_delta_decode(p_msg_in->tai_utc_delta);
	    
	time_client_callbacks.time_status_cb(p_client, &p_rx_msg->meta_data, &in_data);
    }
}

static const access_opcode_handler_t m_opcode_handlers[] = {
    {ACCESS_OPCODE_SIG(TIME_OPCODE_STATUS), handle_time_status},
};

static void periodic_publish_client_cb(access_model_handle_t handle, void * p_args) {
    time_client_t * p_client = (time_client_t *) p_args;
    
    if (time_client_callbacks.periodic_publish_cb != NULL) {
	    time_client_callbacks.periodic_publish_cb(handle, p_args);
    }
}

static void transaction_status(access_model_handle_t model_handle, 
                               access_reliable_status_t status, 
                               void * p_args) {
    time_client_t * p_client = (time_client_t *) p_args;
    
    if (time_client_callbacks.ack_transaction_status_cb != NULL) {
	    time_client_callbacks.ack_transaction_status_cb(model_handle, p_args, status);
    }
	
}

static void message_create(time_client_t * p_client, uint16_t tx_opcode, const uint8_t * p_buffer,
                           uint16_t length, access_message_tx_t *p_message) {
    p_message->opcode.opcode = tx_opcode;
    p_message->opcode.company_id = ACCESS_COMPANY_ID_NONE;
    p_message->p_buffer = p_buffer;
    p_message->length = length;
    p_message->force_segmented = p_client->settings.force_segmented;
    p_message->transmic_size = p_client->settings.transmic_size;
    p_message->access_token = nrf_mesh_unique_token_get();
}

static void reliable_context_create(time_client_t * p_client, uint16_t reply_opcode,
                                    access_reliable_t * p_reliable) {
    p_reliable->model_handle = p_client->model_handle;
    p_reliable->reply_opcode.opcode = reply_opcode;
    p_reliable->reply_opcode.company_id = ACCESS_COMPANY_ID_NONE;
    p_reliable->timeout = p_client->settings.timeout;
    p_reliable->status_cb = transaction_status;
}

/** Interface functions */
uint32_t time_client_init(time_client_t * p_client, uint8_t element_index) {
    if (p_client == NULL) {
        return NRF_ERROR_NULL;
    }

    if (p_client->settings.timeout == 0) {
        p_client->settings.timeout = MODEL_ACKNOWLEDGED_TRANSACTION_TIMEOUT;
    }

    access_model_add_params_t add_params = {
        .model_id = ACCESS_MODEL_SIG(TIME_CLIENT_MODEL_ID),
        .element_index = element_index,
        .p_opcode_handlers = &m_opcode_handlers[0],
        .opcode_count = ARRAY_SIZE(m_opcode_handlers),
        .p_args = p_client,
        .publish_timeout_cb = periodic_publish_client_cb
    };

    uint32_t status  = access_model_add(&add_params, &p_client->model_handle);

    if (status == NRF_SUCCESS) {
        status = access_model_subscription_list_alloc(p_client->model_handle);
    }

    return status;
}

void time_client_set_callbacks(time_client_callbacks_t * p_callbacks) {
    memcpy(&time_client_callbacks, p_callbacks, sizeof(time_client_callbacks_t));
}

uint32_t time_client_time_get(time_client_t * p_client) {
    if (p_client == NULL) {
        return NRF_ERROR_NULL;
    }

    if (access_reliable_model_is_free(p_client->model_handle)) {
        message_create(p_client, TIME_OPCODE_GET, NULL, 0, &p_client->access_message.message);
        reliable_context_create(p_client, TIME_OPCODE_STATUS, &p_client->access_message);

        return access_model_reliable_publish(&p_client->access_message);
    }
    else {
        return NRF_ERROR_BUSY;
    }
}