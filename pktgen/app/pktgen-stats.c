/*-
 * Copyright (c) <2010-2017>, Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* Created 2010 by Keith Wiles @ intel.com */

#include <stdio.h>

#include "pktgen-cmds.h"
#include "pktgen-display.h"
#include "pktgen-tool.h"
#include "pktgen-interface.h"
#include "pktgen.h"

#if RTE_VERSION >= RTE_VERSION_NUM(17, 5, 0, 0)
#include <rte_bus_pci.h>
#endif

/**************************************************************************//**
 *
 * pktgen_page_stats - Display the statistics on the screen for all ports.
 *
 * DESCRIPTION
 * Display the port statistics on the screen for all ports.
 *
 * RETURNS: N/A
 *
 * SEE ALSO:
 */
void pktgen_page_stats(void)
{
	uint32_t row;

	pktgen_display_set_color("top.page");
	display_topline("");

	row = PORT_STATE_ROW;
	row++;
	pktgen_display_set_color("stats.port.label");

	pktgen_display_set_color("stats.dyn.label");
	if (traffic_gen_as == IL_TRAFFIC_GEN) {
		/* Labels for dynamic fields (update every second) */
 		char tmp_start_ip[18] = {'\0'}, tmp_end_ip[18] = {'\0'}; 
		uint32_t ip_num = htonl(start_ip);
		inet_ntop(AF_INET, (struct in_addr*)&ip_num, tmp_start_ip, 18);
		ip_num = htonl(end_ip);
		inet_ntop(AF_INET, (struct in_addr*)&ip_num, tmp_end_ip, 18);
		if (enable_gtpu) {
			scrn_printf(row++, 1, "UE(s)			: %s to %s	(Total: %lu)",
						tmp_start_ip, tmp_end_ip, subs);
		} else {
			scrn_printf(row++, 1, "Src IP(s)  Used	: %s to %s	(Total: %lu)",
						tmp_start_ip, tmp_end_ip, subs);
		}
		if (enable_gtpu) {
			ip_num = htonl(gtpu_info.enb_start_ip);
			inet_ntop(AF_INET, (struct in_addr*)&ip_num, tmp_start_ip, 18);
			ip_num = htonl(gtpu_info.enb_end_ip);
			inet_ntop(AF_INET, (struct in_addr*)&ip_num, tmp_end_ip, 18);
			scrn_printf(row++, 1, "EnB(s)			: %s to %s	(Total: %lu)",
						tmp_start_ip, tmp_end_ip,
						gtpu_info.enb_end_ip-gtpu_info.enb_start_ip+1);
		}
		ip_num = gtpu_info.as_srvr_start_ip;
		inet_ntop(AF_INET, (struct in_addr*)&ip_num, tmp_start_ip, 18);
		ip_num = gtpu_info.as_srvr_end_ip;
		inet_ntop(AF_INET, (struct in_addr*)&ip_num, tmp_end_ip, 18);
		if (enable_gtpu) {
			scrn_printf(row++, 1, "App Srvr(s)		: %s to %s	(Total: %lu)",
						tmp_start_ip, tmp_end_ip, 1);
		} else {
			scrn_printf(row++, 1, "Dst IP(s) Used	: %s to %s	(Total: %lu)",
						tmp_start_ip, tmp_end_ip, 1);
		}
		/* empty row */
		row++;
		scrn_printf(row++, 1, "PCI:: %s = P0", gen_host_pci);
		/* empty row */
		row++;
		scrn_printf(row++, 1, "help:: start 0 --> start traffic on P0");
		scrn_printf(row++, 1, "       stp     --> stop traffic on P0");
		scrn_printf(row++, 1, "       quit    --> exit application");
		/* empty row */
		row++;
							
		/* Get S1U stats */  
		uint16_t rx_cnt = pktgen.l2p->ports[0].rx_qid;
  		uint16_t tx_cnt = pktgen.l2p->ports[0].tx_qid;;
		memset(&s1u_stats_data, 0, sizeof(s1u_stats_data));
        uint8_t i = 0;
  		for (i = 0; i < rx_cnt; ++i) {
    		s1u_stats_data.rx_cnt += pid_rx_tx_cnts[0].rx_qid_cnts[i];
  		}
  		for (i = 0; i < tx_cnt; ++i) {
    		s1u_stats_data.tx_cnt += pid_rx_tx_cnts[0].tx_qid_cnts[i];
  		}

		/* Get SGI stats */
		char data_req = 'a';
		udp_send_socket(&data_req, 1);
		if (udp_recv_socket((void*)&sgi_stats_data,
			sizeof(sgi_stats_data)) < 0) {
			printf("Failed to recv message!!!\n");
		}
		scrn_printf(row++, 1, "User Plane Downlink Table");
		if (enable_gtpu) {
			scrn_printf(row++, 1, "%20s%10s%20s", "AS_PktTx", "--->", "S1u_PktRx");
		} else {
			scrn_printf(row++, 1, "%20s%10s%20s", "P1_PktTx", "--->", "P0_PktRx");
		}
		scrn_printf(row++, 1, " %19lu%30lu", sgi_stats_data.tx_cnt, s1u_stats_data.rx_cnt);

		scrn_printf(row++, 1, "User Plane Uplink Table");
		if (enable_gtpu) {
			scrn_printf(row++, 1, "%20s%10s%20s", "S1u_PktTx", "--->", "AS_PktRx");
		} else {
			scrn_printf(row++, 1, "%20s%10s%20s", "P0_PktTx", "--->", "P1_PktRx");
		}
		scrn_printf(row++, 1, " %19lu%30lu", s1u_stats_data.tx_cnt, sgi_stats_data.rx_cnt);
	} else { /* RESPONDER or RESPONDER_AS_REFLETOR */
		scrn_printf(row++, 1, "PCI:: %s = P0", resp_host_pci);
		/* empty row */
		row++;
		scrn_printf(row++, 1, "help:: start 0 --> start traffic on P0");
		scrn_printf(row++, 1, "       stp     --> stop traffic on P0");
		scrn_printf(row++, 1, "       quit    --> exit application");
		/* empty row */
		row++;
		uint16_t rx_cnt = pktgen.l2p->ports[0].rx_qid;
  		uint16_t tx_cnt = pktgen.l2p->ports[0].tx_qid;;
		memset(&sgi_stats_data, 0, sizeof(sgi_stats_data));
        uint8_t i = 0;
  		for (i = 0; i < rx_cnt; ++i) {
    		sgi_stats_data.rx_cnt += pid_rx_tx_cnts[0].rx_qid_cnts[i];
  		}
  		for (i = 0; i < tx_cnt; ++i) {
    		sgi_stats_data.tx_cnt += pid_rx_tx_cnts[0].tx_qid_cnts[i];
  		}
		scrn_printf(row++, 1, "%20s%9s%20s", "AS_PktRx", "||", "AS_PktTx");
		scrn_printf(row++, 1, " %19lu%9s%20lu", sgi_stats_data.rx_cnt, "||", sgi_stats_data.tx_cnt);
	}	
	pktgen_display_set_color("stats.stat.values");
	pktgen.last_row = ++row;
	display_dashline(pktgen.last_row+1);
	scrn_eol();
	pktgen_display_set_color(NULL);

	pktgen.flags &= ~PRINT_LABELS_FLAG;
	scrn_eol();
}
#define LINK_RETRY  16
/**************************************************************************//**
 *
 * pktgen_get_link_status - Get the port link status.
 *
 * DESCRIPTION
 * Try to get the link status of a port. The <wait> flag if set tells the
 * routine to try and wait for the link status for 3 seconds. If the <wait> flag
 * is zero the try three times to get a link status if the link is not up.
 *
 * RETURNS: N/A
 *
 * SEE ALSO:
 */
