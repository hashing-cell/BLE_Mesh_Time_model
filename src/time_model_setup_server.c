#include "time_model_setup_server.h"

#include "time_model_common.h"
#include "time_model_messages.h"

#include <stdint.h>
#include <stdlib.h>

#include "access.h"
#include "access_config.h"
#include "device_state_manager.h"

static time_server_callbacks_t time_serv_callbacks = {0};
static time_setup_server_callbacks_t time_setup_serv_callbacks = {0};

#if TIME_MODEL_USE_APP_TIMER
#include "app_timer.h"

#define ONE_SEC (APP_TIMER_TICKS(1000))

APP_TIMER_DEF(m_time_model_timer);
#endif

/********************************************************************* 
    TIME SERVER AND SETUP SERVER STATES IMPLEMENTATION
**********************************************************************/

static void current_time_check_time_changes(time_server_t * p_server) {
    if (p_server->server_state.tai_seconds == p_server->server_state.time_zone_change &&
	    p_server->server_state.time_zone_change != TIME_ZONE_CHANGE_UNKNOWN) {
	    p_server->server_state.time_zone_offset_current = p_server->server_state.time_zone_offset_new;
    }

    if (p_server->server_state.tai_seconds == p_server->server_state.tai_utc_delta_change &&
	    p_server->server_state.tai_utc_delta_change != TAI_DELTA_CHANGE_UNKNOWN) {
	    p_server->server_state.tai_utc_delta_current = p_server->server_state.tai_utc_delta_new;
    }
}

void time_state_update_time(time_server_t * p_server, uint64_t tai_seconds, uint8_t subsecond) {
    if (p_server == NULL) {
        return;
    }

    if (!validate_tai_time_arg(tai_seconds)) {
	    return;
    }

    p_server->server_state.tai_seconds = tai_seconds;
    p_server->server_state.subsecond = subsecond;
    current_time_check_time_changes(p_server);
}

void time_state_update_time_delta(time_server_t * p_server, uint64_t delta_tai_seconds, uint8_t delta_subsecond) {
    if (p_server == NULL) {
        return;
    }
    
    if (!validate_tai_time_arg(p_server->server_state.tai_seconds + delta_tai_seconds)) {
	    return;
    }

    p_server->server_state.tai_seconds += delta_tai_seconds;
    p_server->server_state.subsecond += delta_subsecond;
    current_time_check_time_changes(p_server);
}


void time_state_update_uncertainty(time_server_t * p_server, uint8_t uncertainty) {
    if (p_server == NULL) {
        return;
    }

    p_server->server_state.uncertainty = uncertainty;
}

#if TIME_MODEL_USE_APP_TIMER
static void time_model_app_timer_cb(void * p_context) {
    time_server_t * p_server = (time_server_t *) p_context;

    time_state_update_time_delta(p_server, 1, 0);
}
#endif

uint32_t time_server_state_set_time(time_server_t * p_server, time_set_params_t * time_params) { 
    if (p_server == NULL) {
        return NRF_ERROR_NULL;
    }

    bool validate_args = (validate_tai_time_arg(time_params->tai_seconds) &&
			              validate_tai_utc_delta_arg(time_params->tai_utc_delta) &&
			              validate_time_zone_offset_arg(time_params->time_zone_offset));
    if (!validate_args) {
	    return NRF_ERROR_INVALID_PARAM;
    }

#if TIME_MODEL_USE_APP_TIMER
    app_timer_stop(m_time_model_timer);
#endif   
    p_server->server_state.tai_seconds = time_params->tai_seconds;
    p_server->server_state.subsecond = time_params->subsecond;
    p_server->server_state.uncertainty = time_params->uncertainty;
    p_server->server_state.time_authority = time_params->time_authority;
    p_server->server_state.time_zone_offset_current = time_params->time_zone_offset;
    p_server->server_state.tai_utc_delta_current = time_params->tai_utc_delta;
#if TIME_MODEL_USE_APP_TIMER
    app_timer_start(m_time_model_timer, ONE_SEC, p_server);
#endif 

    if (p_server->settings.publish_upon_state_change) {
	    return time_server_time_status_publish(p_server);
    } else {
	    return NRF_SUCCESS;
    }
}

