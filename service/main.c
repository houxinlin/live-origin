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
    lat_long *lat_long = get_lat_long(ip);
    if (lat_long == NULL) return;
    broadcast(application_context, *lat_long);
}

int main(int argc, char **argv) {
    signal(SIGINT, cleanup_and_exit);
    signal(SIGTERM, cleanup_and_exit);

    application_context = (application_context_t *) malloc(sizeof(application_context_t));
    memset(application_context, 0, sizeof(application_context_t));

    if (!init_mmdb("./GeoLite2-City.mmdb")) {
        printf("MMDB 初始化失败，程序退出\n");
        free(application_context);
        return 1;
    }

    init_ws(application_context);
    lat_long * lat_long = get_lat_long(get_my_ip());
    if (lat_long==NULL) {
        printf("%s\n","无法获取当前主机位置");
    }
    printf("My lat=%f lng=%f\n",lat_long->lat,lat_long->lng);
    fflush(stdout);
    application_context->my_lat_long=lat_long;
    run_capture(send_to_web, argc, argv);
    cleanup_mmdb();
    free(application_context);
    return 0;
}
