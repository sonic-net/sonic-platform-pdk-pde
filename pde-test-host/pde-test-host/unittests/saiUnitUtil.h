/*********************************************************************
 *
 * Broadcom Proprietary and Confidential. (C) Copyright 2016-2018 Broadcom.
 * All rights reserved.
 *
 **********************************************************************
 *
 * @filename  saiUnitUtil.h
 *
 * @purpose   This file contains the definitions for utility functions
 *            used within the unit tests
 *
 * @component unit test
 *
 * @comments
 *
 * @create    20 Jan 2017
 *
 * @end
 *
 **********************************************************************/
#ifndef INCLUDE_SAI_UT_UTIL_H
#define INCLUDE_SAI_UT_UTIL_H

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include "sai.h"
#include "saiswitch.h"
#include "brcm_sai_common.h"
#include "saiUnitTest.h"
#include "CUnit.h"
#include "Basic.h"
#include "Console.h"
#include "Automated.h"
extern sai_switch_api_t *sai_switch_apis;
extern sai_port_api_t *sai_port_apis;
extern sai_queue_api_t *sai_qos_queue_apis;
extern sai_vlan_api_t *sai_vlan_apis;
extern sai_fdb_api_t *sai_fdb_apis;
extern sai_virtual_router_api_t *sai_router_apis;
extern sai_router_interface_api_t *sai_router_intf_apis;
extern sai_neighbor_api_t *sai_neighbor_apis;
extern sai_next_hop_api_t *sai_next_hop_apis;
extern sai_next_hop_group_api_t *sai_next_hop_grp_apis;
extern sai_route_api_t *sai_route_apis;
extern sai_hostif_api_t *sai_hostif_apis;
extern sai_lag_api_t *sai_lag_apis;
extern sai_tunnel_api_t *sai_tunnel_apis;
extern sai_bridge_api_t *sai_bridge_apis;
extern sai_policer_api_t *sai_policer_apis;
extern sai_acl_api_t *sai_acl_apis;
extern sai_mirror_api_t *sai_mirror_apis;
extern sai_qos_map_api_t *sai_qos_map_apis;
extern sai_udf_api_t *sai_udf_apis;
extern sai_hash_api_t *sai_hash_apis;
extern sai_wred_api_t *sai_wred_apis;
extern sai_scheduler_api_t *sai_qos_scheduler_apis;
extern sai_scheduler_group_api_t *sai_qos_sched_group_apis;
extern sai_buffer_api_t *sai_buffer_apis;
extern int sai_driver_shell();
extern int sai_driver_process_command(char *commandBuf);
int saiUnitGetSwitchPortCount(int *port_count);
sai_object_id_t saiUnitGetPortObject(int index);
int saiUnitGetLogicalPort(int index);
sai_object_id_t saiUnitGetBridgePortObject(int index);
int saiUnitGetSwitchAttributes(uint32_t attr_count,
                               sai_attribute_t *switch_attr);
int saiUnitSetSwitchAttribute(sai_attribute_t *switch_attr);
int saiUnitSetPortAttribute(sai_object_id_t port_id, 
                            sai_attribute_t *port_attr);
int saiUnitGetPortAttribute(sai_object_id_t port_id, 
                            sai_attribute_t *port_attr);
void saiHideConsoleOutput(void);
void saiShowConsoleOutput(void);
int saiUnitValidateStats(int port, int expected);
int saiUnitValidateStatsRx(sai_object_id_t port_obj, int expected);
sai_status_t saiUnitSendFramesGeneric(char *commandbuf, char *port);/* Generic routine to send frames */
sai_status_t saiUnitValidateQueueStats(int port, sai_object_id_t queue_id, int expected);
sai_status_t saiUnitValidateQueueStatsCount(int port, sai_object_id_t queue_id, int expected);
int saiUnitFormDHCPv6Packet(char *port,char *commandBuf);
sai_status_t saiUnitClearStats();
sai_status_t saiUnitSetLogLevel(sai_log_level_t level);
int saiUnitGetPortFromConfig(int index);
int saiUnitGetLogicalPortFromConfig(int index);
char* saiUnitGetBcmPortName(int index);
sai_status_t saiUnitSetQueueAttribute(int port, int queue, sai_attribute_t *queue_attr);
sai_status_t saiUnitSetRIFAttribute(sai_object_id_t rif_obj, sai_attribute_t *rattr);
uint64_t saiUnitGetPortStats(int port);
int saiUnitCheckObjInList(sai_object_id_t obj, sai_object_list_t objlist);
sai_status_t saiUnitRemovePortsFromDefaultVLAN(int max_ports);
sai_status_t saiUnitRestorePortsToDefaultVLAN(int max_ports);
sai_status_t saiUnitGetLagMemberObject(sai_object_id_t *lag_mbr_obj,
                                       sai_object_id_t lag_obj,
                                       sai_object_id_t port_obj);
