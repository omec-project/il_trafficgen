#include "pktgen-tool.h"
#include "cli-functions.h"
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "pktgen-cmds.h"
#include "pktgen-main.h"
#include "lpktgenlib.h"
#include "pktgen-display.h"
#include "pktgen-random.h"
#include "pktgen-log.h"
#include "pg_ether.h"
#include "pktgen-stats.h"
#include "pktgen.h"
#include "pktgen-interface.h"
#include<pthread.h>
#include <libconfig.h>

/* is_first_tx flag is used to determine the start time for test_duration run.
 * As soon as the first pkt is transmitted, test_duration counter is started */
bool is_first_tx = true;
uint8_t traffic_gen_as = IL_TRAFFIC_GEN;
struct pid_rx_tx_cnt pid_rx_tx_cnts[10];
struct sgi_stats sgi_stats_data = {0}, s1u_stats_data = {0};
gtpu_params gtpu_info;
#ifdef PCAP_GEN
pcap_dumper_t *pcap_dumper_p0 = NULL, *pcap_dumper_p1 = NULL;
#endif /* PCAP_GEN */

/* FILE poiner to log data to pktgen-debug file */
FILE *pktgen_fp = NULL;

/* no. of subscribers is computed based on the user values in user_input.cfg */
uint64_t subs = 0;

/* start ip in binary form. It is used to store per flow based counts */
uint32_t start_ip = 0, end_ip = 0;

/*pkt_size. User specified value - FCS */
uint64_t pkt_size = 0;

/* per UE packet cnts maintained at TX-P0, RX-P0 */
uint64_t **per_flow_tx_cnt, **per_flow_rx_cnt;

/* test duration value will be read from user_input.cfg */
uint32_t test_duration = 0, total_duration = 0;

/* enable_gtpu value will be read from user_input.cfg */
uint8_t enable_gtpu = 0;

/* Generator and Responder host IP */
struct in_addr gen_host_ip, resp_host_ip;

/* Generator and Responder host port */
uint16_t gen_host_port = 0, resp_host_port = 0;

/* Generator and Responder host PCI port */
char gen_host_pci[20] = {'\0'}, resp_host_pci[20] = {'\0'};


/* Function: get_ipkts_info 
 * Input and output param: set of values that are used to display stats
 * Return values: void
 * Brief: This function is used to set ipkts_per_sec, max_ipkts_per_sec,
          ibps_per_sec, total_ipkts, itotal_mb, tot_ipkts_per_sec
          tot_max_ipkts_per_sec and tot_ibps_per_sec for P0-Rx and P1-Rx
**/
inline void get_ipkts_info(uint16_t pid, uint64_t *ipkts_per_sec,
       uint64_t *max_ipkts_per_sec, uint64_t *ibps_per_sec,
       uint64_t *total_ipkts, uint64_t *itotal_mb, uint64_t *tot_ipkts_per_sec,
       uint64_t *tot_max_ipkts_per_sec, uint64_t *tot_ibps_per_sec) {
  uint16_t rx_cnt = pktgen.l2p->ports[pid].rx_qid, i = 0;
  uint64_t curr_cnt = 0;
  for (; i < rx_cnt; ++i) {
    curr_cnt += pid_rx_tx_cnts[pid].rx_qid_cnts[i];
  }
  *ipkts_per_sec = curr_cnt - pid_rx_tx_cnts[pid].iprev_cnt;
  *max_ipkts_per_sec = pid_rx_tx_cnts[pid].imax_pkt_sec;
  if (pid_rx_tx_cnts[pid].imax_pkt_sec < *ipkts_per_sec) {
    *max_ipkts_per_sec = *ipkts_per_sec;
    pid_rx_tx_cnts[pid].imax_pkt_sec = *ipkts_per_sec;
  }
  *ibps_per_sec = ((*ipkts_per_sec)*(INTER_FRAME_GAP + PKT_PREAMBLE_SIZE +
                                     FCS_SIZE + pkt_size)*8)/Million;
  *total_ipkts = curr_cnt;
  *itotal_mb = ((*total_ipkts)*(INTER_FRAME_GAP + PKT_PREAMBLE_SIZE +
                                FCS_SIZE + pkt_size)*8)/Million;
  pid_rx_tx_cnts[pid].iprev_cnt = curr_cnt;

  /* Total port counts */
  *tot_ipkts_per_sec += *ipkts_per_sec;
  *tot_max_ipkts_per_sec += *max_ipkts_per_sec;
  *tot_ibps_per_sec += *ibps_per_sec;
}

