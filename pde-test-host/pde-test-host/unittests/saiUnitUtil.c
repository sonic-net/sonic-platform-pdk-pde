/*********************************************************************
 *
 * Broadcom Proprietary and Confidential. (C) Copyright 2016-2018 Broadcom.
 * All rights reserved.
 *
 **********************************************************************
 *
 * @filename  saiUnitUtil.c
 *
 * @purpose   This file contains utility functions for unit tests
 *
 * @component unit test
 *
 * @comments
 *
 * @create    20 Jan 2013
 *
 * @end
 *
 **********************************************************************/
#include "saiUnitUtil.h"

static sai_object_id_t ports[_BRCM_SAI_MAX_PORTS];
static sai_object_list_t port_list = { _BRCM_SAI_MAX_PORTS, ports };
/* Bridge Ports */
static sai_object_id_t b_ports[_BRCM_SAI_MAX_PORTS];
static sai_object_list_t b_port_list = { _BRCM_SAI_MAX_PORTS, b_ports };
bool switch_mac_init = FALSE;
int stdoutBackupFd;
FILE *nullOut = NULL;

void saiHideConsoleOutput(void)
{
    /* Open re-directed stdout */
    stdoutBackupFd = dup(fileno(stdout));
    fflush(stdout);
    nullOut = fopen("/dev/null", "w");
    dup2(fileno(nullOut), fileno(stdout));
}

void saiShowConsoleOutput(void)
{
    /* Close re-directed stdout */
    fflush(stdout);
    fclose(nullOut);

    /* Restore stdout */
    dup2(stdoutBackupFd, fileno(stdout));
    close(stdoutBackupFd);
}

int 
saiUnitGetSwitchAttributes(uint32_t attr_count, sai_attribute_t *switch_attr)
{
   sai_status_t rv = SAI_STATUS_SUCCESS;

   rv = sai_switch_apis->get_switch_attribute(0, attr_count, switch_attr);
   if (SAI_STATUS_SUCCESS != rv)
   {
       return SAI_STATUS_FAILURE;
   }

   return rv;
}

int 
saiUnitSetSwitchAttribute(sai_attribute_t *switch_attr)
{
   sai_status_t rv = SAI_STATUS_SUCCESS;
   sai_mac_t null_mac = { 0 };
   sai_mac_t default_switch_mac = { 0x00, 0x05, 0x06, 0x08, 0x05, 0x00 };

   if (SAI_SWITCH_ATTR_SRC_MAC_ADDRESS == switch_attr->id)
   {
       if (TRUE == switch_mac_init)
       {
           return rv;
       }
       if (0 == strncmp((const char *) switch_attr->value.mac, 
                        (const char *) null_mac,
                        sizeof(switch_attr->value.mac)))
       {
           memcpy(switch_attr->value.mac, default_switch_mac, 
                      sizeof(switch_attr->value.mac));
       }
       rv = sai_switch_apis->set_switch_attribute(0, switch_attr);
       if (rv == SAI_STATUS_SUCCESS || rv == SAI_STATUS_NOT_SUPPORTED)
       {
           return SAI_STATUS_SUCCESS;//As we might have already set during CUnit _setup_config
       }
       else{
	   return rv;
       }
   }

   rv = sai_switch_apis->set_switch_attribute(0, switch_attr);
   if (SAI_STATUS_SUCCESS != rv)
   {
       return SAI_STATUS_FAILURE;
   }

   if (SAI_SWITCH_ATTR_SRC_MAC_ADDRESS == switch_attr->id)
   {
       switch_mac_init = TRUE;
   }

   return rv;
}

int 
saiUnitSetPortAttribute(sai_object_id_t port_id, sai_attribute_t *port_attr)
{
   sai_status_t rv = SAI_STATUS_SUCCESS;

   rv = sai_port_apis->set_port_attribute(port_id, port_attr);
   if (SAI_STATUS_SUCCESS != rv)
   {
       return SAI_STATUS_FAILURE;
   }

   return rv;
}

int 
saiUnitGetPortAttribute(sai_object_id_t port_id, sai_attribute_t *port_attr)
{
   sai_status_t rv = SAI_STATUS_SUCCESS;

   rv = sai_port_apis->get_port_attribute(port_id, 1, port_attr);
   if (SAI_STATUS_SUCCESS != rv)
   {
       return SAI_STATUS_FAILURE;
   }

   return rv;
}


int saiUnitSaiIndexGet(int bcmport)
{
    int i, port_count;
    saiUnitGetSwitchPortCount(&port_count);
    for (i=0;i<port_list.count;i++)
    {
        if ((port_list.list[i] & 0xffff) == bcmport)
            return i;
    }
    return -1;
}

