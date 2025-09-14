//
// Created by hxl on 2025/9/2.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "ws/ws.h"
#include "ws_manager.h"

static ws_client_manager_t g_client_manager = {0};

void init_client_manager() {
    g_client_manager.capacity = 10;
    g_client_manager.count = 0;
    g_client_manager.clients = (ws_cli_conn_t*)malloc(sizeof(ws_cli_conn_t) * g_client_manager.capacity);
    if (!g_client_manager.clients) {
        printf("Failed to allocate memory for client manager\n");
        exit(1);
    }
}

void expand_client_array() {
    g_client_manager.capacity *= 2;
    ws_cli_conn_t *new_clients = (ws_cli_conn_t*)realloc(g_client_manager.clients,
                                                         sizeof(ws_cli_conn_t) * g_client_manager.capacity);
    if (!new_clients) {
        printf("Failed to expand client array\n");
        return;
    }
    g_client_manager.clients = new_clients;
}

void add_client(ws_cli_conn_t client) {
    for (int i = 0; i < g_client_manager.count; i++) {
        if (g_client_manager.clients[i] == client) {
            return;
        }
    }

    if (g_client_manager.count >= g_client_manager.capacity) {
        expand_client_array();
    }
    g_client_manager.clients[g_client_manager.count] = client;
    g_client_manager.count++;
    printf("Client added. Total clients: %d\n", g_client_manager.count);
}

void remove_client(ws_cli_conn_t client) {
    for (int i = 0; i < g_client_manager.count; i++) {
        if (g_client_manager.clients[i] == client) {
            for (int j = i; j < g_client_manager.count - 1; j++) {
                g_client_manager.clients[j] = g_client_manager.clients[j + 1];
            }
            g_client_manager.count--;
            printf("Client removed. Total clients: %d\n", g_client_manager.count);
            return;
        }
    }
}


void cleanup_client_manager() {
    if (g_client_manager.clients) {
        free(g_client_manager.clients);
        g_client_manager.clients = NULL;
        g_client_manager.count = 0;
        g_client_manager.capacity = 0;
    }
}

void broadcast_message(const char *message) {
    for (int i = 0; i < g_client_manager.count; i++) {
        ws_sendframe_txt(g_client_manager.clients[i], message);
    }
    printf("Broadcasted message to %d clients: %s\n", g_client_manager.count, message);
}

int get_client_count() {
    return g_client_manager.count;
}

void onopen(ws_cli_conn_t client)
{
    char *cli;
    cli = ws_getaddress(client);
    printf("Connection opened, addr: %s\n", cli);

    add_client(client);
}

void onclose(ws_cli_conn_t client)
{
    char *cli;
    cli = ws_getaddress(client);
    printf("Connection closed, addr: %s\n", cli);
    remove_client(client);
}
void init_ws(application_context_t *application_context) {
    init_client_manager();
    application_context->client_manager = &g_client_manager;
    ws_socket(&(struct ws_server){
     .host = "0.0.0.0",
     .port = 8088,
     .thread_loop   = 0,
     .timeout_ms    = 1000,
     .evs.onopen    = &onopen,
     .evs.onclose   = &onclose,
 });
}
