/**
 * @file turing-machine.c
 * @author Rastislav Szabo <raszabo@cisco.com>, Lukas Macko <lmacko@cisco.com>
 * @brief Example plugin for sysrepo datastore - turing machine.
 *
 * @copyright
 * Copyright 2016 Cisco Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdio>
#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <sstream>

extern "C" {
#include "sysrepo.h"

int kea_ctrl_channel_connect() {
    return SR_ERR_OK;
}

int kea_ctrl_channel_close() {
    return SR_ERR_OK;
}

int kea_ctrl_send(const char* json) {
    if (!json || !strlen(json)) {
	printf("Must provide non-empty configuration");
	return SR_ERR_INVAL_ARG;
    }
    printf("STUB: sending configuration: [%s]\n",
	   json);
    return SR_ERR_OK;
}

/* logging macro for unformatted messages */
#define log_msg(MSG) \
    do { \
        fprintf(stderr, MSG "\n"); \
        syslog(LOG_INFO, MSG); \
    } while(0)

/* logging macro for formatted messages */
#define log_fmt(MSG, ...) \
    do { \
        fprintf(stderr, MSG "\n", __VA_ARGS__); \
        syslog(LOG_INFO, MSG, __VA_ARGS__); \
    } while(0)

/* prints one value retrieved from sysrepo */
static void
print_value(sr_val_t *value)
{
    switch (value->type) {
        case SR_CONTAINER_T:
        case SR_CONTAINER_PRESENCE_T:
        case SR_LIST_T:
            /* do not print */
            break;
        case SR_STRING_T:
            log_fmt("%s = '%s'", value->xpath, value->data.string_val);
            break;
        case SR_BOOL_T:
            log_fmt("%s = %s", value->xpath, value->data.bool_val ? "true" : "false");
            break;
        case SR_UINT8_T:
            log_fmt("%s = %u", value->xpath, value->data.uint8_val);
            break;
        case SR_UINT16_T:
            log_fmt("%s = %u", value->xpath, value->data.uint16_val);
            break;
        case SR_UINT32_T:
            log_fmt("%s = %u", value->xpath, value->data.uint32_val);
            break;
        case SR_IDENTITYREF_T:
            log_fmt("%s = %s", value->xpath, value->data.identityref_val);
            break;
        case SR_ENUM_T:
            log_fmt("%s = %s", value->xpath, value->data.enum_val);
            break;
        default:
            log_fmt("%s (unprintable)", value->xpath);
    }
}