uint32_t time_server_state_set_time_zone_offset(time_server_t * p_server, time_zone_set_params_t * time_zone_params) {
    if (p_server == NULL) {
        return NRF_ERROR_NULL;
    }

    bool validate_args = (validate_tai_time_arg(time_zone_params->time_zone_change) &&
                          validate_time_zone_offset_arg(time_zone_params->time_zone_offset_new));

    if (!validate_args) {
	    return NRF_ERROR_INVALID_PARAM;
    }
    
    p_server->server_state.time_zone_offset_new = time_zone_params->time_zone_offset_new;
    p_server->server_state.time_zone_change = time_zone_params->time_zone_change;

    if (p_server->settings.publish_upon_state_change) {
	    return time_server_time_zone_status_publish(p_server);
    } else {
	    return NRF_SUCCESS;
    } 
}

uint32_t time_server_state_set_tai_utc_delta(time_server_t * p_server, tai_utc_delta_set_params_t * tai_utc_delta_params) {
    if (p_server == NULL) {
        return NRF_ERROR_NULL;
    }

    bool validate_args = (validate_tai_time_arg(tai_utc_delta_params->tai_utc_delta_change) &&
			              validate_tai_utc_delta_arg(tai_utc_delta_params->tai_utc_delta_new));

    if (!validate_args) {
	    return NRF_ERROR_INVALID_PARAM;
    }
    
    p_server->server_state.tai_utc_delta_new = tai_utc_delta_params->tai_utc_delta_new;
    p_server->server_state.tai_utc_delta_change = tai_utc_delta_params->tai_utc_delta_change;

    if (p_server->settings.publish_upon_state_change) {
	    return time_server_tai_utc_delta_status_publish(p_server);
    } else {
	    return NRF_SUCCESS;
    }
}

uint32_t time_setup_server_state_set_time_role(time_setup_server_t * p_s_server, time_role_set_params_t * time_role_params) {
    if (p_s_server == NULL) {
	    return NRF_ERROR_NULL;
    }

    p_s_server->time_server.server_state.time_role = time_role_params->time_role;

    /* 
        Extra thing ambiguous in the specification: If the Time Role is not an authority, 
	    the authority state should be set to FALSE if the Time role set to a CLIENT or a RELAY,
	    otherwise it should be set to TRUE if it is an AUTHORITY
    */
    if (time_role_params->time_role == TIME_ROLE_AUTHORITY) {
	    p_s_server->time_server.server_state.time_authority = true;
    } else if (time_role_params->time_role == TIME_ROLE_CLIENT || time_role_params->time_role == TIME_ROLE_RELAY) {
	    p_s_server->time_server.server_state.time_authority = false;
    }

    if (p_s_server->settings.publish_upon_state_change) {
	    return time_setup_server_time_role_status_publish(p_s_server);
    } else {
	    return NRF_SUCCESS;
    }
}

/********************************************************************* 
    TIME SERVER IMPLEMENTATION
**********************************************************************/

