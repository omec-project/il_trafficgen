/*-
 * Copyright (c) <2010-2017>, Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/* Created 2010 by Keith Wiles @ intel.com */

#ifndef _PKTGEN_CONSTANTS_H_
#define _PKTGEN_CONSTANTS_H_

#include <rte_mbuf.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	DEFAULT_PKT_BURST       = 64,	/* Increasing this number consumes memory very fast */
#ifdef RTE_LIBRTE_VMXNET3_PMD
//	DEFAULT_RX_DESC         = (DEFAULT_PKT_BURST * 32),		/* DEFAULT_RX_DESC = 2048 */
	DEFAULT_RX_DESC         = (DEFAULT_PKT_BURST * 64),		/* DEFAULT_RX_DESC = 4096 */
	DEFAULT_TX_DESC         = DEFAULT_RX_DESC * 1,			/* DEFAULT_TX_DESC = 2048 */
#else
	DEFAULT_RX_DESC         = (DEFAULT_PKT_BURST * 8),
	DEFAULT_TX_DESC         = DEFAULT_RX_DESC * 2,
#endif

	MBUF_CACHE_SIZE         = 512,								/* MBUF_CACHE_SIZE = 512 */

	/* number of buffers to support per port: MAX_MBUFS_PER_PORT = 4,096 */
	MAX_MBUFS_PER_PORT      = ((DEFAULT_TX_DESC * 2) > (1.5 * MBUF_CACHE_SIZE) ? \
								(DEFAULT_TX_DESC*2) : (2 * MBUF_CACHE_SIZE)),

	MAX_SPECIAL_MBUFS       = 64,

	DEFAULT_PRIV_SIZE       = 0,
	/* See: http://dpdk.org/dev/patchwork/patch/4479/ */
	/* MBUF_SIZE = 2048 +128 = 2176 */
	MBUF_SIZE		= RTE_MBUF_DEFAULT_BUF_SIZE + DEFAULT_PRIV_SIZE,

	NUM_Q                   = 8,						/**< Number of cores per port. */
};

#ifdef __cplusplus
}
#endif

#endif  /* _PKTGEN_CONSTANTS_H_ */