void
pktgen_get_link_status(port_info_t *info, int pid, int wait) {
	int i;

	/* get link status */
	for (i = 0; i < LINK_RETRY; i++) {
		memset(&info->link, 0, sizeof(info->link));
		rte_eth_link_get_nowait(pid, &info->link);
		if (info->link.link_status)
			return;
		if (!wait)
			break;
		rte_delay_us(250000);
	}
	/* Setup a few default values to prevent problems later. */
#if RTE_VERSION >= RTE_VERSION_NUM(17,2,0,0)
	info->link.link_speed   = ETH_SPEED_NUM_10G;
#else
	info->link.link_speed   = 10000;
#endif
	info->link.link_duplex  = ETH_LINK_FULL_DUPLEX;
}

/**************************************************************************//**
 *
 * pktgen_process_stats - Process statistics for all ports on timer1
 *
 * DESCRIPTION
 * When timer1 callback happens then process all of the port statistics.
 *
 * RETURNS: N/A
 *
 * SEE ALSO:
 */

void
pktgen_process_stats(struct rte_timer *tim __rte_unused, void *arg __rte_unused)
{
	unsigned int pid;
	struct rte_eth_stats stats, *rate, *init, *prev;
	port_info_t *info;
	static unsigned int counter = 0;

	counter++;
	if (pktgen.flags & BLINK_PORTS_FLAG) {
		for (pid = 0; pid < pktgen.nb_ports; pid++) {
			if ( (pktgen.blinklist & (1ULL << pid)) == 0)
				continue;

			if (counter & 1)
				rte_eth_led_on(pid);
			else
				rte_eth_led_off(pid);
		}
    }
	for (pid = 0; pid < pktgen.nb_ports; pid++) {
		info = &pktgen.info[pid];

		memset(&stats, 0, sizeof(stats));
		rte_eth_stats_get(pid, &stats);

		init = &info->init_stats;
		rate = &info->rate_stats;
		prev = &info->prev_stats;

		/* Normalize counts to the initial state, used for clearing statistics */
		stats.ipackets  -= init->ipackets;
		stats.opackets  -= init->opackets;
		stats.ibytes    -= init->ibytes;
		stats.obytes    -= init->obytes;
		stats.ierrors   -= init->ierrors;
		stats.oerrors   -= init->oerrors;
		stats.imissed   -= init->imissed;
		stats.rx_nombuf -= init->rx_nombuf;

#if RTE_VERSION < RTE_VERSION_NUM(2, 2, 0, 0)
		stats.ibadcrc   -= init->ibadcrc;
		stats.ibadlen   -= init->ibadlen;
#endif
#if RTE_VERSION < RTE_VERSION_NUM(16, 4, 0, 0)
		stats.imcasts   -= init->imcasts;
#endif

		rate->ipackets   = stats.ipackets - prev->ipackets;
		rate->opackets   = stats.opackets - prev->opackets;
		rate->ibytes     = stats.ibytes - prev->ibytes;
		rate->obytes     = stats.obytes - prev->obytes;
		rate->ierrors    = stats.ierrors - prev->ierrors;
		rate->oerrors    = stats.oerrors - prev->oerrors;
		rate->imissed    = stats.imissed - prev->imissed;
		rate->rx_nombuf  = stats.rx_nombuf - prev->rx_nombuf;

#if RTE_VERSION < RTE_VERSION_NUM(2, 2, 0, 0)
		rate->ibadcrc    = stats.ibadcrc - prev->ibadcrc;
		rate->ibadlen    = stats.ibadlen - prev->ibadlen;
#endif
#if RTE_VERSION < RTE_VERSION_NUM(16, 4, 0, 0)
		rate->imcasts    = stats.imcasts - prev->imcasts;
#endif

		if (rate->ipackets > info->max_ipackets)
			info->max_ipackets = rate->ipackets;
		if (rate->opackets > info->max_opackets)
			info->max_opackets = rate->opackets;

		/* Use structure move to copy the data. */
		*prev = *(struct rte_eth_stats *)&stats;
	}
}

