import pytest
import sys
import imp
import subprocess
import time

PLATFORM_SPECIFIC_MODULE_NAME = "psuutil"
PLATFORM_SPECIFIC_CLASS_NAME = "PsuUtil"

def test_for_num_psus(json_config_data):
    """Test Purpose:  Verify that the numer of PSUs reported as supported by the PSU plugin matches what the platform supports.

    Args:
        arg1 (json): platform-<sonic_platform>-config.json

    Example:
        For a system that physically supports 2 power supplies

        platform-<sonic_platform>-config.json
        {
            "PLATFORM": {
                "num_psus": 2
            }
        }
        """
    module = imp.load_source("psuutil","/usr/share/sonic/platform/plugins/psuutil.py")
    platform_psuutil_class = getattr(module, PLATFORM_SPECIFIC_CLASS_NAME)
    platform_psuutil = platform_psuutil_class()
    assert platform_psuutil.get_num_psus() == json_config_data['PLATFORM']['num_psus'],"System plugin reports that {} PSUs are supported in platform".format(platform_psuutil.get_num_psus())

def test_for_psu_present(json_config_data, json_test_data):
    """Test Purpose:  Test Purpose: Verify that the PSUs that are present report as present in the PSU plugin.

    Args:
        arg1 (json): platform-<sonic_platform>-config.json
        arg2 (json): test-<sonic_platform>-config.json

    Example:
        For a system that has 2 power supplies present

        test-<sonic_platform>-config.json
        {
            "PLATFORM": {
                "PSU": {
                    "present": [
                        1,
                        2
                    ],
                }
            }
       }

    """

    module = imp.load_source("psuutil","/usr/share/sonic/platform/plugins/psuutil.py")
    platform_psuutil_class = getattr(module, PLATFORM_SPECIFIC_CLASS_NAME)
    platform_psuutil = platform_psuutil_class()

    for key in json_config_data:
        psupresentlist = json_test_data[key]['PSU']['present']
        for x in psupresentlist:
            assert platform_psuutil.get_psu_presence(x-1) == True, "System plugin reported PSU {} was not present".format(x)

def test_for_psu_notpresent(json_config_data, json_test_data):
    """Test Purpose: Verify that the PSUs that are not present report as not present in the PSU plugin.

    Args:
        arg1 (json): platform-<sonic_platform>-config.json
        arg2 (json): test-<sonic_platform>-config.json

    Example:
        For a system that only has power supply 2 present

        {
            "PLATFORM": {
                "PSU": {
                    "present": [
                        2
                     ]
                }
            }
       }

    """

    module = imp.load_source("psuutil","/usr/share/sonic/platform/plugins/psuutil.py")
    platform_psuutil_class = getattr(module, PLATFORM_SPECIFIC_CLASS_NAME)
    platform_psuutil = platform_psuutil_class()
    num_psus = platform_psuutil.get_num_psus()
    for key in json_config_data:
        for x in range (1, num_psus):
            if platform_psuutil.get_psu_presence(x-1) == True:
                Found = False;
            for y in json_test_data[key]['PSU']['present']:
                if x == y:
                    Found = True
            assert (Found == True), "System plugin reported PSU {} was present".format(x)

def test_for_psu_status(json_config_data, json_test_data):
    """Test Purpose: Verify that the PSUs that are not present report proper status (True if operating properly, False if not operating properly)

    Args:
        arg1 (json): platform-<sonic_platform>-config.json
        arg2 (json): test-<sonic_platform>-config.json

    Example:
        For a system that only has power supply 2 present and both are operating properly

        test-<sonic_platform>-config.json
        {
            "PLATFORM": {
                "PSU": {
                    "present": [
                        1,
                        2
                    ],
                "status": [
                    "True",
                    "True"
                ]
            }
        }

    """

    module = imp.load_source("psuutil","/usr/share/sonic/platform/plugins/psuutil.py")
    platform_psuutil_class = getattr(module, PLATFORM_SPECIFIC_CLASS_NAME)
    platform_psuutil = platform_psuutil_class()

    for key in json_config_data:
        psupresentlist = json_test_data[key]['PSU']['present']
        for x in psupresentlist:
            assert platform_psuutil.get_psu_status(x-1) != json_test_data[key]['PSU']['status'], "System plugin reported PSU {} state did not match test state {}".format(x, json_test_data[key]['PSU']['status'])


