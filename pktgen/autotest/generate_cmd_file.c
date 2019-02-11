#include<stdio.h>
#include<unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> 
#include <arpa/inet.h>
#include <libconfig.h>

int pps = 0, subs = 0, test_duration = 0, pkt_size = 0,
    src_ip_start = 0, src_ip_end = 0, dst_ip_start = 0, dst_ip_end = 0;
char src_ip_start_str[32] = {0}, src_ip_end_str[32] = {0},
     dst_ip_start_str[32] = {0}, dst_ip_end_str[32] = {0},
     p0_src_mac_str[18] = {0}, p1_src_mac_str[18] = {0},
     p0_dst_mac_str[18] = {0}, p1_dst_mac_str[18] = {0},
     protocol[32] = "udp", proto_type[32] = "ipv4";
     

/* Function: remove_spaces_and_newline 
 * Input param: char pointer
 * Output param: None
 * Return values: None
 * Brief: This function is used to remove spaces and new lines from
 *        the input string(str)
 */
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
/* Function: convert_mac_to_num 
 * Input param: char pointer
 * Output param: None
 * Return values: None
 * Brief: This function is used to compute a number from mac
 *        in xx:xx:xx:xx:xx:xx format
 */
uint64_t convert_mac_to_num(char *mac) {
  int mac_addr[6];
  if (sscanf(mac, "%x:%x:%x:%x:%x:%x", &mac_addr[0], &mac_addr[1],
             &mac_addr[2], &mac_addr[3], &mac_addr[4], &mac_addr[5]) < 6) {
    printf("Error in redaing MAC values\n");
  }
  int i = 0;
  for (; i < sizeof(mac_addr)/sizeof(int); ++i) {
    printf ("Mac is %d\t", mac_addr[i]);
  }
  printf("\n");
  uint64_t mac_num = 0, left_shift = 0;
  for (i = 0; i < sizeof(mac_addr)/sizeof(int); ++i) {
    mac_num <<= left_shift;
    mac_num |= mac_addr[i];
    left_shift = 8; 
  }
  return mac_num;
}

/* Function: convert_ip_to_num 
 * Input param: char *
 * Output param: None
 * Return values: uint32_t - converted numeric value of IP
 * Brief: This function converts IP in "1.2.3.4" format to numeric
 */
uint32_t convert_ip_to_num(char *ip) {
  char tmp_str[32] = {0};
  char *single_ip_val = NULL, *tmp_ip_val = tmp_str;
  int ip_num = 0, left_shift = 0;
  if (!strcmp(proto_type, "ipv4") || !strcmp(proto_type, "IPV4")) {
  	strncpy(tmp_str, ip, 32);
  	while (single_ip_val = strtok(tmp_ip_val, ".")) {
    		ip_num <<=left_shift;
    		ip_num |= atoi(single_ip_val);
    		left_shift = 8;
    		tmp_ip_val = NULL;
  	}
  } else {
        int colon_cnt = 0;
	while (colon_cnt < 6) {
	  if (*ip++ == ':') {
	    ++colon_cnt;
	  }
        }
  	strncpy(tmp_str, ip, 32);
  	while (single_ip_val = strtok(tmp_ip_val, ":")) {
    		ip_num <<=left_shift;
    		ip_num |= atoi(single_ip_val);
    		left_shift = 16;
    		tmp_ip_val = NULL;
  	}
  }
  return ip_num;
}

/* Function: create_pktgen_cmdfile 
 * Input param: char
 * Output param: None
 * Return values: bool
 *        0: Successfully created input command file
 *        false: Error in creatinf input file
 * Brief: This function creates input command file "input.txt" in PWD based on the user inputs
 */
