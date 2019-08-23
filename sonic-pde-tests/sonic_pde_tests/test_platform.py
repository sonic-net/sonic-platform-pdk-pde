try:
    import pytest
    import json
    import time
    import os
    import imp
    import sys
    import subprocess
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))


# Get Proc module info
PLATFORM_PATH = "/proc/modules"


def get_proc_module_info():
    file = open(PLATFORM_PATH, "r")
    str = file.read()
    file.close()
    return str

def getMacAddress(ifname):
    ETHERNET_PATH = "/sys/class/net/"+ifname+"/address"
    file = open(ETHERNET_PATH, "r")
    macaddr= file.read().strip()
    file.close
    return macaddr

def driver_unload(driver_name):
    cmd = "rmmod "+ driver_name
    os.system(cmd)
        
def driver_load(driver_name):
    cmd = "modprobe "+ driver_name
    os.system(cmd)


def test_for_cpld_driver_loading(json_config_data):

    """Test Purpose:  Verify that the cpld driver defined in platform config JSON
                      is successfully loaded in PDE

       Args:
           arg1 (json): platform-<sonic_platform>-config.json

       Example:

          "drivers": {
              "CPLD": {
               "driver_info": {
                   "support": "true",
                    "type": "ODM",
                    "name": "xxxx"
               }
              },
          }

    """

    if json_config_data['PLATFORM']['drivers']['CPLD']['driver_info']['support'] == "false":
       pytest.skip("Skip the testing due to the driver not supported")

    module_info = get_proc_module_info()
    cpld_driver_name = json_config_data['PLATFORM']['drivers']['CPLD']['driver_info']['name']

    assert module_info.find(cpld_driver_name) >= 0, \
    "Find {} driver is not loading".format(cpld_driver_name)

def test_for_psu_driver_loading(json_config_data):

    """Test Purpose:  Verify that the PSU driver defined in platform config JSON
                      is successfully loaded in PDE

       Args:
           arg1 (json): platform-<sonic_platform>-config.json

       Example:

          "drivers": {
              "PSU": {
               "driver_info": {
                   "support": "true",
                    "type": "ODM",
                    "name": "xxxx"
               }
              },
          }

    """

    if json_config_data['PLATFORM']['drivers']['PSU']['driver_info']['support'] == "false":
       pytest.skip("Skip the testing due to the driver not supported")

    module_info = get_proc_module_info()
    psu_driver_name = json_config_data['PLATFORM']['drivers']['PSU']['driver_info']['name']

    assert module_info.find(psu_driver_name) >= 0, \
    "Find {} driver is not loading successfully".format(psu_driver_name)

def test_for_psu_driver_unloading(json_config_data):

    """Test Purpose:  Verify that the PSU driver defined in platform config JSON
                      is successfully UNloaded in PDE

       Args:
           arg1 (json): platform-<sonic_platform>-config.json

       Example:

          "drivers": {
              "PSU": {
               "driver_info": {
                   "support": "true",
                    "type": "ODM",
                    "name": "xxxx"
               }
              },
          }

    """

    if json_config_data['PLATFORM']['drivers']['PSU']['driver_info']['support'] == "false":
       pytest.skip("Skip the testing due to the driver not supported")

    psu_driver_name = json_config_data['PLATFORM']['drivers']['PSU']['driver_info']['name']
    if get_proc_module_info().find(psu_driver_name) < 0:
       pytest.skip("Skip the testing due to the driver not loading")

    driver_unload(psu_driver_name)
    module_info = get_proc_module_info()

    if module_info.find(psu_driver_name) < 0:
       driver_load(psu_driver_name)

    assert module_info.find(psu_driver_name) < 0, \
    "Find {} driver is not unloading successfully".format(psu_driver_name)


