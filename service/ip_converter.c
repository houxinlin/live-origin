//
// Created by hxl on 2025/9/1.
//
#include "ip_converter.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <curl/curl.h>
#include <cJSON.h>
#include <maxminddb.h>

static MMDB_s g_mmdb;
static int g_mmdb_initialized = 0;


const char *ip_urls[] = {
    "https://ipinfo.io/%s/json",
    "http://ipwho.is/%s",
    "http://ip-api.com/json/%s"
};

struct HTTPResponse {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, struct HTTPResponse *response) {
    size_t realsize = size * nmemb;
    char *ptr = realloc(response->memory, response->size + realsize + 1);
    if (!ptr) {
        printf("内存不足!\n");
        return 0;
    }
    response->memory = ptr;
    memcpy(&(response->memory[response->size]), contents, realsize);
    response->size += realsize;
    response->memory[response->size] = 0;
    
    return realsize;
}

int parse_ipinfo_json(const char *json, double *lat, double *lon) {
    cJSON *json_obj = cJSON_Parse(json);
    if (json_obj == NULL) {
        return 0;
    }
    cJSON *loc = cJSON_GetObjectItemCaseSensitive(json_obj, "loc");
    int success = 0;
    
    if (cJSON_IsString(loc) && loc->valuestring != NULL) {
        char *loc_str = strdup(loc->valuestring);
        char *comma = strchr(loc_str, ',');
        if (comma != NULL) {
            *comma = '\0';
            *lat = atof(loc_str);
            *lon = atof(comma + 1);
            success = 1;
        }
        free(loc_str);
    }

    cJSON_Delete(json_obj);
    return success;
}

int parse_ipwho_json(const char *json, double *lat, double *lon) {
    cJSON *json_obj = cJSON_Parse(json);
    if (json_obj == NULL) {
        printf("JSON解析失败 (ipwho.is)\n");
        return 0;
    }
    
    // ipwho.is的响应格式: {"latitude": 39.9042, "longitude": 116.4074, ...}
    cJSON *latitude = cJSON_GetObjectItemCaseSensitive(json_obj, "latitude");
    cJSON *longitude = cJSON_GetObjectItemCaseSensitive(json_obj, "longitude");
    
    int success = 0;
    if (cJSON_IsNumber(latitude) && cJSON_IsNumber(longitude)) {
        *lat = latitude->valuedouble;
        *lon = longitude->valuedouble;
        success = 1;
    }
    
    cJSON_Delete(json_obj);
    return success;
}

int parse_ipapi_json(const char *json, double *lat, double *lon) {
    cJSON *json_obj = cJSON_Parse(json);
    if (json_obj == NULL) {
        printf("JSON解析失败 (ip-api.com)\n");
        return 0;
    }
    
    cJSON *status = cJSON_GetObjectItemCaseSensitive(json_obj, "status");
    if (!cJSON_IsString(status) || strcmp(status->valuestring, "success") != 0) {
        printf("ip-api.com 查询失败或状态不是success\n");
        cJSON_Delete(json_obj);
        return 0;
    }
    
    cJSON *latitude = cJSON_GetObjectItemCaseSensitive(json_obj, "lat");
    cJSON *longitude = cJSON_GetObjectItemCaseSensitive(json_obj, "lon");
    
    int success = 0;
    if (cJSON_IsNumber(latitude) && cJSON_IsNumber(longitude)) {
        *lat = latitude->valuedouble;
        *lon = longitude->valuedouble;
        success = 1;
        printf("从ip-api.com获取到经纬度: lat=%.6f, lon=%.6f\n", *lat, *lon);
    }
    
    cJSON_Delete(json_obj);
    return success;
}

typedef int (*parse_func_t)(const char *json, double *lat, double *lon);
parse_func_t parse_functions[] = {
    parse_ipinfo_json,
    parse_ipwho_json,
    parse_ipapi_json
};

int init_mmdb(const char *mmdb_path) {
    if (g_mmdb_initialized) {
        printf("MMDB 已经初始化\n");
        return 1;
    }
    
    int status = MMDB_open(mmdb_path, MMDB_MODE_MMAP, &g_mmdb);
    if (status != MMDB_SUCCESS) {
        printf("无法打开MaxMind数据库文件: %s - %s\n", mmdb_path, MMDB_strerror(status));
        return 0;
    }
    
    g_mmdb_initialized = 1;
    printf("MMDB 初始化成功: %s\n", mmdb_path);
    return 1;
}

void cleanup_mmdb(void) {
    if (g_mmdb_initialized) {
        MMDB_close(&g_mmdb);
        g_mmdb_initialized = 0;
        printf("MMDB 已清理\n");
    }
}

