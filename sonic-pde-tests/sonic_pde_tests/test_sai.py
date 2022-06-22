#!/usr/bin/python

import sys
import json
import time
import pytest
import subprocess
import os.path

from . import saiut

PLATFORM_PATH = "/usr/share/sonic/platform"
HWSKU_PATH = "/usr/share/sonic/hwsku"

# Global switch core initialization flag
core_init = False

# Global port dictionaries
port_dict = None

def parse_port_config_file(port_config_file):
    ports = {}
    port_alias_map = {}
    # Default column definition
    titles = ['name', 'lanes', 'alias', 'index']
    with open(port_config_file) as data:
        for line in data:
            if line.startswith('#'):
                if "name" in line:
                    titles = line.strip('#').split()
                continue;
            tokens = line.split()
            if len(tokens) < 2:
                continue
            name_index = titles.index('name')
            name = tokens[name_index]
            data = {}
            for i, item in enumerate(tokens):
                if i == name_index:
                    continue
                data[titles[i]] = item
            data.setdefault('alias', name)
            ports[name] = data
            port_alias_map[data['alias']] = name
    return (ports, port_alias_map)

# Load the platform specific port dictionaries
def load_platform_portdict():
    global port_dict

    if port_dict is not None:
        return port_dict

    port_dict = {}
    file = open(HWSKU_PATH + '/port_config.ini', 'r')
    line = file.readline()
    while line is not None and len(line) > 0:
        line = line.strip()
        if not line.startswith("#"):
            lst = line.split()
            if len(lst) >= 2:
                key = lst[0].strip()
                val = lst[1].split(',')
                port_dict[key] = val
        line = file.readline()
    file.close()

    return port_dict

# Reload the platform specific port dictionaries
def reload_platform_portdict():
    global port_dict

    port_dict = None
    return load_platform_portdict()

# Start the switch core
def start_switch_core():
    global core_init

    rv = saiut.SAI_STATUS_SUCCESS
    if core_init:
        return rv

    rv = saiut.switchInitialize("00:11:22:33:44:55")
    if rv != saiut.SAI_STATUS_SUCCESS:
        return rv

    core_init = True
    if os.path.exists(PLATFORM_PATH + '/led_proc_init.soc'):
        saiut.bcmDiagCommand('rcload ' + PLATFORM_PATH + '/led_proc_init.soc')
    return rv

# Stop the switch core
def stop_switch_core():
    global core_init

    saiut.switchShutdown()
    core_init = False
    return saiut.SAI_STATUS_SUCCESS

# Test for switch port enumeration
def test_for_switch_port_enumeration():
    """
    Test Purpose:
        Verify the SONiC port enumeration to ensure all the SONiC ports
        are mapped to SAI ports with valid object ids

    Args:
        None
    """
    rv = load_platform_portdict()
    assert rv is not None, "Unable to load the port dictionary"

    rv = start_switch_core()
    assert rv == saiut.SAI_STATUS_SUCCESS, "Unable to initialize the switch"

    for key in port_dict:
        port = saiut.portGetId(int(port_dict[key][0], 10))
        assert port != 0, "{}: port id not found".format(key)
    return

# Test for switch port loopback
def test_for_switch_port_loopback():
    """
    Test Purpose:  Verify the per port packet transmission in MAC loopback mode

    Args:
        None
    """
    rv = load_platform_portdict()
    assert rv is not None, "Unable to load the port dictionary"

    rv = start_switch_core()
    assert rv == saiut.SAI_STATUS_SUCCESS, "Unable to initialize the switch"

    attr = saiut.sai_attribute_t()

    # Enable the port in MAC loopback mode, and transmit the packets
    for key in port_dict:
        port = saiut.portGetId(int(port_dict[key][0], 10))
        assert port != 0, "{}: port id not found".format(key)

        # Enable the port
        attr.id = saiut.SAI_PORT_ATTR_ADMIN_STATE
        attr.value.booldata = True
        saiut.portSetAttribute(port, attr)

        # Enable MAC loopback mode
        attr.id = saiut.SAI_PORT_ATTR_INTERNAL_LOOPBACK_MODE
        attr.value.u32 = saiut.SAI_PORT_INTERNAL_LOOPBACK_MODE_MAC
        saiut.portSetAttribute(port, attr)

        # Perform loopback packet transmission test
        saiut.portClearCounter(port, saiut.SAI_PORT_STAT_ETHER_OUT_PKTS_512_TO_1023_OCTETS)
        saiut.portClearCounter(port, saiut.SAI_PORT_STAT_ETHER_IN_PKTS_512_TO_1023_OCTETS)
        saiut.bcmSendPackets(port, 99, 512)

    # Allow 2 seconds for counter updates
    time.sleep(2)

    # Verify the packet counters, and exit loopback
    for key in port_dict:
        port = saiut.portGetId(int(port_dict[key][0], 10))
        assert port != 0, "{}: port id not found".format(key)

        xmit = saiut.portGetCounter(port, saiut.SAI_PORT_STAT_ETHER_OUT_PKTS_512_TO_1023_OCTETS)
        recv = saiut.portGetCounter(port, saiut.SAI_PORT_STAT_ETHER_IN_PKTS_512_TO_1023_OCTETS)

        # Exit loopback mode
        attr.id = saiut.SAI_PORT_ATTR_INTERNAL_LOOPBACK_MODE
        attr.value.u32 = saiut.SAI_PORT_INTERNAL_LOOPBACK_MODE_NONE
        saiut.portSetAttribute(port, attr)

        # Validate the test result
        assert xmit >= 99, "{}: loopback tx failed".format(key)
        assert recv >= 99, "{}: loopback rx failed".format(key)