/* Function: get_opkts_info 
 * Input and output param: set of values that are used to display stats
 * Return values: void
 * Brief: This function is used to set opkts_per_sec, max_opkts_per_sec,
          obps_per_sec, total_opkts, ototal_mb, tot_opkts_per_sec
          tot_max_opkts_per_sec and tot_obps_per_sec for P0-Tx and P1-Tx
 */
inline void get_opkts_info(uint16_t pid, uint64_t *opkts_per_sec,
       uint64_t *max_opkts_per_sec, uint64_t *obps_per_sec,
       uint64_t *total_opkts, uint64_t *ototal_mb, uint64_t *tot_opkts_per_sec,
       uint64_t *tot_max_opkts_per_sec, uint64_t *tot_obps_per_sec) {
  uint16_t tx_cnt = pktgen.l2p->ports[pid].tx_qid, i = 0;
  uint64_t curr_cnt = 0;
  for (; i < tx_cnt; ++i) {
    curr_cnt += pid_rx_tx_cnts[pid].tx_qid_cnts[i];
  }
  *opkts_per_sec = curr_cnt - pid_rx_tx_cnts[pid].oprev_cnt;
  *max_opkts_per_sec = pid_rx_tx_cnts[pid].omax_pkt_sec;
  if (pid_rx_tx_cnts[pid].omax_pkt_sec < *opkts_per_sec) {
    *max_opkts_per_sec = *opkts_per_sec;
    pid_rx_tx_cnts[pid].omax_pkt_sec = *opkts_per_sec;
  }
  *obps_per_sec = ((*opkts_per_sec)*(INTER_FRAME_GAP + PKT_PREAMBLE_SIZE +
                                     FCS_SIZE + pkt_size)*8)/Million;
  *total_opkts = curr_cnt;
  *ototal_mb = ((*total_opkts)*(INTER_FRAME_GAP + PKT_PREAMBLE_SIZE +
                                FCS_SIZE + pkt_size)*8)/Million;
  pid_rx_tx_cnts[pid].oprev_cnt = curr_cnt;

  /* Total port counts */
  *tot_opkts_per_sec += *opkts_per_sec;
  *tot_max_opkts_per_sec += *max_opkts_per_sec;
  *tot_obps_per_sec += *obps_per_sec;
}

/* Function: log_func
 * Input param: variable number of format specifiers
 * Output param: None
 * Return values: int
 * Brief: This function is log data for pktgen-debug file
**/
int log_func(const char *fmt, ...) {
  if (pktgen_fp == NULL) {
    const char *dir = "autotest/log";
    struct stat sb;
    if (stat(dir, &sb) != 0 && !S_ISDIR(sb.st_mode)) {
      mkdir(dir, 755);
    }
    char date_time[100];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(date_time, sizeof(date_time)-1, "%d_%m_%Y_%H_%M_%S", t);
    char pktgen_debug_file[256] = {0};
    if (traffic_gen_as == IL_TRAFFIC_GEN) {
      sprintf(pktgen_debug_file, "autotest/log/il_trafficgen-generator_%s.log",
            date_time);
    } else {
      sprintf(pktgen_debug_file, "autotest/log/il_trafficgen-responder_%s.log",
            date_time);
    }
    pktgen_fp = fopen (pktgen_debug_file, "w");
    if (pktgen_fp == NULL) {
      printf("Logging couldn't be started\n");
      return 1;
    }
  }
  char curr_time[50];
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  strftime(curr_time, sizeof(curr_time)-1, "%H:%M:%S", t);
  fprintf (pktgen_fp, "%s: ", curr_time);
  va_list ap;
  va_start(ap, fmt);
  int ret = vfprintf(pktgen_fp, fmt, ap);
  va_end(ap);
  fprintf (pktgen_fp, "\n");
  return ret;
}

