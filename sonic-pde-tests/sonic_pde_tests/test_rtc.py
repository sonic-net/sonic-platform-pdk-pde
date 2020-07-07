#!/usr/bin/python

try:
    import pytest
    import sys
    import subprocess
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

# Test for RTC read
def test_for_rtc_read():
    """Test Purpose:  Verify that if the RTC is readable on the DUT

    Args:
        None
        """
    rc = subprocess.call("hwclock > /dev/null 2>&1", shell=True)
    assert rc == 0, "RTC read failed"
    return

# Test for RTC write
def test_for_rtc_write():
    """Test Purpose:  Verify that if the RTC is writable on the DUT

    Args:
        None
        """
    rc = subprocess.call("hwclock -w > /dev/null 2>&1", shell=True)
    assert rc == 0, "RTC write failed"
    return