static uint32_t time_status_send(const time_server_t * p_server, const access_message_rx_t * p_message) {
    time_status_msg_pkt_t msg_pkt = {
	    .tai_seconds = p_server->server_state.tai_seconds,
    };
    
    uint8_t msg_len;
    if (p_server->server_state.tai_seconds == TAI_TIME_UNKNOWN) {
	    msg_len = TIME_STATUS_MINLEN;
    } else {
        msg_pkt.subsecond = p_server->server_state.subsecond;
        msg_pkt.uncertainty = p_server->server_state.uncertainty;
        msg_pkt.time_authority = p_server->server_state.time_authority;
        msg_pkt.time_zone_offset = time_zone_offset_encode(p_server->server_state.time_zone_offset_current);
        msg_pkt.tai_utc_delta = tai_utc_delta_encode(p_server->server_state.tai_utc_delta_current);
        msg_len = TIME_STATUS_MAXLEN;
    }

    access_message_tx_t reply = {
        .opcode = ACCESS_OPCODE_SIG(TIME_OPCODE_STATUS),
        .p_buffer = (const uint8_t *) &msg_pkt,
        .length = msg_len,
        .force_segmented = p_server->settings.force_segmented,
        .transmic_size = p_server->settings.transmic_size
    };

    if (p_message == NULL) {
	    uint8_t previous_ttl;
	    uint32_t status = NRF_SUCCESS;
	if (p_server->server_state.tai_seconds == TAI_TIME_UNKNOWN) {
	    return NRF_ERROR_INVALID_STATE;
	}
	access_model_publish_ttl_get(p_server->model_handle, &previous_ttl);
	
	status = access_model_publish_ttl_set(p_server->model_handle, TIME_STATUS_MSG_TTL);
	if (status == NRF_SUCCESS) {
	    status = access_model_publish(p_server->model_handle, &reply);
	}
	access_model_publish_ttl_set(p_server->model_handle, previous_ttl);
	return status;
    }
    else {
        return access_model_reply(p_server->model_handle, p_message, &reply);
    }

}

static uint32_t time_zone_status_send(const time_server_t * p_server, const access_message_rx_t * p_message) {
    time_zone_status_msg_pkt_t msg_pkt = {
        .time_zone_offset_current = time_zone_offset_encode(p_server->server_state.time_zone_offset_current),
        .time_zone_offset_new = time_zone_offset_encode(p_server->server_state.time_zone_offset_new),
        .time_zone_change = p_server->server_state.time_zone_change
    };

    access_message_tx_t reply = {
        .opcode = ACCESS_OPCODE_SIG(TIME_OPCODE_ZONE_STATUS),
        .p_buffer = (const uint8_t *) &msg_pkt,
        .length = TIME_ZONE_STATUS_LEN,
        .force_segmented = p_server->settings.force_segmented,
        .transmic_size = p_server->settings.transmic_size
    };

    if (p_message == NULL) {
        return access_model_publish(p_server->model_handle, &reply);
    }
    else {
        return access_model_reply(p_server->model_handle, p_message, &reply);
    }
}

static uint32_t tai_utc_delta_status_send(const time_server_t * p_server, const access_message_rx_t * p_message) {
    tai_utc_delta_status_msg_pkt_t msg_pkt = {
        .tai_utc_delta_current = tai_utc_delta_encode(p_server->server_state.tai_utc_delta_current),
        .tai_utc_delta_new = tai_utc_delta_encode(p_server->server_state.tai_utc_delta_new),
        .tai_utc_delta_change = p_server->server_state.tai_utc_delta_change,
        .padding1 = 0,
        .padding2 = 0
    };

    access_message_tx_t reply = {
        .opcode = ACCESS_OPCODE_SIG(TIME_OPCODE_TAI_UTC_DELTA_STATUS),
        .p_buffer = (const uint8_t *) &msg_pkt,
        .length = TAI_UTC_DELTA_STATUS_LEN,
        .force_segmented = p_server->settings.force_segmented,
        .transmic_size = p_server->settings.transmic_size
    };

    if (p_message == NULL) {
        return access_model_publish(p_server->model_handle, &reply);
    }
    else {
        return access_model_reply(p_server->model_handle, p_message, &reply);
    }
}


static void handle_time_get(access_model_handle_t model_handle, const access_message_rx_t * p_rx_msg, void * p_args) {
    time_server_t * p_server = (time_server_t *) p_args;

    if (time_serv_callbacks.time_get_cb != NULL) {
	    time_serv_callbacks.time_get_cb(p_server, &p_rx_msg->meta_data);
    }

    time_status_send(p_server, p_rx_msg);
}

