import pytest
import sys
import imp
import subprocess
import time


# Global platform-specific cpldutil class instance
platform_cpldutil = None

# Loads platform specific cpldutil module from source
def load_platform_cpldutil(json_config_data):
    global platform_cpldutil

    if platform_cpldutil is not None:
       return

    try:
        if json_config_data['PLATFORM']['modules']['CPLD']['support'] == "false":
           pytest.skip("Skip the testing due to the module is not supported in BSP")

        modules_dir = json_config_data['PLATFORM']['modules']['CPLD']['path']
        modules_name = json_config_data['PLATFORM']['modules']['CPLD']['name']
        class_name = json_config_data['PLATFORM']['modules']['CPLD']['class']
        cpld_module = "/usr/share/sonic/classes/" + modules_dir + "/" + modules_name + '.py'
        platform_cpldutil_module = imp.load_source(modules_name,cpld_module)
        platform_cpldutil_class = getattr(platform_cpldutil_module,class_name)
        platform_cpldutil = platform_cpldutil_class()

    except AttributeError as e:
        print("Failed to instantiate '%s' class: %s" % (class_name, str(e)), True)

    return


def test_for_num_cpld(json_config_data):
    """Test Purpose:  Verify that the numer of CPLD reported as supported
                      by the CPLD plugin matches what the platform supports

       Args:
           arg1 (json): platform-<sonic_platform>-config.json

       Example:
           For a system that physically supports 2 CPLD

           platform-<sonic_platform>-config.json
           {
               "PLATFORM": {
                   "num_cplds": 2
               }
           }
    """

    load_platform_cpldutil(json_config_data)
    assert platform_cpldutil.get_num_cplds() == json_config_data['PLATFORM']['num_cplds'],\
    "verify CPLD numbers {} not matching with platform JSON".format(platform_cpldutil.get_num_cplds())

def test_for_cpld_read(json_config_data,json_test_data):
    """Test Purpose:  Test Purpose: Verify that the CPLD version able to read and value
                      is matching with the value defined in test config JSON

       Args:
           arg1 (json): platform-<sonic_platform>-config.json
           arg2 (json): test-<sonic_platform>-config.json

       Example:
           For a system that physically supports 2 CPLD, the CPLD version defined in the
           test-<sonic_platform>-config.json

           "CPLD": {
                 "CPLD1": {
                   "version": "2"
                 },
                 "CPLD2": {
                   "version": "1"
                 }
               },


    """

    load_platform_cpldutil(json_config_data)
    for key in json_config_data:
        for x in range(json_config_data[key]['num_cplds']):
            assert platform_cpldutil.get_cpld_version(x+1).strip() == json_test_data[key]['CPLD']['CPLD'+str(x+1)]['version'], \
            "verify" + " CPLD"+str(x+1)+" version={} is False".format(platform_cpldutil.get_cpld_version(x+1))

if __name__ == '__main__':
   unittest.main()

