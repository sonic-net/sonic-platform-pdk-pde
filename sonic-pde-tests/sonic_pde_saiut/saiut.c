/**********************************************************************
 * Copyright 2019 Broadcom. All rights reserved.
 * The term “Broadcom” refers to Broadcom Inc. and/or its subsidiaries.
 **********************************************************************/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <inttypes.h>
#include <sai.h>

#define _SAI_API_ID_MIN       (SAI_API_UNSPECIFIED + 1)
#define _SAI_API_ID_MAX       SAI_API_BRIDGE
#define _SAI_MAX_PORTS        512 /* max. number of front panel ports */
#define _SAI_PROFILE_MAX_KVPS 64  /* max. number of KVP */
#define COUNTOF(x)            (sizeof(x) / sizeof((x)[0]))
#define TRUE                  1
#define FALSE                 0

static int sdk_pfc_dl_recovery = 0;
static int pfc_deadlock_detect_event = 0;
static int pfc_deadlock_recover_event = 0;

sai_object_id_t g_switch_id = 0;

void *g_switch_apis[_SAI_API_ID_MAX + 1];

/*
 * SAI: front port list
 * BCM: logical port list
 */
sai_object_id_t g_switch_ports[_SAI_MAX_PORTS];

/*
 * SAI: Mapping port to lane
 * BCM: Mapping logical port to physical port
 */
uint32_t g_switch_l2p[_SAI_MAX_PORTS];

/*
 * SAI: Mapping lane to port
 * BCM: Mapping physical port to logical port
 */
sai_object_id_t g_switch_p2l[_SAI_MAX_PORTS];

typedef struct _profile_kvp_s
{
    char *key;
    char *val;
} profile_kvp_t;

static profile_kvp_t profile_kvps[_SAI_PROFILE_MAX_KVPS];

/**
 * skip_spaces - Removes leading whitespace from @str.
 * @str: The string to be stripped.
 *
 * Returns a pointer to the first non-whitespace character in @str.
 */
static char *skip_spaces(const char *str)
{
    while (isspace(*str))
    {
        ++str;
    }
    return (char *)str;
}

/**
 * strim - Removes leading and trailing whitespace from @s.
 * @s: The string to be stripped.
 *
 * Note that the first trailing whitespace is replaced with a %NUL-terminator
 * in the given string @s. Returns a pointer to the first non-whitespace
 * character in @s.
 */
static char *strim(char *s)
{
    size_t size;
    char *end;

    s = skip_spaces(s);
    size = strlen(s);
    if (!size)
    {
        return s;
    }

    end = s + size - 1;
    while (end >= s && isspace(*end))
    {
        end--;
    }
    *(end + 1) = '\0';

    return s;
}

static int _profile_reload(const char *filename)
{
    FILE *fp = NULL;
    char buf[256] = { 0 };
    char *ptr = NULL, *key = NULL, *val = NULL;
    unsigned int idx = 0;

    fp = fopen(filename, "r");
    if (!fp)
    {
        printf("Unable to open '%s'\n", filename);
        return -1;
    }

    while (fgets(buf, sizeof(buf) - 1, fp))
    {
        /* skip comments */
        ptr = strchr(buf, '#');
        if (ptr)
        {
            *ptr = 0;
        }

        /* skip empty lines */
        ptr = strim(buf);
        if (ptr[0] == 0)
        {
            continue;
        }

        key = ptr;
        val = strchr(key, '=');
        if (val == NULL)
        {
            continue;
        }
        *(val++) = 0;

        if (profile_kvps[idx].key)
        {
            free(profile_kvps[idx].key);
        }
        profile_kvps[idx].key = strdup(key);

        if (profile_kvps[idx].val)
        {
            free(profile_kvps[idx].val);
        }
        profile_kvps[idx].val = strdup(val);

        ++idx;
    }

    fclose(fp);
}

/*
 * Get variable value given its name
 */
static const char *_profile_get_value(sai_switch_profile_id_t id,
                                      const char *key)
{
    int i;

    if (id != 0)
    {
        return NULL;
    }

    for (i = 0; profile_kvps[i].key; ++i)
    {
        if (0 == strcmp(key, profile_kvps[i].key))
        {
            return profile_kvps[i].val;
        }
    }
    return NULL;
}

