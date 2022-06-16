import pytest
import sys
import imp
import subprocess
import time

PLATFORM_PATH = "/usr/share/sonic/platform"
platform_chassis = None

def _wrapper_init():
    global platform_chassis

    # Load new platform api class
    if platform_chassis is None:
        try:
            import sonic_platform.platform
            platform_chassis = sonic_platform.platform.Platform().get_chassis()
        except Exception as e:
            print("Failed to load chassis due to {}".format(repr(e)))

def _wrapper_get_num_temp():
    _wrapper_init()
    if platform_chassis is not None:
        try:
            return platform_chassis.get_num_thermals()
        except NotImplementedError:
            pass
    return 0

def _wrapper_get_temperature(index):
    _wrapper_init()
    if platform_chassis is not None:
        try:
            return platform_chassis.get_thermal(index).get_temperature()
        except NotImplementedError:
            pass
    return -255


def _wrapper_get_high_threshold(index):
    _wrapper_init()
    if platform_chassis is not None:
        try:
            return platform_chassis.get_thermal(index).get_high_threshold()
        except NotImplementedError:
            pass
    return -255

def _wrapper_get_high_critical_threshold(index):
    _wrapper_init()
    if platform_chassis is not None:
        try:
            return platform_chassis.get_thermal(index).get_high_critical_threshold()
        except NotImplementedError:
            pass
    return -255

def _wrapper_get_fan_name(index):
    _wrapper_init()
    if platform_chassis is not None:
       try:
          return platform_chassis.get_fan(index).get_name()
       except NotImplementedError:
          pass

def _wrapper_get_fan_presence(index):
    _wrapper_init()
    if platform_chassis is not None:
       try:
           return platform_chassis.get_fan(index).get_presence()
       except NotImplementedError:
           pass

def _wrapper_get_fan_duty(index):
    _wrapper_init()
    if platform_chassis is not None:
       try:
          return platform_chassis.get_fan(index).get_target_speed()
       except NotImplementedError:
          pass

def _wrapper_get_fan_direction(index):
    _wrapper_init()
    if platform_chassis is not None:
       try:
           return platform_chassis.get_fan(index).get_direction()
       except NotImplementedError:
           pass

def _wrapper_get_psus_presence(index):
    _wrapper_init()
    if platform_chassis is not None:
       try:
           return platform_chassis.get_psu(index).get_presence()
       except NotImplementedError:
           pass

# test cases
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
    if json_config_data['PLATFORM']['modules']['TEMP']['support'] == "false":
       pytest.skip("Skip the testing due to the python module is not supported")

    assert _wrapper_get_num_temp() == json_config_data['PLATFORM']['num_temps']


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
    if json_config_data['PLATFORM']['modules']['TEMP']['support'] == "false":
       pytest.skip("Skip the testing due to the python module is not supported")

    for x in range(json_config_data['PLATFORM']['num_temps']):
        print("tmp{}: {}".format(x, _wrapper_get_temperature(x)))
        assert _wrapper_get_temperature(x) != -255, "tmp{}: invalid temperature".format(x)


def test_for_temp_high_threshold_read(json_config_data):
    """Test Purpose:  Verify that high threshold of each Temp sensors defined in the platform config
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
    if json_config_data['PLATFORM']['modules']['TEMP']['support'] == "false":
       pytest.skip("Skip the testing due to the python module is not supported")

    for x in range(json_config_data['PLATFORM']['num_temps']):
        print("tmp{}: {}".format(x, _wrapper_get_high_threshold(x)))
        assert _wrapper_get_high_threshold(x) != -255, "tmp{}: invalid high threshold".format(x)


def test_for_temp_high_critical_threshold_read(json_config_data):
    """Test Purpose:  Verify that high critical threshold of each Temp sensors defined in the platform config
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
    if json_config_data['PLATFORM']['modules']['TEMP']['support'] == "false":
       pytest.skip("Skip the testing due to the python module is not supported")

    for x in range(json_config_data['PLATFORM']['num_temps']):
        print("tmp{}: {}".format(x, _wrapper_get_high_critical_threshold(x)))
        assert _wrapper_get_high_critical_threshold(x) != -255, "tmp{}: invalid high critical threshold".format(x)

def test_for_thermal_daemon(json_config_data,json_test_data):

    if json_config_data['PLATFORM']['thermal_policy_support'] == "false":
       pytest.skip("Skip the testing due to thermal policy not supported")

    cp1 = []
    app = json_test_data['PLATFORM']['THERMAL_POLICY']['service_name']
    cmd = "cat /var/log/syslog | grep -c -i " + app + ".service"

    # check for syslog messages
    pin = subprocess.Popen(cmd,
                           shell=True,
                           close_fds=True,
                           stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT)
    cnt = pin.communicate()[0].decode('ascii')
    cp1.append(int(cnt))
    assert cp1, "flooding syslog [" + app + "] not detected"


def test_for_thermal_policy(json_config_data,json_test_data):

    if json_config_data['PLATFORM']['thermal_policy_support'] == "false":
       pytest.skip("Skip the testing due to thermal policy not supported")

    temp=0
    for x in range(json_config_data['PLATFORM']['num_temps']):
        temp += _wrapper_get_temperature(x)
        temp_avg = temp // json_config_data['PLATFORM']['num_temps']

    dir = _wrapper_get_fan_direction(x)
    for index in range(json_test_data['PLATFORM']['THERMAL_POLICY']['POLICY_NUM']):
        if dir == "exhaust":
           low = json_test_data['PLATFORM']['THERMAL_POLICY']['B2F'][str(index)][0]
           high = json_test_data['PLATFORM']['THERMAL_POLICY']['B2F'][str(index)][1]
           duty = json_test_data['PLATFORM']['THERMAL_POLICY']['B2F'][str(index)][2]
        else:
           low = json_test_data['PLATFORM']['THERMAL_POLICY']['F2B'][str(index)][0]
           high = json_test_data['PLATFORM']['THERMAL_POLICY']['F2B'][str(index)][1]
           duty = json_test_data['PLATFORM']['THERMAL_POLICY']['F2B'][str(index)][2]


        if temp_avg > high :
           continue
        else :
           assert duty == _wrapper_get_fan_duty(x), \
                  "FAN{}: duty mismatched".format(x+1)
           return