/***************************************************************************/

void
pktgen_page_phys_stats(void)
{
    unsigned int pid, col, row;
    struct rte_eth_stats stats, *s, *r;
    struct ether_addr ethaddr;
    char buff[32], mac_buf[32];

    s = &stats;
    memset(s, 0, sizeof(struct rte_eth_stats));

    pktgen_display_set_color("top.page");
    display_topline("<Real Port Stats Page>");

    row = 3;
    col = 1;
	pktgen_display_set_color("stats.port.status");
	scrn_printf(row++, col, "Port Name");
    pktgen_display_set_color("stats.stat.label");
    for (pid = 0; pid < pktgen.nb_ports; pid++) {
        snprintf(buff, sizeof(buff), "%2d-%s", pid, rte_eth_devices[pid].data->name);
        scrn_printf(row++, col, "%-*s", COLUMN_WIDTH_0 - 4, buff);
    }

    row = 4;
    /* Display the colon after the row label. */
    pktgen_display_set_color("stats.colon");
    for (pid = 0; pid < pktgen.nb_ports; pid++)
        scrn_printf(row++, COLUMN_WIDTH_0 - 4, ":");

    display_dashline(++row);

    row = 3;
    col = COLUMN_WIDTH_0 - 3;
	pktgen_display_set_color("stats.port.status");
    scrn_printf(row++, col, "%*s", COLUMN_WIDTH_3, "Pkts Rx/Tx");
    pktgen_display_set_color("stats.stat.values");
    for (pid = 0; pid < pktgen.nb_ports; pid++) {

        rte_eth_stats_get(pid, &stats);

        snprintf(buff, sizeof(buff), "%lu/%lu", s->ipackets, s->opackets);

        /* Total Rx/Tx */
        scrn_printf(row++, col, "%*s", COLUMN_WIDTH_3, buff);
    }

    row = 3;
    col = (COLUMN_WIDTH_0 + COLUMN_WIDTH_3) - 3;
	pktgen_display_set_color("stats.port.status");
    scrn_printf(row++, col, "%*s", COLUMN_WIDTH_3, "Rx Errors/Missed");
    pktgen_display_set_color("stats.stat.values");
    for (pid = 0; pid < pktgen.nb_ports; pid++) {

        rte_eth_stats_get(pid, &stats);

        snprintf(buff, sizeof(buff), "%lu/%lu", s->ierrors, s->imissed);

        scrn_printf(row++, col, "%*s", COLUMN_WIDTH_3, buff);
    }

    row = 3;
    col = (COLUMN_WIDTH_0 + (COLUMN_WIDTH_3 * 2)) - 3;
	pktgen_display_set_color("stats.port.status");
    scrn_printf(row++, col, "%*s", COLUMN_WIDTH_3, "Rate Rx/Tx");
    pktgen_display_set_color("stats.stat.values");

    for (pid = 0; pid < pktgen.nb_ports; pid++) {

        rte_eth_stats_get(pid, &stats);

        r = &pktgen.info[pid].rate_stats;
        snprintf(buff, sizeof(buff), "%lu/%lu", r->ipackets, r->opackets);

        scrn_printf(row++, col, "%*s", COLUMN_WIDTH_3, buff);
    }
    row = 3;
    col = (COLUMN_WIDTH_0 + (COLUMN_WIDTH_3 * 3)) - 3;
	pktgen_display_set_color("stats.port.status");
    scrn_printf(row++, col, "%*s", COLUMN_WIDTH_3, "MAC Address");
    pktgen_display_set_color(NULL);
    for (pid = 0; pid < pktgen.nb_ports; pid++) {

        rte_eth_macaddr_get(pid, &ethaddr);

        ether_format_addr(mac_buf, sizeof(mac_buf), &ethaddr);
        snprintf(buff, sizeof(buff), "%s", mac_buf);
        scrn_printf(row++, col, "%*s", COLUMN_WIDTH_3, buff);
    }

    pktgen_display_set_color(NULL);
    scrn_eol();
}