lat_long* get_lat_long_from_api(char *ip, api_type_t api_type) {
    CURL *curl;
    CURLcode res;
    struct HTTPResponse response;
    char url[256];
    
    response.memory = malloc(1);
    response.size = 0;
    snprintf(url, sizeof(url), ip_urls[api_type], ip);
    printf("ip服务提供URL=: %s\n",url);
    curl = curl_easy_init();
    if (!curl) {
        free(response.memory);
        return NULL;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    lat_long *pos = malloc(sizeof(lat_long));
    if (!pos) {
        free(response.memory);
        return NULL;
    }
    
    pos->lat = 0.0;
    pos->lng = 0.0;
    memset(pos->city, 0, sizeof(pos->city));
    memset(pos->country, 0, sizeof(pos->country));
    
    if (res == CURLE_OK) {
        double lat = 0.0, lon = 0.0;
        if (parse_functions[api_type](response.memory, &lat, &lon)) {
            pos->lat = lat;
            pos->lng = lon;
            strcpy(pos->city, "Unknown");
            strcpy(pos->country, "Unknown");
        }
    }
    free(response.memory);
    return pos;
}

lat_long* get_lat_long_from_mmdb(const char *ip) {
    if (!g_mmdb_initialized) {
        printf("MMDB 未初始化，请先调用 init_mmdb()\n");
        return NULL;
    }
    
    int gai_error, mmdb_error;
    MMDB_lookup_result_s result = MMDB_lookup_string(&g_mmdb, ip, &gai_error, &mmdb_error);
    
    if (gai_error != 0) {
        printf("getaddrinfo错误: %s\n", gai_strerror(gai_error));
        return NULL;
    }
    
    if (mmdb_error != MMDB_SUCCESS) {
        printf("libmaxminddb错误: %s\n", MMDB_strerror(mmdb_error));
        return NULL;
    }
    
    if (!result.found_entry) {
        printf("在数据库中未找到IP %s 的记录\n", ip);
        return NULL;
    }
    
    lat_long *pos = malloc(sizeof(lat_long));
    if (!pos) {
        printf("内存分配失败\n");
        return NULL;
    }
    
    pos->lat = 0.0;
    pos->lng = 0.0;
    memset(pos->city, 0, sizeof(pos->city));
    memset(pos->country, 0, sizeof(pos->country));
    
    MMDB_entry_data_s entry_data;
    const char *latitude_path[] = {"location", "latitude", NULL};
    int exit_code = MMDB_aget_value(&result.entry, &entry_data, latitude_path);
    
    if (exit_code == MMDB_SUCCESS && entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_DOUBLE) {
        pos->lat = entry_data.double_value;
    }
    
    const char *longitude_path[] = {"location", "longitude", NULL};
    exit_code = MMDB_aget_value(&result.entry, &entry_data, longitude_path);
    if (exit_code == MMDB_SUCCESS && entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_DOUBLE) {
        pos->lng = entry_data.double_value;
    }
    
    const char *city_zh_path[] = {"city", "names", "zh-CN", NULL};
    exit_code = MMDB_aget_value(&result.entry, &entry_data, city_zh_path);
    if (exit_code == MMDB_SUCCESS && entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_UTF8_STRING) {
        size_t city_len = entry_data.data_size < sizeof(pos->city) - 1 ? entry_data.data_size : sizeof(pos->city) - 1;
        memcpy(pos->city, entry_data.utf8_string, city_len);
        pos->city[city_len] = '\0';
    } else {
        const char *city_en_path[] = {"city", "names", "en", NULL};
        exit_code = MMDB_aget_value(&result.entry, &entry_data, city_en_path);
        if (exit_code == MMDB_SUCCESS && entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_UTF8_STRING) {
            size_t city_len = entry_data.data_size < sizeof(pos->city) - 1 ? entry_data.data_size : sizeof(pos->city) - 1;
            memcpy(pos->city, entry_data.utf8_string, city_len);
            pos->city[city_len] = '\0';
        }
    }
    
    const char *country_zh_path[] = {"country", "names", "zh-CN", NULL};
    exit_code = MMDB_aget_value(&result.entry, &entry_data, country_zh_path);
    if (exit_code == MMDB_SUCCESS && entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_UTF8_STRING) {
        size_t country_len = entry_data.data_size < sizeof(pos->country) - 1 ? entry_data.data_size : sizeof(pos->country) - 1;
        memcpy(pos->country, entry_data.utf8_string, country_len);
        pos->country[country_len] = '\0';
    } else {
        const char *country_en_path[] = {"country", "names", "en", NULL};
        exit_code = MMDB_aget_value(&result.entry, &entry_data, country_en_path);
        if (exit_code == MMDB_SUCCESS && entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_UTF8_STRING) {
            size_t country_len = entry_data.data_size < sizeof(pos->country) - 1 ? entry_data.data_size : sizeof(pos->country) - 1;
            memcpy(pos->country, entry_data.utf8_string, country_len);
            pos->country[country_len] = '\0';
        }
    }
    if (pos->lat == 0.0 && pos->lng == 0.0) {
        printf("未能从数据库中获取到有效的经纬度信息\n");
        free(pos);
        return NULL;
    }
    printf("从本地数据库获取到IP %s 的位置: 纬度=%.6f, 经度=%.6f, 城市=%s, 国家=%s\n", 
           ip, pos->lat, pos->lng, pos->city, pos->country);
    return pos;
}

lat_long* get_lat_long(char *ip) {
    lat_long *result = NULL;
    printf("开始获取IP %s 的地理位置信息...\n", ip);
    srand(time(NULL));
    int r = rand() % API_COUNT;
    api_type_t api = (api_type_t)r;
    result =  get_lat_long_from_api(ip,r);
    return result;
}


char * get_my_ip() {
    CURL *curl;
    CURLcode res;

    struct HTTPResponse http_response;
    http_response.memory = malloc(1);
    http_response.size = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://ipinfo.io/json");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&http_response);

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

        res = curl_easy_perform(curl);

        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            cJSON *json = cJSON_Parse(http_response.memory);
            if(json) {
                cJSON *ip_item = cJSON_GetObjectItem(json, "ip");
                if(cJSON_IsString(ip_item) && (ip_item->valuestring != NULL)) {
                   return ip_item->valuestring;
                }
                cJSON_Delete(json);
            }
        }
        curl_easy_cleanup(curl);
    }
    free(http_response.memory);
    curl_global_cleanup();
    return NULL;
}
