import pytest
import sys
import imp
import subprocess
import time

PLATFORM_SPECIFIC_CLASS_NAME = "board"

def test_for_eeprom_read():
    """Test Purpose:  Verify that the EEPROM read is ok and chcksum is valid
    """

    module = imp.load_source("eeprom","/usr/share/sonic/platform/plugins/eeprom.py")
    board = getattr(module, PLATFORM_SPECIFIC_CLASS_NAME)
    eeprom = board('',0,'',True)
    assert eeprom.is_checksum_valid(eeprom.read_eeprom()),\
    "verify checksum is invalid".format(eeprom.read_eeprom())


def test_for_eeprom_mac(json_config_data,json_test_data):
    """Test Purpose:  Verify that the MAC read from EEPROM is matching with the
                      test config JSON setting

       Args:
           arg1 (json): platform-<sonic_platform>-config.json
           arg2 (json): test-<sonic_platform>-config.json

       Example:
            For a system that physically supports 1 syseeprom, the MAC address is
            configured in the test-<sonic_platform>-config.json

           test-<sonic_platform>-config.json
           "EEPROM": {
                "mac": "00:11:22:33:44:55",
                "ser": "AABBCCDDEEFF",
                "model": "AAAA"
            },
    """

    module = imp.load_source("eeprom","/usr/share/sonic/platform/plugins/eeprom.py")
    board = getattr(module, PLATFORM_SPECIFIC_CLASS_NAME)
    eeprom = board('',0,'',True)
    for key in json_config_data:
        assert eeprom.base_mac_addr(eeprom.read_eeprom()) == json_test_data[key]['EEPROM']['mac'],\
        "verify MAC is invalid".format(eeprom.read_eeprom())


def test_for_eeprom_SER(json_config_data,json_test_data):
    """Test Purpose:  Verify that the SERIAL read from EEPROM is matching with the
                      test config JSON setting

       Args:
           arg1 (json): platform-<sonic_platform>-config.json
           arg2 (json): test-<sonic_platform>-config.json

       Example:
            For a system that physically supports 1 syseeprom, the SERIAL number is
            configured in the test-<sonic_platform>-config.json

           test-<sonic_platform>-config.json
           "EEPROM": {
                "mac": "00:11:22:33:44:55",
                "ser": "AABBCCDDEEFF",
                "model": "AAAA"
            },
    """

    module = imp.load_source("eeprom","/usr/share/sonic/platform/plugins/eeprom.py")
    board = getattr(module, PLATFORM_SPECIFIC_CLASS_NAME)
    eeprom = board('',0,'',True)

    for key in json_config_data:
        assert eeprom.serial_number_str(eeprom.read_eeprom()) == json_test_data[key]['EEPROM']['ser'],\
        "verify SER NUMBER is invalid".format(eeprom.read_eeprom())


def test_for_eeprom_MODEL(json_config_data,json_test_data):
    """Test Purpose:  Verify that the MODEL read from EEPROM is matching with the
                      test config JSON setting

       Args:
           arg1 (json): platform-<sonic_platform>-config.json
           arg2 (json): test-<sonic_platform>-config.json

       Example:
            For a system that physically supports 1 syseeprom, the MODEL is
            configured in the test-<sonic_platform>-config.json

           test-<sonic_platform>-config.json
           "EEPROM": {
                "mac": "00:11:22:33:44:55",
                "ser": "AABBCCDDEEFF",
                "model": "AAAA"
            },
    """
    module = imp.load_source("eeprom","/usr/share/sonic/platform/plugins/eeprom.py")
    board = getattr(module, PLATFORM_SPECIFIC_CLASS_NAME)
    eeprom = board('',0,'',True)

    for key in json_config_data:
        assert eeprom.modelstr(eeprom.read_eeprom()) == json_test_data[key]['EEPROM']['model'],\
        "verify Model Name is invalid".format(eeprom.read_eeprom())


def test_for_platform_id(json_config_data,json_test_data):
    module = imp.load_source("eeprom","/usr/share/sonic/platform/plugins/eeprom.py")
    board = getattr(module, PLATFORM_SPECIFIC_CLASS_NAME)
    eeprom = board('',0,'',True)

    valid, t = eeprom.get_tlv_field(eeprom.read_eeprom(), \
               eeprom._TLV_CODE_PLATFORM_NAME)
    assert valid, "Unable to get platform name from EEPROM"

    cmd = "cat /host/machine.conf | grep onie_platform | cut -d '=' -f 2"
    pin = subprocess.Popen(cmd,
                          shell=True,
                          close_fds=True,
                          stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT)
    id = pin.communicate()[0]
    id = id.strip()
    assert id == t[2], "platform id mismatch"
    return




if __name__ == '__main__':
   unittest.main()

