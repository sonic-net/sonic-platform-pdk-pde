import pytest
import sys
import imp
import subprocess
import time

PLATFORM_PATH = "/usr/share/sonic/platform"

# Global platform class instance
platform_chassis = None

# Loads platform module from source
def _wrapper_init():
    global platform_chassis

    # Load new platform api class
    if platform_chassis is None:
       try:
           import sonic_platform.platform
           platform_chassis = sonic_platform.platform.Platform().get_chassis()
       except Exception as e:
           print("Failed to load chassis due to {}".format(repr(e)))

def _wrapper_get_num_psus():
    _wrapper_init()
    if platform_chassis is not None:
       try:
           return platform_chassis.get_num_psus()
       except NotImplementedError:
           pass

def _wrapper_get_psus_presence(psu_index):
    _wrapper_init()
    if platform_chassis is not None:
       try:
           return platform_chassis.get_psu(psu_index).get_presence()
       except NotImplementedError:
           pass

def _wrapper_get_psus_status(psu_index):
    _wrapper_init()
    if platform_chassis is not None:
       try:
           return platform_chassis.get_psu(psu_index).get_powergood_status()
       except NotImplementedError:
           pass

def _wrapper_get_psus_serial(psu_index):
    _wrapper_init()
    if platform_chassis is not None:
       try:
           return platform_chassis.get_psu(psu_index).get_serial()
       except NotImplementedError:
           pass

def _wrapper_get_psus_model(psu_index):
    _wrapper_init()
    if platform_chassis is not None:
       try:
           return platform_chassis.get_psu(psu_index).get_model()
       except NotImplementedError:
           pass

def _wrapper_get_psus_power(psu_index):
    _wrapper_init()
    if platform_chassis is not None:
       try:
           return platform_chassis.get_psu(psu_index).get_power()
       except NotImplementedError:
           pass

def _wrapper_get_psus_current(psu_index):
    _wrapper_init()
    if platform_chassis is not None:
       try:
           return platform_chassis.get_psu(psu_index).get_current()
       except NotImplementedError:
           pass

def _wrapper_get_psus_voltage(psu_index):
    _wrapper_init()
    if platform_chassis is not None:
       try:
          return platform_chassis.get_psu(psu_index).get_voltage()
       except NotImplementedError:
          pass

def _wrapper_get_psus_input_voltage(psu_index):
    _wrapper_init()
    if platform_chassis is not None:
       try:
          return platform_chassis.get_psu(psu_index).get_input_voltage()
       except NotImplementedError:
          pass

def _wrapper_get_psus_input_current(psu_index):
    _wrapper_init()
    if platform_chassis is not None:
       try:
          return platform_chassis.get_psu(psu_index).get_input_current()
       except NotImplementedError:
          pass

def _wrapper_get_psus_type(psu_index):
    _wrapper_init()
    if platform_chassis is not None:
       try:
          return platform_chassis.get_psu(psu_index).get_type()
       except NotImplementedError:
          pass

def _wrapper_get_psus_capacity(psu_index):
    _wrapper_init()
    if platform_chassis is not None:
       try:
          return platform_chassis.get_psu(psu_index).get_capacity()
       except NotImplementedError:
          pass

