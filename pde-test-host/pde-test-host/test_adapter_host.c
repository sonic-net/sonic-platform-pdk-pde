/*********************************************************************
 *
 * (C) Copyright Broadcom Corporation 2013-2018
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sai.h"
#include "saiUnitTest.h"
#include <getopt.h>
#include "CUnit.h"
#include "Basic.h"
#include "Console.h"
#include "Automated.h"

#define _REPEAT_COUNT 1

#define PRINT_LINK_STATUS
#define SET_SWITCH_MAC

#define _SAI_API_ID_MIN SAI_API_UNSPECIFIED+1
#define _SAI_API_ID_MAX SAI_API_BRIDGE

//Custom attrobutes for as per internal design
#define SAI_SWITCH_ATTR_GET_SDK_BOOT_TIME SAI_SWITCH_ATTR_CUSTOM_RANGE_START
#define SAI_SWITCH_ATTR_SDK_SHUT_TIME_GET_FN SAI_SWITCH_ATTR_CUSTOM_RANGE_START+1

extern char *_brcm_sai_api_type_strings[];

#define _SAI_MAX_PORTS 256

#define NUM_TEST_PROFILES    4
#define NUM_PROFILES         4
#define MAX_PROFILE_KEY_SIZE 64
#define MAX_PROFILE_VAL_SIZE 128
#define _SAI_MAX_QUEUES      64

#define TRUE 1
#define FALSE 0

#ifndef TEST_LOG_OFF
#define TEST_LOG(__fmt,__args...)                               \
  do { printf("%d: " __fmt, __LINE__ ,##__args); } while(0)
#else
#define TEST_LOG(...)
#endif

#define CHECK_RV_RETURN(rv) if (rv!=SAI_STATUS_SUCCESS) \
    { fprintf(stderr, "Error in %s():%d rv=%d\n", __FUNCTION__, __LINE__, rv); return rv; }
#define CHECK_RV_STR_RETURN(str, rv) if (rv!=SAI_STATUS_SUCCESS) \
    { fprintf(stderr, "Error in %s > %s():%d rv=%d\n", str, __FUNCTION__, __LINE__, rv); return rv; }

#define WAIT_FOR_EVENT(n) \
    { TEST_LOG("Wait %d secs for events to complete...\n", n); sleep(n); }

#define _SAI_GET_OBJ_VAL(type, var) ((type)var)

#define COUNTOF(ary)        ((int) (sizeof (ary) / sizeof ((ary)[0])))

#define _BRCM_SAI_MAX_SCHEDULER_PROFILES   128 /* Max profiles */

#define CONFIG_STR_SIZE 64
#define SYSTEM_CMD_STR_LEN 20
#define NETIF_NAME_STR_LEN SYSTEM_CMD_STR_LEN

//Struct to maintain lag,port and lag member info 
typedef struct _lag_member_port_info{
   sai_object_id_t lag_obj;
   sai_object_id_t lag_mbr_obj;
   sai_object_id_t port_obj;
   struct _lag_member_port_info *next;
}lag_member_port_info;


#define BRCM_SAI_CREATE_OBJ(type, value) ((((sai_object_id_t)type) << 32) | value)
#define BRCM_SAI_CREATE_OBJ_SUB(type, subtype, value) ((((sai_object_id_t)subtype) << 40) | \
                                                       (((sai_object_id_t)type) << 32) | value)
#define BRCM_SAI_CREATE_OBJ_SUB_MAP(type, subtype, map, value) ((((sai_object_id_t)map) << 48) | \
                                                                (((sai_object_id_t)subtype) << 40) | \
                                                                (((sai_object_id_t)type) << 32) | value)
#define BRCM_SAI_GET_OBJ_TYPE(var) ((uint8_t)(var >> 32))
#define BRCM_SAI_GET_OBJ_SUB_TYPE(var) ((uint8_t)(var >> 40))
#define BRCM_SAI_GET_OBJ_MAP(var) ((uint16_t)(var >> 48))
#define BRCM_SAI_GET_OBJ_VAL(type, var) ((type)var)
static char usage[] =
" Syntax :    ./pde-test-host  [-m|--cunit_mode]                                                          \n\r"
"             eg: ./pde-test_host -m2                                                                     \n\r"
" Parameters :                                                                                            \n\r"
"     -m|--cunit_mode      : CUnit mode                                                                   \n\r"
"                             cunit_mode - what mode to run the CUNIT tests                               \n\r"
"                             0 - run in BASIC mode where summary is printed inline                       \n\r"
"                             1 - run in AUTOMATED mode where summary is printed to files in /tmp         \n\r"
"                             2 - run in CONSOLE mode where you can run the tests interactively           \n\r"
"                             default - 2                                                                 \n\r"
" Return : returns 0                                                                                      \n\r";

