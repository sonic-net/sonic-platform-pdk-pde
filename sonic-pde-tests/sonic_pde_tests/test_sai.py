#!/usr/bin/python

import sys
import time
import pytest
import subprocess
import os.path

import saiut

PLATFORM_PATH = "/usr/share/sonic/platform"
HWSKU_PATH = "/usr/share/sonic/hwsku"

# Global switch core initialization flag
core_init = False

# Global port dictionaries
port_dict = None

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
                key = lst[0];
                val = int(lst[1].split(',')[0], 10)
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
        port = saiut.portGetId(port_dict[key])
        assert port != 0, "{}: port id not found".format(key)
    return

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
    for key in port_dict:
        port = saiut.portGetId(port_dict[key])
        assert port != 0, "{}: port id not found".format(key)
        saiut.portSetAdminState(port, True)
    time.sleep(3)

    lnks = json_test_data['PLATFORM']['SELF_LOOPS']
    for idx in range(0, len(lnks)):
        p1 = lnks[idx].split(':')[0]
        p2 = lnks[idx].split(':')[1]
        pid1 = saiut.portGetId(port_dict[p1])
        pid2 = saiut.portGetId(port_dict[p2])

        print("xmit: {} --> {}".format(p1, p2));
        saiut.portClearCounter(pid1, saiut.SAI_PORT_STAT_ETHER_OUT_PKTS_512_TO_1023_OCTETS)
        saiut.portClearCounter(pid2, saiut.SAI_PORT_STAT_ETHER_IN_PKTS_512_TO_1023_OCTETS)
        saiut.bcmSendPackets(pid1, 9, 512)
        time.sleep(3)
        c1 = saiut.portGetCounter(pid1, saiut.SAI_PORT_STAT_ETHER_OUT_PKTS_512_TO_1023_OCTETS)
        c2 = saiut.portGetCounter(pid2, saiut.SAI_PORT_STAT_ETHER_IN_PKTS_512_TO_1023_OCTETS)
        assert c1 >= 9, "{} --> {}: tx failed".format(p1, p2)
        assert c2 >= 9, "{} --> {}: rx failed".format(p1, p2)

        print("xmit: {} <-- {}".format(p1, p2));
        saiut.portClearCounter(pid1, saiut.SAI_PORT_STAT_ETHER_IN_PKTS_512_TO_1023_OCTETS)
        saiut.portClearCounter(pid2, saiut.SAI_PORT_STAT_ETHER_OUT_PKTS_512_TO_1023_OCTETS)
        saiut.bcmSendPackets(pid2, 9, 512)
        time.sleep(3)
        c1 = saiut.portGetCounter(pid1, saiut.SAI_PORT_STAT_ETHER_IN_PKTS_512_TO_1023_OCTETS)
        c2 = saiut.portGetCounter(pid2, saiut.SAI_PORT_STAT_ETHER_OUT_PKTS_512_TO_1023_OCTETS)
        assert c1 >= 9, "{} <-- {}: rx failed".format(p1, p2)
        assert c2 >= 9, "{} <-- {}: tx failed".format(p1, p2)

# Test for switch port breakout modes
def test_for_switch_port_breakout(json_test_data):
    """
    Test Purpose:  Verify the QSFP port breakout support

    Args:
        arg1 (json): test-<sonic_platform>-config.json

    Example:
        For a system with 2 expandable ports

        test-<sonic_platform>-config.json
        {
            "PLATFORM": {
                "PORT_BREAKOUT" : {
                    # port Name      port modes <default>,<breakout1>,<breakout2>
                    "Ethernet128" : "1x100,4x10,4x25",
                    "Ethernet136" : "1x100,4x10,4x25"
                }
            }
        }
    """
    try:
        intf = json_test_data['PLATFORM']['PORT_BREAKOUT']
    except KeyError:
        pytest.skip("test configuration not found")

    # load key-value-pairs from sai.profile
    kvp = {}
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

    # make a backup of config.bcm and port_config.ini
    file = kvp['SAI_INIT_CONFIG_FILE']
    rc = subprocess.call("cp -f {} {}.orig >/dev/null 2>&1".format(file, file), shell=True)
    assert rc == 0, "Unable to make a backup of the config.bcm"
    file = HWSKU_PATH + '/port_config.ini'
    rc = subprocess.call("cp -f {} {}.orig >/dev/null 2>&1".format(file, file), shell=True)
    assert rc == 0, "Unable to make a backup of the port_config.ini"

    # perform port breakout test
    for intf in json_test_data['PLATFORM']['PORT_BREAKOUT']:
        # mode = <root>,<breakout1>,<breakout2>
        mode = json_test_data['PLATFORM']['PORT_BREAKOUT'][intf]
        list = mode.split(',')
        for i in range(1, len(list)):
            print("port_breakout.py -p {} -o {}".format(intf, list[i]));
            # root port mode
            rc = subprocess.call("port_breakout.py -p {} -o {} >/dev/null 2>&1".format(intf, list[0]), shell=True)
            assert rc == 0, "{}: Unable to do {}".format(intf, list[i])
            # target breakout mode
            rc = subprocess.call("port_breakout.py -p {} -o {} >/dev/null 2>&1".format(intf, list[i]), shell=True)
            assert rc == 0, "{}: Unable to do {}".format(intf, list[i])

            # load sonic portmap
            intf_dict = {}
            file = open(HWSKU_PATH + '/port_config.ini', 'r')
            line = file.readline()
            while line is not None and len(line) > 0:
                line = line.strip()
                if line.startswith("#"):
                    line = file.readline()
                    continue
                tok = line.split()
                if len(tok) >= 2:
                    key = tok[0];
                    val = int(tok[1].split(',')[0], 10)
                    assert key not in intf_dict, "Duplicated interface in port_config.ini"
                    intf_dict[key] = val
                line = file.readline()
            file.close()

            # load bcm portmap
            brcm_dict = {}
            file = open(kvp['SAI_INIT_CONFIG_FILE'], 'r')
            line = file.readline()
            while line is not None and len(line) > 0:
                line = line.strip()
                if line.startswith("#") or ("portmap_" not in line):
                    line = file.readline()
                    continue
                # portmap_<lport>=<pport>:<speed>:<lane>
                tok = line.split('=')
                lport = int(tok[0].split('_')[1], 10)
                pport = int(tok[1].split(':')[0], 10)
                brcm_dict[pport] = lport
                line = file.readline()
            file.close()

            for port in intf_dict:
                lane = intf_dict[port]
                assert lane in brcm_dict, "{}: portmap not found".format(port)

    # restore the config.bcm and port_config.ini
    file = kvp['SAI_INIT_CONFIG_FILE']
    rc = subprocess.call("cp -f {}.orig {} >/dev/null 2>&1".format(file, file), shell=True)
    assert rc == 0, "Unable to restore the config.bcm"
    file = HWSKU_PATH + '/port_config.ini'
    rc = subprocess.call("cp -f {}.orig {} >/dev/null 2>&1".format(file, file), shell=True)
    assert rc == 0, "Unable to restore the port_config.ini"
