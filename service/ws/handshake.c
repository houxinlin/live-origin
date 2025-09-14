#define _POSIX_C_SOURCE 200809L
#include <ctype.h>

#include "base64.h"
#include "sha1.h"
#include "ws.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "../html.h"
#include "../geojson.h"
int get_handshake_accept(char *wsKey, unsigned char **dest) {
    unsigned char hash[SHA1HashSize]; /* SHA-1 Hash.                   */
    SHA1Context ctx; /* SHA-1 Context.                */
    char *str; /* WebSocket key + magic string. */

    /* Invalid key. */
    if (!wsKey)
        return (-1);

    str = calloc(1, sizeof(char) * (WS_KEY_LEN + WS_MS_LEN + 1));
    if (!str)
        return (-1);

    strncpy(str, wsKey, WS_KEY_LEN);
    strcat(str, MAGIC_STRING);

    SHA1Reset(&ctx);
    SHA1Input(&ctx, (const uint8_t *) str, WS_KEYMS_LEN);
    SHA1Result(&ctx, hash);

    *dest = base64_encode(hash, SHA1HashSize, NULL);
    *(*dest + strlen((const char *) *dest) - 1) = '\0';
    free(str);
    return (0);
}


static char *strstricase(const char *haystack, const char *needle) {
    size_t length;
    for (length = strlen(needle); *haystack; haystack++)
        if (!strncasecmp(haystack, needle, length))
            return (char *) haystack;
    return (NULL);
}


int get_http_response(char **hsresponse, char *http_path) {
    if (strcmp(http_path, "/src/assets/countries.geojson")==0) {
        char *http_header = "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Connection: close\r\n"
                "\r\n";

        char *buffer = malloc(geojson_data_len + strlen(http_header));
        strcpy(buffer, http_header);
        strcat(buffer, geojson_data);
        *hsresponse = buffer;
        return 0;
    }

    char *http_header = "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=UTF-8\r\n"
            "Connection: close\r\n"
            "\r\n";

    char *buffer = malloc(html_data_len + strlen(http_header));
    strcpy(buffer, http_header);
    strcat(buffer, html_data);
    *hsresponse = buffer;
    return 0;
}

char *parse_http_path(const char *request_line) {
    if (!request_line) return NULL;
    const char *start = strchr(request_line, ' ');
    if (!start) return NULL;
    start++;
    const char *end = strchr(start, ' ');
    if (!end) return NULL;
    size_t len = end - start;
    char *path = (char *) malloc(len + 1);
    if (!path) return NULL;

    strncpy(path, start, len);
    path[len] = '\0';
    return path;
}

bool start_with(const char *str, const char *prefix) {
    if (!str || !prefix) return false;

    size_t len_prefix = strlen(prefix);
    size_t len_str = strlen(str);
    if (len_str < len_prefix) return false;

    return strncasecmp(str, prefix, len_prefix) == 0;
}

int get_handshake_response(char *hsrequest, char **hsresponse) {
    unsigned char *accept; /* Accept message.     */
    char *saveptr; /* strtok_r() pointer. */
    char *s; /* Current string.     */
    int ret; /* Return value.       */

    saveptr = NULL;
    char *http_path = NULL;
    bool is_websocket=false;
    for (s = strtok_r(hsrequest, "\r\n", &saveptr); s != NULL; s = strtok_r(NULL, "\r\n", &saveptr)) {
        if (start_with(s, "GET")) {
            http_path = parse_http_path(s);
        }
        if (strstricase(s, WS_HS_REQ) != NULL) {
            is_websocket=true;
            break;
        }
    }
    if (http_path != NULL && !is_websocket) {
        get_http_response(hsresponse, http_path);
        return (2);
    }
    saveptr = NULL;
    s = strtok_r(s, " ", &saveptr);
    s = strtok_r(NULL, " ", &saveptr);

    ret = get_handshake_accept(s, &accept);
    if (ret < 0)
        return (ret);

    *hsresponse = malloc(sizeof(char) * WS_HS_ACCLEN);
    if (*hsresponse == NULL)
        return (-1);

    strcpy(*hsresponse, WS_HS_ACCEPT);
    strcat(*hsresponse, (const char *) accept);
    strcat(*hsresponse, "\r\n\r\n");

    free(accept);
    return (0);
}