/*
################################################################################
#                               Global state                                   #
################################################################################
*/
/* Declare API pointers and table */
sai_switch_api_t *sai_switch_apis;
sai_port_api_t *sai_port_apis;
sai_vlan_api_t *sai_vlan_apis;
sai_fdb_api_t *sai_fdb_apis;
sai_virtual_router_api_t *sai_router_apis;
sai_router_interface_api_t *sai_router_intf_apis;
sai_neighbor_api_t *sai_neighbor_apis;
sai_next_hop_api_t *sai_next_hop_apis;
sai_next_hop_group_api_t *sai_next_hop_grp_apis;
sai_route_api_t *sai_route_apis;
sai_hostif_api_t *sai_hostif_apis;
sai_acl_api_t *sai_acl_apis;
sai_policer_api_t *sai_policer_apis;
sai_qos_map_api_t *sai_qos_map_apis;
sai_queue_api_t *sai_qos_queue_apis;
sai_wred_api_t *sai_wred_apis;
sai_scheduler_api_t *sai_qos_scheduler_apis;
sai_scheduler_group_api_t *sai_qos_sched_group_apis;
sai_buffer_api_t *sai_buffer_apis;
sai_lag_api_t *sai_lag_apis;
sai_tunnel_api_t *sai_tunnel_apis;
sai_mirror_api_t *sai_mirror_apis;
sai_udf_api_t *sai_udf_apis;
sai_hash_api_t *sai_hash_apis;
sai_bridge_api_t *sai_bridge_apis;

static void *adapter_apis[_SAI_API_ID_MAX+1];

#define SAI_BUFFER_PROFILE_ATTR_BRCM_CUSTOM_USE_QGROUP_MIN (SAI_BUFFER_PROFILE_ATTR_CUSTOM_RANGE_START)

typedef struct _profile_kvp_s {
    char k[MAX_PROFILE_KEY_SIZE];
    char v[MAX_PROFILE_VAL_SIZE];
} profile_kvp_t;

static const profile_kvp_t profile_kvp_table[NUM_TEST_PROFILES][NUM_PROFILES] = {
    {
        { SAI_KEY_INIT_CONFIG_FILE, "/tmp/brcm_sai_config.bcm" },
        { SAI_KEY_BOOT_TYPE, "0" },
        { SAI_KEY_WARM_BOOT_READ_FILE, "/tmp/brcm_sai.wb" },
#ifdef WB_PERF
        { SAI_KEY_NUM_ECMP_MEMBERS, "64" }
#else
        { SAI_KEY_NUM_ECMP_MEMBERS, "32" }
#endif
    },
    {
        { SAI_KEY_INIT_CONFIG_FILE, "/tmp/brcm_sai_config.bcm" },
        { SAI_KEY_BOOT_TYPE, "1" },
        { SAI_KEY_WARM_BOOT_READ_FILE, "/tmp/brcm_sai.wb" },
#ifdef WB_PERF
        { SAI_KEY_NUM_ECMP_MEMBERS, "64" }
#else
        { SAI_KEY_NUM_ECMP_MEMBERS, "32" }
#endif
    },
    {
        { SAI_KEY_INIT_CONFIG_FILE, "/tmp/brcm_sai_config.bcm" },
        { SAI_KEY_BOOT_TYPE, "2" },
        { SAI_KEY_WARM_BOOT_READ_FILE, "/tmp/brcm_sai.wb" },
#ifdef WB_PERF
        { SAI_KEY_NUM_ECMP_MEMBERS, "32" }
#else
        { SAI_KEY_NUM_ECMP_MEMBERS, "32" }
#endif
    },
    {
        { "KEY1_0", "VALUE1_0" },
        { "KEY1_1", "VALUE1_1" },
        { "KEY1_2", "VALUE1_2" }
    }
};

static int wb = 0; /* Warm boot */
static int queue_count = 0;
int pfc_deadlock_detect_event = 0, pfc_deadlock_recover_event = 0;
sai_uint32_t port_loopback_mode;
static struct timeval begin, end;
/***************************Persistent State***********************************/
static sai_object_id_t switch_id = 0;
static sai_object_id_t dot1q_bridge = 0;
static sai_object_id_t cpu_port = 0;
static sai_object_id_t ports[_SAI_MAX_PORTS];
static sai_object_list_t port_list = { _SAI_MAX_PORTS, ports };
static sai_object_id_t b_ports[_SAI_MAX_PORTS];
static sai_object_list_t b_port_list = { _SAI_MAX_PORTS, b_ports };
static sai_object_id_t vlan_objs[10];
static sai_object_id_t lag0, lag1;
static sai_object_id_t bp_lag0, bp_lag1;
static sai_object_id_t vr_id[3] = { 0 };
static sai_object_id_t rif_id[12] = { 0 };
static sai_object_id_t nh_id[7] = { -1, -1, -1, -1, -1, -1, -1 };
static sai_object_id_t nh_gid[3] = { -1, -1, -1 };
static sai_object_id_t t_nh_g_membr[2] = { 0, 0 };
static sai_object_id_t nh_g_membr[6] = { 0, 0, 0, 0, 0, 0 };
static sai_object_id_t hif_id[10] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};
static sai_object_id_t policer_id0 = -1;
static sai_object_id_t policer_id[3] = { -1, -1, -1 };
static sai_object_id_t acl_counter_id = -1;
sai_object_id_t qid[_SAI_MAX_PORTS][_SAI_MAX_QUEUES];
static sai_object_id_t acl_entry_id, acl_entry_id2;
static sai_object_id_t scheduler_id1, scheduler_id2, scheduler_id3,
                       scheduler_id4, scheduler_id5, scheduler_id6,
                       scheduler_id7;
static sai_object_id_t sched_grp[_SAI_MAX_PORTS][3][10]; /* port, level, nodes */
static sai_object_id_t lm[5];
static sai_object_id_t acl_range;
uint32_t sdk_shut_time;//to measure sdk shut time from callback
lag_member_port_info *root_node = NULL;//Dont make static as we need it in CUnit.

static sai_attribute_t lag_mbr_attr[] = {
        {SAI_LAG_MEMBER_ATTR_LAG_ID},
        {SAI_LAG_MEMBER_ATTR_PORT_ID},
};