def test_for_fan_driver_loading(json_config_data):

    """Test Purpose:  Verify that the FAN driver defined in platform config JSON
                  is successfully loaded in PDE

       Args:
           arg1 (json): platform-<sonic_platform>-config.json

       Example:

          "drivers": {
              "FAN": {
               "driver_info": {
                   "support": "true",
                    "type": "ODM",
                    "name": "xxxx"
               }
              },
          }

    """

    if json_config_data['PLATFORM']['drivers']['FAN']['driver_info']['support'] == "false":
       pytest.skip("Skip the testing due to the driver not supported")

    module_info = get_proc_module_info()
    fan_driver_name = json_config_data['PLATFORM']['drivers']['FAN']['driver_info']['name']

    assert module_info.find(fan_driver_name) >= 0, \
    "Find {} driver is not loading".format(fan_driver_name)

def test_for_fan_driver_unloading(json_config_data):

    """Test Purpose:  Verify that the FAN driver defined in platform config JSON
                      is successfully unloaded in PDE

       Args:
           arg1 (json): platform-<sonic_platform>-config.json

       Example:

          "drivers": {
              "FAN": {
               "driver_info": {
                   "support": "true",
                    "type": "ODM",
                    "name": "xxxx"
               }
              },
          }

    """
    
    if json_config_data['PLATFORM']['drivers']['FAN']['driver_info']['support'] == "false":
       pytest.skip("Skip the testing due to the driver not supported")

    fan_driver_name = json_config_data['PLATFORM']['drivers']['FAN']['driver_info']['name']
    if get_proc_module_info().find(fan_driver_name) < 0:
       pytest.skip("Skip the testing due to the driver not loading")

    driver_unload(fan_driver_name)
    module_info = get_proc_module_info()

    if module_info.find(fan_driver_name) < 0:
       driver_load(fan_driver_name)

    assert module_info.find(fan_driver_name) < 0, \
    "Find {} driver is not unloading successfully".format(fan_driver_name)


def test_for_sfp_driver_loading(json_config_data):

    """Test Purpose:  Verify that the sfp driver defined in platform config JSON
                  is successfully loaded in PDE 

       Args:
           arg1 (json): platform-<sonic_platform>-config.json

       Example:

          "drivers": {
              "SFP": {
               "driver_info": {
                   "support": "true",
                    "type": "ODM",
                    "name": "xxxx"
               }
              },
          }

    """

    if json_config_data['PLATFORM']['drivers']['SFP']['driver_info']['support'] == "false":
       pytest.skip("Skip the testing due to the driver not supported")

    module_info = get_proc_module_info()
    sfp_driver_name = json_config_data['PLATFORM']['drivers']['SFP']['driver_info']['name']

    assert module_info.find(sfp_driver_name) >= 0, \
    "Find {} driver is not loading".format(sfp_driver_name)


def test_for_sfp_driver_unloading(json_config_data):

    """Test Purpose:  Verify that the sfp driver defined in platform config JSON
                      is successfully unloaded in PDE

       Args:
           arg1 (json): platform-<sonic_platform>-config.json

       Example:

          "drivers": {
              "SFP": {
               "driver_info": {
                   "support": "true",
                    "type": "ODM",
                    "name": "xxxx"
               }
              },
          }

    """

    if json_config_data['PLATFORM']['drivers']['SFP']['driver_info']['support'] == "false":
       pytest.skip("Skip the testing due to the driver not supported")

    sfp_driver_name = json_config_data['PLATFORM']['drivers']['SFP']['driver_info']['name']
    if get_proc_module_info().find(sfp_driver_name) < 0:
       pytest.skip("Skip the testing due to the driver not loading")

    driver_unload(sfp_driver_name)
    module_info = get_proc_module_info()

    if module_info.find(sfp_driver_name) < 0:
       driver_load(sfp_driver_name)

    assert module_info.find(sfp_driver_name) < 0, \
    "Find {} driver is not unloading successfully".format(sfp_driver_name)