extern int saiUnitAddMemberToLagList(sai_object_id_t lag_obj, sai_object_id_t port_obj);
extern int saiUnitRemoveMemberFromLagList(sai_object_id_t lag_obj, sai_object_id_t port_obj);
extern int saiUnitSaiIndexGet(int bcmport);
#define _SAI_MAX_PORTS _BRCM_SAI_MAX_PORTS
#define MAX_FRAME_SIZE 255
#define _SAI_MAX_QUEUES 64

typedef struct _lag_member_port_info{
   sai_object_id_t lag_obj;
   sai_object_id_t lag_mbr_obj;
   sai_object_id_t port_obj;
   struct _lag_member_port_info *next;
}lag_member_port_info;

/* To fetch test config data */
extern int num_ports_in_test;
extern int num_ports_in_traffic_test;
extern int bcm_port_list[_SAI_MAX_PORTS];
extern int sai_port_index_list[_SAI_MAX_PORTS];
extern char* bcm_port_list_names[_SAI_MAX_PORTS];
extern int bcm_loopback_port_list[_SAI_MAX_PORTS];
extern int pde_traffic_test_port_list[_SAI_MAX_PORTS];
extern char *mmu_init_config;
extern int capture_enable;
extern int warmboot_enable;
extern sai_object_id_t qid[_SAI_MAX_PORTS][_SAI_MAX_QUEUES];
extern sai_uint32_t port_loopback_mode;
extern uint32_t sdk_shut_time;
#ifndef _SAI_API_ID_MIN
#define _SAI_API_ID_MIN SAI_API_UNSPECIFIED+1
#endif
#ifndef _SAI_API_ID_MAX
#define _SAI_API_ID_MAX SAI_API_BRIDGE
#endif
extern char* chipid;
extern lag_member_port_info *root_node;

#define CPU_PG 7
/** Intrinsic buffer descriptor.
 *
 * NOTE: This is intended to be used as a basic data type that can be
 *       passed directly between functions -- keep it small and simple.
 *       To use this as an IN/OUT or OUT value, pass a pointer to the
 *       element so that the called routine can update the 'size' field
 *       with the actual content length being output.
 *
 * NOTE: When setting the 'size' field to indicate the amount of data
 *       contained in the buffer, the conventional usage is to account
 *       for all bytes including any string termination characters
 *       (e.g. strlen()+1).
 */

typedef struct
{
  /** total buffer size (IN) / content length (OUT) */
  uint32_t                size;
  /** ptr to buffer starting location */
  char                   *pstart;
} saiBuffDesc;


#if 0 //Enable this for TH3 testing
#define SAI_PORT_INTERNAL_LOOPBACK_MODE_PHY SAI_PORT_INTERNAL_LOOPBACK_MODE_MAC
#endif

#define SAI_CUNIT_PRINTF(__verbosity, __format, ...)                       \
   do                                                                      \
   {                                                                       \
       char           buf[200];                                            \
       saiBuffDesc    message;                                             \
       int            len;                                                 \
                                                                           \
       if (saiDebugLvlGet() >= __verbosity)                                \
       {                                                                   \
           len = snprintf(buf, sizeof(buf), __format, __VA_ARGS__);        \
           if (sizeof(buf) <= len)                                         \
           {                                                               \
               buf[sizeof(buf) - 1] = 0;                                   \
               len = sizeof(buf) - 1;                                      \
           }                                                               \
                                                                           \
           message.pstart = buf;                                           \
           message.size   = len + 1;                                       \
           saiCltLogBuf(0, message);                                       \
       }                                                                   \
   } while (0)

#endif /* INCLUDE_SAI_UT_UTIL_H */

