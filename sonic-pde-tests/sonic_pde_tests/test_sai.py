#!/usr/bin/python

import sys
import json
import time
import pytest
import subprocess
import os.path
from utilities_common.intf_filter import parse_interface_in_filter

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

# Test for dynamic port breakout support in platform.json
def test_for_switch_platform_json_dpb():
    """
    Test Purpose:  Verify the dynamic port breakout support in platform.json

    Args:
        None
    """
    try:
        with open(HWSKU_PATH +'/platform.json') as data:
            cfg = json.load(data)
    except:
        assert False, "Unable to open platform.json"

    port_dict, port_alias = parse_port_config_file(HWSKU_PATH + '/port_config.ini')

    for intf in port_dict:
        speed = int(port_dict[intf].get('speed'))
        lanes = port_dict[intf].get('lanes').split(',')

        # lanes
        assert port_dict[intf].get('lanes') == cfg[intf].get('lanes'), \
               "invalid lanes"

        # index
        idxes = cfg[intf].get('index').split(',')
        if len(lanes) == 2 and speed == 100000:
            assert 1 == len(idxes), \
                   "only 1 index should be specified for 100G PAM4"
        else:
            assert len(lanes) == len(idxes), \
                   "please provide index for each lane"

        for n in range(len(idxes)):
            assert idxes[n] == port_dict[intf].get('index'), "invalid index"

        # alias_at_lanes
        alias = cfg[intf].get('alias_at_lanes').split(',')
        if len(lanes) == 2 and speed == 100000:
            assert 1 == len(alias), \
                   "only 1 alias name should be specified for 100G PAM4"
        else:
            assert len(lanes) == len(alias), \
                   "please provide alias name for each lane"

        # breakout_modes
        modes = cfg[intf].get('breakout_modes')
        if speed == 400000:
            assert modes.startswith("1x400G"), \
                   "invalid breakout modes: {0}".format(modes)

        if len(lanes) == 4 and speed == 100000:
            assert modes.startswith("1x100G[40G]"), \
                   "invalid breakout modes: {0}".format(modes)

        if len(lanes) == 2 and speed == 100000:
            assert modes.startswith("1x100G[50G]"), \
                   "invalid breakout modes: {0}".format(modes)

        if speed == 40000:
            assert modes.startswith("1x40G"), \
                   "invalid breakout modes: {0}".format(modes)

        if len(lanes) == 1 and speed == 25000:
            assert modes.startswith("1x25G[10G]"), \
                   "invalid breakout modes: {0}".format(modes)

        if len(lanes) == 1 and speed == 10000:
            assert modes.startswith("1x10G"), \
                   "invalid breakout modes: {0}".format(modes)

        # default_brkout_mode
        mode = cfg[intf].get('default_brkout_mode')
        assert modes.split(',')[0] == mode, \
               "invalid default breakout mode: {0}".format(mode)

# Test for dynamic port breakout support in config.bcm
def test_for_switch_config_bcm_dpb():
    """
    Test Purpose:  Verify the dynamic port breakout support in config.bcm

    Args:
        None
    """
    kvp = {}
    p2l = {}

    # platform.json
    try:
        with open(HWSKU_PATH +'/platform.json') as data:
            cfg = json.load(data)
    except:
        assert False, "Unable to open platform.json"

    # sai.profile
    try:
        file = open(HWSKU_PATH + '/sai.profile', 'r')
    except:
        assert False, "Unable to open sai.profile"
    line = file.readline()
    while line is not None and len(line) > 0:
        line = line.strip()
        if not line.startswith("#"):
            lst = line.split('=')
            if len(lst) != 2:
                continue
            kvp[lst[0]] = lst[1]
        line = file.readline()
    file.close()

    # config.bcm: initialize the port map from physical to logical
    file = kvp['SAI_INIT_CONFIG_FILE']
    try:
        file = open(file, 'r')
    except:
        assert False, "Unable to open config.bcm"
    line = file.readline()
    while line is not None and len(line) > 0:
        line = line.strip()
        if not line.startswith("#"):
            lst = line.split('=')
            if len(lst) != 2:
                line = file.readline()
                continue
            key = lst[0].strip()
            if not key.startswith("portmap_"):
                line = file.readline()
                continue
            val = lst[1].split(':')[0]
            p2l[int(val, 10)] = int(key[8:], 10)
        line = file.readline()
    file.close()

    # load port_config.ini
    port_dict, port_alias = parse_port_config_file(HWSKU_PATH + '/port_config.ini')

    # scan for the conflicts in port breakout support
    for intf in port_dict:
        port = int(intf[8:], 10)    # Ethernet[XXX]
        lanes = port_dict[intf].get('lanes').split(',') # Physical Port/Lane

        # skip if the number of lanes <= 1
        if len(lanes) <= 1:
            continue

        # determine the maximum number of breakout ports
        max_bko = 1
        mod_bko = cfg[intf].get('breakout_modes')
        if mod_bko.find("8x") > -1:
            max_bko = 8
        elif mod_bko.find("4x") > -1:
            max_bko = 4
        elif mod_bko.find("2x") > -1:
            max_bko = 2

        # skip if maximum number of breakout ports <= 1
        if max_bko <= 1:
            continue

        # check for the conflict in config.bcm
        for off in range(1, max_bko):
            # skip the special case in TD3, where 128 is a shared physical port
            if int(lanes[off], 10) == 128:
                continue
            assert (int(lanes[off], 10) not in p2l), \
                   "{0}: config.bcm: portmap conflict detected".format(intf)

        # check for the conflict in port_config.ini
        for off in range(1, max_bko):
            name = "Ethernet{0}".format(port + off)
            assert (name not in port_dict), \
                   "{0}: port_config.ini: conflict detected".format(intf)



