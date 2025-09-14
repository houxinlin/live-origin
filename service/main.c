#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "capture.h"
#include "ws_manager.h"
#include "context.h"
#include "ip_converter.h"

application_context_t *application_context;

void cleanup_and_exit(int sig) {
    cleanup_mmdb();
    if (application_context) {
        free(application_context);
    }
    exit(0);
}

void send_to_web(char *ip) {
    if (strcmp(ip, "127.0.0.1") == 0)return;
    lat_long *lat_long = strcmp(application_context->use_model, "net") == 0
                             ? get_lat_long(ip)
                             : get_lat_long_from_mmdb(ip);
    if (lat_long == NULL) return;
    broadcast(application_context, *lat_long);
}

int init_parameter(application_context_t *application_context, int argc, char **argv) {
    int32_t c;
    application_context->use_dev = "lo";
    application_context->port = 8080;
    application_context->use_model = "net";
    application_context->proxy_header="ip_proxy";
    while ((c = getopt(argc, argv, "d:p:h:m:")) != EOF) {
        switch (c) {
            case 'd':
                application_context->use_dev = optarg;
                break;
            case 'h': {
                application_context->proxy_header = optarg;
                break;
            }
            case 'p': {
                uint16_t value = atoi(optarg);
                if (value > 0) {
                    application_context->port = value;
                }
                break;
            }
            case 'm':
                application_context->use_model = optarg;
                break;
            default:
                usage();
        }
    }
    if (strcmp(application_context->use_model, "db") == 0) {
        if (!init_mmdb("./GeoLite2-City.mmdb")) {
            printf("MMDB 初始化失败，程序退出\n");
            free(application_context);
            return -1;
        }
    }
    printf("dev=%s,port=%d,model=%s\n", application_context->use_dev, application_context->port,application_context->use_model);
    return 0;
}

int init_my_location() {
    lat_long *lat_long = get_lat_long(get_my_ip());
    if (lat_long == NULL) {
        printf("%s\n", "无法获取当前主机位置");
        return -1;
    }
    application_context->my_lat_long = lat_long;
    printf("My lat=%f lng=%f\n", lat_long->lat, lat_long->lng);
    fflush(stdout);
    return 0;
}

int main(int argc, char **argv) {
    signal(SIGINT, cleanup_and_exit);
    signal(SIGTERM, cleanup_and_exit);

    application_context = (application_context_t *) malloc(sizeof(application_context_t));
    memset(application_context, 0, sizeof(application_context_t));

    if (init_parameter(application_context, argc, argv) < 0) {
        return -1;
    }
    if (init_my_location() < 0) {
        return -1;
    }
    init_ws(application_context);
    run_capture(send_to_web, application_context);
    return 0;
}