/* test config file params */
int num_ports_in_test;
int num_ports_in_traffic_test;
int bcm_port_list[_SAI_MAX_PORTS];
int sai_port_index_list[_SAI_MAX_PORTS];
int pde_traffic_test_port_list[_SAI_MAX_PORTS];
char* bcm_port_list_names[_SAI_MAX_PORTS];
char* mmu_init_config;
int bcm_loopback_port_list[_SAI_MAX_PORTS];
int capture_enable;//Decides whether capturing needs to be enable in CUnits
int warmboot_enable;//Decides whether warmshut/warmboot to be run in CUnits
int sdk_pfc_dl_recovery, collect_syslog; 


extern char* saiUnitGetBcmPortName(int index);
/* Note: Any data added in the above 'persistent state section will need to be 
 *       stored and loaded in the below routines in the same order: 
 */
/******************************************************************************/

#define _TEST_DATA_STORE(data) fprintf(fp, "%lx\n", (long unsigned int)data);
#define _TEST_DATA_STORE_1(data)    \
    for (i=0; i<COUNTOF(data); i++) \
    {                               \
        _TEST_DATA_STORE(data[i]);  \
    }                               
#define _TEST_DATA_STORE_v1(data, max) \
    for (i=0; i<max; i++)              \
    {                                  \
        _TEST_DATA_STORE(data[i]);     \
    }
#define _TEST_DATA_STORE_v2(data, max1, max2) \
    for (i=0; i<max1; i++)                    \
    {                                         \
        for (j=0; j<max2; j++)                \
        {                                     \
            _TEST_DATA_STORE(data[i][j]);     \
        }                                     \
    }
#define _TEST_DATA_STORE_v3(data, max1, max2, max3) \
    for (i=0; i<max1; i++)                          \
    {                                               \
        for (j=0; j<max2; j++)                      \
        {                                           \
            for (k=0; k<max3; k++)                  \
            {                                       \
                _TEST_DATA_STORE(data[i][j][k]);    \
            }                                       \
        }                                           \
    }

#define _TEST_DATA_LOAD(data) getline(&line, &len, fp); \
                              sscanf(line, "%lx", (long unsigned int*)&data);

#define _TEST_DATA_LOAD_1(data)     \
    for (i=0; i<COUNTOF(data); i++) \
    {                               \
        _TEST_DATA_LOAD(data[i]);   \
    }                               

#define _TEST_DATA_LOAD_v1(data, max) \
    for (i=0; i<max; i++)             \
    {                                 \
        _TEST_DATA_LOAD(data[i]);     \
    }

#define _TEST_DATA_LOAD_v2(data, max1, max2) \
    for (i=0; i<max1; i++)                   \
    {                                        \
        for (j=0; j<max2; j++)               \
        {                                    \
            _TEST_DATA_LOAD(data[i][j]);     \
        }                                    \
    }

#define _TEST_DATA_LOAD_v3(data, max1, max2, max3) \
    for (i=0; i<max1; i++)                         \
    {                                              \
        for (j=0; j<max2; j++)                     \
        {                                          \
            for (k=0; k<max3; k++)                 \
            {                                      \
                _TEST_DATA_LOAD(data[i][j][k]);    \
            }                                      \
        }                                          \
    }


int
_test_ws_save()
{
    int i, j, k;
    FILE *fp;
    
    fp = fopen("/tmp/test_objects.dat", "w+");
    if (fp == NULL)
    {
        return -1;
    }
    _TEST_DATA_STORE(switch_id);
    _TEST_DATA_STORE(cpu_port);
    _TEST_DATA_STORE_v1(ports, _SAI_MAX_PORTS);
    _TEST_DATA_STORE(lag0);
    _TEST_DATA_STORE(lag1);
    _TEST_DATA_STORE(bp_lag0);
    _TEST_DATA_STORE(bp_lag1);
    _TEST_DATA_STORE_1(vr_id);
    _TEST_DATA_STORE_1(rif_id);
    _TEST_DATA_STORE_1(nh_id);
    _TEST_DATA_STORE_1(nh_gid);
    _TEST_DATA_STORE_1(nh_g_membr);
    _TEST_DATA_STORE_1(t_nh_g_membr);
    _TEST_DATA_STORE_1(hif_id);
    _TEST_DATA_STORE(policer_id0);
    _TEST_DATA_STORE_1(policer_id);
    _TEST_DATA_STORE(acl_counter_id);
    _TEST_DATA_STORE_v2(qid, _SAI_MAX_PORTS, 64);
    _TEST_DATA_STORE(acl_entry_id);
    _TEST_DATA_STORE(acl_entry_id2);
    _TEST_DATA_STORE(scheduler_id1);
    _TEST_DATA_STORE(scheduler_id2);
    _TEST_DATA_STORE(scheduler_id3);
    _TEST_DATA_STORE(scheduler_id4);
    _TEST_DATA_STORE(scheduler_id5);
    _TEST_DATA_STORE(scheduler_id6);
    _TEST_DATA_STORE(scheduler_id7);
    _TEST_DATA_STORE_v3(sched_grp, _SAI_MAX_PORTS, 3, 10);
    _TEST_DATA_STORE(acl_range);
    fclose(fp);
    return 0;
}

