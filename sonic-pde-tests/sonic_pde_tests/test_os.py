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
        cnt = pin.communicate()[0].decode('ascii')
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
        cnt = pin.communicate()[0].decode('ascii')
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
        cnt = pin.communicate()[0].decode('ascii')
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
        cnt = pin.communicate()[0].decode('ascii')
        cp2.append(int(cnt))

    for idx in range(len(pat)):
        assert cp2[idx] == cp1[idx], "flooding syslog [" + pat[idx] + "] detected"

    return

def test_for_fstrim():
    """Test Purpose:  Verify that if TRIM properlly enabled for SSDs

    Args:
        None

    Note:
        Periodic TRIM is always enabled on the SONiC platforms, and it launches
        '/sbin/fstrim -av' weekly
        """
    ssd = False
    opts = subprocess.check_output("lsblk -l -o NAME,TYPE | grep disk | head -1", shell=True).decode('ascii')
    disk = opts.split()[0]

    chk1 = subprocess.check_output("cat /sys/class/block/{0}/queue/discard_granularity".format(disk), shell=True).decode('ascii')
    chk1 = chk1.strip()
    chk2 = subprocess.check_output("cat /sys/class/block/{0}/queue/discard_max_bytes".format(disk), shell=True).decode('ascii')
    chk2 = chk2.strip()
    if (int(chk1, 10) == 0) or (int(chk2, 10) == 0):
        pytest.skip("'{0}' does not support TRIM".format(disk))

    if disk.startswith("nvme"):
        ssd = True
    else:
        rota = subprocess.check_output("cat /sys/class/block/{0}/queue/rotational".format(disk), shell=True).decode('ascii')
        rota = rota.strip()
        ssd = (int(rota, 10) > 0)

    if not ssd:
        pytest.skip("'{0}' is not a SSD".format(disk))

    mtab = subprocess.check_output("cat /etc/mtab | grep '/dev/{0}' | head -1".format(disk), shell=True).decode('ascii')
    opts = mtab.split()[3]
    assert "discard" in opts, "{0}: discard/TRIM is not enabled".format(disk)
    return

def test_for_cpuload():
    """Test Purpose:  Verify that if CPU load is not higher than expected

    Args:
        None

    Note:
        This test should only be launched when swss/syncd is disabled or without running traffic
        """
    lower_bound = 90.0
    data = subprocess.check_output("iostat -c 1 3", shell=True).decode('ascii')
    info = False
    load = 0.0
    for row in data.split('\n'):
        if row.startswith('avg-cpu'):
            info = True
            continue
        if info:
           load += float(row.split()[5])
        info = False

    load /= 3
    load = 100 - load
    assert load < lower_bound, "CPU load is too high"