/* Function: store_packet_cnt
 * Input param: None
 * Output param: None
 * Return values: void
 * Brief: This function stores packet count for each of the corresponding flows
**/
inline uint8_t store_packet_cnt(uint32_t network_ip, uint32_t pid,
                              uint16_t qid, uint8_t is_tx) {
  if (network_ip == 0) {
    /* Broadcast pkt recvd. */
    return 0;
  }
  if ((network_ip > end_ip) || (network_ip < start_ip)) {
    /* Invalid IP recvd. */
    return 0;
  }
  if (is_tx == 0) {
     /* Increment RX pkt cnt for the RX port: pid on queue:qid */
     ++(pid_rx_tx_cnts[pid].rx_qid_cnts[qid]);
     per_flow_rx_cnt[qid][network_ip-start_ip]++;
  } else {
     /* Increment TX pkt cnt for the TX port: pid on queue:qid */
     ++(pid_rx_tx_cnts[pid].tx_qid_cnts[qid]);
     per_flow_tx_cnt[qid][network_ip-start_ip]++;
  }
  return 1;
}

/* Function: remove_spaces_and_newline
 * Input param: char pointer
 * Output param: None
 * Return values: None
 * Brief: This function is used to remove spaces and new lines from
 *        the input string(str)
**/
void remove_spaces_and_newline(char *str)
{
  int count = 0, i = 0;
  for (; str[i]; i++) {
    if (str[i] != ' ' && str[i] != '\n') {
      str[count++] = str[i];
    }
  }
  str[count] = '\0';
}