int
_test_wb_load()
{
    int i, j, k;
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    
    fp = fopen("/tmp/test_objects.dat", "r+");
    if (fp == NULL)
    {
        return -1;
    }
    _TEST_DATA_LOAD(switch_id);
    _TEST_DATA_LOAD(cpu_port);
    _TEST_DATA_LOAD_v1(ports, _SAI_MAX_PORTS);
    _TEST_DATA_LOAD(lag0);
    _TEST_DATA_LOAD(lag1);
    _TEST_DATA_LOAD(bp_lag0);
    _TEST_DATA_LOAD(bp_lag1);
    _TEST_DATA_LOAD_1(vr_id);
    _TEST_DATA_LOAD_1(rif_id);
    _TEST_DATA_LOAD_1(nh_id);
    _TEST_DATA_LOAD_1(nh_gid);
    _TEST_DATA_LOAD_1(nh_g_membr);
    _TEST_DATA_LOAD_1(t_nh_g_membr);
    _TEST_DATA_LOAD_1(hif_id);
    _TEST_DATA_LOAD(policer_id0);
    _TEST_DATA_LOAD_1(policer_id);
    _TEST_DATA_LOAD(acl_counter_id);
    _TEST_DATA_LOAD_v2(qid, _SAI_MAX_PORTS, 20);
    _TEST_DATA_LOAD(acl_entry_id);
    _TEST_DATA_LOAD(acl_entry_id2);
    _TEST_DATA_LOAD(scheduler_id1);
    _TEST_DATA_LOAD(scheduler_id2);
    _TEST_DATA_LOAD(scheduler_id3);
    _TEST_DATA_LOAD(scheduler_id4);
    _TEST_DATA_LOAD(scheduler_id5);
    _TEST_DATA_LOAD(scheduler_id6);
    _TEST_DATA_LOAD(scheduler_id7);
    _TEST_DATA_LOAD_v3(sched_grp, _SAI_MAX_PORTS, 3, 10);
    _TEST_DATA_LOAD(acl_range);
    fclose(fp);
    return 0;
}


/*
################################################################################
#                           Common Utility functions                           #
################################################################################
*/
typedef struct _sai_attribute_info_s {
    char *str;
    int size;
} _sai_attribute_info_t;

void
_test_verify_print_attributes(char *prepend, const sai_attribute_t *act_attr,
                              sai_attribute_t *attr)
{
    if (attr->value.u32 != act_attr->value.u32)
    {
        TEST_LOG("--Set attrib failed.\n");
    }
    else
    {
        TEST_LOG("--Set attrib passed.\n");
    }
}

void
_test_verify_print_num_attributes(char *prepend, int num,
                                  const sai_attribute_t *act_attr,
                                  sai_attribute_t *attr)
{
    if (attr->value.u32 != act_attr->value.u32)
    {
        TEST_LOG("--Set attrib failed for %d.\n", num);
    }
    else
    {
        TEST_LOG("--Set attrib passed for %d.\n", num);
    }
}

sai_object_id_t
_get_cpu_port_object()
{
    return cpu_port;
}

int
_get_port_count()
{
    return port_list.count;
}

sai_object_id_t
_get_port_object(int index)
{
    if (index > port_list.count)
    {
        return (sai_object_id_t)NULL;
    }
    return port_list.list[index];
}

sai_object_id_t
_get_bridge_port_object(int index)
{
    if (index > b_port_list.count)
    {
        return (sai_object_id_t)NULL;
    }
    return b_port_list.list[index];
}

int parse_values_int(char* temp, int* arr)
{
  char* token;
  int i=0;
  if (isdigit(*temp) && strstr(temp, ",") != 0)
  {
    /* Comma sep values */
    while (1)
    {
      token =  strtok(temp, ",");
      if (token != NULL)
      {
        arr[i++] = atoi(token);
        temp = NULL;
      }
    else break;
    }
  }
  else
    /* Single value */
  {
    while (1)
    {
      token =  strtok(temp, "\n");
      if (token != NULL)
      {
        arr[i++] = atoi(token);
        temp =NULL;
      }
    else break;
    }
  }
  return 0;
}

int parse_values_str(char* temp, char** arr)
{
  char* token;
  int i=0;
  if (strstr(temp, ",") != 0)
  {
    while (1)
    {
      token =  strtok(temp, ",\n");//multiple delimiters to avoid /n append to last token
      if (token != NULL)
      {
        arr[i] = (char*)malloc(sizeof(token));
        memset(arr[i],'\0',sizeof(token));
        strcpy(arr[i++], token);
        temp =NULL;
      }
      else break;
    }
  }
  else
  {
    while (1)
    {
      token =  strtok(temp, "\n");
      if (token != NULL)
       {
         arr[i] = (char*)malloc(strlen(token)+1);
         memset(arr[i],'\0',strlen(token)+1);
         strcpy(arr[i++], token);
         temp =NULL;
       }
      else break;
    }
  }
  return 0;
}

