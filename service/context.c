//
// Created by hxl on 2025/9/2.
//

#include "context.h"
#include "lib/cJSON/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include "ws_manager.h"
void broadcast(application_context_t *application_context, lat_long lat_long) {
    if (!application_context || !application_context->client_manager) {
        printf("应用上下文或客户端管理器为空\n");
        return;
    }

    cJSON *json = cJSON_CreateObject();
    if (!json) {
        printf("创建JSON对象失败\n");
        return;
    }

    // 创建 from 对象
    cJSON *from_obj = cJSON_CreateObject();
    if (!from_obj) {
        printf("创建from对象失败\n");
        cJSON_Delete(json);
        return;
    }

    cJSON *from_lat = cJSON_CreateNumber(lat_long.lat);
    cJSON *from_lng = cJSON_CreateNumber(lat_long.lng);
    cJSON *from_city = cJSON_CreateString(lat_long.city);
    cJSON *from_country = cJSON_CreateString(lat_long.country);

    if (!from_lat || !from_lng || !from_city || !from_country) {
        printf("创建from对象属性失败\n");
        cJSON_Delete(json);
        cJSON_Delete(from_obj);
        if (from_lat) cJSON_Delete(from_lat);
        if (from_lng) cJSON_Delete(from_lng);
        if (from_city) cJSON_Delete(from_city);
        if (from_country) cJSON_Delete(from_country);
        return;
    }

    cJSON_AddItemToObject(from_obj, "lat", from_lat);
    cJSON_AddItemToObject(from_obj, "lng", from_lng);
    cJSON_AddItemToObject(from_obj, "city", from_city);
    cJSON_AddItemToObject(from_obj, "country", from_country);

    cJSON *to_obj = cJSON_CreateObject();
    if (!to_obj) {
        printf("创建to对象失败\n");
        cJSON_Delete(json);
        cJSON_Delete(from_obj);
        return;
    }

    cJSON *to_lat = cJSON_CreateNumber(application_context->my_lat_long->lat);
    cJSON *to_lng = cJSON_CreateNumber(application_context->my_lat_long->lng);
    cJSON *to_city = cJSON_CreateString("");
    cJSON *to_country = cJSON_CreateString("");

    if (!to_lat || !to_lng || !to_city || !to_country) {
        printf("创建to对象属性失败\n");
        cJSON_Delete(json);
        cJSON_Delete(from_obj);
        cJSON_Delete(to_obj);
        if (to_lat) cJSON_Delete(to_lat);
        if (to_lng) cJSON_Delete(to_lng);
        if (to_city) cJSON_Delete(to_city);
        if (to_country) cJSON_Delete(to_country);
        return;
    }

    cJSON_AddItemToObject(to_obj, "lat", to_lat);
    cJSON_AddItemToObject(to_obj, "lng", to_lng);
    cJSON_AddItemToObject(to_obj, "city", to_city);
    cJSON_AddItemToObject(to_obj, "country", to_country);

    // 将 from 和 to 添加到主 JSON 对象
    cJSON_AddItemToObject(json, "from", from_obj);
    cJSON_AddItemToObject(json, "to", to_obj);

    char *json_string = cJSON_Print(json);
    if (!json_string) {
        printf("JSON序列化失败\n");
        cJSON_Delete(json);
        return;
    }

    int client_count = application_context->client_manager->count;
    ws_cli_conn_t *clients =application_context->client_manager->clients;
    printf("准备广播位置信息给 %d 个客户端: %s\n", client_count, json_string);
    for (int i = 0; i < client_count; i++) {
        int result = ws_sendframe_txt(clients[i], json_string);
        if (result < 0) {
            printf("向客户端 %d 发送消息失败\n", i);
        } else {
            printf("成功向客户端 %d 发送位置信息\n", i);
        }
    }

    free(json_string);
    cJSON_Delete(json);
}