int 
saiUnitGetSwitchPortCount(int *port_count)
{
   sai_status_t rv = SAI_STATUS_SUCCESS;
   static bool ports_inited = FALSE;
   sai_attribute_t switch_attr = { SAI_SWITCH_ATTR_PORT_LIST, .value.objlist = b_port_list };
   sai_attribute_t b_attr = { SAI_BRIDGE_ATTR_PORT_LIST, .value.objlist = b_port_list };
   sai_attribute_t bp_attr = { SAI_BRIDGE_PORT_ATTR_PORT_ID };
   int p;

   if (FALSE == ports_inited)
   {
       rv = sai_switch_apis->get_switch_attribute(0, 1, &switch_attr);
       if (SAI_STATUS_SUCCESS != rv)
       {
           return SAI_STATUS_FAILURE;
       }
       switch_attr.id = SAI_SWITCH_ATTR_DEFAULT_1Q_BRIDGE_ID;
       rv = sai_switch_apis->get_switch_attribute(0, 1, &switch_attr);
       if (SAI_STATUS_SUCCESS != rv)
       {
           return SAI_STATUS_FAILURE;
       }
       rv = sai_bridge_apis->get_bridge_attribute(switch_attr.value.oid, 1, &b_attr);
       if (SAI_STATUS_SUCCESS != rv)
       {
           return SAI_STATUS_FAILURE;
       }
       else
       {
           port_list.count = b_port_list.count = b_attr.value.objlist.count;
           for (p=0; p<b_port_list.count; p++)
           {
               bp_attr.value.oid = b_port_list.list[p];
               rv = sai_bridge_apis->get_bridge_port_attribute(bp_attr.value.oid, 1, &bp_attr);
               if (SAI_STATUS_SUCCESS != rv)
               {
                   return SAI_STATUS_FAILURE;
               }
               else
               {
                   port_list.list[p] = bp_attr.value.oid;
               }
           }
       }
       ports_inited = TRUE;
   }
   *port_count = port_list.count;

   return rv;
}

sai_object_id_t
saiUnitGetPortObject(int index)
{
   if (index > port_list.count)
   {
       return (sai_object_id_t) NULL;
   }
   else
   {
       return port_list.list[index];
   }
}

int
saiUnitGetLogicalPort(int index)
{
   if (index > port_list.count)
   {
       return (sai_object_id_t) NULL;
   }
   else
   {
       return index + 1;
   }
}

sai_object_id_t
saiUnitGetBridgePortObject(int index)
{
   if (index > b_port_list.count)
   {
       return (sai_object_id_t) NULL;
   }
   else
   {
       return b_port_list.list[index];
   }
}

int saiUnitValidateStatsRx(sai_object_id_t port_obj, int expected)
{
   int rv = SAI_STATUS_SUCCESS;
   uint64_t counters[1] = { 0 };
   sai_port_stat_t cids[1] = 
   {
       SAI_PORT_STAT_IF_IN_UCAST_PKTS
   };

   rv = sai_port_apis->get_port_stats(port_obj, 1, cids, &counters[0]);
   if (SAI_STATUS_SUCCESS != rv)
   {
       return rv;
   }

   if (expected != counters[0])
   {
       return SAI_STATUS_FAILURE;
   }
   return rv;
}

int saiUnitValidateStats(int port, int expected)
{
   int rv = SAI_STATUS_SUCCESS;
   uint64_t counters[1] = { 0 };
   sai_object_id_t port_obj;
   sai_port_stat_t cids[1] = 
   {
       SAI_PORT_STAT_IF_OUT_UCAST_PKTS
   };

   port_obj = saiUnitGetPortObject(port);
   rv = sai_port_apis->get_port_stats(port_obj, 1, cids, &counters[0]);
   if (SAI_STATUS_SUCCESS != rv)
   {
       return rv;
   }

   if (expected != counters[0])
   {
       return SAI_STATUS_FAILURE;
   }
   return rv;
}

/* Retreive port stats */
uint64_t saiUnitGetPortStats(int port)
{
   int rv = SAI_STATUS_SUCCESS;
   uint64_t counters[1] = { 0 };
   sai_object_id_t port_obj;
   sai_port_stat_t cids[1] = 
   {
       SAI_PORT_STAT_IF_OUT_UCAST_PKTS
   };

   port_obj = saiUnitGetPortObject(port);
   rv = sai_port_apis->get_port_stats(port_obj, 1, cids, &counters[0]);
   if (rv != SAI_STATUS_SUCCESS){
       printf("Failed to get port stats.Error is %d\n", rv);
       return -1;
   }  
   return counters[0]; 
}

/*Genric function to send the Frames usning driver_process_command*/
sai_status_t saiUnitSendFramesGeneric(char *commandBuf, char *port){
     /* Hide console output */ 
     saiHideConsoleOutput();
     sai_driver_process_command(commandBuf);
     memset(commandBuf,'\0',MAX_FRAME_SIZE);
     sprintf(commandBuf,"tx 1 pbm=%s data=\'\'",port);
     sai_driver_process_command(commandBuf);
     /* Show console output */
     saiShowConsoleOutput();
     return SAI_STATUS_SUCCESS;
}