/* Function: parse_user_config
 * Input param: None
 * Output param: None
 * Return values: bool
 *        0: Successfully parsed user inputs
 *        false: Error in parsing user input
 * Brief: This function reads all user confgured input parameters from
 *        "user_input.cfg" located in autotest directory
**/
int parse_user_config(void) {
  config_t cfg;
  int val = 0;
  const char *val_str = NULL;
  config_init(&cfg);
  char cfg_file[] = "./autotest/user_input.cfg";
  if (!config_read_file(&cfg, cfg_file)) {
    fprintf(stderr,"\n%s:%d - %s", config_error_file(&cfg),
            config_error_line(&cfg), config_error_text(&cfg));
    config_destroy(&cfg);
    return -1;
  }
  
  if (config_lookup_string(&cfg, "gen_host_ip", &val_str)) {
    inet_aton(val_str, &gen_host_ip);
  } else {
    printf("No 'gen_host_ip' argument setting in config. file.\n");
    return -1;
  }
  if (config_lookup_int(&cfg, "gen_host_port", &val)) {
	gen_host_port = val;
  } else {
    printf("No 'gen_host_port' argument setting in config. file.\n");
    return -1;
  }
  if (config_lookup_string(&cfg, "resp_host_ip", &val_str)) {
    inet_aton(val_str, &resp_host_ip);
  } else {
    printf("No 'resp_host_ip' argument setting in config. file.\n");
    return -1;
  }
  if (config_lookup_int(&cfg, "resp_host_port", &val)) {
	resp_host_port = val;
  } else {
    printf("No 'resp_host_port' argument setting in config. file.\n");
    return -1;
  }
  if (config_lookup_string(&cfg, "s1u_port", &val_str)) {
    strcpy(gen_host_pci, val_str);
  } else {
    printf("No 's1u_port' argument setting in config. file.\n");
    return -1;
  }
  if (config_lookup_string(&cfg, "sgi_port", &val_str)) {
    strcpy(resp_host_pci, val_str);
  } else {
    printf("No 'sgi_port' argument setting in config. file.\n");
    return -1;
  }
  if (config_lookup_int(&cfg, "test_duration", &val)) {
    test_duration = val;
    if (test_duration == 0) {
      printf("Test Duration is set to infinity\n");
    } else {
      printf("Test Duration is %u\n", test_duration);
    }
  } else {
    printf("No 'Test Duration' argument setting in config. file.\n");
    return -1;
  }

  if (config_lookup_int(&cfg, "enable_gtpu", &val)) {
    enable_gtpu = val;
    if (enable_gtpu == 1) {
      printf("GTP-U packets handling is enabled\n");
    }
  } else {
    printf("No 'enable_gtpu' argument setting in config. file.\n");
    return -1;
  }
  if (config_lookup_int(&cfg, "no_of_enb", &val)) {
    gtpu_info.no_of_enb = val;
  } else if (enable_gtpu == 1) {
    printf("No 'no_of_enb' argument setting in config. file.\n");
    return -1;
  }
  if (config_lookup_string(&cfg, "enb_ip_range", &val_str)) {
    char *str = strdup(val_str);
    inet_aton(strtok(str, " to "), (struct in_addr *)&(gtpu_info.enb_start_ip));
    inet_aton(strtok(NULL, " to "), (struct in_addr *)&(gtpu_info.enb_end_ip));
    /* Storing in host order for ease of eNB IP range computations during pktgen */
    gtpu_info.enb_start_ip = ntohl(gtpu_info.enb_start_ip);
    gtpu_info.enb_end_ip = ntohl(gtpu_info.enb_end_ip);
    if ((gtpu_info.enb_start_ip + gtpu_info.no_of_enb - 1) >
	gtpu_info.enb_end_ip) {
      printf ("End range of eNB is smaller than the start range!!!\n");
      return -1;	  
    } else {
      /* Storing IP in host order */
      gtpu_info.enb_end_ip = gtpu_info.enb_start_ip +
				gtpu_info.no_of_enb - 1;
    }
    printf("enb_start_ip is %u, enb_end_ip is %u\n",
            gtpu_info.enb_start_ip, gtpu_info.enb_end_ip);
  } else if (enable_gtpu == 1) {
    printf("No 'enb_ip_range' value is specified in user_input.cfg file.\n");
    return -1;
  }
  if (config_lookup_int(&cfg, "flows", &val)) {
    gtpu_info.no_of_ue = val;
  } else {
    printf("No 'flows' argument setting in config. file.\n");
    return -1;
  }
  if (config_lookup_string(&cfg, "ue_ip_range", &val_str)) {
    char *str = strdup(val_str);
    inet_aton(strtok(str, " to "), (struct in_addr *)&(gtpu_info.ue_start_ip));

    inet_aton(strtok(NULL, " to "), (struct in_addr *)&(gtpu_info.ue_end_ip));
    if ((ntohl(gtpu_info.ue_start_ip) + gtpu_info.no_of_ue - 1) >
	ntohl(gtpu_info.ue_end_ip)) {
      printf ("End range of UE is smaller than the start range!!!\n");
      return -1;	  
    } else {
      /* Storing IP in network order */
      gtpu_info.ue_end_ip = htonl(ntohl(gtpu_info.ue_start_ip) +
				gtpu_info.no_of_ue - 1);
    }
  } else {
    printf("No 'ue_ip_range' value is specified in user_input.cfg file.\n");
    return -1;
  }
  if (config_lookup_string(&cfg, "app_srvr_ip_range", &val_str)) {
    char *str = strdup(val_str);
    inet_aton(strtok(str, " to "), (struct in_addr *)&(gtpu_info.as_srvr_start_ip));

    inet_aton(strtok(NULL, " to "), (struct in_addr *)&(gtpu_info.as_srvr_end_ip));
  } else {
    printf("No 'app_srvr_ip_range' value is specified in user_input.cfg file.\n");
    return -1;
  }
  if (config_lookup_string(&cfg, "s1u_sgw_ip", &val_str)) {
    inet_aton(val_str, (struct in_addr *)&(gtpu_info.s1u_sgw_ip));
  } else if (enable_gtpu == 1) {
    printf("No 's1u_sgw_ip' value is specified in user_input.cfg file.\n");
    return -1;
  }
  long long int uval = 0;
  if (config_lookup_int64(&cfg, "enb_teid", &uval)) {
    gtpu_info.enb_teid = uval;
    printf("end_teid is %d\n", gtpu_info.enb_teid);
  } else if (enable_gtpu == 1) {
    printf("No 'enb_teid' argument setting in config. file.\n");
    return -1;
  }
  if (config_lookup_int64(&cfg, "s1u_teid", &uval)) {
    gtpu_info.s1u_teid = uval;
    printf("s1u_teid is %u\n", gtpu_info.s1u_teid);
  } else if (enable_gtpu == 1) {
    printf("No 's1u_teid' argument setting in config. file.\n");
    return -1;
  }
  return 0;
}
/* Function: register_for_timeout
 * Input param: None
 * Output param: None
 * Return values: None
 * Brief: This function invokes stop_pkt_frwding() automatically by registering
 *        for SIGALRM signal with user specified test-duration value
**/
void register_for_timeout(void) {
    /* parse_test_duration_value will always be successful, because
       existence of test_duration param is already done */
    if (test_duration == 0) {
      printf("Test Duration is set to infinity\n");
    } else {
      printf("Test Duration is %u\n", test_duration);
    }
	total_duration += test_duration;
    if (test_duration != 0) {
      signal(SIGALRM, stop_pkt_frwding);
      alarm(test_duration);
    }
}

