//
// Created by hxl on 2025/9/2.
//
#include "ws/ws.h"
#include "context.h"

#ifndef WS_MANAGER_H
#define WS_MANAGER_H

typedef struct ws_client_manager {
    ws_cli_conn_t *clients;
    int count;
    int capacity;
} ws_client_manager_t;

void init_ws(application_context_t *application_context);

#endif //WS_MANAGER_H