/*
 * This file is part of the Soletta Project
 *
 * Copyright (C) 2015 Intel Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <fcntl.h>
#include <linux/kd.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include "sol-log.h"
#include "sol-mainloop.h"
#include "sol-network.h"
#include "sol-str-slice.h"
#include "sol-vector.h"

#include "sol-coap.h"

#define DEFAULT_UDP_PORT 5683

#define OC_CORE_JSON_SEPARATOR ","
#define OC_CORE_ELEM_JSON_START "{\"oc\":[{\"href\":\"%s\",\"rep\":{"
#define OC_CORE_PROP_JSON_NUMBER "\"%s\":%d"
#define OC_CORE_PROP_JSON_STRING "\"%s\":\"%s\""
#define OC_CORE_PROP_JSON_BOOLEAN "\"%s\":%s"
#define OC_CORE_ELEM_JSON_END "}}]}"


struct light_context {
    struct sol_coap_server *server;
    struct sol_coap_resource *resource;
};

static int console_fd;

static bool
get_scrolllock_led(void)
{
    char value;

    if (ioctl(console_fd, KDGETLED, (char *)&value)) {
        perror("Could not get led state");
        return false;
    }

    return value & LED_SCR;
}

static void
set_scrolllock_led(bool on)
{
    char old;

    if (ioctl(console_fd, KDGETLED, (char *)&old)) {
        perror("Could not get led state");
        return;
    }

    if (ioctl(console_fd, KDSETLED, on ? (old | LED_SCR) : (old & ~LED_SCR))) {
        perror("Could not set led state");
        return;
    }
}

static int
light_resource_to_rep(const struct sol_coap_resource *resource,
    bool state, struct sol_buffer *buf)
{
    SOL_BUFFER_DECLARE_STATIC(buffer, 64);
    int r;

    r = sol_coap_uri_path_to_buf(resource->path, &buffer, 0, NULL);
    SOL_INT_CHECK(r, < 0, r);

    r = sol_buffer_append_printf(buf,
        OC_CORE_ELEM_JSON_START, (char *)sol_buffer_steal(&buffer, NULL));
    SOL_INT_CHECK(r, < 0, r);

    r = sol_buffer_append_printf(buf,
        OC_CORE_PROP_JSON_NUMBER, "power", 100);
    SOL_INT_CHECK(r, < 0, r);

    r = sol_buffer_append_printf(buf, OC_CORE_JSON_SEPARATOR);
    SOL_INT_CHECK(r, < 0, r);

    r = sol_buffer_append_printf(buf,
        OC_CORE_PROP_JSON_STRING, "name", "Soletta LAMP!");
    SOL_INT_CHECK(r, < 0, r);

    r = sol_buffer_append_printf(buf, OC_CORE_JSON_SEPARATOR);
    SOL_INT_CHECK(r, < 0, r);

    r = sol_buffer_append_printf(buf,
        OC_CORE_PROP_JSON_BOOLEAN, "state", state ? "true" : "false" );
    SOL_INT_CHECK(r, < 0, r);

    r = sol_buffer_append_printf(buf, OC_CORE_ELEM_JSON_END);
    SOL_INT_CHECK(r, < 0, r);

    return 0;
}

static int
light_method_put(struct sol_coap_server *server,
    const struct sol_coap_resource *resource, struct sol_coap_packet *req,
    const struct sol_network_link_addr *cliaddr, void *data)
{
    sol_coap_responsecode_t code = SOL_COAP_RSPCODE_CONTENT;
    struct sol_coap_packet *resp;
    struct sol_buffer *buf;
    char *sub = NULL;
    size_t offset;
    bool value;
    int r;

    sol_coap_packet_get_payload(req, &buf, &offset);

    if (buf)
        sub = strstr((char *)sol_buffer_at(buf, offset), "state\":");
    if (!sub) {
        code = SOL_COAP_RSPCODE_BAD_REQUEST;
        goto done;
    }

    value = !memcmp(sub + strlen("state\":"), "true", sizeof("true") - 1);

    SOL_INF("Changing light state to %s", value ? "on" : "off");

    set_scrolllock_led(value);

done:
    resp = sol_coap_packet_new(req);
    if (!resp) {
        SOL_WRN("Could not build response packet");
        return -1;
    }
    r = sol_coap_header_set_type(resp, SOL_COAP_TYPE_ACK);
    SOL_INT_CHECK_GOTO(r, < 0, err);
    r = sol_coap_header_set_code(resp, code);
    SOL_INT_CHECK_GOTO(r, < 0, err);

    return sol_coap_send_packet(server, resp, cliaddr);

err:
    sol_coap_packet_unref(resp);
    return r;
}

static bool
update_light(void *data)
{
    struct light_context *context = data;
    struct sol_coap_server *server = context->server;
    struct sol_coap_resource *resource = context->resource;
    struct sol_coap_packet *pkt;
    struct sol_buffer *buf;
    int r;

    SOL_INF("Emitting notification");

    pkt = sol_coap_packet_notification_new(server, resource);
    SOL_NULL_CHECK(pkt, false);

    r = sol_coap_header_set_code(pkt, SOL_COAP_RSPCODE_CONTENT);
    SOL_INT_CHECK_GOTO(r, < 0, err);
    r = sol_coap_packet_get_payload(pkt, &buf, NULL);
    SOL_INT_CHECK_GOTO(r, < 0, err);
    r = light_resource_to_rep(resource, get_scrolllock_led(), buf);
    SOL_INT_CHECK_GOTO(r, < 0, err);

    return !sol_coap_packet_send_notification(server, resource, pkt);

err:
    sol_coap_packet_unref(pkt);
    return false;
}

static int
light_method_get(struct sol_coap_server *server,
    const struct sol_coap_resource *resource, struct sol_coap_packet *req,
    const struct sol_network_link_addr *cliaddr, void *data)
{
    struct sol_coap_packet *resp;
    struct sol_buffer *buf;
    int r;

    resp = sol_coap_packet_new(req);
    if (!resp) {
        SOL_WRN("Could not build response packet");
        return -1;
    }
    r = sol_coap_header_set_type(resp, SOL_COAP_TYPE_ACK);
    SOL_INT_CHECK_GOTO(r, < 0, err);
    r = sol_coap_header_set_code(resp, SOL_COAP_RSPCODE_CONTENT);
    SOL_INT_CHECK_GOTO(r, < 0, err);
    r = sol_coap_packet_get_payload(resp, &buf, NULL);
    SOL_INT_CHECK_GOTO(r, < 0, err);

    r = light_resource_to_rep(resource, get_scrolllock_led(), buf);
    SOL_INT_CHECK_GOTO(r, < 0, err);

    return sol_coap_send_packet(server, resp, cliaddr);

err:
    sol_coap_packet_unref(resp);
    return r;
}

static struct sol_coap_resource light = {
    SOL_SET_API_VERSION(.api_version = SOL_COAP_RESOURCE_API_VERSION, )
    .get = light_method_get,
    .put = light_method_put,
    .flags = SOL_COAP_FLAGS_WELL_KNOWN,
    .path = {
        SOL_STR_SLICE_LITERAL("a"),
        SOL_STR_SLICE_LITERAL("light"),
        SOL_STR_SLICE_EMPTY,
    }
};

int
main(int argc, char *argv[])
{
    struct light_context context = { .resource = &light };
    struct sol_coap_server *server;
    char old_led_state;
    struct sol_network_link_addr servaddr = { .family = SOL_NETWORK_FAMILY_INET6,
                                              .port = DEFAULT_UDP_PORT };

    sol_init();

    server = sol_coap_server_new(&servaddr);
    if (!server) {
        SOL_WRN("Could not create a coap server using port %d.", DEFAULT_UDP_PORT);
        return -1;
    }

    if (sol_coap_server_register_resource(server, &light, NULL) < 0) {
        SOL_WRN("Could not register light resource");
        return -1;
    }

    console_fd = open("/dev/console", O_RDWR);
    if (console_fd < 0) {
        perror("Could not open '/dev/console'");
        return -1;
    }

    if (ioctl(console_fd, KDGETLED, (char *)&old_led_state)) {
        perror("Could not get the keyboard leds state");
        return -1;
    }

    context.server = server;
    sol_timeout_add(5000, update_light, &context);

    sol_run();

    sol_coap_server_unref(server);

    if (ioctl(console_fd, KDSETLED, old_led_state)) {
        perror("Could not return the leds to the old state");
        return -1;
    }

    return 0;
}
