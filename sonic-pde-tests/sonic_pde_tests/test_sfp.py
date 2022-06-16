#!/usr/bin/env python

try:
    import pytest
    import os
    import sys
    import subprocess
    import imp
    import json
    from natsort import natsorted
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

PLATFORM_PATH = "/usr/share/sonic/platform"

# Global class instance
platform_sfputil = None
platform_chassis = None

# Global port dictionaries
port_dict = None
first_phy_port =1

# Loads platform module from source
def _wrapper_init():
    global platform_chassis
    global platform_sfputil

    # Load new platform api class
    if platform_chassis is None:
       try:
           import sonic_platform.platform
           platform_chassis = sonic_platform.platform.Platform().get_chassis()
       except Exception as e:
           print("Failed to load chassis due to {}".format(repr(e)))

    #Load SfpUtilHelper class
    if platform_sfputil is None:
       try:
           import sonic_platform_base.sonic_sfp.sfputilhelper
           platform_sfputil = sonic_platform_base.sonic_sfp.sfputilhelper.SfpUtilHelper()
       except Exception as e:
           print("Failed to load chassis due to {}".format(repr(e)))

# Get platform specific HwSKU path
def get_hwsku_path():
    file = open(PLATFORM_PATH + "/default_sku", "r")
    line = file.readline()
    list = line.split()
    file.close()
    return PLATFORM_PATH + "/" + list[0]

# Loads platform port dictionaries
def load_platform_portdict():
    global port_dict

    if port_dict is not None:
        return

    port_dict = {}
    idx = 0
    file = open(get_hwsku_path() + "/port_config.ini", "r")
    line = file.readline()
    while line is not None and len(line) > 0:
        line = line.strip()
        if line.startswith("#"):
            line = file.readline()
            continue
        list = line.split()
        if len(list) >= 4:
            idx = int(list[3])
        port_dict[list[0]] = { "index": str(idx) }
        idx += 1
        line = file.readline()

    return port_dict

# Find out the physical port is 0-based or 1-based
def find_port_index_based():
    global first_phy_port
    
    # Load port info
    try:
         port_config_file_path = get_hwsku_path() + "/port_config.ini"
         platform_sfputil.read_porttab_mappings(port_config_file_path)
    except Exception as e:
         print("Failed to read port info: %s".format(repr(e)))

    if platform_sfputil.logical_to_physical[platform_sfputil.logical[0]][0] == 0:
       first_phy_port = 0
    elif platform_sfputil.logical_to_physical[platform_sfputil.logical[0]][0] == 1:
       first_phy_port = 1
    else:
       # default is 1-based
       first_phy_port = 1


def _wrapper_is_qsfp_port(physical_port):
    _wrapper_init()
    find_port_index_based()
    if "QSFP" in platform_chassis.get_sfp(physical_port).get_transceiver_info().get("type","N/A"):
        return True
    else:
        return False

def _wrapper_get_transceiver_info_type(physical_port):
    find_port_index_based()
    try:
        return platform_chassis.get_sfp(physical_port).get_transceiver_info().get("type","N/A")
    except NotImplementedError:
        pass

def _wrapper_get_num_sfps():
    _wrapper_init()
    load_platform_portdict()
    try:
       return platform_chassis.get_num_sfps()
    except NotImplementedError:
       pass

def _wrapper_get_presence(physical_port):
    _wrapper_init()
    find_port_index_based()
    try:
       return platform_chassis.get_sfp(physical_port).get_presence()
    except NotImplementedError:
       pass

def _wrapper_get_low_power_mode(physical_port):
    _wrapper_init()
    find_port_index_based()
    try:
       return platform_chassis.get_sfp(physical_port).get_lpmode()
    except NotImplementedError:
       pass

def _wrapper_set_low_power_mode(physical_port, enable):
    _wrapper_init()
    find_port_index_based()
    try:
       return platform_chassis.get_sfp(physical_port).set_lpmode(enable)
    except NotImplementedError:
       pass

def _wrapper_reset(physical_port):
    _wrapper_init()
    find_port_index_based()
    try:
       return platform_chassis.get_sfp(physical_port).reset()
    except NotImplementedError:
       pass

