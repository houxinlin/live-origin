//
// Created by hxl on 2025/9/1.
//

#ifndef IP_CONVERTER_H
#define IP_CONVERTER_H

typedef struct {
    double lat;
    double lng;
    char city[256];
    char country[256];
} lat_long;

typedef enum {
    API_IPINFO,
    API_IPWHO,
    API_COUNT
} api_type_t;

int init_mmdb(const char *mmdb_path);
void cleanup_mmdb(void);

lat_long* get_lat_long(char *ip);

lat_long* get_lat_long_from_api(char *ip, api_type_t api_type);

lat_long* get_lat_long_from_mmdb(const char *ip);

int parse_ipinfo_json(const char *json, double *lat, double *lon);
int parse_ipwho_json(const char *json, double *lat, double *lon);
int parse_ipapi_json(const char *json, double *lat, double *lon);
char *get_my_ip();
#endif //IP_CONVERTER_H