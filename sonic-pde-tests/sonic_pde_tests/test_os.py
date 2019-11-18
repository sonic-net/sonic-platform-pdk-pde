#!/usr/bin/python

try:
    import pytest
    import json
    import time
    import os
    import sys
    import subprocess
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

# Global platform specific service port
service_port = None

# Init platform specific service port
def init_service_port(json_config_data):
    global service_port

    if service_port is not None:
        return service_port

    service_port = json_config_data["PLATFORM"].get("serviceport")
    if service_port is None:
        service_port = "eth0"

    return service_port

def test_for_service_port_presence(json_config_data):
    """Test Purpose:  Verify that if the service port is present on the DUT

    Args:
        arg1 (json): platform-<sonic_platform>-config.json

    Example:
        For a system that physically supports 1 service port

        platform-<sonic_platform>-config.json
        {
            "PLATFORM": {
                "num_serviceports": 1,
                "serviceport": "eth0"
            }
        }
        """
    init_service_port(json_config_data)
    path = "/sys/class/net/" + service_port
    assert os.path.isdir(path), service_port + ": no such device"
    return

def test_for_service_port_dhcp(json_config_data):
    """Test Purpose:  Verify that if the service port is able to obtain IP address via DHCP

    Args:
        arg1 (json): platform-<sonic_platform>-config.json

    Example:
        For a system that physically supports 1 service port

        platform-<sonic_platform>-config.json
        {
            "PLATFORM": {
                "num_serviceports": 1,
                "serviceport": "eth0"
            }
        }
        """
    init_service_port(json_config_data)
    # This is for test only, the IP of the interafce will not be changed
    # As the udhcpc script is not available in PDE docker, and it's intentional
    cmd = "busybox udhcpc -i " + service_port
    ret = subprocess.call(cmd + " > /dev/null 2>&1", shell=True)
    assert ret == 0, service_port + ": Unable to acquire DHCP address"
    return

def test_for_flooding_dmesg(json_test_data):
    """Test Purpose:  Verify that if the flooding dmesg message detected

    Args:
        arg1 (json): test-<sonic_platform>-config.json

    Example:
        To scan for the flooding dmesg message by keywords: error, i2c, usb, pci

        test-<sonic_platform>-config.json
        {
            "PLATFORM": {
                "OS":
                {
                    "flooding": {
                        "dmesg": [
                            "error",
                            "i2c",
                            "usb",
                            "pci"
                        ]
                    }
                }
            }
        }
        """
    cp1 = []
    cp2 = []
    pat = json_test_data["PLATFORM"]["OS"]["flooding"]["dmesg"]
    # sampling pattern counts - start point
    for idx in range(len(pat)):
        cmd = "dmesg | grep -c -i " + pat[idx]
        pin = subprocess.Popen(cmd,
                               shell=True,
                               close_fds=True,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT)
        cnt = pin.communicate()[0]
        cp1.append(int(cnt))

    # sampling period
    time.sleep(3)

    # sampling pattern counts - end point
    for idx in range(len(pat)):
        cmd = "dmesg | grep -c -i " + pat[idx]
        pin = subprocess.Popen(cmd,
                               shell=True,
                               close_fds=True,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT)
        cnt = pin.communicate()[0]
        cp2.append(int(cnt))

    for idx in range(len(pat)):
        assert cp2[idx] == cp1[idx], "flooding dmesg [" + pat[idx] + "] detected"

    return

def test_for_flooding_syslog(json_test_data):
    """Test Purpose:  Verify that if the flooding syslog message detected

    Args:
        arg1 (json): test-<sonic_platform>-config.json

    Example:
        To scan for the flooding syslog message by keywords: error, i2c, usb, pci

        test-<sonic_platform>-config.json
        {
            "PLATFORM": {
                "OS":
                {
                    "flooding": {
                        "syslog": [
                            "error",
                            "i2c",
                            "usb",
                            "pci"
                        ]
                    }
                }
            }
        }
        """
    cp1 = []
    cp2 = []
    pat = json_test_data["PLATFORM"]["OS"]["flooding"]["syslog"]
    # sampling pattern counts - start point
    for idx in range(len(pat)):
        cmd = "cat /var/log/syslog | grep -c -i " + pat[idx]
        pin = subprocess.Popen(cmd,
                               shell=True,
                               close_fds=True,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT)
        cnt = pin.communicate()[0]
        cp1.append(int(cnt))

    # sampling period
    time.sleep(3)

    # sampling pattern counts - end point
    for idx in range(len(pat)):
        cmd = "cat /var/log/syslog | grep -c -i " + pat[idx]
        pin = subprocess.Popen(cmd,
                               shell=True,
                               close_fds=True,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT)
        cnt = pin.communicate()[0]
        cp2.append(int(cnt))

    for idx in range(len(pat)):
        assert cp2[idx] == cp1[idx], "flooding syslog [" + pat[idx] + "] detected"

    return