/* Function: stop_pkt_frwding
 * Input param: None
 * Output param: None
 * Return values: None
 * Brief: This function is invoked automatically at the end of test duration
 *        timeout. This function stops transmission of packets on ports
**/
void stop_pkt_frwding(int sig __rte_unused) {
  char *str[] = { strdup("stop"), strdup("0") };
  start_stop_cmd(2, str);
  printf("Packet transmission stopped. Press ENTER.\n");
  is_first_tx = true;
}

/* Function: get_tx_rx_counts
 * Input param: None
 * Output param: None
 * Return values: None
 * Brief: This function is reads the port_info_t->rate_stats to get the counts
          of packets transmiited and received at each port
**/
void get_tx_rx_counts(void)
{
#if 0
  /* Get per flow count for each of the queues of Port 0 and Port 1 */
  uint64_t **final_flow_tx = (uint64_t**)calloc(2, sizeof(uint64_t*)), 
           **final_flow_rx = (uint64_t**)calloc(2, sizeof(uint64_t*));
  uint16_t pid = 0;
  for (pid = 0; pid < 2; ++pid) { 
    final_flow_tx[pid] = (uint64_t*)calloc(subs, sizeof(uint64_t));
    final_flow_rx[pid] = (uint64_t*)calloc(subs, sizeof(uint64_t));
  }
  
  /* Get sum of per flows for each of the queues of Port 0 */
  uint16_t rx_cnt = pktgen.l2p->ports[0].rx_qid;
  uint16_t tx_cnt = pktgen.l2p->ports[0].tx_qid;;
  uint64_t i = 0, j = 0;
  for (i = 0; i < subs; ++i) {
    for (j = 0; j < rx_cnt; ++j) {
      final_flow_rx[0][i] += per_flow_rx_cnt[j][i];
    }
    for (j = 0; j < tx_cnt; ++j) {
      final_flow_tx[0][i] += per_flow_tx_cnt[j][i];
    }
  }
  /* If il_trafficgen is GENERATOR, get the per flow counts of
   * RESPONDER using UDP socket communication */
  if (traffic_gen_as == IL_TRAFFIC_GEN) {
    /* TODO(Vishnu): Get per flow RX and TX cnt of REPSONDER */
  }
  for (i = 0; i < subs; ++i) {
    char addr[32] = {0};
    uint32_t ip_num = htonl(start_ip+i);
    inet_ntop(AF_INET, (struct in_addr*)&ip_num, addr, 32);
    if (traffic_gen_as == IL_TRAFFIC_GEN) {
      /* for GENERATOR, capture TX and RX pkts of both P0/S1U and P1/SGI */ 
      FUNC_TRACE("No. of packets for %s are P0-Tx: %lu <--> P1_Rx: %lu ||| "
               "P1_Tx: %lu <--> P0_Rx: %lu", addr, final_flow_tx[0][i],
               final_flow_rx[1][i], final_flow_tx[1][i], final_flow_rx[0][i]);
    } else {
      /* for RESPONDER/RESP_AS_REFLECTOR, capture TX and RX pkts of P1/SGI */ 
      FUNC_TRACE("No. of packets for %s are Rx: %lu <--> Tx: %lu",
                 addr, final_flow_rx[0][i], final_flow_tx[0][i]);
    }
  }
  for (pid = 0; pid < 2; ++pid) {
    free (final_flow_tx[pid]);
    free (final_flow_rx[pid]);
  }
  free(final_flow_tx);
  final_flow_tx = NULL;
  free(final_flow_rx);
  final_flow_rx = NULL;
#endif
  FUNC_TRACE("No. of subs is %d", subs);

  uint64_t i = 0;
  uint16_t rx_cnt = pktgen.l2p->ports[0].rx_qid;
  uint16_t tx_cnt = pktgen.l2p->ports[0].tx_qid;;
  /* Get total pkt counts on each port */
  uint64_t pid_rx[2] = { 0, 0 }, pid_tx[2] = { 0, 0 };
  for (i = 0; i < rx_cnt; ++i) {
    pid_rx[0] += pid_rx_tx_cnts[0].rx_qid_cnts[i];
  }
  for (i = 0; i < tx_cnt; ++i) {
    pid_tx[0] += pid_rx_tx_cnts[0].tx_qid_cnts[i];
  }
  
  if (traffic_gen_as == IL_TRAFFIC_GEN) {
    pid_rx[1] = sgi_stats_data.rx_cnt;
    pid_tx[1] = sgi_stats_data.tx_cnt;
    if (pid_tx[0] < pid_rx[1]) {
      pid_tx[0] = pid_rx[1];
    }
    if (pid_tx[1] < pid_rx[0]) {
      pid_tx[1] = pid_rx[0];
    }
	if (system("clear") != 0) {
		perror("clear failed");
	}
    printf("%17s", "\n********** Uplink Table **************\n");
	if (enable_gtpu) {
    	printf("%8s >>   %20s ", "S1U_PktTx (Total Pkts)",
               "AS_PktRx (Total Pkts)\n");
	} else {
    	printf("%8s >>   %20s ", "P0-Tx(Total Pkts)",
               "P1-Rx (Total Pkts)\n");
	}
    printf("%10lu %27lu\n", pid_tx[0], pid_rx[1]);
    printf("%17s", "\n********** Downlink Table **************\n");
	if (enable_gtpu) {
    	printf("%8s <<   %20s \n", "S1u_PktRx (Total Pkts)", "AS_PktTx (Total Pkts)");
    } else {
		printf("%8s <<   %20s \n", "P0-Rx(Total Pkts)", "P1-Tx (Total Pkts)");
 	}
	printf("%10lu %27lu \n", pid_rx[0], pid_tx[1]);
    printf("\n"); /* empty line */
    if (pid_tx[0]) {
		if (enable_gtpu) {
      		printf("UL Loss = S1u_PktTx - AS_PktRx = "
                 "%8lu(pkts); %5.2f(%%)\n", (uint64_t)(pid_tx[0]-pid_rx[1]),
                 (((double)(pid_tx[0] - pid_rx[1])/ (double)pid_tx[0]))*100);
		} else {
      		printf("UL Loss = P0-Tx - P1-Rx = "
                 "%8lu(pkts); %5.2f(%%)\n", (uint64_t)(pid_tx[0]-pid_rx[1]),
                 (((double)(pid_tx[0] - pid_rx[1])/ (double)pid_tx[0]))*100);
		}
    } else {
		if (enable_gtpu) {
      		printf("UL Loss = S1u_PktTx - AS_PktRx = "
                 "%8d(pkts); %5.2f(%%)\n", 0, 0.00);
      	} else {
			printf("UL Loss = P0-Tx - P1-Rx = "
                 "%8d(pkts); %5.2f(%%)\n", 0, 0.00);
    	}
    }
    if (pid_tx[1]) {
		if (enable_gtpu) {
      		printf("DL Loss = AS_PktTx - S1u_PktRx = "
                 "%8lu(pkts); %5.2f(%%)\n", (uint64_t)(pid_tx[1]-pid_rx[0]),
               (((double)(pid_tx[1]-pid_rx[0])/(double)pid_tx[1]))*100);
      	} else {
			printf("DL Loss = P1-Tx - P0-Rx = "
                 "%8lu(pkts); %5.2f(%%)\n", (uint64_t)(pid_tx[1]-pid_rx[0]),
               (((double)(pid_tx[1]-pid_rx[0])/(double)pid_tx[1]))*100);
		}
    } else {
		if (enable_gtpu) {
      		printf("DL Loss = AS_PktTx - S1u_PktRx = "
                 "%8d(pkts); %5.2f(%%)\n", 0, 0.00);
      	} else {
			printf("DL Loss = P1-Tx - P0-Rx = "
                 "%8d(pkts); %5.2f(%%)\n", 0, 0.00);
		}
    }
	printf("\n\n");

    FUNC_TRACE("%17s", "************************************* Uplink Table "
                       "*************************************");
    FUNC_TRACE("%8s %22s   >>   %20s %25s", "P0-Tx(Total Pkts)",
               "P0-Tx (Pkts/Sec)", "P1-Rx (Total Pkts)", "P1-Rx (Pkt/s)");
    FUNC_TRACE("%10lu %20lu %27lu %27lu",
               pid_tx[0],
				(total_duration?(pid_tx[0]/total_duration):pid_rx_tx_cnts[0].omax_pkt_sec),
               pid_rx[1],
				(total_duration?(pid_tx[0]/total_duration):pid_rx_tx_cnts[1].imax_pkt_sec));
    FUNC_TRACE("%17s", "************************************* Downlink Table "
                       "*************************************");
    FUNC_TRACE("%8s %22s   <<   %20s %25s", "P0-Rx(Total Pkts)",
               "P0-Rx (Pkts/Sec)", "P1-Tx (Total Pkts)", "P1-Tx (Pkt/s)");
	uint32_t dl_duration = sgi_stats_data.sgi_duration;
	if (pid_tx[1] && !dl_duration) {
		dl_duration = total_duration;
	}
    FUNC_TRACE("%10lu %20lu %27lu %27lu",
               pid_rx[0],
				(dl_duration?(pid_rx[0]/dl_duration):pid_rx_tx_cnts[0].imax_pkt_sec),
               pid_tx[1],
				(dl_duration?(pid_tx[1]/dl_duration):pid_rx_tx_cnts[1].omax_pkt_sec));
    FUNC_TRACE(""); /* empty line */
    if (pid_tx[0]) {
      FUNC_TRACE("UL Loss = [P0-Tx (Total Pkts) - P1-Rx (Total Pkts)] = "
                 "%8lu(pkts); %5.2f(%)", (pid_tx[0]-pid_rx[1]),
                 (((double)(pid_tx[0] - pid_rx[1])/ (double)pid_tx[0]))*100);
    } else {
      FUNC_TRACE("UL Loss = [P0-Tx (Total Pkts) - P1-Rx (Total Pkts)] = "
                 "%8lu(pkts); %5.2f(%)", 0, 0.00);
    }
    if (pid_tx[1]) {
      FUNC_TRACE("DL Loss = [P1-Tx (Total Pkts) - P0-Rx (Total Pkts)] = "
                 "%8lu(pkts); %5.2f(%)", (pid_tx[1]-pid_rx[0]),
               (((double)(pid_tx[1]-pid_rx[0])/(double)pid_tx[1]))*100);
    } else {
      FUNC_TRACE("DL Loss = [P1-Tx (Total Pkts) - P0-Rx (Total Pkts)] = "
                 "%8lu(pkts); %5.2f(%)", 0, 0.00);
    }
  } else {
    /* For RESPONDER/RESPONDER_AS_REFLECTOR, capture pkts sent and recvd. */
    FUNC_TRACE("%8s %22s   ||   %20s %25s", "P0-Tx(Total Pkts)",
               "P0-Tx (Pkts/Sec)", "P0-Rx (Total Pkts)", "P0-Rx (Pkt/s)");
    FUNC_TRACE("%10lu %20lu %27lu %27lu",
               pid_tx[0], pid_rx_tx_cnts[0].omax_pkt_sec,
               pid_rx[0], pid_rx_tx_cnts[0].imax_pkt_sec);
  }
}

