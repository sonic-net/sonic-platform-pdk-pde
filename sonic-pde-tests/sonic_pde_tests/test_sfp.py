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
PLATFORM_SPECIFIC_MODULE_NAME = "sfputil"
PLATFORM_SPECIFIC_CLASS_NAME = "SfpUtil"

# Global platform-specific sfputil class instance
platform_sfputil = None

# Global port dictionaries
port_dict = None

# Loads platform specific sfputil module from source
def load_platform_sfputil():
    global platform_sfputil

    if platform_sfputil is not None:
        return

    try:
        module_file = "/".join([PLATFORM_PATH, "plugins", PLATFORM_SPECIFIC_MODULE_NAME + ".py"])
        module = imp.load_source(PLATFORM_SPECIFIC_MODULE_NAME, module_file)
    except IOError, e:
        print("Failed to load platform module '%s': %s" % (PLATFORM_SPECIFIC_MODULE_NAME, str(e)), True)

    assert module is not None

    try:
        platform_sfputil_class = getattr(module, PLATFORM_SPECIFIC_CLASS_NAME)
        platform_sfputil = platform_sfputil_class()
    except AttributeError, e:
        print("Failed to instantiate '%s' class: %s" % (PLATFORM_SPECIFIC_CLASS_NAME, str(e)), True)

    assert platform_sfputil is not None
    return

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
    load_platform_sfputil()
    load_platform_portdict()
    num = 0
    for intf in natsorted(port_dict.keys()):
        port = int(port_dict[intf]['index'])
        if not platform_sfputil._is_valid_port(port):
            continue
        num += 1

    for plat in json_config_data:
        assert num == int(json_config_data[plat]["num_sfps"])

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
    load_platform_sfputil()
    load_platform_portdict()
    for intf in natsorted(port_dict.keys()):
        port = int(port_dict[intf]['index'])
        if not platform_sfputil._is_valid_port(port):
            continue
        bool = platform_sfputil.get_presence(port)
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
    load_platform_sfputil()
    load_platform_portdict()
    for intf in natsorted(port_dict.keys()):
        port = int(port_dict[intf]['index'])
        if not platform_sfputil._is_valid_port(port):
            continue
        bool = platform_sfputil.get_presence(port)
        for plat in json_config_data:
            list = json_test_data[plat]['SFP']['present']
            if port in list:
                assert bool == True, "SFP{} is not present unexpectedly".format(port)
                code = 0
                data = platform_sfputil.get_eeprom_raw(port)
                assert data != None, "SFP{}: unable to read EEPROM".format(port)
                if port in platform_sfputil.osfp_ports:
                    #OSFP/QSFP-DD
                    for i in range(128, 222):
                        code += int(data[i], 16)
                    assert (code & 0xff) == int(data[222], 16), "check code error"
                elif port in platform_sfputil.qsfp_ports:
                    #QSFP
                    for i in range(128, 191):
                        code += int(data[i], 16)
                    assert (code & 0xff) == int(data[191], 16), "check code error"
                else:
                    #SFP/SFP+
                    for i in range(0, 63):
                        code += int(data[i], 16)
                    assert (code & 0xff) == int(data[63], 16), "check code error"
            else:
                assert bool == False, "SFP{} is present".format(port)
    return

# Test for SFP LPMODE
def test_for_sfp_lpmode():
    """Test Purpose:  Verify that the LPMODE of QSFP is manipulatable via SFP plugin

    Args:
        None
        """
    load_platform_sfputil()
    load_platform_portdict()
    for intf in natsorted(port_dict.keys()):
        port = int(port_dict[intf]['index'])
        if not platform_sfputil._is_valid_port(port):
            continue
        if port not in platform_sfputil.qsfp_ports:
            continue
        try:
            bool = platform_sfputil.get_low_power_mode(port)
        except NotImplementedError:
            assert False, (intf + ': get_low_power_mode() is not implemented')
        if bool:
            try:
                platform_sfputil.set_low_power_mode(port, False)
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
    load_platform_sfputil()
    load_platform_portdict()
    for intf in natsorted(port_dict.keys()):
        port = int(port_dict[intf]['index'])
        if not platform_sfputil._is_valid_port(port):
            continue
        if port not in platform_sfputil.qsfp_ports:
            continue
        try:
            bool = platform_sfputil.reset(port)
            assert bool, "reset failed"
        except NotImplementedError:
            # By defalt, it does no harm to have this unimplemented
            # This failure will only be observed when users try to manually
            # reset the module via CLI
            pytest.skip(intf + ": reset() is not implemented")
    return
