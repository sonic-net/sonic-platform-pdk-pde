import pytest
import sys
import imp
import subprocess
import time

PLATFORM_SPECIFIC_MODULE_NAME = "eeprom"
PLATFORM_SPECIFIC_CLASS_NAME = "board"


# Global platform-specific sfputil class instance
platform_eeprom_data = None
platform_eeprom = None
platform_chassis = None


# Loads platform specific module from source
def _wrapper_init():
    global platform_chassis
    global platform_eeprom
    global platform_eeprom_data
    
    # Load new platform api class
    if platform_eeprom is None:
       try:
           import sonic_platform.platform
           platform_chassis = sonic_platform.platform.Platform().get_chassis()
           platform_eeprom = platform_chassis.get_eeprom()
           platform_eeprom_data = platform_eeprom.read_eeprom()
       except Exception as e:
           print("Failed to load chassis due to {}".format(repr(e)))

    # Load platform-specific fanutil class
    if platform_chassis is None:
       try:
             module_file = "/".join([PLATFORM_PATH, "plugins", PLATFORM_SPECIFIC_MODULE_NAME + ".py"])
             module = imp.load_source(PLATFORM_SPECIFIC_MODULE_NAME, module_file)
             platform_eeprom_class = getattr(module, PLATFORM_SPECIFIC_CLASS_NAME)
             platform_eeprom = platform_eeprom_class()
             platform_eeprom_data = platform_eeprom.read_eeprom()

       except Exception as e:
             print("Failed to load eeprom due to {}".format(repr(e)))

       assert (platform_chassis is not None) or (platform_eeprom is not None), "Unable to load platform module"


def is_valid_string(str):
    for c in str:
        v = ord(c)
        if (v < 32) or (v > 126):
            return False
    return True

def test_for_eeprom_read():
    """Test Purpose:  Verify that the EEPROM read is ok and checksum is valid
    """
    _wrapper_init()
    assert platform_eeprom.is_checksum_valid(platform_eeprom_data), \
           "checksum failed"

def test_for_eeprom_has_mac():
    _wrapper_init()
    assert platform_eeprom.is_valid_tlvinfo_header(platform_eeprom_data), "Invalid TlvInfo format"

    has_mac = False
    total_len = ((platform_eeprom_data[9]) << 8) | (platform_eeprom_data[10])
    tlv_index = platform_eeprom._TLV_INFO_HDR_LEN
    tlv_end   = platform_eeprom._TLV_INFO_HDR_LEN + total_len
    while (tlv_index + 2) < len(platform_eeprom_data) and tlv_index < tlv_end:
        assert platform_eeprom.is_valid_tlv(platform_eeprom_data[tlv_index:]), \
               "Invalid TLV @ {}".format(tlv_index)
        tlv = platform_eeprom_data[tlv_index:tlv_index + 2 + (platform_eeprom_data[tlv_index + 1])]
        if (tlv[0]) == platform_eeprom._TLV_CODE_CRC_32:
            break

        if (tlv[0]) == platform_eeprom._TLV_CODE_MAC_BASE:
            if (tlv[1]) == 6:
                has_mac = True
            break
        tlv_index += (platform_eeprom_data[tlv_index+1]) + 2

    assert has_mac, "MAC not found"

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
    _wrapper_init()
    for key in json_config_data:
        tst = json_test_data[key]['EEPROM']['mac']
        if platform_chassis is not None:
           val = platform_chassis.get_base_mac()
        else:
           val = platform_eeprom.base_mac_addr(platform_eeprom_data)
        assert val == tst

def test_for_eeprom_has_sn():
    _wrapper_init()
    assert platform_eeprom.is_valid_tlvinfo_header(platform_eeprom_data), "Invalid TlvInfo format"

    has_sn = False
    total_len = ((platform_eeprom_data[9]) << 8) | (platform_eeprom_data[10])
    tlv_index = platform_eeprom._TLV_INFO_HDR_LEN
    tlv_end   = platform_eeprom._TLV_INFO_HDR_LEN + total_len
    while (tlv_index + 2) < len(platform_eeprom_data) and tlv_index < tlv_end:
        assert platform_eeprom.is_valid_tlv(platform_eeprom_data[tlv_index:]), \
               "Invalid TLV @ {}".format(tlv_index)
        tlv = platform_eeprom_data[tlv_index:tlv_index + 2 + (platform_eeprom_data[tlv_index + 1])]
        if (tlv[0]) == platform_eeprom._TLV_CODE_CRC_32:
            break

        if (tlv[0]) == platform_eeprom._TLV_CODE_SERIAL_NUMBER:
            if ((tlv[1]) <= 0):
                break

            name, value = platform_eeprom.decoder(None, tlv)
            value = value.strip()
            if len(value) > 0 and is_valid_string(value):
                has_sn = True

            break
        tlv_index += (platform_eeprom_data[tlv_index+1]) + 2

    assert has_sn, "serial number not found"

def test_for_eeprom_sn(json_config_data,json_test_data):
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
    _wrapper_init()
    for key in json_config_data:
        tst = json_test_data[key]['EEPROM']['ser']

        if platform_chassis is not None:
           val = platform_chassis.get_serial_number()
        else:
           val = platform_eeprom.serial_number_str(platform_eeprom_data)

        assert val == tst

def test_for_eeprom_model(json_config_data,json_test_data):
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
    _wrapper_init()
    for key in json_config_data:
        tst = json_test_data[key]['EEPROM']['model']
        if platform_chassis is not None:
           val = platform_chassis.get_name()
        else:
           val = platform_eeprom.modelstr(platform_eeprom_data)

        assert val == tst

def test_for_platform_id(json_config_data,json_test_data):
    _wrapper_init()
    v, t = platform_eeprom.get_tlv_field(platform_eeprom_data, \
                                         platform_eeprom._TLV_CODE_PLATFORM_NAME)
    assert v, "Unable to get platform name from EEPROM"

    cmd = "cat /host/machine.conf | grep onie_platform | cut -d '=' -f 2"
    pin = subprocess.Popen(cmd,
                          shell=True,
                          close_fds=True,
                          stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT)
    id = pin.communicate()[0].decode('ascii')
    id = id.strip()
    assert id == t[2].decode('ascii'), "platform id mismatch"

def test_for_eeprom_junk_characters():
    _wrapper_init()
    assert platform_eeprom.is_valid_tlvinfo_header(platform_eeprom_data), "Invalid TlvInfo format"

    total_len = ((platform_eeprom_data[9]) << 8) | (platform_eeprom_data[10])
    tlv_index = platform_eeprom._TLV_INFO_HDR_LEN
    tlv_end   = platform_eeprom._TLV_INFO_HDR_LEN + total_len
    while (tlv_index + 2) < len(platform_eeprom_data) and tlv_index < tlv_end:
        assert platform_eeprom.is_valid_tlv(platform_eeprom_data[tlv_index:]), \
               "Invalid TLV @ {}".format(tlv_index)
        tlv = platform_eeprom_data[tlv_index:tlv_index + 2 + (platform_eeprom_data[tlv_index + 1])]
        if (tlv[0]) == platform_eeprom._TLV_CODE_CRC_32:
            break

        name, value = platform_eeprom.decoder(None, tlv)
        #print("%-20s 0x%02X %3d %s" % (name, (tlv[0]), (tlv[1]), value))
        assert is_valid_string(name + value), "junk character detected"
        tlv_index += (platform_eeprom_data[tlv_index+1]) + 2