static void handle_time_status(access_model_handle_t model_handle, const access_message_rx_t * p_rx_msg, void * p_args) {
    time_server_t * p_server = (time_server_t *) p_args;
    
    if (p_server->server_state.time_role == TIME_ROLE_NONE || p_server->server_state.time_role == TIME_ROLE_AUTHORITY) {
	    return;
    }

    /*
	    Not explicity specified in the specification but 100% necessary to prevent
	    a TIME RELAY node from killing itself due to infinitely recieving then relaying 
	    its own message
    */
    dsm_local_unicast_address_t address;
    dsm_local_unicast_addresses_get(&address);
    if (address.address_start == p_rx_msg->meta_data.src.value) {
	    return;
    }

#if TIME_MODEL_USE_APP_TIMER
    app_timer_stop(m_time_model_timer);
#endif    
    time_status_msg_pkt_t * p_msg_in = (time_status_msg_pkt_t *) p_rx_msg->p_data;

    p_server->server_state.tai_seconds = p_msg_in->tai_seconds;
    p_server->server_state.subsecond = p_msg_in->subsecond;
    p_server->server_state.uncertainty = p_msg_in->uncertainty;
    p_server->server_state.time_zone_offset_current = time_zone_offset_decode(p_msg_in->time_zone_offset);
    p_server->server_state.tai_utc_delta_current = tai_utc_delta_decode(p_msg_in->tai_utc_delta);
    current_time_check_time_changes(p_server);
#if TIME_MODEL_USE_APP_TIMER
    app_timer_start(m_time_model_timer, ONE_SEC, p_server);
#endif 

    if (time_serv_callbacks.time_status_cb != NULL) {
        time_status_params_t in_data;
        in_data.tai_seconds = p_msg_in->tai_seconds;
        in_data.subsecond = p_msg_in->subsecond;
        in_data.uncertainty = p_msg_in->uncertainty;
        in_data.time_zone_offset = time_zone_offset_decode(p_msg_in->time_zone_offset);
        in_data.tai_utc_delta = tai_utc_delta_decode(p_msg_in->tai_utc_delta);

        time_serv_callbacks.time_status_cb(p_server, &p_rx_msg->meta_data, &in_data);
    }

    if (p_server->server_state.time_role == TIME_ROLE_RELAY) {
	/* 
	    Extra thing that may or may not go against the specification:

	    The specification states that a time server with the role of a TIME RELAY
	    shall publish a time status message after syncing. HOWEVER, the specification
	    failed to consider the case of where 2 TIME_RELAY nodes continuously relay
	    with each other which can caused an infinte relay of messages between multiple
	    TIME RELAY nodes. 

	    To prevent this issue, my implementation makes it so that a time server with the role of TIME RELAY
	    should only relay messages that have the time authority field set to TRUE, and enforce
	    that all TIME RELAY nodes have their own time authority state set to false. This way, 
	    a TIME RELAY node will never relay the message sent from another TIME RELAY node in order
	    to prevent this infinte relaying
	*/
	if (p_msg_in->time_authority) {
	    p_server->server_state.time_authority = false;
	    time_status_send(p_server, NULL);
	}
    }
}

static void handle_time_zone_get(access_model_handle_t model_handle, const access_message_rx_t * p_rx_msg, void * p_args) {
    time_server_t * p_server = (time_server_t *) p_args;
    
    if (time_serv_callbacks.time_zone_get_cb != NULL) {
	    time_serv_callbacks.time_zone_get_cb(p_server, &p_rx_msg->meta_data);
    }

    time_zone_status_send(p_server, p_rx_msg);
}

static void handle_tai_utc_delta_get(access_model_handle_t model_handle, const access_message_rx_t * p_rx_msg, void * p_args) {
    time_server_t * p_server = (time_server_t *) p_args;
    
    if (time_serv_callbacks.tai_utc_delta_get_cb != NULL) {
	    time_serv_callbacks.tai_utc_delta_get_cb(p_server, &p_rx_msg->meta_data);
    }

    tai_utc_delta_status_send(p_server, p_rx_msg);
}