bool create_pktgen_cmdfile(char option) {
  FILE *cmd_fp = fopen("autotest/input.txt", "w");
  if (cmd_fp == NULL) {
    printf ("Failed to open input.txt for writing!!!!");
    return 1;
  }
  uint16_t p0 = 0;
  if (subs > 0) {
    /* enable range of flows on port 0 */
    fprintf(cmd_fp, "enable %d range\n", p0);
  }
  fprintf(cmd_fp, "set %d count %d\n", p0, 0); 
  fprintf(cmd_fp, "set %d size %u\n", p0, pkt_size); 
  fprintf(cmd_fp, "set %d pps %d\n", p0, pps); 
  fprintf(cmd_fp, "set %d burst %d\n", p0, 32); 
  fprintf(cmd_fp, "set %d type %s\n", p0, proto_type); 
  fprintf(cmd_fp, "set %d proto %s\n", p0, protocol);
  if (option == 'g') {
    /* Generator settings */
    fprintf(cmd_fp, "set %d src ip %s/%d\n", p0, src_ip_start_str, 24);
    fprintf(cmd_fp, "set %d dst ip %s\n", p0, dst_ip_start_str);
    fprintf(cmd_fp, "set %d src mac %s\n", p0, p0_src_mac_str);
    fprintf(cmd_fp, "set %d dst mac %s\n", p0, p0_dst_mac_str);
    fprintf(cmd_fp, "set %d sport %d\n", p0, 1234);
    fprintf(cmd_fp, "set %d dport %d\n", p0, 5678);
    /* range 0 src ip start 12.12.12.12 */
    fprintf(cmd_fp, "range %d src ip start %s\n", p0, src_ip_start_str);
    /* range 0 src ip min 12.12.12.12 */
    fprintf(cmd_fp, "range %d src ip min %s\n", p0, src_ip_start_str);
    uint32_t max_ip_num = src_ip_start+subs-1;
    struct in_addr struct_maxip = { htonl(max_ip_num) };
    char max_ip[32] = {0};
    if (inet_ntop(AF_INET, &struct_maxip, max_ip, 32) == NULL) {
      printf("Failed to compute Max IP!!!");
      return 1;
    }
    /* range 0 src ip max 12.12.12.30 */
    fprintf(cmd_fp, "range %d src ip max %s\n", p0, max_ip);
    /* range 0 src ip inc 0.0.0.1 */
    fprintf(cmd_fp, "range %d src ip inc %s\n", p0, "0.0.0.1");
    /* range 0 dst ip start 12.12.12.255 */
    fprintf(cmd_fp, "range %d dst ip start %s\n", p0, dst_ip_start_str);
    /* range 0 dst ip min 12.12.12.255 */
    fprintf(cmd_fp, "range %d dst ip min %s\n", p0, dst_ip_start_str);
    /* range 0 dst ip max 12.12.12.255 */
    fprintf(cmd_fp, "range %d dst ip max %s\n", p0, dst_ip_start_str);
    /* range 0 dst ip inc 0.0.0.0 */
    fprintf(cmd_fp, "range %d dst ip inc %s\n", p0, "0.0.0.0");
  
    /* GENERATOR src mac addr range commands */
    /* range 0 mac src start 01:02:03:04:05:00 */
    fprintf(cmd_fp, "range %d src mac start %s\n", p0, p0_src_mac_str);
    /* range 0 mac src min 01:02:03:04:05:00 */
    fprintf(cmd_fp, "range %d src mac min %s\n", p0, p0_src_mac_str);
    /* range 0 mac src max 01:02:03:04:05:00 */
    fprintf(cmd_fp, "range %d src mac max %s\n", p0, p0_src_mac_str);
    /* range 0 mac src inc 00:00:00:00:00:00 */
    fprintf(cmd_fp, "range %d src mac inc %s\n", p0, "00:00:00:00:00:00");
  
    /* GENERATOR dst mac addr range commands */
    /* range 0 mac dst start 01:02:03:04:05:00 */
    fprintf(cmd_fp, "range %d dst mac start %s\n", p0, p0_dst_mac_str);
    /* range 0 mac dst min 01:02:03:04:05:00 */
    fprintf(cmd_fp, "range %d dst mac min %s\n", p0, p0_dst_mac_str);
    /* range 0 mac dst max 01:02:03:04:05:00 */
    fprintf(cmd_fp, "range %d dst mac max %s\n", p0, p0_dst_mac_str);
    /* range 0 mac dst inc 00:00:00:00:00:00 */
    fprintf(cmd_fp, "range %d dst mac inc %s\n", p0, "00:00:00:00:00:00");
    /* range 0 protocol */
    fprintf(cmd_fp, "range %d proto %s\n", p0, protocol);
  
    /* GENERATOR src port range */
    fprintf(cmd_fp, "range %d src port start 1234\n", p0);
    fprintf(cmd_fp, "range %d src port min 1234\n", p0);
    fprintf(cmd_fp, "range %d src port max 1234\n", p0);
    fprintf(cmd_fp, "range %d src port inc 0\n", p0);
  
    /* GENERATOR dst port range */
    fprintf(cmd_fp, "range %d dst port start 5678\n", p0);
    fprintf(cmd_fp, "range %d dst port min 5678\n", p0);
    fprintf(cmd_fp, "range %d dst port max 5678\n", p0);
    fprintf(cmd_fp, "range %d dst port inc 0\n", p0);
  
    /* pkt size ranges */
    fprintf(cmd_fp, "range %d size start %u\n", p0, pkt_size);
    fprintf(cmd_fp, "range %d size min %u\n", p0, pkt_size);
  } else {
    /* RESPONDER settings */
    /* swapping src IP and dst IP for RESPONDER */ 
    fprintf(cmd_fp, "set %d src ip %s/%d\n", p0, dst_ip_start_str, 24);
    fprintf(cmd_fp, "set %d dst ip %s\n", p0, src_ip_start_str);
  
    /* RESPONDER src and mac addresses */
    fprintf(cmd_fp, "set %d src mac %s\n", p0, p1_src_mac_str);
    fprintf(cmd_fp, "set %d dst mac %s\n", p0, p1_dst_mac_str);
  
    /* RESPONDER port setting */
    fprintf(cmd_fp, "set %d sport %d\n", p0, 5678);
    fprintf(cmd_fp, "set %d dport %d\n", p0, 1234);
  
    /* RESPONDER Src IP range */
    /* range 1 src ip start 12.12.12.255 */
    fprintf(cmd_fp, "range %d src ip start %s\n", p0, dst_ip_start_str);
    /* range 1 src ip min 12.12.12.255 */
    fprintf(cmd_fp, "range %d src ip min %s\n", p0, dst_ip_start_str);
    /* range 1 src ip max 12.12.12.255 */
    fprintf(cmd_fp, "range %d src ip max %s\n", p0, dst_ip_start_str);
    /* range 1 src ip inc 0.0.0.0 */
    fprintf(cmd_fp, "range %d src ip inc %s\n", p0, "0.0.0.0");
  
    /* Creating ranges for RESPONDER dst IP address */
    /* range 0 dst ip start 12.12.12.0 */ 
    fprintf(cmd_fp, "range %d dst ip start %s\n", p0, src_ip_start_str);
    /* range 0 dst ip min 12.12.12.0 */
    fprintf(cmd_fp, "range %d dst ip min %s\n", p0, src_ip_start_str);
    uint32_t max_ip_num = src_ip_start+subs-1;
    struct in_addr struct_maxip = { htonl(max_ip_num) };
    char max_ip[32] = {0};
    if (inet_ntop(AF_INET, &struct_maxip, max_ip, 32) == NULL) {
      printf("Failed to compute Max IP!!!");
      return 1;
    }
    /* range 0 dst ip max 12.12.12.24 */
    fprintf(cmd_fp, "range %d dst ip max %s\n", p0, max_ip);
    /* range 0 src ip inc 0.0.0.1 */
    fprintf(cmd_fp, "range %d dst ip inc %s\n", p0, "0.0.0.1");
  
    /* RESPONDER src mac addr range commands */
    /* range 1 mac src start 07:08:09:0a:0b:0 */
    fprintf(cmd_fp, "range %d src mac start %s\n", p0, p1_src_mac_str);
    /* range 1 mac src min 07:08:09:0a:0b:0c */
    fprintf(cmd_fp, "range %d src mac min %s\n", p0, p1_src_mac_str);
    /* range 1 mac src max 07:08:09:0a:0b:0c */
    fprintf(cmd_fp, "range %d src mac max %s\n", p0, p1_src_mac_str);
    /* range 1 mac src inc 00:00:00:00:00:00 */
    fprintf(cmd_fp, "range %d src mac inc %s\n", p0, "00:00:00:00:00:00");
  
    /* RESPONDER dst mac addr range commands */
    /* range 1 mac dst start 01:02:03:04:05:00 */
    fprintf(cmd_fp, "range %d dst mac start %s\n", p0, p1_dst_mac_str);
    /* range 1 mac dst min 01:02:03:04:05:00 */
    fprintf(cmd_fp, "range %d dst mac min %s\n", p0, p1_dst_mac_str);
    /* range 1 mac dst max 01:02:03:04:05:00 */
    fprintf(cmd_fp, "range %d dst mac max %s\n", p0, p1_dst_mac_str);
    /* range 1 mac src inc 00:00:00:00:00:00 */
    fprintf(cmd_fp, "range %d dst mac inc %s\n", p0, "00:00:00:00:00:00");
  
    /* RESPONDER src port range */
    fprintf(cmd_fp, "range %d src port start 5678\n", p0);
    fprintf(cmd_fp, "range %d src port min 5678\n", p0);
    fprintf(cmd_fp, "range %d src port max 5678\n", p0);
    fprintf(cmd_fp, "range %d src port inc 0\n", p0);
  
    /* RESPONDER dst port range */
    fprintf(cmd_fp, "range %d dst port start 1234\n", p0);
    fprintf(cmd_fp, "range %d dst port min 1234\n", p0);
    fprintf(cmd_fp, "range %d dst port max 1234\n", p0);
    fprintf(cmd_fp, "range %d dst port inc 0\n", p0);
  
    /* RESPONDER protocol range */
    fprintf(cmd_fp, "range %d proto %s\n", p0, protocol);
  
    /* RESPONDER pkt size ranges */
    fprintf(cmd_fp, "range %d size start %u\n", p0, pkt_size);
    fprintf(cmd_fp, "range %d size min %u\n", p0, pkt_size);
  }
  fclose(cmd_fp); 
}
/* Function: parse_user_input_values
 * Input param: char
 * Output param: None
 * Return values: bool
 *        0: Successfully parsed user inputs
 *        false: Error in parsing user input
 * Brief: This function reads all user confgured input parameters from
 *        "user_input.cfg" in PWD/autotest/
 */