/*
 * Enumerate all the K/V pairs in a profile.
 * Pointer to NULL passed as variable restarts enumeration.
 * Function returns 0 if next value exists, -1 at the end of the list.
 */
static int _profile_get_next_value(sai_switch_profile_id_t id,
                                   const char **key,
                                   const char **value)
{
    static int index = 0;

    if (id != 0)
    {
        return -1;
    }

    /* enumeration reset */
    if (NULL == *key)
    {
        index = 0;
        return 0;
    }

    if (NULL == profile_kvps[index].key)
    {
        return -1;
    }

    *key = profile_kvps[index].key;
    if (NULL != value)
    {
        *value = profile_kvps[index].val;
    }

    return profile_kvps[++index].key ? 0 : -1;
}

static sai_service_method_table_t saiut_services =
{
    .profile_get_value = _profile_get_value,
    .profile_get_next_value = _profile_get_next_value
};

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
static void _switch_state_change_event(sai_object_id_t switch_id,
                                       sai_switch_oper_status_t status)
{
    printf("\n----Host received switch status event: %d from switch: 0x%lx\n",
           status, switch_id);
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
void _switch_fdb_event(uint32_t count,
                       sai_fdb_event_notification_data_t *data)
{
    /* skipped */
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
void _switch_port_state_change_event(uint32_t count,
                                     sai_port_oper_status_notification_t *data)
{
    printf("\n----Host received port link status event on port %3d: %s\n",
           (int)(data->port_id),
           (SAI_PORT_OPER_STATUS_UP == data->port_state) ? "  UP" : "DOWN");
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
void _switch_queue_pfc_deadlock_event(uint32_t count,
                                      sai_queue_deadlock_notification_data_t *data)
{
    switch (data->event)
    {
        case SAI_QUEUE_PFC_DEADLOCK_EVENT_TYPE_DETECTED:
            printf("Deadlock detected on queue id %d count number is %d\n",
                   (int)(data->queue_id), count);
            pfc_deadlock_detect_event++;
            break;

        case SAI_QUEUE_PFC_DEADLOCK_EVENT_TYPE_RECOVERED:
            printf("Deadlock recovered on queue id %d count number is %d\n",
                   (int)(data->queue_id), count);
            pfc_deadlock_recover_event++;
            break;

        default:
            printf("Unknown deadlock event on queue id %d count number is %d\n",
                   (int)(data->queue_id), count);
            break;
    }

    if (!sdk_pfc_dl_recovery)
    {
        /* Should not return true if we want to have sdk based pfc dl recovery */
        data->app_managed_recovery = TRUE;
    }
}

static sai_attribute_t saiut_attr[] =
{
    { SAI_SWITCH_ATTR_INIT_SWITCH, .value.booldata = TRUE },
    { SAI_SWITCH_ATTR_SWITCH_PROFILE_ID, .value.u32 = 0 },
    { SAI_SWITCH_ATTR_SWITCH_STATE_CHANGE_NOTIFY, .value.ptr = _switch_state_change_event },
    { SAI_SWITCH_ATTR_FDB_EVENT_NOTIFY, .value.ptr = _switch_fdb_event },
    { SAI_SWITCH_ATTR_PORT_STATE_CHANGE_NOTIFY, .value.ptr = _switch_port_state_change_event },
    { SAI_SWITCH_ATTR_QUEUE_PFC_DEADLOCK_NOTIFY, .value.ptr = _switch_queue_pfc_deadlock_event },
    { SAI_SWITCH_ATTR_SRC_MAC_ADDRESS, .value.mac = { 0x00, 0x05, 0x06, 0x08, 0x05, 0x00 } },
};

sai_status_t portGetAttribute(sai_object_id_t port, sai_attribute_t *attr)
{
    sai_port_api_t *apis = g_switch_apis[SAI_API_PORT];

    if (g_switch_id == 0)
    {
        return SAI_STATUS_FAILURE;
    }
    return apis->get_port_attribute(port, 1, attr);
}

sai_status_t portSetAttribute(sai_object_id_t port, sai_attribute_t *attr)
{
    sai_port_api_t *apis = g_switch_apis[SAI_API_PORT];

    if (g_switch_id == 0)
    {
        return SAI_STATUS_FAILURE;
    }
    return apis->set_port_attribute(port, attr);
}

sai_object_id_t portGetId(uint32_t lane)
{
    return (lane < COUNTOF(g_switch_p2l)) ? g_switch_p2l[lane] : 0;
}

bool portGetAdminState(sai_object_id_t port)
{
    sai_attribute_t attr;

    if (port > 0)
    {
        attr.id = SAI_PORT_ATTR_ADMIN_STATE;
        attr.value.booldata = FALSE;
        if (SAI_STATUS_SUCCESS == portGetAttribute(port, &attr))
        {
            return attr.value.booldata;
        }
    }
    return FALSE;
}

sai_status_t portSetAdminState(sai_object_id_t port, bool enable)
{
    sai_attribute_t attr;

    if (port > 0)
    {
        attr.id = SAI_PORT_ATTR_ADMIN_STATE;
        attr.value.booldata = enable;
        return portSetAttribute(port, &attr);
    }
    return SAI_STATUS_FAILURE;
}

uint32_t portGetSpeed(sai_object_id_t port)
{
    sai_attribute_t attr;

    if (port > 0)
    {
        attr.id = SAI_PORT_ATTR_SPEED;
        attr.value.u32 = 0;
        if (SAI_STATUS_SUCCESS == portGetAttribute(port, &attr))
        {
            return attr.value.u32;
        }
    }
    return 0;
}

sai_status_t portSetSpeed(sai_object_id_t port, uint32_t speed)
{
    sai_attribute_t attr;

    if (port > 0)
    {
        attr.id = SAI_PORT_ATTR_SPEED;
        attr.value.u32 = 0;
        return portSetAttribute(port, &attr);
    }
    return SAI_STATUS_FAILURE;
}

uint64_t portGetCounter(sai_object_id_t port, sai_stat_id_t id)
{
    sai_port_api_t *apis = g_switch_apis[SAI_API_PORT];
    uint64_t counter = 0;

    if ((g_switch_id == 0) || (apis == NULL))
    {
        return 0;
    }

    if (SAI_STATUS_SUCCESS != apis->get_port_stats(port, 1, &id, &counter))
    {
        counter = 0;
        printf("port %d: unable to get counter\n", (int)port);
    }
    return counter;
}

sai_status_t portClearCounter(sai_object_id_t port, sai_stat_id_t id)
{
    sai_port_api_t *apis = g_switch_apis[SAI_API_PORT];

    if ((g_switch_id == 0) || (apis == NULL))
    {
        return SAI_STATUS_FAILURE;
    }
    return apis->clear_port_stats(port, 1, &id);
}

sai_status_t switchGetAttribute(sai_attribute_t *attr)
{
    sai_switch_api_t *apis = g_switch_apis[SAI_API_SWITCH];

    if (g_switch_id == 0)
    {
        return SAI_STATUS_FAILURE;
    }
    return apis->get_switch_attribute(g_switch_id, 1, attr);
}

sai_status_t switchSetAttribute(sai_attribute_t *attr)
{
    sai_switch_api_t *apis = g_switch_apis[SAI_API_SWITCH];

    if (g_switch_id == 0)
    {
        return SAI_STATUS_FAILURE;
    }
    return apis->set_switch_attribute(g_switch_id, attr);
}

uint32_t switchGetPortNumber(void)
{
    sai_status_t rv = SAI_STATUS_FAILURE;
    sai_attribute_t attr;

    attr.id = SAI_SWITCH_ATTR_PORT_NUMBER;
    attr.value.u32 = 0;

    rv = switchGetAttribute(&attr);
    if (SAI_STATUS_SUCCESS != rv)
    {
        printf("Unable to get port number\n");
        return 0;
    }
    return (uint32_t)attr.value.u32;
}

sai_status_t switchShell(bool enable)
{
    sai_attribute_t attr;
    sai_status_t rv = SAI_STATUS_FAILURE;

    if (enable)
    {
        attr.id = SAI_SWITCH_ATTR_SWITCH_SHELL_ENABLE;
        attr.value.booldata = TRUE;
        rv = switchSetAttribute(&attr);
    }
    return rv;
}

sai_status_t switchInitialize(const char *mac)
{
    sai_status_t rv = SAI_STATUS_FAILURE;
    sai_switch_api_t *sai_switch_apis = NULL;
    sai_object_list_t list;
    sai_attribute_t attr;
    uint32_t i, lanes[32];

    if (0 != _profile_reload("/usr/share/sonic/hwsku/sai.profile"))
    {
        profile_kvps[0].key = SAI_KEY_INIT_CONFIG_FILE;
        profile_kvps[0].val = "./config.bcm";
    }

    /* Initialize SAI library core */
    rv = sai_api_initialize(0, &saiut_services);
    if (SAI_STATUS_SUCCESS != rv)
    {
        printf("Unable to initialize SAI library\n");
        return rv;
    }

    /* Initialize SAI API function pointers */
    for (i = _SAI_API_ID_MIN; i <= _SAI_API_ID_MAX; i++)
    {
        rv = sai_api_query(i, &g_switch_apis[i]);
        if (SAI_STATUS_SUCCESS != rv)
        {
            printf("API %u is not available\n", i);
        }
    }

    /* Create the switch instance */
    sai_switch_apis = g_switch_apis[SAI_API_SWITCH];
    rv = sai_switch_apis->create_switch(&g_switch_id, COUNTOF(saiut_attr),
                                        saiut_attr);
    if (SAI_STATUS_SUCCESS != rv)
    {
        printf("Unable to created switch\n");
        return rv;
    }
    printf("Created switch: 0x%lx, port number: %d\n",
           g_switch_id, switchGetPortNumber());

    /* Initialize the switch port list and maps */
    list.count = _SAI_MAX_PORTS;
    list.list = g_switch_ports;
    attr.id = SAI_SWITCH_ATTR_PORT_LIST;
    attr.value.objlist = list;
    rv = switchGetAttribute(&attr);
    if (SAI_STATUS_SUCCESS != rv)
    {
        printf("Unable to get switch port list\n");
        return rv;
    }
    if (g_switch_ports[0] == 0)
    {
        printf("None of valid port is available on the switch?\n");
        return SAI_STATUS_FAILURE;
    }
    for (i = 0; i < _SAI_MAX_PORTS; ++i)
    {
        if (g_switch_ports[i] == 0)
        {
            break;
        }
        attr.id = SAI_PORT_ATTR_HW_LANE_LIST;
        attr.value.u32list.count = COUNTOF(lanes);
        attr.value.u32list.list = lanes;
        rv = portGetAttribute(g_switch_ports[i], &attr);
        if (SAI_STATUS_SUCCESS != rv)
        {
            printf("Unable to get lane list of port %d\n",
                   (int)g_switch_ports[i]);
            return rv;
        }
        /* logical port to physical port */
        g_switch_l2p[g_switch_ports[i] & 0x1ff] = lanes[0];
        g_switch_p2l[lanes[0]] = g_switch_ports[i];
    }

    return SAI_STATUS_SUCCESS;
}

sai_status_t switchShutdown(void)
{
    sai_status_t ret = SAI_STATUS_SUCCESS;
    sai_switch_api_t *apis = g_switch_apis[SAI_API_SWITCH];

    if (g_switch_id)
    {
        ret = apis->remove_switch(g_switch_id);
        g_switch_id = 0;
    }
    return ret;
}

extern int sai_driver_process_command(char *cmd);
sai_status_t bcmDiagCommand(char *cmd)
{
  int rv;

  rv = sai_driver_process_command(cmd);
  return (rv == 0) ? SAI_STATUS_SUCCESS : SAI_STATUS_FAILURE;
}

extern char *bcm_port_name(int unit, int port);
sai_status_t bcmSendPackets(sai_object_id_t port, uint32_t num, uint32_t len)
{
    char cmd[128];

    num = (num == 0) ? 1 : num;
    len = (len == 0) ? 68 : len;
    sprintf(cmd, "tx %u pbm=%s L=%d", num,
            bcm_port_name(0, (uint32_t)port), len);
    return bcmDiagCommand(cmd);
}