/* Function: set_ip_pkt_counters 
 * Input param: subs: no. of flows
 * Output param: None
 * Return values: None
 * Brief: Based on the number of flows defined, allocate memory dynamically
          to store the per flow counts
**/
void set_ip_pkt_counters(uint64_t subs) {
  if (per_flow_tx_cnt == NULL) {
    per_flow_tx_cnt = (uint64_t**)calloc(PKTGEN_TOOL_MAX_QUEUES, sizeof(uint64_t*));
    int q_cnt = 0;
    for (; q_cnt < PKTGEN_TOOL_MAX_QUEUES; ++q_cnt) {
      per_flow_tx_cnt[q_cnt] = (uint64_t*)calloc(subs, sizeof(uint64_t));
    }
  }
  if (per_flow_rx_cnt == NULL) {
    per_flow_rx_cnt = (uint64_t**)calloc(PKTGEN_TOOL_MAX_QUEUES, sizeof(uint64_t*));
    int q_cnt = 0;
    for (; q_cnt < PKTGEN_TOOL_MAX_QUEUES; ++q_cnt) {
      per_flow_rx_cnt[q_cnt] = (uint64_t*)calloc(subs, sizeof(uint64_t));
    }
  }
}

/* Function: delete_dynamic_pkt_cnt
 * Input param: None
 * Output param: None
 * Return values: void
 * Brief: This function deletes all dynamically allocated tx/rx packet counters
**/
void delete_dynamic_pkt_cnt (void) {
  if (per_flow_rx_cnt) {
    int q_cnt = 0;
    for (q_cnt = 0; q_cnt < PKTGEN_TOOL_MAX_QUEUES; ++q_cnt) {
      free (per_flow_rx_cnt[q_cnt]);
    }
  }
  free (per_flow_rx_cnt);
  per_flow_rx_cnt = NULL;
  if (per_flow_tx_cnt) {
    int q_cnt = 0;
    for (q_cnt = 0; q_cnt < PKTGEN_TOOL_MAX_QUEUES; ++q_cnt) {
      free (per_flow_tx_cnt[q_cnt]);
    }
  }
  free (per_flow_tx_cnt);
  per_flow_tx_cnt = NULL;
}
#ifdef PCAP_GEN
/**
 * Initialize pcap dumper.
 * @param pcap_filename
 * Return: pointer to pcap output filename
*/
pcap_dumper_t *init_pcap(char *pcap_filename) {
  pcap_t *pcap = pcap_open_dead(DLT_EN10MB, UINT16_MAX);
  return pcap_dump_open(pcap, pcap_filename); 
}