int saiUnitFormDHCPv6Packet(char *port,char *commandBuf){
        /* To From DHCPV6 generic packet */
        sprintf(commandBuf,"tx 1 pbm=%s data=\'000000000004000AF7819F9786dd6000000000361140"
                           "1111111111111111111111111111111133333333333333333333333333333333"
                           "022302220036423511121314151617181920\'",port);
        return SAI_STATUS_SUCCESS;

}


/* This is a specific routine to check queue stats for Service pool-0 destined traffic 
 * Service pool 1 destined traffic has internal priority set to 7
 * */ 
int saiUnitValidateQueueStatsCount(int port, sai_object_id_t queue, int expected)
{
   int rv = SAI_STATUS_SUCCESS;
   uint64_t counters[1] = { 0 };
   sai_queue_stat_t cids[1] =
   {
       SAI_QUEUE_STAT_PACKETS
   };
   rv = sai_qos_queue_apis->get_queue_stats(qid[port][queue], 1, cids, &counters[0]);
   if (SAI_STATUS_SUCCESS != rv)
   {
       return rv;
   }

   if (expected != counters[0])
   {
       return SAI_STATUS_FAILURE;
   }
   return rv;
}
/*Do clear stats*/
sai_status_t saiUnitClearStats(){
   int rv=SAI_STATUS_FAILURE;
   rv = sai_driver_process_command("clear c");
   return rv;
}

sai_status_t saiUnitSetLogLevel(sai_log_level_t level)
{
    sai_status_t rv;
    sai_api_t api;

    for (api = SAI_API_UNSPECIFIED+1; api <= _SAI_API_ID_MAX; api++)
    {
        rv = sai_log_set(api, level);
        if (rv != SAI_STATUS_SUCCESS)
        {
            return rv;
        }
    }
    return SAI_STATUS_SUCCESS;
}

int saiUnitTrafficGetPortFromConfig(int index){
        if(index > num_ports_in_test)
                return -1;
        else
           return pde_traffic_test_port_list[index];
}

int saiUnitGetPortFromConfig(int index){
        if(index > num_ports_in_test)
                return -1;
        else
           return sai_port_index_list[index];
}


int saiUnitGetLogicalPortFromConfig(int index){
        if(index > num_ports_in_test)
                return -1;
        else
           return bcm_port_list[index];
}

char* saiUnitGetBcmPortName(int index){
        if(index > num_ports_in_test)
                return NULL;
        else
                return bcm_port_list_names[index];
}

sai_status_t saiUnitSetQueueAttribute(int port, int queue, sai_attribute_t *qattr){
        sai_status_t rv = SAI_STATUS_SUCCESS;
        rv = sai_qos_queue_apis->set_queue_attribute(qid[port][queue], qattr);
        return rv;
}

sai_status_t saiUnitSetRIFAttribute(sai_object_id_t rif_obj, sai_attribute_t *rattr){
        sai_status_t rv = SAI_STATUS_SUCCESS;
        rv = sai_router_intf_apis->set_router_interface_attribute(rif_obj, rattr);
        return rv;
}

int saiUnitCheckObjInList(sai_object_id_t obj, sai_object_list_t objlist){
   int i = 0;
   for(i = 0;i < objlist.count; i++){
       if(obj == objlist.list[i])
          return SAI_STATUS_SUCCESS;
   }
   return SAI_STATUS_FAILURE;
}

/* Get the port list and bridge port list after warmboot */
sai_status_t saiUnitGetSwitchPortCountAfterWB(){
   sai_status_t rv = SAI_STATUS_SUCCESS;

   port_list.count = _BRCM_SAI_MAX_PORTS;//Reset them to max
   port_list.list = ports;
   b_port_list.count = _BRCM_SAI_MAX_PORTS;
   b_port_list.list = b_ports;

   sai_attribute_t switch_attr = { SAI_SWITCH_ATTR_PORT_LIST, .value.objlist = b_port_list };
   sai_attribute_t b_attr = { SAI_BRIDGE_ATTR_PORT_LIST, .value.objlist = b_port_list };
   sai_attribute_t bp_attr = { SAI_BRIDGE_PORT_ATTR_PORT_ID };
   int p;
   rv = sai_switch_apis->get_switch_attribute(0, 1, &switch_attr);
   if (SAI_STATUS_SUCCESS != rv)
   {
       return SAI_STATUS_FAILURE;
   }
   switch_attr.id = SAI_SWITCH_ATTR_DEFAULT_1Q_BRIDGE_ID;
   rv = sai_switch_apis->get_switch_attribute(0, 1, &switch_attr);
   if (SAI_STATUS_SUCCESS != rv)
   {
       return SAI_STATUS_FAILURE;
   }
   rv = sai_bridge_apis->get_bridge_attribute(switch_attr.value.oid, 1, &b_attr);
   if (SAI_STATUS_SUCCESS != rv)
   {
       return SAI_STATUS_FAILURE;
   }
   else
   {
       port_list.count = b_port_list.count = b_attr.value.objlist.count;
       for (p=0; p<b_port_list.count; p++)
       {
           bp_attr.value.oid = b_port_list.list[p];
           rv = sai_bridge_apis->get_bridge_port_attribute(bp_attr.value.oid, 1, &bp_attr);
           if (SAI_STATUS_SUCCESS != rv)
           {
               return SAI_STATUS_FAILURE;
           }
           else
           {
               port_list.list[p] = bp_attr.value.oid;
           }
       }
   }
   //*port_count = port_list.count;
   return rv;
}