/* Expects a line with \n */
int parse_line(char* lineptr, int len)
{
  char* temp = lineptr;
  if (!temp || (len <= 1)) return 0;

  while (isspace(*temp)) {temp++;}

  // Ignore lines with comments
  if (*temp == '#') return 0;

  // Look for a known word
  /* All words to be parsed from file */
  if ((strstr(lineptr, "BCM_PORT_LIST") == 0)     ||    \
      (strstr(lineptr, "NUM_PORTS_IN_TEST") == 0) ||    \
      (strstr(lineptr, "SAI_PORT_INDEX_LIST") == 0)  || \
      (strstr(lineptr, "BCM_PORT_LIST_NAMES") == 0)||   \
      (strstr(lineptr, "BCM_PORT_NAMES") == 0)||   \
      (strstr(lineptr, "MMU_INIT_CONFIG") == 0)    ||   \
      (strstr(lineptr, "PORT_CONFIG_INDEX_LIST") == 0) || \
      (strstr(lineptr, "BCM_LOOPBACK_PORTS_LIST") == 0))
  {
    temp = strchr(lineptr, '=');
    if (temp == NULL)
      return 0; // bad string?

    if (*temp == '=')
    {
      int i = 0;
      char* token;
      temp++;

      while(isspace(*temp)) {
        temp++;
      }
      if (strstr(lineptr, "BCM_PORT_LIST") != 0)
      {
        parse_values_int(temp, bcm_port_list);
      }
      if (strstr(lineptr, "NUM_PORTS_IN_TRAFFIC_TEST") != 0)
      {
        parse_values_int(temp, &num_ports_in_traffic_test);
      }
      if (strstr(lineptr, "NUM_PORTS_IN_TEST") != 0)
      {
        parse_values_int(temp, &num_ports_in_test);
      }
      if (strstr(lineptr, "SAI_PORT_INDEX_LIST") != 0)
      {
        parse_values_int(temp, sai_port_index_list);
      }
      if (strstr(lineptr, "BCM_PORT_NAMES") != 0)
      {
        parse_values_str(temp, bcm_port_list_names);
      }
      if (strstr(lineptr, "MMU_INIT_CONFIG") != 0)
      {
        parse_values_str(temp, &mmu_init_config);
      }
      if (strstr(lineptr, "BCM_LOOPBACK_PORTS_LIST") != 0)
      {
        parse_values_int(temp, bcm_loopback_port_list);
      }
      if (strstr(lineptr, "PORT_CONFIG_INDEX_LIST") != 0)
      {
        parse_values_int(temp, pde_traffic_test_port_list);
      }
    }
  }
  return 0;
}




/*
################################################################################
#                                Services and events                           #
################################################################################
*/

/*
* Get variable value given its name
*/
const char*
profile_get_value(_In_ sai_switch_profile_id_t profile_id,
                  _In_ const char* variable)
{
    int i;

    for (i = 0; i<NUM_PROFILES; i++)
    {
        if (0 == strncmp(variable, profile_kvp_table[profile_id][i].k,
                         MAX_PROFILE_KEY_SIZE)) {
            return profile_kvp_table[profile_id][i].v;
        }
    }
    return NULL;
}

/*
* Enumerate all the K/V pairs in a profile.
* Pointer to NULL passed as variable restarts enumeration.
* Function returns 0 if next value exists, -1 at the end of the list.
*/
int
profile_get_next_value(_In_ sai_switch_profile_id_t profile_id,
                       _Out_ const char** variable,
                       _Out_ const char** value)
{
    static int i[NUM_TEST_PROFILES] = { 0, 0, 0 };

    if (profile_id >= NUM_TEST_PROFILES) {
        return -1;
    }
    if (NULL == *variable)
    {
        /*TEST_LOG("KVP reset\n");*/
        i[profile_id] = 0;
        return 0;
    }
    if (variable)
    {
        /*TEST_LOG("KVP profile %d offset %d key: %s\n", profile_id, i[profile_id],
               profile_kvp_table[profile_id][i[profile_id]].k);*/
        *variable = profile_kvp_table[profile_id][i[profile_id]].k;
    }
    if (value)
    {
        *value = profile_kvp_table[profile_id][i[profile_id]].v;
    }
    if ((NUM_PROFILES-1) == i[profile_id])
    {
        return -1;
    }
    i[profile_id]++;

    return 0;
}

/*
* Routine Description:
*   Switch shutdown request callback.
*   Adapter DLL may request a shutdown due to an unrecoverable failure
*   or a maintenance operation
*
* Arguments:
*   None
*
* Return Values:
*   None
*/
void
sai_switch_shutdown_request(void)
{
    TEST_LOG("----Host received switch shutdown request event\n");
}

/*
* Routine Description:
*   Switch oper state change notification
*
* Arguments:
*   [in] switch_oper_status - new switch oper state
*
* Return Values:
*    None
*/
void
sai_switch_state_change_notification(_In_ sai_object_id_t switch_id,
                                     _In_ sai_switch_oper_status_t switch_oper_status)
{
    TEST_LOG("----Host received switch status event: %d from switch: 0x%lx\n", 
             switch_oper_status, switch_id);
}

/*
* Routine Description:
*     PFC event notifications
*
* Arguments:
*    [in] count - number of notifications
*    [in] data  - pointer to pfc event notification data
*
* Return Values:
*    None
*/
void sai_queue_pfc_deadlock_notification(_In_ uint32_t count,
                                         _In_ sai_queue_deadlock_notification_data_t *data)
{
     switch(data->event)
     {
         case SAI_QUEUE_PFC_DEADLOCK_EVENT_TYPE_DETECTED:
             printf("Deadlock detected on queue id %d count number is %d\n",_SAI_GET_OBJ_VAL(int, data->queue_id), count);
             pfc_deadlock_detect_event++;
             break;
         case SAI_QUEUE_PFC_DEADLOCK_EVENT_TYPE_RECOVERED:
             printf("Deadlock recovered on queue id %d count number is %d\n",_SAI_GET_OBJ_VAL(int, data->queue_id), count);
             pfc_deadlock_recover_event++;
             break;
         default :
             printf("Unknown event occured in PFC callback\n");
             break;
     }
     if(!sdk_pfc_dl_recovery){
        data->app_managed_recovery = TRUE;//Should not return true if we want to have sdk based pfc dl recovery
     }
}