# Test for SFP port number
def test_for_sfp_number(json_config_data):
    """Test Purpose:  Verify that the numer of SFPs reported as supported by the SFP plugin matches what the platform supports.

    Args:
        arg1 (json): platform-<sonic_platform>-config.json

    Example:
        For a system that physically supports 32 (Q)SFP transceivers

        platform-<sonic_platform>-config.json
        {
            "PLATFORM": {
                "num_sfps": 32
            }
        }
        """
    for plat in json_config_data:
        assert _wrapper_get_num_sfps() == int(json_config_data[plat]["num_sfps"])

    return

# Test for SFP presence
def test_for_sfp_present(json_config_data, json_test_data):
    """Test Purpose:  Verify that the presence of SFPs reported by the SFP plugin matches the physical states of the DUT.

    Args:
        arg1 (json): platform-<sonic_platform>-config.json
        arg2 (json): test-<sonic_platform>-config.json

    Example:
        For a system with 4 (Q)SFP transceivers attached on index 13,17,52,68

        platform-<sonic_platform>-config.json
        {
            "PLATFORM": {
                "num_sfps": 32
            }
        }

        test-<sonic_platform>-config.json
        {
            "PLATFORM": {
                "present": [
                    13,
                    17,
                    52,
                    68
                ]
            }
        }
        """
    load_platform_portdict()
    for intf in natsorted(port_dict.keys()):
        port = int(port_dict[intf]['index'])
        bool = _wrapper_get_presence(port)
        for plat in json_config_data:
            list = json_test_data[plat]['SFP']['present']
            if port in list:
                assert bool == True, "SFP{} is not present unexpectedly".format(port)
            else:
                assert bool == False, "SFP{} is present unexpectedly".format(port)
    return

# Test for SFP EEPROM
def test_for_sfp_eeprom(json_config_data, json_test_data):
    """Test Purpose:  Verify that the content of SFP EEPROMs pass the check code test.

    Args:
        arg1 (json): platform-<sonic_platform>-config.json
        arg2 (json): test-<sonic_platform>-config.json

    Example:
        For a system with 4 (Q)SFP transceivers attached on index 13,17,52,68

        platform-<sonic_platform>-config.json
        {
            "PLATFORM": {
                "num_sfps": 32
            }
        }

        test-<sonic_platform>-config.json
        {
            "PLATFORM": {
                "present": [
                    13,
                    17,
                    52,
                    68
                ]
            }
        }
        """
    load_platform_portdict()
    for intf in natsorted(port_dict.keys()):
        port = int(port_dict[intf]['index'])
        bool = _wrapper_get_presence(port)
        for plat in json_config_data:
            list = json_test_data[plat]['SFP']['present']
            if port in list:
                assert bool == True, "SFP{} is not present unexpectedly".format(port)
                assert _wrapper_get_transceiver_info_type(port) is not None,"SFP{} EEPROM is fail to get".format(port)
            else:
                assert bool == False, "SFP{} is present".format(port)
    return

# Test for SFP LPMODE
def test_for_sfp_lpmode():
    """Test Purpose:  Verify that the LPMODE of QSFP is manipulatable via SFP plugin

    Args:
        None
        """
    load_platform_portdict()
    for intf in natsorted(port_dict.keys()):
        port = int(port_dict[intf]['index'])
        if _wrapper_get_presence(port):
          if not _wrapper_is_qsfp_port(port):
             continue
          try:
              bool = _wrapper_get_low_power_mode(port)
          except NotImplementedError:
              assert False, (intf + ': get_low_power_mode() is not implemented')
          if bool:
              try:
                  _wrapper_set_low_power_mode(port, False)
              except NotImplementedError:
                  assert False, (intf + ': low power detected while ' +
                                 'set_low_power_mode() is not implemented,' +
                                 'link errors are expected on high power modules')
    return

# Test for SFP LPMODE
def test_for_sfp_reset():
    """Test Purpose:  Verify that the RESET of QSFP is manipulatable via SFP plugin

    Args:
        None
        """
    load_platform_portdict()
    for intf in natsorted(port_dict.keys()):
        port = int(port_dict[intf]['index'])
        if _wrapper_get_presence(port):
          if not _wrapper_is_qsfp_port(port):
             continue
          try:
               bool = _wrapper_reset(port)
               assert bool, "reset failed"
          except NotImplementedError:
            # By defalt, it does no harm to have this unimplemented
            # This failure will only be observed when users try to manually
            # reset the module via CLI
            pytest.skip(intf + ": reset() is not implemented")
    return
