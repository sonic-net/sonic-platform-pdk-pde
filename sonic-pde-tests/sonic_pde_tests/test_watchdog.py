import pytest
import sys
import imp
import subprocess
import time

VERSION = "1.0"
WATCHDOG_LOAD_ERROR = -1
CHASSIS_LOAD_ERROR = -2
WDT_COMMON_ERROR = -1

# Global platform watchdog class instance
platform_watchdog = None
platform_chassis = None

# ==================== Methods for initialization ====================

# Loads platform specific watchdog module from source
def load_platform_watchdog():
    global platform_chassis
    global platform_watchdog

    # Load 2.0 Watchdog class
    if platform_chassis is None:
       try:
            import sonic_platform.platform
            platform_chassis = sonic_platform.platform.Platform().get_chassis()
            platform_watchdog = platform_chassis.get_watchdog()

       except Exception as e:
            print("Failed to load 2.0 watchdog API due to {}".format(repr(e)))

def _wrapper_get_watchdog_dev():
    load_platform_watchdog()
    if platform_watchdog is not None:
       try:
           return platform_watchdog._get_wdt()
       except NotImplementedError:
           pass

def _wrapper_arm_watchdog(sec):
    load_platform_watchdog()
    if platform_watchdog is not None:
       try:
           return platform_watchdog.arm(sec)
       except NotImplementedError:
           pass

def _wrapper_get_watchdog_remaining_time():
    load_platform_watchdog()
    if platform_watchdog is not None:
       try:
           return platform_watchdog.get_remaining_time()
       except NotImplementedError:
           pass


def _wrapper_watchdog_is_armed():
    load_platform_watchdog()
    if platform_watchdog is not None:
       try:
           return platform_watchdog.is_armed()
       except NotImplementedError:
           pass

def _wrapper_watchdog_disarm():
    load_platform_watchdog()
    if platform_watchdog is not None:
       try:
           return platform_watchdog.disarm()
       except NotImplementedError:
           pass

def test_for_get_watchdog_dev():
    """
      Test Purpose:  Verify the watchdog dev can be get

      Args:
         None
    """
    assert _wrapper_get_watchdog_dev() != (0, None) , "watch dev fail to get"

def test_for_arm_watchdog(json_config_data, json_test_data):
    """
    Test Purpose:  Verify the watchdog can be armed

    Args:
        arg1 (json): platform-<sonic_platform>-config.json
        arg2 (json): test-<sonic_platform>-config.json

    Example:
        Test config JSON setting for Watchdog arm Timeout configured

          "WDT":
          {
            "arm":{
              "timeout":190
            }
          }
    """
    sec = json_test_data['PLATFORM']['WDT']['arm']['timeout']
    if _wrapper_arm_watchdog(sec) != WDT_COMMON_ERROR:
       assert _wrapper_watchdog_is_armed() == True, "watchdog is not active"

def test_for_get_watchdog_remaining_time():
    """
      Test Purpose:  Verify the watchdog remaining time can be retrieved
      
      Args:
        None
    """
    assert _wrapper_get_watchdog_remaining_time() is not WDT_COMMON_ERROR, "get watchdog remaining time fail"

def test_for_remaining_time_changed():
    """
      Test Purpose:  Verify the watchdog remaining time is keep changing ( Couting down )

      Args:
        None
    """
    Time1 = _wrapper_get_watchdog_remaining_time();
    time.sleep(5)
    Time2 = _wrapper_get_watchdog_remaining_time();
    assert Time1 != Time2, "Watchdog remaining time is the same without changed"



def test_for_disarm_watchdog():  
    """
      Test Purpose:  Verify the watchdog can be disarmed

      Args:
        None
    """
    if _wrapper_watchdog_disarm() == True:
       assert _wrapper_watchdog_is_armed() == False, "watchdog is not disabled"