static const access_opcode_handler_t m_opcode_handlers_server[] = {
    {ACCESS_OPCODE_SIG(TIME_OPCODE_GET), handle_time_get},
    {ACCESS_OPCODE_SIG(TIME_OPCODE_STATUS), handle_time_status},
    {ACCESS_OPCODE_SIG(TIME_OPCODE_ZONE_GET), handle_time_zone_get},
    {ACCESS_OPCODE_SIG(TIME_OPCODE_TAI_UTC_DELTA_GET), handle_tai_utc_delta_get},
};

static void periodic_publish_serv_cb(access_model_handle_t handle, void * p_args) {
    time_server_t * p_server = (time_server_t *) p_args;
    
    if (time_serv_callbacks.time_get_cb != NULL) {
	    time_serv_callbacks.time_get_cb(p_server, NULL);
    }

    time_status_send(p_server, NULL);
}

static uint32_t time_server_init(time_server_t * p_server, uint8_t element_index) {
    uint32_t status;

    if (p_server == NULL) {
        return NRF_ERROR_NULL;
    }

    access_model_add_params_t init_params = {
        .model_id = ACCESS_MODEL_SIG(TIME_SERVER_MODEL_ID),
        .element_index = element_index,
        .p_opcode_handlers = m_opcode_handlers_server,
        .opcode_count = ARRAY_SIZE(m_opcode_handlers_server),
        .p_args = p_server,
        .publish_timeout_cb = periodic_publish_serv_cb
    };

    status = access_model_add(&init_params, &p_server->model_handle);
    if (status == NRF_SUCCESS) {
	    status = access_model_subscription_list_alloc(p_server->model_handle);
    }
    return status;


}

void time_server_set_callbacks(time_server_callbacks_t * p_callbacks) {
    memcpy(&time_serv_callbacks, p_callbacks, sizeof(time_server_callbacks_t));
}

uint32_t time_server_time_status_publish(const time_server_t * p_server) {
    if (p_server->server_state.time_role == TIME_ROLE_CLIENT) {
	    return NRF_ERROR_FORBIDDEN;
    }
    return time_status_send(p_server, NULL);
}

uint32_t time_server_time_zone_status_publish(const time_server_t * p_server) {
    if (p_server->server_state.time_role == TIME_ROLE_CLIENT) {
	    return NRF_ERROR_FORBIDDEN;
    }
    return time_zone_status_send(p_server, NULL);
}

uint32_t time_server_tai_utc_delta_status_publish(const time_server_t * p_server) {
    if (p_server->server_state.time_role == TIME_ROLE_CLIENT) {
	    return NRF_ERROR_FORBIDDEN;
    }
    return tai_utc_delta_status_send(p_server, NULL);
}


/********************************************************************* 
    TIME SETUP SERVER IMPLEMENTATION
**********************************************************************/
static uint32_t time_role_status_send(const time_setup_server_t * p_s_server, const access_message_rx_t * p_message) {
    time_role_status_msg_pkt_t msg_pkt = {
	    .time_role = p_s_server->time_server.server_state.time_role
    };

    access_message_tx_t reply = {
        .opcode = ACCESS_OPCODE_SIG(TIME_OPCODE_ROLE_STATUS),
        .p_buffer = (const uint8_t *) &msg_pkt,
        .length = TIME_ROLE_STATUS_LEN,
        .force_segmented = p_s_server->settings.force_segmented,
        .transmic_size = p_s_server->settings.transmic_size
    };

    if (p_message == NULL) {
        return access_model_publish(p_s_server->model_handle, &reply);
    }
    else {
        return access_model_reply(p_s_server->model_handle, p_message, &reply);
    }

}