def test_for_temp_driver_loading(json_config_data):

    """Test Purpose:  Verify that the temperature sensor driver defined in platform config JSON
                      is successfully loaded in PDE

       Args:
           arg1 (json): platform-<sonic_platform>-config.json

       Example:

          "drivers": {
              "TEMP": {
               "driver_info": {
                   "support": "true",
                    "type": "ODM",
                    "name": "xxxx"
               }
              },
          }

    """

    if json_config_data['PLATFORM']['drivers']['TEMP']['driver_info']['support'] == "false":
       pytest.skip("Skip the testing due to the driver not supported")

    module_info = get_proc_module_info()
    temp_driver_name = json_config_data['PLATFORM']['drivers']['TEMP']['driver_info']['name']

    assert module_info.find(temp_driver_name) >= 0, \
    "Find {} driver is not loading".format(temp_driver_name)


def test_for_temp_driver_unloading(json_config_data):

    """Test Purpose:  Verify that the temperature driver defined in platform config JSON
                      is successfully unloaded in PDE

       Args:
           arg1 (json): platform-<sonic_platform>-config.json

       Example:

          "drivers": {
              "TEMP": {
               "driver_info": {
                   "support": "true",
                    "type": "ODM",
                    "name": "xxxx"
               }
              },
          }

    """

    if json_config_data['PLATFORM']['drivers']['TEMP']['driver_info']['support'] == "false":
       pytest.skip("Skip the testing due to the driver not supported")

    temp_driver_name = json_config_data['PLATFORM']['drivers']['TEMP']['driver_info']['name']
    if get_proc_module_info().find(temp_driver_name) < 0:
       pytest.skip("Skip the testing due to the driver not loading")

    driver_unload(temp_driver_name)
    module_info = get_proc_module_info()

    if module_info.find(temp_driver_name) < 0:
       driver_load(temp_driver_name)

    assert module_info.find(temp_driver_name) < 0, \
    "Find {} driver is not unloading successfully".format(temp_driver_name)



def test_for_led_driver_loading(json_config_data):

    """Test Purpose:  Verify that the SYSLED driver defined in platform config JSON
                      is successfully loaded in PDE

       Args:
           arg1 (json): platform-<sonic_platform>-config.json

       Example:

          "drivers": {
              "SYSLED": {
               "driver_info": {
                   "support": "true",
                    "type": "ODM",
                    "name": "xxxx"
               }
              },
          }

    """

    if json_config_data['PLATFORM']['drivers']['SYSLED']['driver_info']['support'] == "false":
       pytest.skip("Skip the testing due to the driver not supported")

    module_info = get_proc_module_info()
    led_driver_name = json_config_data['PLATFORM']['drivers']['SYSLED']['driver_info']['name']

    assert module_info.find(led_driver_name) >= 0, \
    "Find {} driver is not loading".format(led_driver_name)



def test_for_led_driver_unloading(json_config_data):

    """Test Purpose:  Verify that the SYSLED driver defined in platform config JSON
                      is successfully loaded in PDE

       Args:
           arg1 (json): platform-<sonic_platform>-config.json

       Example:

          "drivers": {
              "SYSLED": {
               "driver_info": {
                   "support": "true",
                    "type": "ODM",
                    "name": "xxxx"
               }
              },
          }

    """

    if json_config_data['PLATFORM']['drivers']['SYSLED']['driver_info']['support'] == "false":
       pytest.skip("Skip the testing due to the driver not supported")

    led_driver_name = json_config_data['PLATFORM']['drivers']['SYSLED']['driver_info']['name']
    if get_proc_module_info().find(led_driver_name) < 0:
       pytest.skip("Skip the testing due to the driver not loading")

    driver_unload(led_driver_name)
    module_info = get_proc_module_info()

    if module_info.find(led_driver_name) < 0:
       driver_load(led_driver_name)

    assert module_info.find(led_driver_name) < 0, \
    "Find {} driver is not unloading successfully".format(led_driver_name)



def test_for_eeprom_driver_loading(json_config_data):

    """Test Purpose:  Verify that the EEPROM driver defined in platform config JSON
                      is successfully loaded in PDE

       Args:
           arg1 (json): platform-<sonic_platform>-config.json

       Example:

          "drivers": {
              "EEPROM": {
               "driver_info": {
                   "support": "true",
                    "type": "ODM",
                    "name": "xxxx"
               }
              },
          }

    """

    if json_config_data['PLATFORM']['drivers']['EEPROM']['driver_info']['support'] == "false":
       pytest.skip("Skip the testing due to the driver not supported")

    module_info = get_proc_module_info()
    eeprom_driver_name = json_config_data['PLATFORM']['drivers']['EEPROM']['driver_info']['name']

    assert module_info.find(eeprom_driver_name) >= 0, \
    "Find {} driver is not loading".format(eeprom_driver_name)