def test_for_thermal_policy_fan_removed(json_config_data,json_test_data):

    """Test Purpose:  Verify that DUTY change to full speed if any fan removed
                  The test require users to remove FAN before testing

            Args:
                    arg1 (json): platform-<sonic_platform>-config.json
                    arg2 (json): test-<sonic_platform>-config.json
            Example:
                    For a system that FAN REMOVE DUTY = 100

                    test-<sonic_platform>-config.json
                    {
                         "THERMAL_POLICY":: {
                             "FAN_REMOVED_DUTY":100
                         }
                    }
    """

    if json_config_data['PLATFORM']['thermal_policy_support'] == "false":
       pytest.skip("Skip the testing due to thermal policy not supported")

    duty = json_test_data['PLATFORM']['THERMAL_POLICY']['FAN_REMOVED_DUTY']

    # test if any one of FAN removed 
    Fan_removed = False
    for x in range(json_config_data['PLATFORM']['num_fans']):
        if _wrapper_get_fan_presence(x) == False:
           Fan_removed = True
           break

    if Fan_removed == False:
       pytest.skip("Skip the test due to All FAN are present")

    for x in range(json_config_data['PLATFORM']['num_fans']):
        if _wrapper_get_fan_presence(x) == True:
           assert duty == _wrapper_get_fan_duty(x), \
                  "FAN{}: duty mismatched".format(x+1)


def test_for_pmon_daemon(json_test_data):

    """Test Purpose:  Verify that if the pmon syslog message detected

    Args:
        arg1 (json): test-<sonic_platform>-config.json

    Example:
        To scan for the pmon syslog message by keywords: psud, fand

        test-<sonic_platform>-config.json
        {
            "PLATFORM": {
                "PMON":
                {
                    "syslog": [
                         "psud",
                         "fand"
                    ]
                }
            }
        }
    """
    cp1 = []
    cp2 = []
    pat = json_test_data["PLATFORM"]["PMON"]["syslog"]
    
    for idx in range(len(pat)):
        cmd = "cat /var/log/syslog | grep -c -i " + "'" + pat[idx] + \
              " entered RUNNING state" + "'"
        pin = subprocess.Popen(cmd,
                               shell=True,
                               close_fds=True,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT)
        cnt = pin.communicate()[0].decode('ascii')
        cp1.append(int(cnt))

    for idx1 in range(len(pat)):
        cmd = "cat /var/log/ramfs/in-memory-syslog-info.log | grep -c -i " + "'" + pat[idx1] + \
              " entered RUNNING state" + "'"
        pin = subprocess.Popen(cmd,
                               shell=True,
                               close_fds=True,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT)
        cnt = pin.communicate()[0]
        cp2.append(int(cnt))

    for index in range(len(pat)):
        assert cp1[index] != 0 or cp2[index] !=0, "pmon syslog [" + pat[index] + "] not detected"

def test_for_pmon_fan_removed_event_log(json_config_data,json_test_data):

    """Test Purpose:  Verify PMON event log if any fan removed
                      The test require users to remove FAN before testing

               Args:
                      arg1 (json): platform-<sonic_platform>-config.json
                      arg2 (json): test-<sonic_platform>-config.json

    """
    Fan_removed = False

    if json_config_data['PLATFORM']['modules']['FAN']['support'] == "false":
       pytest.skip("Skip the testing due to the python module is not supported in BSP")

    for x in range(json_config_data['PLATFORM']['num_fans']):
        if _wrapper_get_fan_presence(x) == False:
           Fan_removed = True
           fan_name = _wrapper_get_fan_name(x)
           cmd = "cat /var/log/syslog | grep -c -i " + "'" + fan_name + " .*removed" + "'"
           pin = subprocess.Popen(cmd,
                                  shell=True,
                                  close_fds=True,
                                  stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT)
           cnt = pin.communicate()[0].decode('ascii')
           assert int(cnt) != 0,\
                  "FAN" + str(x+1)+ "removed event log not detected"

    if Fan_removed == False:
       pytest.skip("Skip the test due to All FAN are present")



def test_for_pmon_psu_removed_event_log(json_config_data,json_test_data):

    """Test Purpose:  Verify PMON event log if any psu removed
                      The test require users to remove PSU before testing

             Args:
                    arg1 (json): platform-<sonic_platform>-config.json
                    arg2 (json): test-<sonic_platform>-config.json

    """
    Psu_removed = False

    for x in range(json_config_data['PLATFORM']['num_psus']):
        if _wrapper_get_psus_presence(x) == False:
           Psu_removed = True
           cmd = "cat /var/log/syslog | grep -c -i " + "'" + "PSU " + str(x+1) + " .*is not present." + "'"
           pin = subprocess.Popen(cmd,
                                  shell=True,
                                  close_fds=True,
                                  stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT)
           cnt = pin.communicate()[0].decode('ascii')
           assert int(cnt) != 0,\
                  "PSU" + str(x+1)+ "removed event log not detected"

    if Psu_removed == False:
       pytest.skip("Skip the test due to All PSU are present")