/**
 * Write to pcap files
 * @param pkts
 *  pointer to mbuf packets
 * @param n
 *  number of packets
 * @param pcap_dumper
 *  pointer to pcap dumper
 */
void dump_pcap(struct rte_mbuf *m, uint8_t pid) {
  RTE_SET_USED(pid);
  pcap_dumper_t *pcap_dumper = pcap_dumper_p0;
  if (traffic_gen_as != IL_TRAFFIC_GEN) {
    pcap_dumper = pcap_dumper_p1;
  }
  uint8_t *pkt = rte_pktmbuf_mtod(m, uint8_t*);
  struct pcap_pkthdr hdr = {
    .len = m->pkt_len,
    .caplen = m->pkt_len
  };
  gettimeofday(&(hdr.ts), NULL);
  pcap_dump((u_char*)pcap_dumper, &hdr, pkt);
  pcap_dump_flush(pcap_dumper);
}
#endif /* PCAP_GEN */
void* send_stats_data(void *arg) {
	RTE_SET_USED(arg);
	while (1) {
         char data_req = 'a';
		 int ret = udp_recv_socket((void*)&data_req, 1);
		 if (ret < 0) {
		 	printf("Failed to recv message!!!\n");
		 } else if (ret > 0) {
	 		sgi_stats_data.tx_cnt = 0;	
	 		sgi_stats_data.rx_cnt = 0;
	 		sgi_stats_data.sgi_duration = total_duration;
  			/* Get total pkt counts on each port */
  			uint16_t rx_cnt = pktgen.l2p->ports[0].rx_qid;
  			uint16_t tx_cnt = pktgen.l2p->ports[0].tx_qid;;
        	uint8_t i = 0;
  			for (i = 0; i < rx_cnt; ++i) {
    			sgi_stats_data.rx_cnt += pid_rx_tx_cnts[0].rx_qid_cnts[i];
  			}
  			for (i = 0; i < tx_cnt; ++i) {
    			sgi_stats_data.tx_cnt += pid_rx_tx_cnts[0].tx_qid_cnts[i];
  			}
			if (udp_send_socket(&sgi_stats_data, sizeof(struct sgi_stats)) < 0) {
				printf("Failed to send message!!!\n");
			}
		}
	}
}
