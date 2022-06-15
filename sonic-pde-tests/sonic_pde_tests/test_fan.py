import pytest
import sys
import imp
import subprocess
import time

DUTY_MIN = 50
DUTY_MAX = 100


PLATFORM_PATH = "/usr/share/sonic/platform"
PLATFORM_SPECIFIC_FAN_MODULE_NAME = "fanutil"
PLATFORM_SPECIFIC_FAN_CLASS_NAME = "FanUtil"
PLATFORM_SPECIFIC_PSU_MODULE_NAME = "psuutil"
PLATFORM_SPECIFIC_PSU_CLASS_NAME = "PsuUtil"

# Global platform-specific fanutil class instance
platform_fanutil = None
platform_chassis = None

# Loads platform specific module from source
def _wrapper_init():
    global platform_chassis
    global platform_fanutil

    # Load new platform api class
    if platform_chassis is None:
        try:
            import sonic_platform.platform
            platform_chassis = sonic_platform.platform.Platform().get_chassis()
        except Exception as e:
            print("Failed to load chassis due to {}".format(repr(e)))


    # Load platform-specific fanutil class
    if platform_chassis is None:
       try:
             module_file = "/".join([PLATFORM_PATH, "plugins", PLATFORM_SPECIFIC_FAN_MODULE_NAME + ".py"])
             module = imp.load_source(PLATFORM_SPECIFIC_FAN_MODULE_NAME, module_file)
             platform_fanutil_class = getattr(module, PLATFORM_SPECIFIC_FAN_CLASS_NAME)
             platform_fanutil = platform_fanutil_class()

             module_file = "/".join([PLATFORM_PATH, "plugins", PLATFORM_SPECIFIC_PSU_MODULE_NAME + ".py"])
             module = imp.load_source(PLATFORM_SPECIFIC_PSU_MODULE_NAME, module_file)
             platform_psuutil_class = getattr(module, PLATFORM_SPECIFIC_PSU_CLASS_NAME)
             platform_psuutil = platform_psuutil_class()

       except Exception as e:
             print("Failed to load fanutil due to {}".format(repr(e)))

       assert (platform_chassis is not None) or (platform_fanutil is not None), "Unable to load platform module"

# wrappers that are compliable with both new platform api and old-style plugin
def _wrapper_get_num_fans():
    _wrapper_init()
    if platform_chassis is not None:
        try:
            return platform_chassis.get_num_fans()
        except NotImplementedError:
            pass
    return platform_fanutil.get_num_fans()

def _wrapper_get_fan_direction(index):
    _wrapper_init()
    if platform_chassis is not None:
        try:
            return platform_chassis.get_fan(index).get_direction()
        except NotImplementedError:
            pass
    return platform_fanutil.get_direction(index+1)

def _wrapper_get_psu_direction(index):
    _wrapper_init()
    if platform_chassis is not None:
        try:
            return platform_chassis.get_psu(index)._fan_list[0].get_direction()
        except NotImplementedError:
            pass
    return platform_psuutil.get_direction(index+1)


def _wrapper_get_fan_status(index):
    _wrapper_init()
    if platform_chassis is not None:
        try:
            return platform_chassis.get_fan(index).get_status()
        except NotImplementedError:
            pass
    return platform_fanutil.get_status(index+1)

def _wrapper_get_fan_presence(index):
    _wrapper_init()
    if platform_chassis is not None:
        try:
            return platform_chassis.get_fan(index).get_presence()
        except NotImplementedError:
            pass
    return platform_fanutil.get_presence(index+1)

def _wrapper_get_fan_duty(index):
    _wrapper_init()
    if platform_chassis is not None:
        try:
            return platform_chassis.get_fan(index).get_speed()
        except NotImplementedError:
            pass
    return platform_fanutil.get_speed(index+1)

def _wrapper_set_fan_duty(index, duty):
    _wrapper_init()
    if platform_chassis is not None:
        try:
            return platform_chassis.get_fan(index).set_speed(duty)
        except NotImplementedError:
            pass
    return platform_fanutil.set_speed(index+1)

# test cases
def test_for_num_fans(json_config_data):
    """Test Purpose:  Verify that the numer of FANs reported as supported
                      by the FAN plugin matches what the platform supports

        Args:
             arg1 (json): platform-<sonic_platform>-config.json

        Example:
             For a system that physically supports 4 FAN

             platform-<sonic_platform>-config.json
             {
                "PLATFORM": {
                              "num_fans": 4
                            }
             }
    """
    if json_config_data['PLATFORM']['modules']['FAN']['support'] == "false":
       pytest.skip("Skip the testing due to the python module is not supported in BSP")

    assert _wrapper_get_num_fans() == json_config_data['PLATFORM']['num_fans'],\
           "The number of FAN does not match the platform JSON"