# Test for Port Group support in platform-def.json
def test_for_switch_platform_def_json_port_group():
    """
    Test Purpose:  Verify the Port Group support in platform-def.json

    Args:
        None
    """
    port_group_required = False
    
    #Auto detected the TD3 X3 or X7
    cmd = "cat /proc/linux-kernel-bde | grep -c -i '0xb37\|0xb87'"
    pin = subprocess.Popen(cmd,
                           shell=True,
                           close_fds=True,
                           stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT)
    cnt = pin.communicate()[0].decode('ascii')
    
    #Skip the test for non X3/X7 switch 
    if int(cnt) == 0:
       pytest.skip("Skip the testing due to the port group not required for this switch platform")

    try:
        with open(HWSKU_PATH +'/platform-def.json') as data:
            platform_dict = json.load(data)
    except:
        assert False, "Unable to open platform-def.json"

    assert (platform_dict is not None) , "platform_dict is None"

    port_dict, port_alias = parse_port_config_file(HWSKU_PATH + '/port_config.ini')

    if 'port-group' in platform_dict:
      for key in platform_dict['port-group']:
        pg = platform_dict['port-group'][key]
        intf_fs = parse_interface_in_filter(pg['members'])

        for interface_name in intf_fs:
            if interface_name in port_dict:
               lanes = port_dict[interface_name].get('lanes').split(',')
               assert len(lanes) < 4, \
               "port {} with mulitple lanes ports not allowed to defined in port group".format(interface_name)
            else:
               assert False, "port {} not be defined in port map".format(interface_name)


# Test for FEC support in platform-def.json
def test_for_switch_platform_def_json_fec():
    """
    Test Purpose:  Verify the FEC support in platform-def.json

    Args:
        None
    """

    try:
        with open(HWSKU_PATH +'/platform-def.json') as data:
            platform_dict = json.load(data)
    except:
        assert False, "Unable to open platform-def.json"

    assert (platform_dict is not None) , "platform_dict is None "

    port_dict, port_alias = parse_port_config_file(HWSKU_PATH + '/port_config.ini')

    if 'fec-mode' not in platform_dict:
      assert False, "fec-mode not defined"
    else:
      for key in platform_dict['fec-mode']:
          intf_fec = parse_interface_in_filter(key)
          lanes = port_dict[intf_fec[0]].get('lanes').split(',')
          if intf_fec[0] not in port_dict :
             assert False, "port {} not be defined in port map".format(intf_fec[0])
          if len(lanes) == 1:
             if intf_fec[-1] not in port_dict :
                assert False, "port {} not be defined in port map".format(intf_fec[-1])
          if len(lanes) == 4:
             if intf_fec[-4] not in port_dict:
                assert False, "port {} not be defined in port map".format(intf_fec[-4])
          if len(lanes) == 8:
             if intf_fec[-8] not in port_dict:
                assert False, "port {} not be defined in port map".format(intf_fec[-8])

# Test for Native port support speed in platform-def.json
def test_for_switch_platform_def_json_native_port_supported_speed():
    """
    Test Purpose:  Verify Native port support speed in platform-def.json

    Args:
        None
    """

    try:
        with open(HWSKU_PATH +'/platform-def.json') as data:
            platform_dict = json.load(data)
    except:
        assert False, "Unable to open platform-def.json"

    assert (platform_dict is not None) , "platform_dict is None "

    port_dict, port_alias = parse_port_config_file(HWSKU_PATH + '/port_config.ini')

    if 'native-port-supported-speeds' not in platform_dict:
        assert False, "native-port-supported-speeds not defined"
    else:
        for key in platform_dict['native-port-supported-speeds']:
            intf_native = parse_interface_in_filter(key)
            lanes = port_dict[intf_native[0]].get('lanes').split(',')
            if intf_native[0] not in port_dict :
               assert False, "port {} not be defined in port map".format(intf_native[0])
            if len(lanes) == 1:
               if intf_native[-1] not in port_dict :
                  assert False, "port {} not be defined in port map".format(intf_native[-1])
            if len(lanes) == 4:
               if intf_native[-4] not in port_dict:
                  assert False, "port {} not be defined in port map".format(intf_native[-4])
            if len(lanes) == 8:
               if intf_native[-8] not in port_dict:
                  assert False, "port {} not be defined in port map".format(intf_native[-8])


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
