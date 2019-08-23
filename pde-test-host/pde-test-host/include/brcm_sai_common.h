/*********************************************************************
 *
 * Copyright: (c) 2017 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.
 *
 *********************************************************************/

#if !defined (_BRM_SAI_COMMON)
#define _BRM_SAI_COMMON

/*
################################################################################
#                                 Public includes                              #
################################################################################
*/
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>

/*
################################################################################
#                               All internal includes                          #
################################################################################
*/
#include "types.h"

#include "brcm_sai_version.h"
#include "brcm_syncdb.h"
#include "driver_util.h"

#ifdef PRINT_TO_SYSLOG
#include <syslog.h>
#endif

/*
################################################################################
#                                   Common defines                             #
################################################################################
*/
#ifndef STATIC
#define STATIC
#endif /* !STATIC */

#ifndef TRUE
#define TRUE      1
#endif
#ifndef FALSE
#define FALSE     0
#endif
#ifndef INGRESS
#define INGRESS   0
#endif
#ifndef EGRESS
#define EGRESS    1
#endif
#ifndef MATCH
#define MATCH     0
#endif

#define COLD_BOOT 0
#define WARM_BOOT 1
#define FAST_BOOT 2

#define _BRCM_SAI_MASK_8           0xFF
#define _BRCM_SAI_MASK_16          0xFFFF
#define _BRCM_SAI_MASK_32          0xFFFFFFFF
#define _BRCM_SAI_MASK_64          0xFFFFFFFFFFFFFFFF
#define _BRCM_SAI_MASK_64_UPPER_32 0xFFFFFFFF00000000
#define _BRCM_SAI_MASK_64_LOWER_32 0x00000000FFFFFFFF
#define _BCAST_MAC_ADDR            { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }
#define _BCM_L3_HOST_AS_ROUTE      0



/*
################################################################################
#                            Platform based defines                            #
################################################################################
*/
#define _CPU_L0_NODES               1
#define _L0_NODES                   2

#define NUM_CPU_L1_NODES            2

/* This is the max so far - seen in TH3 */
#define NUM_QUEUES                  12
#define NUM_L1_NODES                10
#define NUM_L2_NODES                2

#define NUM_TH_QUEUES               10
#define NUM_TH_CPU_L0_NODES         10
#define NUM_TH_CPU_MC_QUEUES        48
#define NUM_TH_L0_NODES             10

#define NUM_TH3_CPU_L1_NODES         2
#define NUM_TH3_CPU_L0_NODES         12
#define NUM_TH3_CPU_MC_QUEUES        48
#define NUM_TH3_L0_NODES             12
#define NUM_TH3_L1_NODES             2


#define NUM_TD2_CPU_QUEUES          8
#define NUM_TD2_CPU_L0_NODES        1
#define NUM_TD2_CPU_MC_QUEUES       16 /* h/w supports 48 but limiting based on expected usage */
#define NUM_TD2_L0_NODES            2
#define NUM_TD2_L1_PER_L0           8
#define NUM_TD2_L2_NODES            2

#define NUM_HX4_QUEUES              8
#define NUM_HX4_L1_NODES            8
#define NUM_HX4_CPU_QUEUES          8
#define NUM_HX4_CPU_L0_NODES        1
#define NUM_HX4_CPU_MC_QUEUES       16 /* h/w supports 48 but limiting based on expected usage */
#define NUM_HX4_L0_NODES            1
#define NUM_HX4_L1_PER_L0           8

#define NUM_TH3_PFC_PRIORITIES      SOC_MMU_CFG_PRI_GROUP_MAX

#define TOTAL_SCHED_NODES_PER_PORT  13 /* This is the current max but will change based on device type */

#define _BRCM_SAI_MAX_HIERARCHY_TD2 3
#define _BRCM_SAI_MAX_HIERARCHY_TH  2

#ifndef MAX
#define MAX(a, b)                   (a > b ? a : b)
#endif
#define NUM_L0_NODES                MAX(NUM_TH3_CPU_L0_NODES, NUM_TD2_CPU_L0_NODES)
#define NUM_CPU_MC_QUEUES           MAX(NUM_TH_CPU_MC_QUEUES, NUM_TD2_CPU_MC_QUEUES)

#define _BRCM_SAI_MAX_ECMP_MEMBERS  16384
#define _BRCM_HX4_MAX_ECMP_MEMBERS  4096
#define _BRCM_HX4_MAX_FDB_ENTRIES   32768
#define _BRCM_TD3_MAX_ECMP_MEMBERS  32768

/* Note: This is chip specific - best to have a SDK/SOC api to retreive this from */
#define TOTAL_TD2_MMU_BUFFER_SIZE (59941*208/1000) /* In KB */
#define TOTAL_TH_MMU_BUFFER_SIZE  (16*1024*1024)   /* In KB */


#if defined(BCM_TRIDENT3_SUPPORT)
#define TOTAL_TD3_MMU_BUFFER_SIZE  (_TD3_MMU_TOTAL_CELLS * 256 / 1000 ) /* In KB */
#else
#define TOTAL_TD3_MMU_BUFFER_SIZE 0
#endif

#if defined(BCM_TOMAHAWK2_SUPPORT)
#define TOTAL_TH2_MMU_BUFFER_SIZE  (_TH2_MMU_TOTAL_CELLS * 208 / 1000 ) /* In KB */
#else
#define TOTAL_TH2_MMU_BUFFER_SIZE 0
#endif

#if defined(BCM_TOMAHAWK3_SUPPORT)
#define TOTAL_TH3_MMU_BUFFER_SIZE  (_TH3_MMU_TOTAL_CELLS_PER_ITM * 2 * 254 / 1000 ) /* In KB */
#define _BRCM_TH3_IFP_TCAM_SMALL_SLICE_SIZE 256
#else
#define TOTAL_TH3_MMU_BUFFER_SIZE 0
#define _BRCM_TH3_IFP_TCAM_SMALL_SLICE_SIZE 0
#endif

/*
################################################################################
#                            Common internal defines                           #
################################################################################
*/
#define BRCM_SAI_API_ID_MIN                SAI_API_UNSPECIFIED+1
#define BRCM_SAI_API_ID_MAX                SAI_API_BRIDGE /* FIXME: Check and Update */

#define _BRCM_SAI_CMD_BUFF_SIZE            512
#define _BRCM_SAI_FILE_PATH_BUFF_SIZE      256

#define _BRCM_SAI_VR_MAX_VID               4094
#define _BRCM_SAI_VR_DEFAULT_TTL           0   /* This is the MCAST ttl threshold */
#define _BRCM_SAI_MAX_PORTS                220 /* Max logical port number used by underlying SDK/driver (TH) */
#define _BRCM_SAI_MAX_HOSTIF               128
#define _BRCM_SAI_MAX_FILTERS_PER_INTF     32
#define _BRCM_SAI_MAX_TUNNELS              (1024+1)
#define _BRCM_SAI_MAX_TUNNEL_RIFS          (_BRCM_SAI_MAX_TUNNELS*2)
#define _BRCM_SAI_MAX_UDF_GROUPS           64
#define _BRCM_SAI_MAX_HASH                 32
#define _BRCM_SAI_MAX_TRAP_GROUPS          64
#define _BRCM_SAI_MAX_TRAPS                128
#define _BRCM_SAI_MAX_HOSTIF_TABLE_ENTRIES 128


#define BRCM_SAI_GET_OBJ_VAL(type, var) ((type)var)



#endif /* _BRM_SAI_COMMON */
