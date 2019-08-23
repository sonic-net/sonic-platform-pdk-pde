#!/usr/bin/env python
#
#  parse_port_config.py
#
# Command-line utility for parsing SONiC database for port entries
# and generating a test_host.ini file for pde-test-host application
#

try:
    import sys
    import os
    import subprocess
    import imp
    import syslog
    import types
    import json
    import logging
    import errno
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

def get_port_config_file_name(hwsku=None, platform=None):
    port_config_candidates = []
    port_config_candidates.append('/usr/share/sonic/hwsku/port_config.ini')
    if hwsku:
        if platform:
            port_config_candidates.append(os.path.join('/usr/share/sonic/device', platform, hwsku, 'port_config.ini'))
        port_config_candidates.append(os.path.join('/usr/share/sonic/platform', hwsku, 'port_config.ini'))
        port_config_candidates.append(os.path.join('/usr/share/sonic', hwsku, 'port_config.ini'))
    for candidate in port_config_candidates:
        if os.path.isfile(candidate):
            return candidate
    return None

def get_port_config(hwsku=None, platform=None, port_config_file=None):
    if not port_config_file:
        port_config_file = get_port_config_file_name(hwsku, platform)
        if not port_config_file:
            return ({}, {})
    return parse_port_config_file(port_config_file)


def parse_port_config_file(port_config_file):
    port_names = {}
    port_lanes = {}
    port_alias = {}
    port_index = {}
    port_speed = {}
    port_valid_speed = {}
    # Default column definition
    titles = ['name', 'lanes', 'alias', 'index' 'speed' 'valid_speeds']
    line_index=0
    with open(port_config_file) as data:
        for line in data:
            if line.startswith('#'):
                if "name" in line:
                    titles = line.strip('#').split()
                continue;
            tokens = line.split()

            if len(tokens) < 6:
                print("Unknown port_config.ini entry!\n")
                continue

            port_names[line_index]=tokens[0]
            port_lanes[line_index]=tokens[1]
            port_alias[line_index]=tokens[2]
            port_index[line_index]=tokens[3]
            port_speed[line_index]=tokens[4]
            port_valid_speed[line_index]=tokens[5]
            line_index+=1

    return (port_names, port_lanes, port_alias, port_index, port_speed, port_valid_speed)

# Get platform name from ONIE
def get_onie_pname():
    cmd = "cat /host/machine.conf | grep onie_platform | cut -d '=' -f 2"
    pin = subprocess.Popen(cmd,
                           shell=True,
                           close_fds=True,
                           stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT)
    id = pin.communicate()[0]
    id = id.strip()
    return id

if __name__ == '__main__':

    total_ports = 0

    bcm_port_list_str = "BCM_PORT_LIST="
    sai_port_index_list_str = "SAI_PORT_INDEX_LIST="
    bcm_port_names_str = "BCM_PORT_NAMES="
    pdk_traffic_test_port_list_str = "PORT_CONFIG_INDEX_LIST="

    (port_names, port_lanes, port_alias, port_index, port_speed, port_valid_speed) = get_port_config()

    for x in port_names:
        number_of_lanes=port_lanes[x].count(",")+1
        lane_list=port_lanes[x].split(",")
        highest_lane_num = max(lane_list)

        if (total_ports > 0):
            bcm_port_list_str = bcm_port_list_str + ","
            sai_port_index_list_str = sai_port_index_list_str + ","

        bcm_port_list_str = bcm_port_list_str + highest_lane_num

        sai_port_index_list_str = sai_port_index_list_str + "%d" % (total_ports + 1)

        total_ports +=1

    filename = "/usr/lib/python2.7/dist-packages/sonic_pde_tests/data/platform/test-" + get_onie_pname() + "-config.json"
    try:
        with open ("/home/pde/test-config.json") as test_config:
            port_dict = json.load(test_config)
    except EnvironmentError as e:
        try:
            with open (filename) as test_config:
                port_dict = json.load(test_config)
        except Exception as e:
            logging.error(e)
            sys.exit()
    except Exception as e:
        logging.error(e)
        sys.exit()

    total_test_ports=0
    list = port_dict["PLATFORM"]["TRAFFIC"]["port_pairs"]
    for ports in list:
        if (total_test_ports > 0):
            pdk_traffic_test_port_list_str = pdk_traffic_test_port_list_str + ","
            bcm_port_names_str = bcm_port_names_str + ","

        pdk_traffic_test_port_list_str = pdk_traffic_test_port_list_str + ports['src_front_portnum'] + "," + ports['dst_front_portnum']
        total_test_ports += 2

        bcm_port_names_str = bcm_port_names_str + ports['src_logical_portnum'] + "," + ports['dst_logical_portnum']

    print "####################################################################################"
    print "#This file was auto generated"
    print "#Please refer to the PDE SONiC Developers Guide for instructions"
    print "#on how to modify contents according to your port configuration."
    print "####################################################################################"
    print ""
    print "NUM_PORTS_IN_TEST=%d" % total_ports
    print "NUM_PORTS_IN_TRAFFIC_TEST=%d" % total_test_ports
    print "%s" % bcm_port_list_str
    print "%s" % sai_port_index_list_str
    print "%s" % bcm_port_names_str
    print "%s" % pdk_traffic_test_port_list_str
