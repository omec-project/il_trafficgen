#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <rte_mbuf.h>
#include <pcap.h>
#include "pktgen.h"

#ifndef PKTGEN_TOOL
#define PKTGEN_TOOL
extern uint64_t subs;
extern uint32_t start_ip, end_ip;
extern FILE *pktgen_fp;
extern uint64_t pkt_size;
extern bool is_first_tx;

extern uint8_t traffic_gen_as;
#define PKTGEN_TOOL_MAX_QUEUES 16
struct pid_rx_tx_cnt {
  uint16_t pid;
  uint64_t iprev_cnt;  // Total count of pkts on the Rx port(q0+q1+...)
  uint64_t oprev_cnt;  // Total count of pkts on the Tx port(q0+q1+...)
  uint64_t imax_pkt_sec;
  uint64_t omax_pkt_sec;
  uint64_t rx_qid_cnts[PKTGEN_TOOL_MAX_QUEUES];
  uint64_t tx_qid_cnts[PKTGEN_TOOL_MAX_QUEUES];
};
extern struct pid_rx_tx_cnt pid_rx_tx_cnts[10];
struct sgi_stats {
	uint64_t tx_cnt;
	uint64_t rx_cnt;
	uint64_t sgi_duration;
};
extern  struct sgi_stats sgi_stats_data, s1u_stats_data;

#define IL_TRAFFIC_GEN 		0
#define IL_TRAFFIC_RESP 	1
#define IL_TRAFFIC_RESP_AS_REF	2
#ifdef PCAP_GEN
extern pcap_dumper_t *pcap_dumper_p0, *pcap_dumper_p1;
#endif /* PCAP_GEN */

/* Struture to store eNB info */
typedef struct gtpu_params {
	uint32_t no_of_enb;
	uint32_t enb_start_ip;
	uint32_t enb_end_ip;
	uint32_t no_of_ue;
	uint32_t ue_start_ip;
	uint32_t ue_end_ip;
	uint32_t as_srvr_start_ip;
	uint32_t as_srvr_end_ip;
	uint32_t s1u_sgw_ip;
	uint32_t enb_teid;
	uint32_t s1u_teid;
} gtpu_params;
extern gtpu_params gtpu_info;

extern uint8_t enable_gtpu;

/* Generator and Responder host IP */
extern struct in_addr gen_host_ip, resp_host_ip;

/* Generator and Responder host port */
extern uint16_t gen_host_port, resp_host_port;

/* Generator and Responder host PCI */
extern char gen_host_pci[20], resp_host_pci[20];

int log_func(const char *fmt, ...);
#define FUNC_TRACE(...) { 					\
  if (traffic_gen_as == IL_TRAFFIC_GEN) {  	\
  	log_func(__VA_ARGS__); 					\
  }											\
}

void register_for_timeout(void);
void stop_pkt_frwding(int);
void get_tx_rx_counts(void);
int parse_test_duration_value(uint32_t *test_duration);
void remove_spaces_and_newline(char *str);
uint8_t store_packet_cnt(uint32_t network_ip, uint32_t pid,
                       uint16_t qid, uint8_t is_tx);
void set_ip_pkt_counters(uint64_t subs);
void delete_dynamic_pkt_cnt (void);
void pktgen_process_rx_as_tx(struct rte_mbuf *m, uint32_t pid);
void pktgen_inc_tx_count(struct rte_mbuf *m, uint16_t pid, uint16_t qid);
void get_ipkts_info(uint16_t pid, uint64_t *ipkts_per_sec,
               uint64_t *max_ipkts_per_sec, uint64_t *ibps_per_sec,
               uint64_t *total_ipkts, uint64_t *itotal_mb,
               uint64_t *tot_ipkts_per_sec,
               uint64_t *tot_max_ipkts_per_sec, uint64_t *tot_ibps_per_sec);
void get_opkts_info(uint16_t pid, uint64_t *opkts_per_sec,
              uint64_t *max_opkts_per_sec, uint64_t *obps_per_sec,
              uint64_t *total_opkts, uint64_t *ototal_mb,
              uint64_t *tot_opkts_per_sec,
              uint64_t *tot_max_opkts_per_sec, uint64_t *tot_obps_per_sec);
int parse_user_config(void);
void* send_stats_data(void *arg);
#ifdef PCAP_GEN
void dump_pcap(struct rte_mbuf *m, uint8_t pid);
pcap_dumper_t *init_pcap(char *pcap_filename);
#endif /* PCAP_GEN */
#endif /* PKTGEN_TOOL */
