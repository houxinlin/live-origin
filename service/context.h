//
// Created by hxl on 2025/9/2.
//

#ifndef CONTEXT_H
#define CONTEXT_H
#include <stdint.h>

#include "ip_converter.h"

typedef struct ws_client_manager ws_client_manager_t;

typedef struct {
    lat_long *my_lat_long;
    ws_client_manager_t *client_manager;
    char *use_dev;
    char *proxy_header;
    uint16_t port ;
    char *use_model;
} application_context_t;

#endif //CONTEXT_H

void broadcast(application_context_t *application_context,lat_long lat_long);