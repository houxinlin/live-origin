
#include <sys/types.h>
#include <stdint.h>
#define VERSION "1.47.1-git"
#include <pcap.h>
#include "context.h"

#define ETHHDR_SIZE 14
#define TOKENRING_SIZE 22
#define PPPHDR_SIZE 4
#define SLIPHDR_SIZE 16
#define RAWHDR_SIZE 0
#define LOOPHDR_SIZE 4
#define FDDIHDR_SIZE 21
#define ISDNHDR_SIZE 16
#define IEEE80211HDR_SIZE 32
#define PFLOGHDR_SIZE 48
#define VLANHDR_SIZE 4
#define IPNETHDR_SIZE 24

#ifndef ETHERTYPE_IP
#define ETHERTYPE_IP      0x0800
#endif
#ifndef ETHERTYPE_IPV6
#define ETHERTYPE_IPV6      0x86dd
#endif

#define EXTRACT_16BITS(p) \
  ((uint16_t)((uint16_t)*((const uint8_t *)(p) + 0) << 8 | \
           (uint16_t)*((const uint8_t *)(p) + 1)))

#define _atoui32(p) \
  ((uint32_t)strtoul((p), (char **)NULL, 10))


#if USE_IPv6
#define BPF_FILTER_IP_TYPE  "(ip || ip6)"
#else
#define BPF_FILTER_IP_TYPE  "(ip)"
#endif

#define BPF_TEMPLATE_IP               BPF_FILTER_IP_TYPE
#define BPF_TEMPLATE_IP_VLAN          "(" BPF_FILTER_IP_TYPE " || (vlan && " BPF_FILTER_IP_TYPE "))"
#define BPF_TEMPLATE_USERSPEC_IP      "( %s) and " BPF_TEMPLATE_IP
#define BPF_TEMPLATE_USERSPEC_IP_VLAN "( %s) and " BPF_TEMPLATE_IP_VLAN

#define WORD_REGEX "((^%s\\W)|(\\W%s$)|(\\W%s\\W))"



#ifndef IP_OFFMASK
#define IP_OFFMASK 0x1fff
#endif

#ifndef TH_ECE
#define TH_ECE 0x40
#endif

#ifndef TH_CWR
#define TH_CWR 0x80
#endif


typedef enum {
    TCP = 'T', UDP = 'U', ICMP = 'I', ICMPv6 = 'I', IGMP = 'G', UNKNOWN = '?'
} netident_t;

int  run_capture(void (*func_ptr)(char *ip),application_context_t *application_context);

int setup_pcap_source(application_context_t *application_context);
void process(u_char *, struct pcap_pkthdr *, u_char *);

void usage(void);
void clean_exit(int32_t);

void dump_packet(struct pcap_pkthdr *, u_char *, uint8_t, unsigned char *, uint32_t,
                 const char *, const char *, uint16_t, uint16_t);

void print_current_time(void);
char* substring(const char *at, size_t length);