static void handle_time_set(access_model_handle_t model_handle, const access_message_rx_t * p_rx_msg, void * p_args) {
    time_setup_server_t * p_s_server = (time_setup_server_t *) p_args;
    
    time_set_msg_pkt_t * p_msg_in = (time_set_msg_pkt_t *) p_rx_msg->p_data;

#if TIME_MODEL_USE_APP_TIMER
    app_timer_stop(m_time_model_timer);
#endif    
    p_s_server->time_server.server_state.tai_seconds = p_msg_in->tai_seconds;
    p_s_server->time_server.server_state.subsecond = p_msg_in->subsecond;
    p_s_server->time_server.server_state.uncertainty = p_msg_in->uncertainty;
    p_s_server->time_server.server_state.time_zone_offset_current = time_zone_offset_decode(p_msg_in->time_zone_offset);
    p_s_server->time_server.server_state.tai_utc_delta_current = tai_utc_delta_decode(p_msg_in->tai_utc_delta);
#if TIME_MODEL_USE_APP_TIMER
    app_timer_start(m_time_model_timer, ONE_SEC, &p_s_server->time_server);
#endif    
    if (time_setup_serv_callbacks.time_set_cb != NULL) {
        time_set_params_t in_data;
        in_data.tai_seconds = p_msg_in->tai_seconds;
        in_data.subsecond = p_msg_in->subsecond;
        in_data.uncertainty = p_msg_in->uncertainty;
        in_data.time_zone_offset = time_zone_offset_decode(p_msg_in->time_zone_offset);
        in_data.tai_utc_delta = tai_utc_delta_decode(p_msg_in->tai_utc_delta);

        time_setup_serv_callbacks.time_set_cb(p_s_server, &p_rx_msg->meta_data, &in_data);
    }

    time_status_send(&p_s_server->time_server, p_rx_msg);
}

static void handle_time_zone_set(access_model_handle_t model_handle, const access_message_rx_t * p_rx_msg, void * p_args) {
    time_setup_server_t * p_s_server = (time_setup_server_t *) p_args;

    time_zone_set_msg_pkt_t * p_msg_in = (time_zone_set_msg_pkt_t *) p_rx_msg->p_data;

    p_s_server->time_server.server_state.time_zone_offset_new = time_zone_offset_decode(p_msg_in->time_zone_offset_new);
    p_s_server->time_server.server_state.time_zone_change = p_msg_in->time_zone_change;
    if (time_setup_serv_callbacks.time_zone_set_cb != NULL) {
        time_zone_set_params_t in_data;
        in_data.time_zone_offset_new = time_zone_offset_decode(p_msg_in->time_zone_offset_new);
        in_data.time_zone_change = p_msg_in->time_zone_change;

        time_setup_serv_callbacks.time_zone_set_cb(p_s_server, &p_rx_msg->meta_data, &in_data);
    }

    time_zone_status_send(&p_s_server->time_server, p_rx_msg);
}

static void handle_tai_utc_delta_set(access_model_handle_t model_handle, const access_message_rx_t * p_rx_msg, void * p_args) {
    time_setup_server_t * p_s_server = (time_setup_server_t *) p_args;

    tai_utc_delta_set_msg_pkt_t * p_msg_in = (tai_utc_delta_set_msg_pkt_t *) p_rx_msg->p_data;

    p_s_server->time_server.server_state.tai_utc_delta_new = tai_utc_delta_decode(p_msg_in->tai_utc_delta_new);
    p_s_server->time_server.server_state.tai_utc_delta_change = p_msg_in->tai_utc_delta_change;
    if (time_setup_serv_callbacks.tai_utc_delta_set_cb != NULL) {
        tai_utc_delta_set_params_t in_data;
        in_data.tai_utc_delta_new = tai_utc_delta_decode(p_msg_in->tai_utc_delta_new);
        in_data.tai_utc_delta_change = p_msg_in->tai_utc_delta_change;

        time_setup_serv_callbacks.tai_utc_delta_set_cb(p_s_server, &p_rx_msg->meta_data, &in_data);
    }

    tai_utc_delta_status_send(&p_s_server->time_server, p_rx_msg);
}

static void handle_time_role_get(access_model_handle_t model_handle, const access_message_rx_t * p_rx_msg, void * p_args) {
    time_setup_server_t * p_s_server = (time_setup_server_t *) p_args;

    if (time_setup_serv_callbacks.time_role_get_cb != NULL) {
	    time_setup_serv_callbacks.time_role_get_cb(p_s_server, &p_rx_msg->meta_data);
    }

    time_role_status_send(p_s_server, p_rx_msg);
}

