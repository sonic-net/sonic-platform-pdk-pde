/*********************************************************************
 *
 * Broadcom Proprietary and Confidential. (C) Copyright 2016-2018 Broadcom.
 * All rights reserved.
 *
 **********************************************************************
 *
 * @filename  saiSonicPde.c
 *
 * @purpose   This file contains the unit tests for SONiC PDE
 *
 * @component unit test
 *
 * @comments
 *
 * @create
 *
 * @author  
 *
 * @end
 *
 **********************************************************************/

#include "saiUnitUtil.h"

#ifndef TEST_LOG_OFF
#define TEST_LOG(__fmt,__args...)                               \
  do { printf("%d: " __fmt, __LINE__ ,##__args); } while(0)
#else
#define TEST_LOG(...)
#endif


CU_pSuite pSonicPdeSuite;
#define MAX_MTU 256
#define TRAFFIC_FRAME_COUNT 5000

/* Frmaes to be sent to host 2.2.2.1 as dest ip */
static sai_status_t saiUnitSendFramesToHost(char *port, int numFrames)
{
	sai_status_t rv = SAI_STATUS_SUCCESS;
	char frameData[MAX_FRAME_SIZE];
	saiHideConsoleOutput();
	/* input packet */
	sprintf(frameData, "tx %d pbm=%s data=\'00445555666600000000000208004500002E0000000040FF73CD01010101"
					"02020201000102030405060708090A0B0C0D0E0F10111213141516171819E240029C\'", numFrames, port);
	rv = sai_driver_process_command(frameData);
	sleep(3);
	saiShowConsoleOutput();
	return rv;
}

/* Below is example of port_config.ini line
 
# name          lanes                 alias     index   speed
Ethernet0       65,66,67,68           Eth1      1       100000
 
*/

typedef struct _sai_port_map_entry {
	sai_object_id_t port_id;
	uint32_t total_lanes;
	uint32_t lane_list[4];
	uint32_t speed;
} sai_port_map_entry;

typedef struct _sai_port_map {
	int	total_ports;
	sai_port_map_entry port_list[_SAI_MAX_PORTS];

} sai_port_map;

sai_port_map pdeTestSaiPortMap;


typedef struct _port_config_map_entry {
    char name[20];
    uint32_t total_lanes;
    uint32_t lane_list[4];
    uint32_t speed;
    int index;
} port_config_map_entry;

typedef struct _port_config_map {
	int	total_ports;
	port_config_map_entry port_list[_SAI_MAX_PORTS];

} port_config_map;

port_config_map pdeTestPortConfigMap;


int saiSonicPdeLoadPortConfig()
{
    char name[20];
    char lanes[20];
    char alias[20];
    char hwsku[255];
    char platform[255];
    int index;
    int speed;
    int total_ports = 0;
    char filepath[255];

    size_t len = 0, num_read = 0;
    char *lineptr;
    FILE *fp;

    sprintf(filepath, "/tmp/port_config.ini", platform, hwsku);

    fp = fopen(filepath, "r");
    if (fp == NULL)
    {
        printf("FAIL: Unable to open file %s\n", filepath);
        TEST_LOG("Please ensure that port_config.ini is present in cwd\n");
        return -1;
    }
    else
    {
        printf("\nPASS: Able to open %s \n", filepath);
    }

    while (num_read = getline(&lineptr, &len, fp) != -1)
    {
        if (*lineptr == '#')
        {
            continue;
        }
        else
        {
            if (5 != sscanf(lineptr, "%s %s %s %d %d", name, lanes, alias, &index, &speed))
            {
                printf("FAIL: Invalid line %s in port_config.ini, please check syntax!", lineptr);
            }
            else
            {
                strcpy(pdeTestPortConfigMap.port_list[total_ports].name, name);
                pdeTestPortConfigMap.port_list[total_ports].speed = speed;
                pdeTestPortConfigMap.port_list[total_ports].index = index;
                pdeTestPortConfigMap.port_list[total_ports].total_lanes = sscanf(lanes, "%d,%d,%d,%d",
                        &pdeTestPortConfigMap.port_list[total_ports].lane_list[0],
                        &pdeTestPortConfigMap.port_list[total_ports].lane_list[1],
                        &pdeTestPortConfigMap.port_list[total_ports].lane_list[2],
                        &pdeTestPortConfigMap.port_list[total_ports].lane_list[3]);

                total_ports++;
            }
        }
    }
    pdeTestPortConfigMap.total_ports = total_ports;

    return total_ports;
}

int saiSonicPdeLoadSaiPortMap()
{
	int max_ports;
	int i, j;
	uint32_t lane_list[4];
	sai_status_t rv = 0;

	sai_attribute_t attr_get[_SAI_MAX_PORTS] =
	{
		{ SAI_PORT_ATTR_HW_LANE_LIST, .value.u32list.list = &lane_list[0] }
	};

	sai_attribute_t attr_get_port_speed[_SAI_MAX_PORTS] =
	{
		{ SAI_PORT_ATTR_SPEED, .value.u32 = 0x00 }
	};

	CU_ASSERT(SAI_STATUS_SUCCESS == saiUnitGetSwitchPortCount(&max_ports));

	pdeTestSaiPortMap.total_ports = max_ports;

	/* For each port, get the lane count and values as well as configured speed of port*/
	for (i = 0; i < max_ports; i++)
	{
		pdeTestSaiPortMap.port_list[i].port_id = saiUnitGetPortObject(i);

		attr_get[0].value.u32list.count = 4;
		CU_ASSERT(SAI_STATUS_SUCCESS ==
								 (rv = saiUnitGetPortAttribute(pdeTestSaiPortMap.port_list[i].port_id, attr_get)));
		if (SAI_STATUS_SUCCESS == rv)
		{
			if (attr_get[0].value.u32list.count)
			{
				pdeTestSaiPortMap.port_list[i].total_lanes = attr_get[0].value.u32list.count;
				for (j = 0; j < attr_get[0].value.u32list.count; j++)
				{
					pdeTestSaiPortMap.port_list[i].lane_list[j] = attr_get[0].value.u32list.list[j];
				}
			}
		}

		CU_ASSERT(SAI_STATUS_SUCCESS ==
								 (rv = saiUnitGetPortAttribute(pdeTestSaiPortMap.port_list[i].port_id, attr_get_port_speed)));
		if (SAI_STATUS_SUCCESS == rv)
		{
			if (attr_get_port_speed[0].value.u32)
			{
				pdeTestSaiPortMap.port_list[i].speed = attr_get_port_speed[0].value.u32;
			}
		}

	}

	return max_ports;
}

int saiGetPortIdfromPortConfig(int index, sai_object_id_t *port_obj)
{
  int i, j;
  int found = 0;

  for (i = 0; i < pdeTestSaiPortMap.total_ports; i++)
  {
    if (pdeTestSaiPortMap.port_list[i].total_lanes == pdeTestPortConfigMap.port_list[index].total_lanes)
    {
      for (j = 0; j < pdeTestPortConfigMap.port_list[index].total_lanes; j++)
      {
        if (pdeTestPortConfigMap.port_list[index].lane_list[j] != pdeTestSaiPortMap.port_list[i].lane_list[j])
        {
          break;
        }
      }
    }

    if (j == pdeTestPortConfigMap.port_list[index].total_lanes)
    {
      *port_obj = pdeTestSaiPortMap.port_list[i].port_id;
      found = 1;
    }
  }

  return found;
}

void dumpPortTables(int i)
{
	int j;

	printf("Port ID: 0x%016lx Speed: %d  Total Lanes: %d Lanes: ", pdeTestSaiPortMap.port_list[i].port_id,
				pdeTestSaiPortMap.port_list[i].speed, pdeTestSaiPortMap.port_list[i].total_lanes);
	for (j = 0; j < pdeTestSaiPortMap.port_list[i].total_lanes; j++)
	{
		printf("%d ", pdeTestSaiPortMap.port_list[i].lane_list[j]);
	}
	printf("\n");

	printf("Port Name: %s %d  Total Lanes: %d Lanes: ", pdeTestPortConfigMap.port_list[i].name,
				 pdeTestPortConfigMap.port_list[i].speed, pdeTestPortConfigMap.port_list[i].total_lanes);
	for (j = 0; j < pdeTestPortConfigMap.port_list[i].total_lanes; j++)
	{
		printf("%d ", pdeTestPortConfigMap.port_list[i].lane_list[j]);
	}
	printf("\n");

}
/* More detail on the SONiC code to parse the SAI port list can be found in portsorch.cpp in SONiC*/
int saiUnitSonicTestPortConfigIniTest()
{
    int i, j, k;
    int matching_entries = 0;
    int failing_entries = 0;
    int entry_found = 0;

    saiSonicPdeLoadPortConfig();
    saiSonicPdeLoadSaiPortMap();

    CU_ASSERT(pdeTestPortConfigMap.total_ports == pdeTestSaiPortMap.total_ports);
    if (pdeTestPortConfigMap.total_ports == pdeTestSaiPortMap.total_ports)
    {
        printf("PASS:");
    }
    else
    {
        printf("FAIL:");
    }
    printf("%d port entries found in port_config.ini, %d entries enumerated by SAI\n", pdeTestPortConfigMap.total_ports, pdeTestSaiPortMap.total_ports);

    for (i = 0; i < pdeTestPortConfigMap.total_ports; i++)
    {
        entry_found = 0;
        for (j = 0; j < pdeTestSaiPortMap.total_ports && entry_found == 0; j++)
        {
            if ((pdeTestSaiPortMap.port_list[j].speed == pdeTestPortConfigMap.port_list[i].speed) &&
                    (pdeTestSaiPortMap.port_list[j].total_lanes == pdeTestPortConfigMap.port_list[i].total_lanes))
            {
                for (k = 0; k < pdeTestPortConfigMap.port_list[i].total_lanes; k++)
                {
                    if (pdeTestPortConfigMap.port_list[i].lane_list[k] != pdeTestSaiPortMap.port_list[j].lane_list[k])
                    {
                        break;
                    }
                }
            }

            if (k == pdeTestPortConfigMap.port_list[i].total_lanes)
            {
                entry_found = 1;
                matching_entries++;
            }
        }

        if (entry_found == 1)
        {
            printf("PASS: %s entry matched with SAI Port ID: 0x%016lx\n", pdeTestPortConfigMap.port_list[i].name, pdeTestSaiPortMap.port_list[i].port_id);
        }
        else
        {
            printf("FAIL: %s entry not found in SAI port list!\n", pdeTestPortConfigMap.port_list[i].name);
            failing_entries++;
        }
    }

    if (matching_entries == pdeTestPortConfigMap.total_ports)
    {
        printf("\nPASS: %d entries matched to SAI port list!", matching_entries);
    }
    else
    {
        printf("\nFAIL: %d entries not matched!", failing_entries);
        CU_ASSERT(failing_entries == 0);
    }

    printf("\n\n");
    printf("******************************************************\n");
    printf("SAI Port List Table - For Reference Only\n");
    printf("******************************************************\n");
    for (i = 0; i < pdeTestSaiPortMap.total_ports; i++)
    {
        printf("Port ID: 0x%016lx Speed: %d  Total Lanes: %d Lanes: ", pdeTestSaiPortMap.port_list[i].port_id,
                pdeTestSaiPortMap.port_list[i].speed, pdeTestSaiPortMap.port_list[i].total_lanes);
        for (j = 0; j < pdeTestSaiPortMap.port_list[i].total_lanes; j++)
        {
            printf("%d ", pdeTestSaiPortMap.port_list[i].lane_list[j]);
        }
        printf("\n");
    }
    printf("******************************************************\n");

    printf("\n\n");
    printf("******************************************************\n");
    printf("port_config.ini Port List Table - For Reference Only\n");
    printf("******************************************************\n");
    for (i = 0; i < pdeTestPortConfigMap.total_ports; i++)
    {
        printf("Port Name: %s %d  Total Lanes: %d Lanes: ", pdeTestPortConfigMap.port_list[i].name,
                pdeTestPortConfigMap.port_list[i].speed, pdeTestPortConfigMap.port_list[i].total_lanes);
        for (j = 0; j < pdeTestPortConfigMap.port_list[i].total_lanes; j++)
        {
            printf("%d ", pdeTestPortConfigMap.port_list[i].lane_list[j]);
        }
        printf("\n");
    }
    printf("******************************************************\n");

    return 0;

}

int saiUnitSonicPdeTrafficTest()
{
  sai_status_t rv = SAI_STATUS_SUCCESS;
  int i, p, max_ports, queue_count;
  sai_attribute_t switch_attr;
  sai_attribute_t port_attr;
  sai_object_id_t port_obj, port1_obj, port2_obj;
  char cmd_str[256];
  int lport1, lport2;
  int test_port1, test_port2;
  char *p1;
  char *p2;

  CU_ASSERT(SAI_STATUS_SUCCESS == saiUnitGetSwitchPortCount(&max_ports));
  switch_attr.id = SAI_SWITCH_ATTR_SRC_MAC_ADDRESS;
  memset(switch_attr.value.mac, 0, sizeof(sai_mac_t));
  sai_mac_t smac = { 0x00, 0x44, 0x55, 0x55, 0x66, 0x66 };
  memcpy(switch_attr.value.mac, smac, sizeof(sai_mac_t));
  CU_ASSERT(SAI_STATUS_SUCCESS == saiUnitSetSwitchAttribute(&switch_attr));

  /* Get Cpu queues */
  switch_attr.id = SAI_SWITCH_ATTR_CPU_PORT;
  CU_ASSERT(SAI_STATUS_SUCCESS == saiUnitGetSwitchAttributes(1, &switch_attr));
  sai_object_id_t cpu_port = switch_attr.value.oid;
  port_attr.id = SAI_PORT_ATTR_QOS_NUMBER_OF_QUEUES;
  CU_ASSERT(SAI_STATUS_SUCCESS == saiUnitGetPortAttribute(cpu_port, &port_attr));
  queue_count = port_attr.value.u32;
  port_attr.id = SAI_PORT_ATTR_QOS_QUEUE_LIST;
  port_attr.value.objlist.count = queue_count;
  port_attr.value.objlist.list = calloc(queue_count, sizeof(sai_object_id_t));
  CU_ASSERT(SAI_STATUS_SUCCESS == saiUnitGetPortAttribute(cpu_port, &port_attr));
  p = BRCM_SAI_GET_OBJ_VAL(sai_uint32_t, cpu_port);
  for (i = 0; i < queue_count; i++)
  {
    qid[p][i] = port_attr.value.objlist.list[i];
  }
  free(port_attr.value.objlist.list);

  saiSonicPdeLoadPortConfig();
  saiSonicPdeLoadSaiPortMap();

  for (i = 0; i < num_ports_in_traffic_test; i += 2)
  {
    test_port1 = saiUnitTrafficGetPortFromConfig(i);
    test_port2 = saiUnitTrafficGetPortFromConfig(i + 1);

    if (saiGetPortIdfromPortConfig(test_port1 - 1, &port1_obj) != 1)
    {
      printf("\nFAIL: Unable to retrieve SAI object for %s\n", pdeTestPortConfigMap.port_list[test_port1 - 1].name);
      printf("Please verify your config.bcm and port_config.ini\n");

      continue;
    }
    if (saiGetPortIdfromPortConfig(test_port2 - 1, &port2_obj) != 1)
    {
      printf("\nFAIL: Unable to retrieve SAI object for %s\n", pdeTestPortConfigMap.port_list[test_port2 - 1].name);
      printf("Please verify your config.bcm and port_config.ini\n");

      continue;
    }

    printf("Test port: %d, SAI Port Object ID: 0x%016lx \n", test_port1, port1_obj);
    printf("Test port: %d, SAI Port Object ID: 0x%016lx \n", test_port2, port2_obj);

    p1 = saiUnitGetBcmPortName(i);
    p2 = saiUnitGetBcmPortName(i +1);

    lport1 = (port1_obj & 0xFF);
    lport2 = (port2_obj & 0xFF);

    printf("\nTesting traffic between ports %d:%s:%s and %d:%s:%s\n", lport1, pdeTestPortConfigMap.port_list[test_port1 - 1].name,
        p1, lport2, pdeTestPortConfigMap.port_list[test_port2 - 1].name, p2);

    saiUnitSetLogLevel(SAI_LOG_LEVEL_DEBUG);

    /* Enable ports in admin mode */
    port_obj = port1_obj;
    port_attr.id = SAI_PORT_ATTR_ADMIN_STATE;
    port_attr.value.booldata = TRUE;
    CU_ASSERT(SAI_STATUS_SUCCESS ==
        saiUnitSetPortAttribute(port_obj, &port_attr));
    port_obj = port2_obj;
    CU_ASSERT(SAI_STATUS_SUCCESS ==
        saiUnitSetPortAttribute(port_obj, &port_attr));

    /* Wait some time for the ports to come up. */
    sleep(3);

    /* Prevent traffic from passing back to sender*/
    sprintf(cmd_str, "port %s stp=block\n", p2);
    sai_driver_process_command(cmd_str);

    CU_ASSERT(SAI_STATUS_SUCCESS == saiUnitClearStats());
    CU_ASSERT(SAI_STATUS_SUCCESS == saiUnitSendFramesToHost(p1, TRAFFIC_FRAME_COUNT));
    rv  = saiUnitValidateStatsRx(port2_obj, TRAFFIC_FRAME_COUNT);

    sprintf(cmd_str, "port %s stp=forward\n", p2);
    sai_driver_process_command(cmd_str);

    if (rv == SAI_STATUS_SUCCESS)
    {
      printf("PASS: Successfully sent %d frames between %s and %s \n", TRAFFIC_FRAME_COUNT,
          pdeTestPortConfigMap.port_list[test_port1 - 1].name, pdeTestPortConfigMap.port_list[test_port2 - 1].name);
    }
    else
    {
      printf("FAIL: Unable to send %d frames between %s and %s \n", TRAFFIC_FRAME_COUNT,
          pdeTestPortConfigMap.port_list[test_port1 - 1].name, pdeTestPortConfigMap.port_list[test_port2 - 1].name);

      CU_ASSERT(SAI_STATUS_SUCCESS == rv);
    }

    /* Disable ports in admin mode */
    port_obj = port1_obj;
    port_attr.id = SAI_PORT_ATTR_ADMIN_STATE;
    port_attr.value.booldata = FALSE;
    CU_ASSERT(SAI_STATUS_SUCCESS ==
        saiUnitSetPortAttribute(port_obj, &port_attr));
    port_obj = port2_obj;
    CU_ASSERT(SAI_STATUS_SUCCESS ==
        saiUnitSetPortAttribute(port_obj, &port_attr));

  }
  return 0;
}

/* Smoke Suite */
int saiSonicPdeAdd(void)
{
	pSonicPdeSuite = CU_add_suite("SONiC PDE tests", NULL, NULL);
	if (NULL == pSonicPdeSuite)
	{
		printf("%s\n", CU_get_error_msg());
		return 1;
	}
	if (NULL == CU_ADD_TEST(pSonicPdeSuite, saiUnitSonicTestPortConfigIniTest))
	{
		printf("%s\n", CU_get_error_msg());
		return 1;
	}

	if (NULL == CU_ADD_TEST(pSonicPdeSuite, saiUnitSonicPdeTrafficTest))
	{
		printf("%s\n", CU_get_error_msg());
		return 1;
	}

	return 0;
}