def is_valid_string(str):
    for c in str:
        v = ord(c)
        if (v < 32) or (v > 126):
            return False
    return True

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
    assert _wrapper_get_num_psus() == json_config_data['PLATFORM']['num_psus'],"System plugin reports that {} PSUs are supported in platform".format(platform_psuutil.get_num_psus())

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
    for key in json_config_data:
        psupresentlist = json_test_data[key]['PSU']['present']
        for x in psupresentlist:
            assert _wrapper_get_psus_presence(x-1) == True, "System plugin reported PSU {} was not present".format(x)

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
    num_psus = _wrapper_get_num_psus()
    for key in json_config_data:
        for x in range (1, num_psus):
            if _wrapper_get_psus_presence(x-1) == True:
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
                    true,
                    true
                ]
            }
        }

    """
    for key in json_config_data:
        psupresentlist = json_test_data[key]['PSU']['present']
        for x in psupresentlist:
            assert _wrapper_get_psus_status(x-1) == json_test_data[key]['PSU']['status'][x-1], "System plugin reported PSU{} state did not match test state {}".format(x, json_test_data[key]['PSU']['status'])



def test_for_psu_serial_num(json_config_data, json_test_data):
    """Test Purpose: Verify that the PSUs serial num is valid

        Args:
              arg1 (json): platform-<sonic_platform>-config.json
              arg2 (json): test-<sonic_platform>-config.json
                
        Example:
              For a system that only has power supply 1 present

              test-<sonic_platform>-config.json
              {
                 "PLATFORM": {
                  "PSU1": {
                      "psu_serial_num": "AAAA"
              }
    """
    if json_config_data['PLATFORM']['modules']['PSU']['support'] == "false":
       pytest.skip("Skip the testing due to the openconfig API in python module is not supported in BSP")


    for key in json_config_data:
        psupresentlist = json_test_data[key]['PSU']['present']
        for x in psupresentlist:
            if _wrapper_get_psus_status(x-1) :
               assert _wrapper_get_psus_serial(x-1) == json_test_data[key]['PSU']['PSU'+str(x)]['psu_serial_num'], \
                      "Verify PSU{} Serail number {} is invalid".format(x, _wrapper_get_psus_serial(x-1))
               assert is_valid_string(_wrapper_get_psus_serial(x-1)), "junk character detected"


def test_for_psu_model(json_config_data, json_test_data):
    """Test Purpose: Verify that the PSUs Model is valid

        Args:
              arg1 (json): platform-<sonic_platform>-config.json
              arg2 (json): test-<sonic_platform>-config.json

        Example:
              For a system that only has power supply 1 present

              test-<sonic_platform>-config.json
              {
                 "PLATFORM": {
                  "PSU1": {
                      "model":"R17-1K6P1AA",
              }
    """

    if json_config_data['PLATFORM']['modules']['PSU']['support'] == "false":
           pytest.skip("Skip the testing due to the openconfig API in python module is not supported in BSP")

    for key in json_config_data:
        psupresentlist = json_test_data[key]['PSU']['present']
        for x in psupresentlist:
            if _wrapper_get_psus_status(x-1):
               assert _wrapper_get_psus_model(x-1) == json_test_data[key]['PSU']['PSU'+str(x)]['model'], \
                      "Verify PSU{} Model ID {} is invalid".format(x, _wrapper_get_psus_model(x-1))
               assert is_valid_string(_wrapper_get_psus_model(x-1)), "junk character detected"

def test_for_psu_voltage(json_config_data, json_test_data):
    """Test Purpose: Verify that the PSUs Output voltage is valid

        Args:
              arg1 (json): platform-<sonic_platform>-config.json
              arg2 (json): test-<sonic_platform>-config.json

        Example:
              For a system that only has power supply 1 present

              test-<sonic_platform>-config.json
              {
                 "PLATFORM": {
                  "PSU1": {
                      "output_voltage":"12000"
              }
    """

    if json_config_data['PLATFORM']['modules']['PSU']['support'] == "false":
       pytest.skip("Skip the testing due to the openconfig API in python module is not supported in BSP")

    for key in json_config_data:
        psupresentlist = json_test_data[key]['PSU']['present']
        for x in psupresentlist:
            if _wrapper_get_psus_status(x-1):
               assert _wrapper_get_psus_voltage(x-1) <= json_test_data[key]['PSU']['PSU'+str(x)]['output_voltage'] * 1.1, \
                      "Verify PSU{} Output voltage {} is either invalid or too high".format(x, _wrapper_get_psus_voltage(x-1))

               assert _wrapper_get_psus_voltage(x-1) >= json_test_data[key]['PSU']['PSU'+str(x)]['output_voltage'] * 0.9, \
                      "Verify PSU{} Output voltage {} is either invalid or too low".format(x, _wrapper_get_psus_voltage(x-1))


def test_for_psu_current(json_config_data, json_test_data):
    """Test Purpose: Verify that the PSUs output current is able to read

        Args:
              arg1 (json): platform-<sonic_platform>-config.json
              arg2 (json): test-<sonic_platform>-config.json

    """

    if json_config_data['PLATFORM']['modules']['PSU']['support'] == "false":
       pytest.skip("Skip the testing due to the openconfig API in python module is not supported in BSP")

    for key in json_config_data:
        psupresentlist = json_test_data[key]['PSU']['present']
        for x in psupresentlist:
            if _wrapper_get_psus_status(x-1):
               assert _wrapper_get_psus_current(x-1) <= json_test_data[key]['PSU']['PSU'+str(x)]['output_current'] * 1.1, \
                      "Verify PSU{} Output current {} is either invalid or too high".format(x, _wrapper_get_psus_current(x-1))

               assert _wrapper_get_psus_current(x-1) >= json_test_data[key]['PSU']['PSU'+str(x)]['output_current'] * 0.9, \
                      "Verify PSU{} Output current {} is either invalid or too low".format(x, _wrapper_get_psus_current(x-1))



def test_for_psu_power(json_config_data, json_test_data):
    """Test Purpose: Verify that the PSUs output Power is able to read

        Args:
             arg1 (json): platform-<sonic_platform>-config.json
             arg2 (json): test-<sonic_platform>-config.json

    """

    if json_config_data['PLATFORM']['modules']['PSU']['support'] == "false":
       pytest.skip("Skip the testing due to the openconfig API in python module is not supported in BSP")

    for key in json_config_data:
        psupresentlist = json_test_data[key]['PSU']['present']
        for x in psupresentlist:
            if _wrapper_get_psus_status(x-1):
               assert _wrapper_get_psus_power(x-1) <= json_test_data[key]['PSU']['PSU'+str(x)]['output_power'] * 1.1, \
                      "Verify PSU{} Output Power {} is invalid too high".format(x, _wrapper_get_psus_power(x-1))

               assert _wrapper_get_psus_power(x-1) >= json_test_data[key]['PSU']['PSU'+str(x)]['output_power'] * 0.9, \
                      "Verify PSU{} Output Power {} is invalid too low".format(x, _wrapper_get_psus_power(x-1))


def test_for_psu_input_voltage(json_config_data, json_test_data):
    """Test Purpose: Verify that the PSUs Input voltage is valid

        Args:
              arg1 (json): platform-<sonic_platform>-config.json
              arg2 (json): test-<sonic_platform>-config.json

        Example:
              For a system that only has power supply 1 present

              test-<sonic_platform>-config.json
              {
                 "PLATFORM": {
                  "PSU1": {
                      "input_voltage":"110"
              }
    """

    if json_config_data['PLATFORM']['modules']['PSU']['support'] == "false":
       pytest.skip("Skip the testing due to the openconfig API in python module is not supported in BSP")

    for key in json_config_data:
        psupresentlist = json_test_data[key]['PSU']['present']
        for x in psupresentlist:
            if _wrapper_get_psus_status(x-1):
               assert _wrapper_get_psus_input_voltage(x-1) <= json_test_data[key]['PSU']['PSU'+str(x)]['input_voltage'] * 1.1, \
                      "Verify PSU{} Input voltage {} is either invalid or too high".format(x, _wrapper_get_psus_input_voltage(x-1))

               assert _wrapper_get_psus_input_voltage(x-1) >= json_test_data[key]['PSU']['PSU'+str(x)]['input_voltage'] * 0.9, \
                      "Verify PSU{} Input voltage {} is either invalid or too low".format(x, _wrapper_get_psus_input_voltage(x-1))
                      
                      
def test_for_psu_input_current(json_config_data, json_test_data):
    """Test Purpose: Verify that the PSUs input current is able to read

        Args:
              arg1 (json): platform-<sonic_platform>-config.json
              arg2 (json): test-<sonic_platform>-config.json

    """

    if json_config_data['PLATFORM']['modules']['PSU']['support'] == "false":
       pytest.skip("Skip the testing due to the openconfig API in python module is not supported in BSP")

    for key in json_config_data:
        psupresentlist = json_test_data[key]['PSU']['present']
        for x in psupresentlist:
            if _wrapper_get_psus_status(x-1):
               assert _wrapper_get_psus_input_current(x-1) <= json_test_data[key]['PSU']['PSU'+str(x)]['input_current'] * 1.1, \
                      "Verify PSU{} Input current {} is either invalid or too high".format(x, _wrapper_get_psus_input_current(x-1))

               assert _wrapper_get_psus_input_current(x-1) >= json_test_data[key]['PSU']['PSU'+str(x)]['input_current'] * 0.9, \
                      "Verify PSU{} Input current {} is either invalid or too low".format(x, _wrapper_get_psus_input_current(x-1))



def test_for_psu_capacity(json_config_data, json_test_data):
    """Test Purpose: Verify that the PSUs Capacity is valid

        Args:
              arg1 (json): platform-<sonic_platform>-config.json
              arg2 (json): test-<sonic_platform>-config.json

        Example:
              For a system that only has power supply 1 present

              test-<sonic_platform>-config.json
              {
                 "PLATFORM": {
                  "PSU1": {
                      "capacity":"1000",
              }
    """

    if json_config_data['PLATFORM']['modules']['PSU']['support'] == "false":
           pytest.skip("Skip the testing due to the openconfig API in python module is not supported in BSP")

    for key in json_config_data:
        psupresentlist = json_test_data[key]['PSU']['present']
        for x in psupresentlist:
            if _wrapper_get_psus_status(x-1):
               assert _wrapper_get_psus_capacity(x-1) == json_test_data[key]['PSU']['PSU'+str(x)]['capacity'], \
                      "Verify PSU{} Capacity {} is invalid".format(x, _wrapper_get_psus_capacity(x-1))



def test_for_psu_type(json_config_data, json_test_data):
    """Test Purpose: Verify that the PSUs Type is valid

        Args:
              arg1 (json): platform-<sonic_platform>-config.json
              arg2 (json): test-<sonic_platform>-config.json

        Example:
              For a system that only has power supply 1 present

              test-<sonic_platform>-config.json
              {
                 "PLATFORM": {
                  "PSU1": {
                      "type":"AC",
              }
    """

    if json_config_data['PLATFORM']['modules']['PSU']['support'] == "false":
           pytest.skip("Skip the testing due to the openconfig API in python module is not supported in BSP")

    for key in json_config_data:
        psupresentlist = json_test_data[key]['PSU']['present']
        for x in psupresentlist:
            if _wrapper_get_psus_status(x-1):
               assert _wrapper_get_psus_type(x-1) == json_test_data[key]['PSU']['PSU'+str(x)]['type'], \
                      "Verify PSU{} Type {} is invalid".format(x, _wrapper_get_psus_type(x-1))