/* retrieves & prints current turing-machine configuration */
static void
retrieve_current_config(sr_session_ctx_t *session)
{
    sr_val_t *all_values = NULL;
    sr_val_t *values = NULL;
    size_t count = 0;
    size_t all_count = 0;
    int rc = SR_ERR_OK;

    log_msg("current plugin-kea configuration:");

    /* Going through all of the nodes */
    log_msg("control-socket parameters:\n");
    sr_session_refresh(session);
    rc = sr_get_items(session, "/ietf-kea-dhcpv6:server//*", &all_values, &all_count);
    if (SR_ERR_OK != rc) {
        printf("Error by sr_get_items: %s", sr_strerror(rc));
        return;
    }

    std::ostringstream s;

    s << "{ \"Dhcp6\": {";

    rc = sr_get_items(session, "/ietf-kea-dhcpv6:server/serv-attributes/control-socket/*", &values, &count);
    if (rc == SR_ERR_OK) {
        s << "\"control-socket\": {";
        for (size_t i = 0; i < count; ++i) {
            if (std::string(values[i].xpath) == "/ietf-kea-dhcpv6:server/serv-attributes/control-socket/socket-type") {
                s << "\"socket-type\": \"" << values[i].data.string_val << "\",";
            }
            if (std::string(values[i].xpath) == "/ietf-kea-dhcpv6:server/serv-attributes/control-socket/socket-name") {
                s << "\"socket-name\": \"" << values[i].data.string_val << "\"";
            }
        }
        s << "},";
        sr_free_values(values, count);
    }

    sr_val_t* value = NULL;
    rc = sr_get_item(session, "/ietf-kea-dhcpv6:server/serv-attributes/interfaces-config/interfaces", &value);
    if (rc == SR_ERR_OK) {
        s << "\"interfaces-config\": { " << std::endl;;
        s << "\"interfaces\": [ \"" << std::endl;;
        s << value->data.string_val << std::endl;
        s << "\" ]" << std::endl;
        s <<  "}," << std::endl;;
        sr_free_values(value, 1);

    }

    rc = sr_get_item(session, "/ietf-kea-dhcpv6:server/serv-attributes/renew-timer", &value);
    if (rc == SR_ERR_OK) {
        s << "\"renew-timer\":" << value->data.uint32_val << "," << std::endl;
        sr_free_values(value, 1);
    }

    rc = sr_get_item(session, "/ietf-kea-dhcpv6:server/serv-attributes/rebind-timer", &value);
    if (rc == SR_ERR_OK) {
        s << "\"rebind-timer\":" << value->data.uint32_val << "," << std::endl;; 
        sr_free_values(value, 1);
    }

    rc = sr_get_item(session, "/ietf-kea-dhcpv6:server/serv-attributes/preferred-lifetime", &value);
    if (rc == SR_ERR_OK) {
        s << "\"preferred-lifetime\":" << value->data.uint32_val << "," << std::endl;; 
        sr_free_values(value, 1);
    }

    rc = sr_get_item(session, "/ietf-kea-dhcpv6:server/serv-attributes/valid-lifetime", &value);
    if (rc == SR_ERR_OK) {
        s << "\"valid-lifetime\":" << value->data.uint32_val << "" << std::endl;; 
        sr_free_values(value, 1);
    }

    s << "}" << std::endl << "}" << std::endl;

    std::ofstream fs;
    fs.open("/tmp/kea-plugin-gen-cfg.json", std::ofstream::out);
    fs << s.str();
    fs.close();

    system ("/Users/marcin/devel/sysrepo-plugin-kea/kea-client/ctrl-channel-cli /tmp/kea-control-channel /tmp/kea-plugin-gen-cfg.json");

    remove("/tmp/kea-plugin-gen-cfg.json");

    std::cout << s.str() << std::endl;

    sr_free_values(all_values, all_count);
}

static int
module_change_cb(sr_session_ctx_t *session, const char *module_name, sr_notif_event_t event, void *private_ctx)
{
    log_msg("plugin-kea configuration has changed");
    retrieve_current_config(session);

    return SR_ERR_OK;
}

int
sr_plugin_init_cb(sr_session_ctx_t *session, void **private_ctx)
{
    sr_subscription_ctx_t *subscription = NULL;
    int rc = SR_ERR_OK;

    //rc = sr_module_change_subscribe(session, "ietf-kea-dhcpv6", module_change_cb, NULL,
    //				    0, SR_SUBSCR_DEFAULT, &subscription);
    rc = sr_subtree_change_subscribe(session, "/ietf-kea-dhcpv6:server/*", module_change_cb, NULL,
				    0, SR_SUBSCR_DEFAULT, &subscription);
    if (SR_ERR_OK != rc) {
        goto error;
    }

    log_msg("plugin-kea initialized successfully");

    retrieve_current_config(session);

    /* set subscription as our private context */
    *private_ctx = subscription;

    return SR_ERR_OK;

error:
    log_fmt("plugin-kea initialization failed: %s", sr_strerror(rc));
    sr_unsubscribe(session, subscription);
    return rc;
}

void
sr_plugin_cleanup_cb(sr_session_ctx_t *session, sr_subscription_ctx_t *private_ctx)
{
    /* subscription was set as our private context */
    sr_unsubscribe(session, private_ctx);

    log_msg("pluging-kea plugin cleanup finished");
}

}
