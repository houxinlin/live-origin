#include <getopt.h>
#include "http_parser.h"
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pcap.h>
#include "capture.h"


typedef struct {
    char *ip_address;
    char *current_header_field;
} parse_ip_address_t;

uint32_t snaplen = 65535, promisc = 1, to = 100;
uint8_t quiet = 0;
void (*ip_callback)(char *ip);
char *usedev = NULL;
uint16_t port = 8080;
char *proxy_header="aa";

char pc_err[PCAP_ERRBUF_SIZE];
uint8_t link_offset;
pcap_t *pd = NULL;
struct in_addr net, mask;


int run_capture(void (*callback)(char *ip), int argc, char **argv) {
    ip_callback = callback;
    int32_t c;

    signal(SIGINT, clean_exit);
    signal(SIGQUIT, clean_exit);
    signal(SIGPIPE, clean_exit);

    while ((c = getopt(argc, argv, "d:p:h:")) != EOF) {
        switch (c) {
            case 'd':
                usedev = optarg;
                break;
            case 'h': {
                proxy_header = optarg;
            }
            case 'p': {
                uint16_t value = atoi(optarg);
                if (value > 0)
                    port = value;
            }
            break;
            default:
                usage();
        }
    }

    if (setup_pcap_source())
        clean_exit(2);

    char filter_exp[64];
    snprintf(filter_exp, sizeof(filter_exp), "tcp port %u", port);
    struct bpf_program fp;

    // 编译 BPF
    if (pcap_compile(pd, &fp, filter_exp, 1, PCAP_NETMASK_UNKNOWN) == -1) {
        fprintf(stderr, "Couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(pd));
        return 1;
    }

    // 设置 BPF
    if (pcap_setfilter(pd, &fp) == -1) {
        fprintf(stderr, "Couldn't install filter %s: %s\n", filter_exp, pcap_geterr(pd));
        return 1;
    }

    while (pcap_loop(pd, -1, (pcap_handler) process, 0));
    clean_exit(0);
    return 0;
}

int setup_pcap_source(void) {
    char *dev = usedev ? usedev : pcap_lookupdev(pc_err);

    if (!dev) {
        perror(pc_err);
        return 1;
    }

    if ((pd = pcap_open_live(dev, snaplen, promisc, to, pc_err)) == NULL) {
        perror(pc_err);
        return 1;
    }

    if (pcap_lookupnet(dev, &net.s_addr, &mask.s_addr, pc_err) == -1) {
        perror(pc_err);
        memset(&net, 0, sizeof(net));
        memset(&mask, 0, sizeof(mask));
    }

    if (!quiet) {
        printf("interface: %s", dev);
        if (net.s_addr && mask.s_addr) {
            printf(" (%s/", inet_ntoa(net));
            printf("%s)", inet_ntoa(mask));
        }
        printf("\n");
    }

    switch (pcap_datalink(pd)) {
        case DLT_EN10MB:
            link_offset = ETHHDR_SIZE;
            break;
        case DLT_NULL:
            link_offset = LOOPHDR_SIZE;
            break;
        case DLT_RAW:
            link_offset = RAWHDR_SIZE;
            break;
        case DLT_LINUX_SLL:
            link_offset = ISDNHDR_SIZE;
            break;
        default:
            fprintf(stderr, "fatal: unsupported interface type %u\n", pcap_datalink(pd));
            return 1;
    }

    return 0;
}


void process(u_char *d, struct pcap_pkthdr *h, u_char *p) {
    struct ip *ip4_pkt = (struct ip *) (p + link_offset);
    uint32_t ip_hl = ip4_pkt->ip_hl * 4;
    uint8_t ip_proto = ip4_pkt->ip_p;
    char ip_src[INET_ADDRSTRLEN];
    char ip_dst[INET_ADDRSTRLEN];
    unsigned char *data;
    uint32_t len = h->caplen;

    inet_ntop(AF_INET, (const void *) &ip4_pkt->ip_src, ip_src, sizeof(ip_src));
    inet_ntop(AF_INET, (const void *) &ip4_pkt->ip_dst, ip_dst, sizeof(ip_dst));

    if (ip_proto == IPPROTO_TCP) {
        struct tcphdr *tcp_pkt = (struct tcphdr *) ((unsigned char *) (ip4_pkt) + ip_hl);
        uint16_t tcphdr_offset = tcp_pkt->th_off * 4;

        data = (unsigned char *) (tcp_pkt) + tcphdr_offset;
        len -= link_offset + ip_hl + tcphdr_offset;

        if ((int32_t) len < 0)
            len = 0;

        dump_packet(h, p, ip_proto, data, len,
                    ip_src, ip_dst, ntohs(tcp_pkt->th_sport), ntohs(tcp_pkt->th_dport));
    }
}

char* substring(const char *at, size_t length) {
    char *buf = malloc(length + 1);
    if (!buf) return NULL;
    memcpy(buf, at, length);
    buf[length] = '\0';
    return buf;
}

int on_url_cb(http_parser *parser, const char *at, size_t length) {
    return 0;
}

int on_header_field_cb(http_parser *parser, const char *at, size_t length) {
    parse_ip_address_t *context = (parse_ip_address_t *)parser->data;
    
    if (context->current_header_field) {
        free(context->current_header_field);
        context->current_header_field = NULL;
    }
    
    context->current_header_field = substring(at, length);
    return 0;
}

int on_header_value_cb(http_parser *parser, const char *at, size_t length) {
    parse_ip_address_t *context = (parse_ip_address_t *)parser->data;
    
    if (context->current_header_field &&
        strcasecmp(context->current_header_field, proxy_header) == 0) {
        
        if (context->ip_address) {
            free(context->ip_address);
        }
        
        context->ip_address = (char *)malloc(length + 1);
        if (context->ip_address) {
            memcpy(context->ip_address, at, length);
            context->ip_address[length] = '\0';
        }
    }
    
    return 0;
}

int on_headers_complete_cb(http_parser *parser) {
    return 0;
}

void print_current_time(void) {
    time_t now;
    struct tm *tm_info;
    char time_buffer[64];
    
    time(&now);
    tm_info = localtime(&now);
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    printf("[%s] ", time_buffer);
}

void dump_packet(struct pcap_pkthdr *h, u_char *p, uint8_t proto, unsigned char *data, uint32_t len,
                 const char *ip_src, const char *ip_dst, uint16_t sport, uint16_t dport) {
    if (len == 0 || dport != port)
        return;

    parse_ip_address_t *address = (parse_ip_address_t *) malloc(sizeof(parse_ip_address_t));
    address->ip_address = NULL;
    address->current_header_field = NULL;
    
    if (strcmp("127.0.0.1",ip_src) == 0) {
        http_parser parser;
        http_parser_settings settings;

        parser.data = address;
        http_parser_init(&parser, HTTP_REQUEST);
        http_parser_settings_init(&settings);
        settings.on_url = on_url_cb;
        settings.on_header_field = on_header_field_cb;
        settings.on_header_value = on_header_value_cb;
        settings.on_headers_complete = on_headers_complete_cb;
        size_t nparsed = http_parser_execute(&parser, &settings, (const char*)data, len);
    }

    fflush(stdout);
    if (address->ip_address) {
        printf("%s\n",address->ip_address);
        if (!quiet) {
            print_current_time();
            printf("%s:%u -> %s:%u\n", ip_src, sport, ip_dst, dport);
        }
        printf("从ip_proxy头获取到IP: %s\n", address->ip_address);
        ip_callback(address->ip_address);
    }
    
    if (address->current_header_field) {
        free(address->current_header_field);
    }
    if (address->ip_address) {
        free(address->ip_address);
    }
    free(address);
}


void usage(void) {
    printf("usage: capture [-d device] [-p port]\n"
           "   -d  is use specified device instead of the pcap default\n"
           "   -p  is set the port number to capture (default: 8080)\n");
    exit(2);
}


void clean_exit(int32_t sig) {
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    if (!quiet && sig >= 0)
        printf("exit\n");

    if (pd) pcap_close(pd);

    exit(0);
}