def test_for_eeprom_driver_unloading(json_config_data):

    """Test Purpose:  Verify that the EEPROM driver defined in platform config JSON
                      is successfully unloaded in PDE 

       Args:
           arg1 (json): platform-<sonic_platform>-config.json

       Example:

          "drivers": {
              "EEPROM": {
               "driver_info": {
                   "support": "true",
                    "type": "ODM",
                    "name": "xxxx"
               }
              },
          }

    """

    if json_config_data['PLATFORM']['drivers']['EEPROM']['driver_info']['support'] == "false":
       pytest.skip("Skip the testing due to the driver not supported")

    eeprom_driver_name = json_config_data['PLATFORM']['drivers']['EEPROM']['driver_info']['name']
    if get_proc_module_info().find(eeprom_driver_name) < 0:
       pytest.skip("Skip the testing due to the driver not loading")

    driver_unload(eeprom_driver_name)
    module_info = get_proc_module_info()

    if module_info.find(eeprom_driver_name) < 0:
       driver_load(eeprom_driver_name)

    assert module_info.find(eeprom_driver_name) < 0, \
    "Find {} driver is not unloading successfully".format(eeprom_driver_name)



def test_for_mac_driver_loading(json_config_data):

    """Test Purpose:  Verify that the MAC driver defined in platform config JSON
                      is successfully loaded in PDE 

       Args:
           arg1 (json): platform-<sonic_platform>-config.json

       Example:

          "drivers": {
              "MAC": {
               "driver_info": {
                   "support": "true",
                    "type": "ODM",
                    "name": "xxxx"
               }
              },
          }

    """

    if json_config_data['PLATFORM']['drivers']['MAC']['driver_info']['support'] == "false":
       pytest.skip("Skip the testing due to the driver not supported")

    module_info = get_proc_module_info()
    mac_driver_name = json_config_data['PLATFORM']['drivers']['MAC']['driver_info']['name']

    assert module_info.find(mac_driver_name) >= 0, \
    "Find {} driver is not loading".format(mac_driver_name)

def test_for_mac_driver_unloading(json_config_data):

    """Test Purpose:  Verify that the MAC driver defined in platform config JSON
                      is successfully unloaded in PDE 

       Args:
           arg1 (json): platform-<sonic_platform>-config.json

       Example:

          "drivers": {
              "MAC": {
               "driver_info": {
                   "support": "true",
                    "type": "ODM",
                    "name": "xxxx"
               }
              },
          }

    """

    if json_config_data['PLATFORM']['drivers']['MAC']['driver_info']['support'] == "false":
       pytest.skip("Skip the testing due to the driver not supported")

    mac_driver_name = json_config_data['PLATFORM']['drivers']['MAC']['driver_info']['name']
    if get_proc_module_info().find(mac_driver_name) < 0:
       pytest.skip("Skip the testing due to the driver not loading")

    driver_unload(mac_driver_name)
    module_info = get_proc_module_info()

    if module_info.find(mac_driver_name) < 0:
       driver_load(mac_driver_name)

    assert module_info.find(mac_driver_name) < 0, \
    "Find {} driver is not unloading successfully".format(mac_driver_name)


def test_for_ethernet_mac_address(json_config_data,json_test_data):

    for key in json_config_data:
        for x in range(json_config_data[key]['num_serviceports']):
            ifname = json_test_data[key]['MAC']['MAC'+str(x+1)]['ifname']
            assert getMacAddress(ifname) == json_test_data['PLATFORM']['MAC']['MAC'+str(x+1)]['macaddr'], \
            "Service MAC address {} not matching with JSON configuration".format(getMacAddress(ifname))



