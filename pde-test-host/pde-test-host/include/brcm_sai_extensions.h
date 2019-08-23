/*********************************************************************
 *
 * Copyright: (c) 2017 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.
 *
 *********************************************************************/

#if !defined (__SAI_H_)
#include <sai.h>
#endif

#if !defined (_BRM_SAI_EXT)
#define _BRM_SAI_EXT

/** Switch module extentions **/

/* Get sdk boot time in sec */
#define SAI_SWITCH_ATTR_GET_SDK_BOOT_TIME    SAI_SWITCH_ATTR_CUSTOM_RANGE_START
/*
 * Register callback fn for getting sdk shutdown time in sec.
 * This will be invoked just before switch_remove exits.
 */
#define SAI_SWITCH_ATTR_SDK_SHUT_TIME_GET_FN SAI_SWITCH_ATTR_CUSTOM_RANGE_START+1
typedef void (*brcm_sai_sdk_shutdown_time_cb_fn)(int unit, unsigned int time_sec);

/** ACL module extentions **/

#define SAI_ACL_TABLE_ATTR_FIELD_BTH_OPCODE    SAI_ACL_TABLE_ATTR_CUSTOM_RANGE_START
#define SAI_ACL_TABLE_ATTR_FIELD_AETH_SYNDROME SAI_ACL_TABLE_ATTR_CUSTOM_RANGE_START + 1
/* Set to true by default; set this to false in order to exclude inPorts from a field group's QSET */
#define SAI_ACL_TABLE_ATTR_QUALIFIER_INPORTS   SAI_ACL_TABLE_ATTR_CUSTOM_RANGE_START + 2

#define SAI_ACL_ENTRY_ATTR_FIELD_BTH_OPCODE SAI_ACL_ENTRY_ATTR_CUSTOM_RANGE_START
#define SAI_ACL_ENTRY_ATTR_FIELD_AETH_SYNDROME SAI_ACL_ENTRY_ATTR_CUSTOM_RANGE_START+1

/* BRCM specific custom attribute for pool id */
#define BRCM_SAI_BUFFER_POOL_CUSTOM_START
#define SAI_BUFFER_POOL_ATTR_BRCM_CUSTOM_POOL_ID (SAI_BUFFER_POOL_ATTR_CUSTOM_RANGE_START)

/* BRCM specific custom attribute for xpe-wide pool stats */
#define SAI_BUFFER_POOL_STAT_CUSTOM_XPE0 (SAI_BUFFER_POOL_STAT_CUSTOM_RANGE_BASE | 1 << 10) 
#define SAI_BUFFER_POOL_STAT_CUSTOM_XPE1 (SAI_BUFFER_POOL_STAT_CUSTOM_RANGE_BASE | 2 << 10) 

#define SAI_BUFFER_POOL_STAT_CURR_OCCUPANCY_BYTES_XPE0  \
  (SAI_BUFFER_POOL_STAT_CURR_OCCUPANCY_BYTES | SAI_BUFFER_POOL_STAT_CUSTOM_XPE0)

#define SAI_BUFFER_POOL_STAT_CURR_OCCUPANCY_BYTES_XPE1  \
  (SAI_BUFFER_POOL_STAT_CURR_OCCUPANCY_BYTES | SAI_BUFFER_POOL_STAT_CUSTOM_XPE1)


/* BRCM specific custom attribute for profile */
#define SAI_BUFFER_PROFILE_ATTR_BRCM_CUSTOM_USE_QGROUP_MIN (SAI_BUFFER_PROFILE_ATTR_CUSTOM_RANGE_START)

#endif /* _BRM_SAI_EXT */