/*
* Routine Description:
*     FDB notifications
*
* Arguments:
*    [in] count - number of notifications
*    [in] data  - pointer to fdb event notification data array
*
* Return Values:
*    None
*/
void
sai_fdb_event_notification(_In_ uint32_t count,
                           _In_ sai_fdb_event_notification_data_t *data)
{
#ifdef PRINT_L2_UPDATES
    uint8_t *mac = data->fdb_entry.mac_address;
    TEST_LOG("----Host received FDB event type: ");
    switch (data->event_type)
    {
        case SAI_FDB_EVENT_LEARNED: TEST_LOG("LEARN vid=%d mac=%x:%x:%x:%x:%x:%x ",
                                             _SAI_GET_OBJ_VAL(int, data->fdb_entry.bv_id), 
                                             mac[0], mac[1],mac[2], mac[3], mac[4], mac[5]);
            break;
        case SAI_FDB_EVENT_AGED: TEST_LOG("AGE vid=%d mac=%x:%x:%x:%x:%x:%x ",
                                          _SAI_GET_OBJ_VAL(int, data->fdb_entry.bv_id), 
                                          mac[0], mac[1],mac[2], mac[3], mac[4], mac[5]);
            break;
        case SAI_FDB_EVENT_FLUSHED: TEST_LOG("FLUSH\n");
            break;
        default: TEST_LOG("UNKNOWN\n");
            break;
    }
    if (data->attr_count && (SAI_FDB_ENTRY_ATTR_BRIDGE_PORT_ID == data->attr[0].id))
    {
        if (SAI_OBJECT_TYPE_LAG == sai_object_type_query(data->attr[0].value.oid))
        {
            TEST_LOG("lag:%d\n", _SAI_GET_OBJ_VAL(int, data->attr[0].value.oid));
        }
        else
        {
            TEST_LOG("port:%d\n", _SAI_GET_OBJ_VAL(int, data->attr[0].value.oid));
        }
    }
    else
    {
        TEST_LOG("\n");
    }
#endif
}

/*
* Routine Description:
*   Port state change notification
*   Passed as a parameter into sai_initialize_switch()
*
* Arguments:
*   [in] count - number of notifications
*   [in] data  - array of port operational status
*
* Return Values:
*    None
*/
//INFO must be reentrant routine
void
sai_port_state_change_notification(_In_ uint32_t count,
                                   _In_ sai_port_oper_status_notification_t *data)
{
#ifdef PRINT_LINK_STATUS
    TEST_LOG("----Host received port link status event on port %3d: %s\n",
        _SAI_GET_OBJ_VAL(int, data->port_id),
        (SAI_PORT_OPER_STATUS_UP == data->port_state) ? "  UP" : "DOWN");
#endif
   lag_member_port_info *lag_mbr_list;
   lag_mbr_list = root_node;
   while (lag_mbr_list != NULL){
          if (lag_mbr_list->port_obj == data->port_id){
              if (data->port_state == SAI_PORT_OPER_STATUS_UP){
                  //Add ports to lag
                  lag_mbr_attr[0].value.oid = lag_mbr_list->lag_obj; 
                  lag_mbr_attr[1].value.oid = data->port_id;
                  CU_ASSERT(SAI_STATUS_SUCCESS == 
                            sai_lag_apis->create_lag_member(&lag_mbr_list->lag_mbr_obj,
                                                            0,
                                                            COUNTOF(lag_mbr_attr),
                                                            lag_mbr_attr));
              }
              else{
                   //Remove ports from lag
                   CU_ASSERT(SAI_STATUS_SUCCESS == 
                             sai_lag_apis->remove_lag_member(lag_mbr_list->lag_mbr_obj));
              }
          }
          //possibilty that the port may belong to other lags also so traverse complete list.
          lag_mbr_list = lag_mbr_list->next;    
   }
}

/*
* Routine Description:
*   Callback fn which returns sdk shutdown time.
*   Passed as a parameter into create_switch()
*
* Arguments:
*   [in] unit      - Device unit number(nothing to do with SAI now better replace with switch_id)
*   [in] time_sec  - time take to do shutdown
*
* Return Values:
*    None
*/

void sai_sdk_shutdown_time_get(int unit, unsigned int time_sec){
     sdk_shut_time = time_sec;//sdk_shut_time is global and externed to use in CUnit
     TEST_LOG("\nSDK took %d secs to shut.", sdk_shut_time);
}

/*
################################################################################
#                                Test Init API                                 #
################################################################################
*/

static const sai_service_method_table_t test_services = {
    .profile_get_value = profile_get_value,
    .profile_get_next_value = profile_get_next_value
};

sai_status_t
test_adapter_api_init(void)
{
    sai_status_t rv;

    rv = sai_api_initialize(0, &test_services);
    return rv;
}