# Test for switch port traffic
def test_for_switch_port_traffic(json_test_data):
    """
    Test Purpose:  Verify the per port packet transmission

    Args:
        arg1 (json): test-<sonic_platform>-config.json

    Example:
        For a system with 2 back-to-back self-loops

        test-<sonic_platform>-config.json
        {
            "PLATFORM": {
                "SELF_LOOPS" : [
                    "Ethernet0:Ethernet24",
                    "Ethernet4:Ethernet28"
                ]
            }
        }
    """
    try:
        lnks = json_test_data['PLATFORM']['SELF_LOOPS']
    except KeyError:
        pytest.skip("test configuration not found")

    rv = load_platform_portdict()
    assert rv is not None, "Unable to load the port dictionary"

    rv = start_switch_core()
    assert rv == saiut.SAI_STATUS_SUCCESS, "Unable to initialize the switch"

    # Enable all the switch ports
    attr = saiut.sai_attribute_t()
    for key in port_dict:
        port = saiut.portGetId(int(port_dict[key][0], 10))
        assert port != 0, "{}: port id not found".format(key)

        attr.id = saiut.SAI_PORT_ATTR_ADMIN_STATE
        attr.value.booldata = True
        saiut.portSetAttribute(port, attr)

    time.sleep(1)

    lnks = json_test_data['PLATFORM']['SELF_LOOPS']
    for idx in range(0, len(lnks)):
        p1 = lnks[idx].split(':')[0]
        p2 = lnks[idx].split(':')[1]
        pid1 = saiut.portGetId(int(port_dict[p1][0], 10))
        pid2 = saiut.portGetId(int(port_dict[p2][0], 10))

        print("xmit: {} --> {}".format(p1, p2));
        saiut.portClearCounter(pid1, saiut.SAI_PORT_STAT_ETHER_OUT_PKTS_512_TO_1023_OCTETS)
        saiut.portClearCounter(pid2, saiut.SAI_PORT_STAT_ETHER_IN_PKTS_512_TO_1023_OCTETS)
        saiut.bcmSendPackets(pid1, 99, 512)
        time.sleep(3)
        c1 = saiut.portGetCounter(pid1, saiut.SAI_PORT_STAT_ETHER_OUT_PKTS_512_TO_1023_OCTETS)
        c2 = saiut.portGetCounter(pid2, saiut.SAI_PORT_STAT_ETHER_IN_PKTS_512_TO_1023_OCTETS)
        assert c1 >= 99, "{} --> {}: tx failed".format(p1, p2)
        assert c2 >= 99, "{} --> {}: rx failed".format(p1, p2)

        print("xmit: {} <-- {}".format(p1, p2));
        saiut.portClearCounter(pid1, saiut.SAI_PORT_STAT_ETHER_IN_PKTS_512_TO_1023_OCTETS)
        saiut.portClearCounter(pid2, saiut.SAI_PORT_STAT_ETHER_OUT_PKTS_512_TO_1023_OCTETS)
        saiut.bcmSendPackets(pid2, 99, 512)
        time.sleep(3)
        c1 = saiut.portGetCounter(pid1, saiut.SAI_PORT_STAT_ETHER_IN_PKTS_512_TO_1023_OCTETS)
        c2 = saiut.portGetCounter(pid2, saiut.SAI_PORT_STAT_ETHER_OUT_PKTS_512_TO_1023_OCTETS)
        assert c1 >= 99, "{} <-- {}: rx failed".format(p1, p2)
        assert c2 >= 99, "{} <-- {}: tx failed".format(p1, p2)




# Test for medis setting json
def test_for_switch_media_setting_json():
    """
    Test Purpose:  Verify preemphasis media setting json

    Args:
        None
    """
    try:
        with open(PLATFORM_PATH +'/media_settings.json') as data:
            media_dict = json.load(data)
    except:
        assert False, "Unable to open media_settings.json"

    assert (media_dict is not None) , "media_dict is None "

    if 'GLOBAL_MEDIA_SETTINGS' not in media_dict and 'SPEED_MEDIA_SETTINGS' not in media_dict \
       and 'PORT_MEDIA_SETTINGS' not in media_dict:
       assert False, "Required *_MEDIA_SETTINGS not defined"
