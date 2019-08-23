import pytest
import sys
import imp
import subprocess
import time

DUTY_MAX = 100


# Global platform-specific fanutil class instance
platform_fanutil = None

# Loads platform specific fanutil module from source
def load_platform_fanutil(json_config_data):
    global platform_fanutil

    if platform_fanutil is not None:
       return

    try:
        if json_config_data['PLATFORM']['modules']['FAN']['support'] == "false":
           pytest.skip("Skip the testing due to the python module is not supported in BSP")

        modules_dir = json_config_data['PLATFORM']['modules']['FAN']['path']
        modules_name = json_config_data['PLATFORM']['modules']['FAN']['name']
        class_name = json_config_data['PLATFORM']['modules']['FAN']['class']
        module_fan = "/usr/share/sonic/classes/" + modules_dir + "/" + modules_name + '.py'
        module = imp.load_source(modules_name, module_fan)
        platform_fanutil_class = getattr(module,class_name)
        platform_fanutil = platform_fanutil_class()

    except AttributeError, e:
        print("Failed to instantiate '%s' class: %s" % (class_name, str(e)), True)

    return


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

    load_platform_fanutil(json_config_data)

    assert platform_fanutil.get_num_fans() == json_config_data['PLATFORM']['num_fans'],\
    "verify FAN numbers = {} not matching with platform JSON".format(platform_fanutil.get_num_fans())

@pytest.mark.semiauto
def test_for_fans_dir(json_config_data,json_test_data):
    """Test Purpose:  Verify that the FAN direction reported as supported
                      by the FAN plugin matches with test config JSON

       Args:
             arg1 (json): platform-<sonic_platform>-config.json
             arg2 (json): test-<sonic_platform>-config.json

       Example:
             Test config JSON setting for FAN direction

            "FAN": {
               "FAN1": {
                  "direction": 0
               },
               "FAN2": {
                  "direction": 0
               },
               "FAN3": {
                  "direction": 0
               },
               "FAN4": {
                  "direction": 0
               }
            }
    """
    load_platform_fanutil(json_config_data)

    for key in json_config_data:
        for x in json_test_data[key]['FAN']['present']:
            assert platform_fanutil.get_fan_dir(x) == json_test_data[key]['FAN']['FAN'+str(x)]['direction'],\
            "verify FAN"+str(x)+" DIR = {} is invalid".format(platform_fanutil.get_fan_dir(x))

def test_for_fans_status(json_config_data,json_test_data):

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

    load_platform_fanutil(json_config_data)

    for key in json_config_data:
        for x in json_test_data[key]['FAN']['present']:
            assert platform_fanutil.get_fan_status(x) == True,\
            "verify FAN"+str(x)+" status = {} is False".format(platform_fanutil.get_fan_status(x))


def test_for_fans_present(json_config_data,json_test_data):
    load_platform_fanutil(json_config_data)

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


    for key in json_config_data:
        list = json_test_data[key]['FAN']['present']
        for x in range(json_config_data['PLATFORM']['num_fans']):
            if x in list:
               assert platform_fanutil.get_fan_status(x) == True,\
               "FAN"+str(x)+" is not present, status = {}".format(platform_fanutil.get_fan_status(x))
            else:
               assert platform_fanutil.get_fan_status(x) == None,\
               "FAN"+str(x)+" is present, status = {}".format(platform_fanutil.get_fan_status(x))


def test_for_fans_duty(json_config_data):
    """Test Purpose:Verify that the FAN Duty Get and Set is working fine by using the FAN module
                    defined in platform config JSON
    
               args: 
                    arg1 (json): platform-<sonic_platform>-config.json

               example: 
                    Platform config JSON for FAN modules support

                    platform-<sonic_platform>-config.json
                    {
                      "modules" : {
                           "FAN": {
                               "support" : "true/false",
                               "path": "XXXX",
                               "name": "XXXX",
                               "class": "XXXX"
                           },
                       }
                    }
                    
    """

    load_platform_fanutil(json_config_data)

    old_duty = platform_fanutil.get_fan_duty_cycle()
    new_duty = old_duty
    platform_fanutil.set_fan_duty_cycle(DUTY_MAX)
    timeout = 0

    while( new_duty < DUTY_MAX ):
           time.sleep(1)
           new_duty = platform_fanutil.get_fan_duty_cycle()
           timeout = timeout+1
           if timeout > 10:
              break

    assert platform_fanutil.get_fan_duty_cycle() == DUTY_MAX,\
           "verify changging FAN Duty to MAX is False".format(platform_fanutil.get_fan_duty_cycle())

    platform_fanutil.set_fan_duty_cycle(old_duty)


if __name__ == '__main__':
   unittest.main()