sai_status_t
test_query_adapter_methods(sai_api_t api)
{
    sai_status_t rv;

    rv = sai_api_query(api, &adapter_apis[api]);
    switch(api)
    {
        case SAI_API_SWITCH: sai_switch_apis = (sai_switch_api_t*)adapter_apis[api];
            break;
        case SAI_API_PORT: sai_port_apis = (sai_port_api_t*)adapter_apis[api];
            break;
        case SAI_API_FDB: sai_fdb_apis = (sai_fdb_api_t*)adapter_apis[api];
            break;
        case SAI_API_VIRTUAL_ROUTER: sai_router_apis = (sai_virtual_router_api_t*)adapter_apis[api];
            break;
        case SAI_API_ROUTER_INTERFACE: sai_router_intf_apis = (sai_router_interface_api_t*)adapter_apis[api];
            break;
        case SAI_API_NEIGHBOR: sai_neighbor_apis = (sai_neighbor_api_t*)adapter_apis[api];
            break;
        case SAI_API_ROUTE: sai_route_apis = (sai_route_api_t*)adapter_apis[api];
            break;
        case SAI_API_NEXT_HOP: sai_next_hop_apis = (sai_next_hop_api_t*)adapter_apis[api];
            break;
        case SAI_API_VLAN: sai_vlan_apis = (sai_vlan_api_t*)adapter_apis[api];
            break;
        case SAI_API_NEXT_HOP_GROUP: sai_next_hop_grp_apis = (sai_next_hop_group_api_t*)adapter_apis[api];
            break;
        case SAI_API_POLICER: sai_policer_apis = (sai_policer_api_t*)adapter_apis[api];
            break;
        case SAI_API_ACL: sai_acl_apis = (sai_acl_api_t*)adapter_apis[api];
            break;
        case SAI_API_HOSTIF: sai_hostif_apis  = (sai_hostif_api_t*)adapter_apis[api];
            break;
        case SAI_API_WRED: sai_wred_apis = (sai_wred_api_t*)adapter_apis[api];
            break;
        case SAI_API_QOS_MAP: sai_qos_map_apis = (sai_qos_map_api_t*)adapter_apis[api];
            break;
        case SAI_API_QUEUE: sai_qos_queue_apis = (sai_queue_api_t*)adapter_apis[api];
            break;
        case SAI_API_SCHEDULER: sai_qos_scheduler_apis = (sai_scheduler_api_t*)adapter_apis[api];
            break;
        case SAI_API_SCHEDULER_GROUP: sai_qos_sched_group_apis = (sai_scheduler_group_api_t*)adapter_apis[api];
            break;
        case SAI_API_BUFFER: sai_buffer_apis = (sai_buffer_api_t*)adapter_apis[api];
            break;
        case SAI_API_LAG: sai_lag_apis = (sai_lag_api_t*)adapter_apis[api];
            break;
        case SAI_API_TUNNEL: sai_tunnel_apis = (sai_tunnel_api_t*)adapter_apis[api];
            break;
        case SAI_API_MIRROR: sai_mirror_apis = (sai_mirror_api_t*)adapter_apis[api];
            break;
        case SAI_API_UDF: sai_udf_apis = (sai_udf_api_t*)adapter_apis[api];
            break;
        case SAI_API_HASH: sai_hash_apis = (sai_hash_api_t*)adapter_apis[api];
            break;
        case SAI_API_BRIDGE: sai_bridge_apis = (sai_bridge_api_t*)adapter_apis[api];
            break;
        default:
            return 0;
    }
    return rv;
}

sai_status_t
test_query_all_adapter_methods()
{
    sai_status_t rv;
    sai_api_t api;

    for (api=_SAI_API_ID_MIN; api<=_SAI_API_ID_MAX; api++)
    {
        rv = test_query_adapter_methods(api);
        CHECK_RV_STR_RETURN("query adpter method", rv);
    }
}

/*
################################################################################
#                                 Set log levels                               #
################################################################################
*/
sai_status_t
test_set_all_log_levels(sai_log_level_t level)
{
    sai_status_t rv;
    sai_api_t api;

    for (api=_SAI_API_ID_MIN; api<=_SAI_API_ID_MAX; api++)
    {
        rv = sai_log_set(api, level);
        if (rv!=SAI_STATUS_SUCCESS)
        {
            return rv;
        }
        TEST_LOG("API %s log level set to %d\n", _brcm_sai_api_type_strings[api], level);
    }
    return SAI_STATUS_SUCCESS;
}

/*
################################################################################
#                                 Test De-Init API                             #
################################################################################
*/
sai_status_t
test_adapter_api_deinit()
{
    sai_status_t rv = SAI_STATUS_FAILURE;

    rv = sai_api_uninitialize();
    return rv;
}

/*
################################################################################
#                                Test Init switch                              #
################################################################################
*/
sai_status_t
test_adapter_switch_create()
{
    sai_status_t rv = SAI_STATUS_FAILURE;
    
    const sai_attribute_t attr_set0[] = {
        { SAI_SWITCH_ATTR_INIT_SWITCH, .value.booldata=TRUE },
        { SAI_SWITCH_ATTR_SWITCH_PROFILE_ID, .value.u32=0 },
        { SAI_SWITCH_ATTR_SWITCH_STATE_CHANGE_NOTIFY, .value.ptr = sai_switch_state_change_notification },
        { SAI_SWITCH_ATTR_FDB_EVENT_NOTIFY, .value.ptr = sai_fdb_event_notification },
        { SAI_SWITCH_ATTR_PORT_STATE_CHANGE_NOTIFY, .value.ptr = sai_port_state_change_notification },
        { SAI_SWITCH_ATTR_QUEUE_PFC_DEADLOCK_NOTIFY, .value.ptr = sai_queue_pfc_deadlock_notification },
        { SAI_SWITCH_ATTR_SDK_SHUT_TIME_GET_FN, .value.ptr = sai_sdk_shutdown_time_get}
#ifndef SET_SWITCH_MAC
        ,
        { SAI_SWITCH_ATTR_SRC_MAC_ADDRESS,
          .value.mac = { 0x00, 0x05, 0x06, 0x08, 0x05, 0x00 } 
        }
#endif
    };
    
    const sai_attribute_t attr_set1[] = {
        { SAI_SWITCH_ATTR_INIT_SWITCH, .value.booldata=TRUE },
        { SAI_SWITCH_ATTR_SWITCH_PROFILE_ID, .value.u32=1 },
        { SAI_SWITCH_ATTR_SWITCH_STATE_CHANGE_NOTIFY, .value.ptr = sai_switch_state_change_notification },
        { SAI_SWITCH_ATTR_FDB_EVENT_NOTIFY, .value.ptr = sai_fdb_event_notification },
        { SAI_SWITCH_ATTR_PORT_STATE_CHANGE_NOTIFY, .value.ptr = sai_port_state_change_notification },
        { SAI_SWITCH_ATTR_QUEUE_PFC_DEADLOCK_NOTIFY, .value.ptr = sai_queue_pfc_deadlock_notification },
        { SAI_SWITCH_ATTR_SDK_SHUT_TIME_GET_FN, .value.ptr = sai_sdk_shutdown_time_get}
    };
    
    if (NULL != sai_switch_apis->create_switch)
    {
        rv = sai_switch_apis->create_switch(&switch_id, COUNTOF(attr_set0), 
                                            wb ? attr_set1 : attr_set0);
        if (SAI_STATUS_SUCCESS == rv)
        {
            TEST_LOG("Created switch: 0x%lx.\n", switch_id);
        }
    }
    return rv;
}