static void handle_time_role_set(access_model_handle_t model_handle, const access_message_rx_t * p_rx_msg, void * p_args) {
    time_setup_server_t * p_s_server = (time_setup_server_t *) p_args;
    
    time_role_set_msg_pkt_t * p_msg_in = (time_role_set_msg_pkt_t *) p_rx_msg->p_data;

    p_s_server->time_server.server_state.time_role = p_msg_in->time_role;
    /* Extra thing not explicitly stated in spec: If the Time Role is not an authority, 
	the authority state should be set to FALSE if the Time role set to a CLIENT or a RELAY,
	otherwise it should be set to TRUE if it is an AUTHORITY
    */
    if (p_msg_in->time_role == TIME_ROLE_AUTHORITY) {
	    p_s_server->time_server.server_state.time_authority = true;
    } else if (p_msg_in->time_role == TIME_ROLE_CLIENT || p_msg_in->time_role == TIME_ROLE_RELAY) {
	    p_s_server->time_server.server_state.time_authority = false;
    }

    if (time_setup_serv_callbacks.time_role_set_cb != NULL) {
	time_role_set_params_t in_data = {
	    .time_role = p_msg_in->time_role
	};  
	
	time_setup_serv_callbacks.time_role_set_cb(p_s_server, &p_rx_msg->meta_data, &in_data);
    }

    time_role_status_send(p_s_server, p_rx_msg);
}

static const access_opcode_handler_t m_opcode_handlers_setup_server[] = {
    {ACCESS_OPCODE_SIG(TIME_OPCODE_SET), handle_time_set},
    {ACCESS_OPCODE_SIG(TIME_OPCODE_ZONE_SET), handle_time_zone_set},
    {ACCESS_OPCODE_SIG(TIME_OPCODE_TAI_UTC_DELTA_SET), handle_tai_utc_delta_set},
    {ACCESS_OPCODE_SIG(TIME_OPCODE_ROLE_GET), handle_time_role_get},
    {ACCESS_OPCODE_SIG(TIME_OPCODE_ROLE_SET), handle_time_role_set},
};

uint32_t time_setup_server_init(time_setup_server_t * p_s_server, uint8_t element_index) {
    uint32_t status;
    if (p_s_server == NULL) {
        return NRF_ERROR_NULL;
    }

    p_s_server->settings.element_index = element_index;

    p_s_server->time_server.settings.force_segmented = p_s_server->settings.force_segmented;
    p_s_server->time_server.settings.transmic_size = p_s_server->settings.transmic_size;
    status = time_server_init(&p_s_server->time_server, element_index);
    if (status != NRF_SUCCESS) {
        return status;
    }


    access_model_add_params_t init_params = {
	    .model_id = ACCESS_MODEL_SIG(TIME_SETUP_SERVER_MODEL_ID),
	    .element_index =  element_index,
	    .p_opcode_handlers = m_opcode_handlers_setup_server,
	    .opcode_count = ARRAY_SIZE(m_opcode_handlers_setup_server),
	    .p_args = p_s_server,
	    .publish_timeout_cb = NULL
    };

    status = access_model_add(&init_params, &p_s_server->model_handle);
#if TIME_MODEL_USE_APP_TIMER
    if (status == NRF_SUCCESS) {
	    status = app_timer_create(&m_time_model_timer, APP_TIMER_MODE_REPEATED, time_model_app_timer_cb);
	    app_timer_start(m_time_model_timer, ONE_SEC, (void *) &p_s_server->time_server);
    }
#endif
    return status;
}


void time_setup_server_set_callbacks(time_setup_server_callbacks_t * p_callbacks) {
    memcpy(&time_setup_serv_callbacks, p_callbacks, sizeof(time_setup_server_callbacks_t));
}

uint32_t time_setup_server_time_role_status_publish(const time_setup_server_t * p_s_server) {
    return time_role_status_send(p_s_server, NULL);
}