sai_status_t saiUnitRemovePortsFromDefaultVLAN(int max_ports)
{
    sai_status_t rv = SAI_STATUS_SUCCESS;
    int i;
    sai_object_id_t *vlan_port_objs;
    sai_attribute_t switch_attr, vattr;
    sai_attribute_t port_list_vlan[] =
    {
        { SAI_VLAN_MEMBER_ATTR_VLAN_ID },
        { SAI_VLAN_MEMBER_ATTR_BRIDGE_PORT_ID },
        { SAI_VLAN_MEMBER_ATTR_VLAN_TAGGING_MODE }
    };

    switch_attr.id = SAI_SWITCH_ATTR_DEFAULT_VLAN_ID;
    rv = saiUnitGetSwitchAttributes(1, &switch_attr);
    if (SAI_STATUS_SUCCESS != rv)
    {
        return rv;
    }

    vlan_port_objs = (sai_object_id_t *) calloc(max_ports, sizeof(sai_object_id_t));
    vattr.id = SAI_VLAN_ATTR_MEMBER_LIST;
    vattr.value.objlist.count = max_ports;
    vattr.value.objlist.list = vlan_port_objs;
    rv = sai_vlan_apis->get_vlan_attribute(switch_attr.value.oid, 1, &vattr);
    if (SAI_STATUS_SUCCESS != rv)
    {
        free(vlan_port_objs);
        return rv;
    }
    port_list_vlan[0].value.oid = switch_attr.value.oid;
    for (i = 0; i < max_ports; i++)
    {
        /* Remove port from default vlan */
        rv = sai_vlan_apis->remove_vlan_member(vlan_port_objs[i]);
        if (SAI_STATUS_SUCCESS != rv)
        {
            free(vlan_port_objs);
            return rv;
        }
    }
    free(vlan_port_objs);
    return rv;
}

sai_status_t saiUnitRestorePortsToDefaultVLAN(int max_ports)
{
    sai_status_t rv = SAI_STATUS_SUCCESS;
    int i;
    sai_object_id_t vlan_port_obj;
    sai_attribute_t switch_attr;
    sai_attribute_t port_list_vlan[] =
    {
        { SAI_VLAN_MEMBER_ATTR_VLAN_ID },
        { SAI_VLAN_MEMBER_ATTR_BRIDGE_PORT_ID },
        { SAI_VLAN_MEMBER_ATTR_VLAN_TAGGING_MODE }
    };

    switch_attr.id = SAI_SWITCH_ATTR_DEFAULT_VLAN_ID;
    rv = saiUnitGetSwitchAttributes(1, &switch_attr);
    if (SAI_STATUS_SUCCESS != rv)
    {
        return rv;
    }

    port_list_vlan[0].value.oid = switch_attr.value.oid;
    for (i = 0; i < max_ports; i++)
    {
        port_list_vlan[1].value.oid = saiUnitGetBridgePortObject(i);
        port_list_vlan[2].value.s32 =  SAI_VLAN_TAGGING_MODE_UNTAGGED;
        rv = sai_vlan_apis->create_vlan_member(&vlan_port_obj,
                                               0,
                                               COUNTOF(port_list_vlan),
                                               port_list_vlan);
    }
    return rv;
}

sai_status_t saiUnitGetLagMemberObject(sai_object_id_t *lag_mbr_obj,
                                       sai_object_id_t lag_obj,
                                       sai_object_id_t port_obj){
   lag_member_port_info *node = root_node;
   //traverse tlll we find the lag member object for the lag_obj and port_obj combination
   while (node){
          if((node->lag_obj == lag_obj) && (node->port_obj == port_obj)){
              *lag_mbr_obj = node->lag_mbr_obj;
               return SAI_STATUS_SUCCESS;
          }
          else{
              node = node->next;
          }
   } 
   return SAI_STATUS_FAILURE;
}