/*
################################################################################
#                               Test Shutdown switch                           #
################################################################################
*/
sai_status_t
test_adapter_switch_shutdown(bool ws)
{
   sai_status_t rv;

   sai_attribute_t switch_attr[] = 
   {
       { SAI_SWITCH_ATTR_RESTART_WARM }
   };

   /* brcm_sai_shutdown_switch */
   if (NULL != sai_switch_apis->set_switch_attribute)
   {
       switch_attr[0].value.booldata = ws;  /* WB hint */
       gettimeofday(&begin, NULL);
       rv = sai_switch_apis->set_switch_attribute(switch_id, switch_attr);
       if (SAI_STATUS_SUCCESS != rv)
       {
           TEST_LOG("set_switch_attribute failed with error %d\n", rv);
           return rv;
       }
   }
   rv = sai_switch_apis->remove_switch(switch_id);
   gettimeofday(&end, NULL);
#ifdef WB_PERF
   TEST_LOG("\nSAI took %d secs to shut.",(int)(end.tv_sec - begin.tv_sec));
#endif
   return rv;
}


static void ds()
{
    TEST_LOG("Hit enter to get drivshell prompt..\n");
    sai_driver_shell();
}

/*********************************************************************
 * \brief Main function for sample test adapter host
 *
 * \param argc, argv commands line arguments
 *
 * \return 0
 ********************************************************************/
int main(int argc, char *argv[])
{
  int i;
  bool ws = FALSE; /* Warm shut - don't cleanup */
  sai_status_t rv;
  int cunit_cfg = 1;
  int cunit_mode = 2;
  capture_enable = 0;
  warmboot_enable = 0;
  size_t len = 0, read = 0;
  char *lineptr;

  static struct option long_options[] = {
    {"cunit_mode",          optional_argument, NULL, 'm'},
    {"help",                no_argument,       NULL, 'h'},
    { NULL,                 0,                 NULL,  0 }
  };
  char opt;
  while((opt = getopt_long(argc, argv, ":m::h", long_options, NULL)) != -1){
    switch(opt){
      case 'm':
        if(optarg != NULL)
          cunit_mode = atoi(optarg);
        else
          cunit_mode = 0;
        break;
      case 'h':
      case ':':
      case '?':
      default :
        printf("%s\n\r", usage);
        exit(-1);
    } 
  }

  FILE *fp = fopen ("/tmp/test_config.ini", "r");
  if (fp == NULL) {
    TEST_LOG("Please make /tmp/test_config.ini is present\n");
    return -1;
  }
  while(read = getline(&lineptr, &len, fp) != -1) {
    parse_line(lineptr, len);
    // free and reset lineptr
    if (lineptr)
      free(lineptr);
    lineptr = 0;
  }

  rv = test_adapter_api_init();
  rv = test_query_all_adapter_methods();
  rv = test_adapter_switch_create();

  for (len=0;len < num_ports_in_test; len++)
  {
    if ( sai_port_index_list[len] == 0)
    {
      sai_port_index_list[len] = saiUnitSaiIndexGet(bcm_port_list[len]);

      if (sai_port_index_list[len] == -1)
      {
        TEST_LOG("Could not convert BCM %d to  SAI port\n",
            bcm_port_list[len]);
      }
      else
      {
        TEST_LOG("BCM %d, SAI Obj Id %d, Name %s\n",
            bcm_port_list[len], sai_port_index_list[len],
            bcm_port_list_names[len]);
      }
    }
  }

  saiUnitTestsRun(cunit_mode, cunit_cfg, SAI_TEST_ID_PDE);
  rv = test_adapter_switch_shutdown(ws);
  CHECK_RV_STR_RETURN("switch shutdown", rv);
  rv = test_adapter_api_deinit();
  CHECK_RV_STR_RETURN("api deinit", rv);
  rv = test_set_all_log_levels(SAI_LOG_LEVEL_ERROR);
  CHECK_RV_STR_RETURN("all log levels set to warn", rv);

  if (ws)
  {
    rv = _test_ws_save();
    CHECK_RV_STR_RETURN("test objects store", rv);
  }
  TEST_LOG("Test adapter complete.\n");

  return 0;
}