int parse_user_input_values(char option) {
    char *cfg_file = "autotest/user_input.cfg";
    config_t cfg;
    int val = 0;
    const char *val_str = NULL;
    char *str = NULL;
    config_init(&cfg);
    printf ("\n******************************************\n");
    printf ("Reading config. file ...!!!");
    printf ("\n******************************************\n");
    if (!config_read_file(&cfg, cfg_file)) {
        fprintf(stderr,"\n%s:%d - %s", config_error_file(&cfg), config_error_line(&cfg),
                config_error_text(&cfg));
        config_destroy(&cfg);
        return -1;
    }
    if (config_lookup_int(&cfg, "pps", &val)) {
        printf("pps is %d\n", val);
        pps = val;
    } else {
        printf("No 'pps' value is specified in user_input.cfg file. \n");
        return -1;
    }
    if (config_lookup_int(&cfg, "pkt_size", &val)) {
        printf("pkt size is %d\n", val);
        pkt_size = val;
    } else {
        printf("No 'pkt_size' value is specified in user_input.cfg file. \n");
        return -1;
    }
    if (config_lookup_int(&cfg, "flows", &val)) {
        printf("No. of flows is %d\n", val);
        subs = val;
    } else {
        printf("No 'flows' value is specified in user_input.cfg file. \n");
        return -1;
    }
    if (config_lookup_int(&cfg, "test_duration", &val)) {
        test_duration = val;
        if (test_duration == 0) {
            printf("Test Duration is set to Infinity\n");
        } else {
            printf("Test Duration is %d\n", test_duration);
        }
    } else {
        printf("No 'test_duration' value is specified in user_input.cfg file. \n");
        return -1;
    }
    if (config_lookup_string(&cfg, "proto_type", &val_str)) {
	str = (char*)val_str;
        strcpy(proto_type, str);
    } else {
        printf("No 'proto_type' value is specified in user_input.cfg file. \n");
        return -1;
    }
    if (config_lookup_string(&cfg, "ue_ip_range", &val_str)) {
	str = (char*)val_str;
        strcpy(src_ip_start_str, strtok(str, " to "));
        strcpy(src_ip_end_str, strtok(NULL, " to "));
        printf("ue_ip_start_str is %s, ue_ip_end_str is %s\n", src_ip_start_str,
		src_ip_end_str);
        src_ip_start = convert_ip_to_num(src_ip_start_str);
        src_ip_end = convert_ip_to_num(src_ip_end_str);
        printf("ue_ip_start is %u, ue_ip_end is %u\n", src_ip_start, src_ip_end);
    } else {
        printf("No 'ue_ip_range' value is specified in user_input.cfg file. \n");
        return -1;
    }
    if (config_lookup_string(&cfg, "app_srvr_ip_range", &val_str)) {
	str = (char*)val_str;
        strcpy(dst_ip_start_str, strtok(str, " to "));
        strcpy(dst_ip_end_str, strtok(NULL, " to "));
        printf("app_srvr_ip_start_str is %s, app_srvr_ip_end_str is %s\n", dst_ip_start_str, dst_ip_end_str);
        dst_ip_start = convert_ip_to_num(dst_ip_start_str);
        dst_ip_end = convert_ip_to_num(dst_ip_end_str);
        printf("app_srvr_ip_start is %u, app_srvr_ip_end is %u\n", dst_ip_start, dst_ip_end);
    } else {
        printf("No 'app_srvr_ip_range' value is specified in user_input.cfg file. \n");
        return -1;
    }
    if (config_lookup_string(&cfg, "p0_src_mac", &val_str)) {
	str = (char*)val_str;
        strcpy(p0_src_mac_str, str);
        printf("p0_src_mac_str is %s\n", p0_src_mac_str);
    } else {
        printf("No 'p0_src_mac' value is specified in user_input.cfg file. \n");
        return -1;
    }
    if (config_lookup_string(&cfg, "p1_src_mac", &val_str)) {
	str = (char*)val_str;
        strcpy(p1_src_mac_str, str);
        printf("p1_src_mac_str is %s\n", p1_src_mac_str);
    } else {
        printf("No 'p1_src_mac' value is specified in user_input.cfg file. \n");
        return -1;
    }
    if (config_lookup_string(&cfg, "p0_dst_mac", &val_str)) {
	str = (char*)val_str;
        strcpy(p0_dst_mac_str, str);
        printf("p0_dst_mac_str is %s\n", p0_dst_mac_str);
    } else {
        printf("No 'p0_dst_mac' value is specified in user_input.cfg file. \n");
        return -1;
    }
    if (config_lookup_string(&cfg, "p1_dst_mac", &val_str)) {
	str = (char*)val_str;
        strcpy(p1_dst_mac_str, str);
        printf("p1_dst_mac_str is %s\n", p1_dst_mac_str);
    } else {
        printf("No 'p1_dst_mac' value is specified in user_input.cfg file. \n");
        return -1;
    }
    if (config_lookup_string(&cfg, "protocol", &val_str)) {
	str = (char*)val_str;
        strcpy(protocol, str);
    } else {
        printf("No 'protocol' value is specified in user_input.cfg file. \n");
        return -1;
    }
    if (subs > (src_ip_end-src_ip_start+1)) {
      printf("No. of Subs is less that the defined start IP ranges\n");
    }
    if (create_pktgen_cmdfile(option) != 0) {
      printf("Error in Creating pktgen command file\n");
      return 1;
    }
    return 0;
}

int main (int argc, char *argv[0]) {
    if (argc < 2) {
      	printf ("Usage ./generate_cmd_file <option>\n");
        printf("options:\n");
        printf("-g, --generator                   Start il_trafficgen as generator\n");
        printf("-r, --responder                   Start il_trafficgen as responder\n");
        printf("-rs, --responder_as_reflector     Start il_trafficgen with responder as reflector\n");
        return -1; 
    }
    char opt = '\0';
    if (!strcmp(argv[1], "-g") || !strcmp(argv[1], "--generator")) {
      opt = 'g'; 
    } else if (!strcmp(argv[1], "-r") || !strcmp(argv[1], "--responder")) {
      opt = 'r'; 
    } else if (strcmp(argv[1], "-rs") && strcmp(argv[1], "responder_as_reflector")) {
      	printf ("Usage ./generate_cmd_file <option>\n");
        printf("options:\n");
        printf("-g, --generator                   Start il_trafficgen as generator\n");
        printf("-r, --responder                   Start il_trafficgen as responder\n");
        printf("-rs, --responder_as_reflector     Start il_trafficgen with responder as reflector\n");
        return -1;
    }
    if (parse_user_input_values(opt) < 0) {
        return -1;
    }
    return 0;
    
}
