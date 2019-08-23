import pytest
import sys
import imp
import subprocess
import time


# Global platform-specific sysledutil class instance
platform_sysledutil = None

# Loads platform specific sysledutil module from source
def load_platform_sysledutil(json_config_data):
    global platform_sysledutil

    if platform_sysledutil is not None:
       return

    try:
    
        if json_config_data['PLATFORM']['modules']['SYSLED']['support'] == "false":
           pytest.skip("Skip the testing due to the python module is not supported")

        modules_dir = json_config_data['PLATFORM']['modules']['SYSLED']['path']
        modules_name = json_config_data['PLATFORM']['modules']['SYSLED']['name']
        class_name = json_config_data['PLATFORM']['modules']['SYSLED']['class']
        sysledutil_module = "/usr/share/sonic/classes/" + modules_dir + "/" + modules_name + '.py'
        platform_sysledutil_module = imp.load_source(modules_name,sysledutil_module)
        platform_sysledutil_class = getattr(platform_sysledutil_module,class_name)
        platform_sysledutil = platform_sysledutil_class()

    except AttributeError, e:
        print("Failed to instantiate '%s' class: %s" % (class_name, str(e)), True)

    return


def test_for_set_fan_sysled(json_config_data, json_test_data):

    """Test Purpose:  Verify that the SYS FAN LED is able to set per the color and state
	                  defined in the test config JSON

       Args:
           arg1 (json): platform-<sonic_platform>-config.json
           arg2 (json): test-<sonic_platform>-config.json

       Example:
            SYSLED Color state setting for the FAN defined in JSON
            configured in the test-<sonic_platform>-config.json

           "SYSLED":
           {
                "FAN":{
                   "color":"GREEN/RED/AMBER",
                   "state" : "SOLID/BLINKING"
                },

    """

    load_platform_sysledutil(json_config_data)
    for key in json_config_data:
        color = json_test_data[key]['SYSLED']['FAN']['color']
        state = json_test_data[key]['SYSLED']['FAN']['state']
        index = 0 
        platform_sysledutil.set_status_led('fan', index, color, state)

        assert platform_sysledutil.get_status_led('fan', index) == state,\
        "fan"+str(index)+" sys led color is fail to changed, status = {}".format(get_status_led('fan', index))

def test_for_set_fantray_sysled(json_config_data, json_test_data):

    """Test Purpose:  Verify that the SYS FANTRAY LED is able to set per the color and state
                      defined in the test config JSON			  

       Args:
           arg1 (json): platform-<sonic_platform>-config.json
           arg2 (json): test-<sonic_platform>-config.json

    Example:
           SYSLED Color state setting for the FANTRAY defined in JSON
           configured in the test-<sonic_platform>-config.json


           "SYSLED":
           {
               "FANTRAY":{
                  "FANTRAY1":{
                    "color":"GREEN/RED/AMBER",
                    "state" : "SOLID/BLINKING"
                  },
                  "FANTRAY2":{
                    "color":"GREEN/RED/AMBER",
                    "state" : "SOLID/BLINKING"
                  }
               },


    """



    load_platform_sysledutil(json_config_data)
    for key in json_config_data:
        for x in range(json_config_data['PLATFORM']['num_fans']):
            index = x+1
            color = json_test_data[key]['SYSLED']['FANTRAY']['FANTRAY'+str(index)]['color']
            state = json_test_data[key]['SYSLED']['FANTRAY']['FANTRAY'+str(index)]['state']
            platform_sysledutil.set_status_led('fantray', index, color, state)
            assert platform_sysledutil.get_status_led('fantray', index) == state,\
            "fantray"+str(index)+" sys led color is fail to changed, status = {}".format(get_status_led('fantray', index))

def test_for_set_psu_sysled(json_config_data, json_test_data):
    """Test Purpose:  Verify that the SYS PSU LED is able to set per the color and state
	                  defined in the test config JSON
					  

       Args:
           arg1 (json): platform-<sonic_platform>-config.json
           arg2 (json): test-<sonic_platform>-config.json

       Example:
           SYSLED Color state setting for the FANTRAY configured in the test-<sonic_platform>-config.json


           "SYSLED":
           {
               "PSU":{
                 "PSU1":{
                   "color":"GREEN/RED/AMBER",
                   "state" : "SOLID/BLINKING"
                 },
                 "PSU2":{
                   "color":"GREEN/RED/AMBER",
                   "state" : "SOLID/BLINKING"
                 }
               },
           
       
    """


    load_platform_sysledutil(json_config_data)
    for key in json_config_data:
        for x in range(json_config_data['PLATFORM']['num_psus']):
            index = x+1
            color = json_test_data[key]['SYSLED']['PSU']['PSU'+str(index)]['color']
            state = json_test_data[key]['SYSLED']['PSU']['PSU'+str(index)]['state']
            platform_sysledutil.set_status_led('psu', index, color, state)
            assert platform_sysledutil.get_status_led('psu', index) == state,\
            "psu"+str(index)+" sys led color is fail to changed, status = {}".format(get_status_led('psu', index))


def test_for_set_sys_sysled(json_config_data, json_test_data):
    """Test Purpose:  Verify that the SYS LED is able to set per the color and stated
                      defined in the test config JSON

       Args:
           arg1 (json): platform-<sonic_platform>-config.json
           arg2 (json): test-<sonic_platform>-config.json

    Example:
           SYSLED Color state setting configured in the test-<sonic_platform>-config.json

           "SYSLED":
           {
               "SYS":{
                 "color":"GREEN/RED/AMBER",
                 "state" : "SOLID/BLINKING"
               },


    """
    load_platform_sysledutil(json_config_data)
    for key in json_config_data:
        color = json_test_data[key]['SYSLED']['SYS']['color']
        state = json_test_data[key]['SYSLED']['SYS']['state']
        index = 0 
        platform_sysledutil.set_status_led('sys', index, color, state)

        assert platform_sysledutil.get_status_led('sys', index) == state,\
        "fan"+str(index)+" sys led color is fail to changed, status = {}".format(get_status_led('sys', index))


def test_for_set_loc_sysled(json_config_data, json_test_data):

    """Test Purpose:  Verify that the SYS LOC LED is able to set per the color and state
	                  defined in the test config JSON

       Args:
           arg1 (json): platform-<sonic_platform>-config.json
           arg2 (json): test-<sonic_platform>-config.json

       Example:
            SYS LOC LED Color state setting configured in the test-<sonic_platform>-config.json


           "SYSLED":
           {
               "LOC":{
                       "color":"GREEN/RED/AMBER",
                       "state" : "SOLID/BLINKING"
                }


    """

    load_platform_sysledutil(json_config_data)
    for key in json_config_data:
        color = json_test_data[key]['SYSLED']['LOC']['color']
        state = json_test_data[key]['SYSLED']['LOC']['state']
        index = 0 
        platform_sysledutil.set_status_led('loc', index, color, state)

        assert platform_sysledutil.get_status_led('loc', index) == state,\
        "fan"+str(index)+" sys led color is fail to changed, status = {}".format(get_status_led('sys', index))