def test_for_fans_dir(json_config_data, json_test_data):
    """Test Purpose:  Verify that the FAN direction reported as supported
                      by the FAN plugin matches with test config JSON

       Args:
             arg1 (json): platform-<sonic_platform>-config.json
             arg2 (json): test-<sonic_platform>-config.json

       Example:
             Test config JSON setting for FAN direction

            "FAN": {
               "FAN1": {
                  "direction": "exhaust"
               },
               "FAN2": {
                  "direction": "exhaust"
               }
            }
    """
    if json_config_data['PLATFORM']['modules']['FAN']['support'] == "false":
       pytest.skip("Skip the testing due to the python module is not supported in BSP")

    for key in json_config_data:
        for x in json_test_data[key]['FAN']['present']:
            assert _wrapper_get_fan_direction(x-1).lower() == json_test_data[key]['FAN']['FAN'+str(x)]['direction'],\
                   "FAN{}: DIR mismatched".format(x)
            for j in json_test_data[key]['PSU']['present']:
                assert _wrapper_get_psu_direction(j-1).lower() == _wrapper_get_fan_direction(x-1).lower(),\
                   "FAN{}: PSU{} DIR mismatched".format(x,j)

def test_for_fans_status(json_config_data, json_test_data):

    """Test Purpose:  Verify that the FAN status of each present FAN
                      reported as supported by the FAN plugin matches
                      with test config JSON

        Args:
              arg1 (json): platform-<sonic_platform>-config.json
              arg2 (json): test-<sonic_platform>-config.json

        Example:
              Test config JSON setting for FAN direction

              "FAN": {
                 "present": [
                        1,
                        2,
                        3,
                        4
                  ],
              },
    """
    if json_config_data['PLATFORM']['modules']['FAN']['support'] == "false":
       pytest.skip("Skip the testing due to the python module is not supported in BSP")

    for key in json_config_data:
        for x in json_test_data[key]['FAN']['present']:
            assert _wrapper_get_fan_status(x-1) == True,\
                   "FAN{}: status is False".format(x)


def test_for_fans_present(json_config_data, json_test_data):
    """Test Purpose:  Verify that the FAN present reported as supported by the 
                      FAN plugin matches with test config JSON

            Args:
                  arg1 (json): platform-<sonic_platform>-config.json
                  arg2 (json): test-<sonic_platform>-config.json

            Example:
                  Test config JSON setting for FAN present

                  "FAN": {
                     "present": [
                            1,
                            2,
                            3,
                            4
                      ],
                  },
    """
    if json_config_data['PLATFORM']['modules']['FAN']['support'] == "false":
       pytest.skip("Skip the testing due to the python module is not supported in BSP")

    for key in json_config_data:
        list = json_test_data[key]['FAN']['present']
        for x in range(json_config_data['PLATFORM']['num_fans']):
            if x + 1 in list:
               assert _wrapper_get_fan_presence(x) == True,\
                      "FAN{}: is not present unexpectedly".format(x + 1)
            else:
               assert _wrapper_get_fan_presence(x) == False,\
                      "FAN{}: is present unexpectedly".format(x + 1)


def test_for_fans_duty(json_config_data, json_test_data):
    """Test Purpose:Verify that the FAN Duty Set is working fine

               args:
                    arg1 (json): platform-<sonic_platform>-config.json

               example:
                    For a system that physically supports 4 FAN

                    platform-<sonic_platform>-config.json
                    {
                        "PLATFORM": {
                            "num_fans": 4
                        }
                    }
    """
    if json_config_data['PLATFORM']['modules']['FAN']['support'] == "false":
       pytest.skip("Skip the testing due to the python module is not supported in BSP")
	   
    if json_config_data['PLATFORM']['modules']['FAN']['fan_duty_control_support'] == "false":
       pytest.skip("Skip the testing due to the BMC platform fan control is not supported in BSP")

    # Only verify FAN Duty set ;; FAN Read will be verified in Thermal Test
    # start FAN speed/duty test
    for key in json_config_data:
        for x in range(0, _wrapper_get_num_fans()):
            if _wrapper_get_fan_presence(x) == False:
                continue;
            # low speed test
            assert _wrapper_set_fan_duty(x, DUTY_MIN), \
                   "FAN{}: Unable to update fan speed".format(x+1)

            # high speed test
            assert _wrapper_set_fan_duty(x, DUTY_MAX), \
                   "FAN{}: Unable to update fan speed".format(x+1)



