#!/usr/bin/python

try:
    import os
    import sys
    import json
    import time
    import pytest
    import subprocess
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

# Test for USB storage support
def test_for_usb_storage():
    """Test Purpose:  Verify that if the USB storage device support is enable in the kernel

    Args:
        None
        """
    cmd = "cat /boot/config-* | grep ^CONFIG_USB_STORAGE="
    ret = subprocess.call(cmd + " > /dev/null 2>&1", shell=True)
    assert ret == 0, "USB storage support is disabled in the kernel"
    return

# Test for FAT filesystem support
def test_for_usb_fat():
    """Test Purpose:  Verify that if the MSDOS/VFAT/FAT filesystem support is enable in the kernel

    Args:
        None
        """
    cmd = "cat /boot/config-* | grep ^CONFIG_FAT_FS="
    ret = subprocess.call(cmd + " > /dev/null 2>&1", shell=True)
    assert ret == 0, "FAT support is disabled in the kernel"
    cmd = "cat /boot/config-* | grep ^CONFIG_VFAT_FS="
    ret = subprocess.call(cmd + " > /dev/null 2>&1", shell=True)
    assert ret == 0, "VFAT support is disabled in the kernel"
    cmd = "cat /boot/config-* | grep ^CONFIG_MSDOS_FS="
    ret = subprocess.call(cmd + " > /dev/null 2>&1", shell=True)
    assert ret == 0, "MSDOS support is disabled in the kernel"
    return

# Test for NTFS filesystem support
def test_for_usb_ntfs():
    """Test Purpose:  Verify that if the NTFS filesystem support is enable in the kernel

    Args:
        None
        """
    cmd = "cat /boot/config-* | grep ^CONFIG_NTFS_FS="
    ret = subprocess.call(cmd + " > /dev/null 2>&1", shell=True)
    assert ret == 0, "NTFS support is disabled in the kernel"
    return

# Test for EXT4 filesystem support
def test_for_usb_ext4():
    """Test Purpose:  Verify that if the EXT4 filesystem support is enable in the kernel

    Args:
        None
        """
    cmd = "cat /boot/config-* | grep ^CONFIG_EXT4_FS="
    ret = subprocess.call(cmd + " > /dev/null 2>&1", shell=True)
    assert ret == 0, "EXT4 support is disabled in the kernel"
    return

# Test for the presence of external USB storage
def test_for_usb_presence(json_test_data):
    """Test Purpose:  Verify that the attached USB storage is detected on the DUT

    Args:
        arg1 (json): test-<sonic_platform>-config.json

    Example:
        To enable the external USB storage test

        test-<sonic_platform>-config.json
        {
            "PLATFORM": {
                "USB": {
                    "enable": "yes",
                    "device": "sdb",
                    "mountpoint": "/media/usb-storage"
                },
            }
        }
        """
    doit = json_test_data["PLATFORM"]["USB"]["enable"]
    if doit is None or "yes" != doit:
        pytest.skip("USB disabled")
        return

    node = json_test_data["PLATFORM"]["USB"]["device"]
    assert node is not None and node.startswith("sd"), "invalid device node"

    path = "/sys/class/block/" + node
    assert os.path.isdir(path), node + ": no such device"

# Test for mounting external USB storage (i.e. filesystem read)
def test_for_usb_mount(json_test_data):
    """Test Purpose:  Verify that the attached USB storage is able to be mounted on the DUT

    Args:
        arg1 (json): test-<sonic_platform>-config.json

    Example:
        To enable the external USB storage test

        test-<sonic_platform>-config.json
        {
            "PLATFORM": {
                "USB": {
                    "enable": "yes",
                    "device": "sdb",
                    "mountpoint": "/media/usb-storage"
                },
            }
        }
        """
    doit = json_test_data["PLATFORM"]["USB"]["enable"]
    if doit is None or "yes" != doit:
        pytest.skip("USB disabled")
        return

    device = json_test_data["PLATFORM"]["USB"]["device"]
    assert device is not None and device.startswith("sd"), "invalid device node"

    mountpoint = json_test_data["PLATFORM"]["USB"]["mountpoint"]
    assert mountpoint is not None and mountpoint.startswith("/media"), "invalid mount point"

    # umount the usb if it's still there
    rc = subprocess.call("mount | grep " + mountpoint + " > /dev/null 2>&1", shell=True)
    if rc == 0:
        rc = subprocess.call("umount " + mountpoint + " > /dev/null 2>&1", shell=True)
        time.sleep(3)

    # create mount point
    rc = subprocess.call("mkdir -p " + mountpoint + " > /dev/null 2>&1", shell=True)
    assert rc == 0, "Unable to create mount point"

    # mount the USB (try to mount partition 1 and then partition 0)
    if os.path.isdir("/sys/class/block/" + device + "1"):
        cmd = "mount /dev/" + device + "1 " + mountpoint + " > /dev/null 2>&1"
    elif os.path.isdir("/sys/class/block/" + device):
        cmd = "mount /dev/" + device + " " + mountpoint + " > /dev/null 2>&1"
    else:
        cmd = "false"
    rc = subprocess.call(cmd, shell=True)
    assert rc == 0, "Unable to mount the external USB"

    # umount the USB
    rc = subprocess.call("umount " + mountpoint + " > /dev/null 2>&1", shell=True)
    return
