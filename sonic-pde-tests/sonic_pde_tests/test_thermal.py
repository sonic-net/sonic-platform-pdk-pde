import pytest
import sys
import imp
import subprocess
import time


# Global platform-specific thermalutil class instance
platform_temputil = None

# Loads platform specific thermalutil module from source
def load_platform_temputil(json_config_data):
    global platform_temputil

    if platform_temputil is not None:
       return

    try:
        if json_config_data['PLATFORM']['modules']['TEMP']['support'] == "false":
           pytest.skip("Skip the testing due to the python module is not supported")

        modules_dir = json_config_data['PLATFORM']['modules']['TEMP']['path']
        modules_name = json_config_data['PLATFORM']['modules']['TEMP']['name']
        class_name = json_config_data['PLATFORM']['modules']['TEMP']['class']
        modules_temp = "/usr/share/sonic/classes/" + modules_dir + "/" + modules_name + '.py'
        platform_temputil_module = imp.load_source(modules_name, modules_temp)
        platform_temputil_class = getattr(platform_temputil_module, class_name)
        platform_temputil = platform_temputil_class()

    except AttributeError, e:
        print("Failed to instantiate '%s' class: %s" % (platform_temputil_class, str(e)), True)

    return


def test_for_num_temp(json_config_data):
    """Test Purpose:  Verify that the numer of Temp sensors reported as supported
                      by the Termal plugin matches what the platform supports

       Args:
            arg1 (json): platform-<sonic_platform>-config.json

       Example:
            For a system that physically supports 4 TEMPs

            platform-<sonic_platform>-config.json
            {
                 "PLATFORM": {
                               "num_temps": 4
                 }
            }
    """

    load_platform_temputil(json_config_data)

    assert platform_temputil.get_num_thermals() == json_config_data['PLATFORM']['num_temps'],\
    "verify TEMP numbers = {} not matching with platform JSON".format(platform_temputil.get_num_thermals())



def test_for_temp_read(json_config_data):
    """Test Purpose:  Verify that each of the Temp sensors defined in the platform config
                      josn is able to read


               Args:
                    arg1 (json): platform-<sonic_platform>-config.json

            Example:
                    For a system that physically supports 4 TEMPs

                    platform-<sonic_platform>-config.json
                    {
                         "PLATFORM": {
                             num_temps": 4
                         }
                    }
    """


    load_platform_temputil(json_config_data)

    for x in range(json_config_data['PLATFORM']['num_temps']):
        assert platform_temputil._get_thermal_node_val(x+1) != None, \
        "verify TEMP"+str(x+1)+" read is False".format(platform_temputil._get_thermal_node_val(x+1))


if __name__ == '__main__':
   unittest.main()



