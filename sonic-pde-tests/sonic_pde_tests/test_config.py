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

def test_for_required_bcm_config_file(json_test_data):
    """ Test Purpose:  Verify the SAI config.bcm file (sai.profile) is present.

    Args:
        arg1 (json): test-<sonic_platform>-config.json

    Example:
    No test or platform JSON changes necesssary.
    """

    with open('/etc/sai.d/sai.profile') as data:
        for line in data:
            if line.find("SAI_PROFILE") == -1:
                contents = line.strip().split("=")
                config_file = contents[1]

    assert config_file != {}, "Chipset configuration file not found"

def test_for_required_bcm_config_settings(json_test_data):
    """Test Purpose:  Verify that the platform config.bcm file contains mandatory Silicon supported features.

    Args:
        arg1 (json): test-<sonic_platform>-config.json

    Example:
        Parity should always be enabled for Broadcom switching silicon.

        "PLATFORM": {
            "CONFIG": {
                "required": {
                    "config.bcm": [
                    "parity_enable=1"
                ]
            }
        }
        """

    with open('/etc/sai.d/sai.profile') as data:
        for line in data:
            if line.find("SAI_PROFILE") == -1:
                contents = line.strip().split("=")
                config_file = contents[1]

    assert config_file != {}, "Chipset configuration file not found"

    pat = json_test_data["PLATFORM"]["CONFIG"]["required"]["config.bcm"]
    # sampling pattern counts - start point
    for idx in range(len(pat)):
        cmd = "cat " + config_file + " | grep -c -i " + pat[idx]
        pin = subprocess.Popen(cmd,
                               shell=True,
                               close_fds=True,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT)
        cnt = pin.communicate()[0]
        print cnt

        assert int(cnt)==1, "Required configuration property [" + pat[idx] + "] not detected"



