/**********************************************************************
 * Copyright 2019 Broadcom. All rights reserved.
 * The term “Broadcom” refers to Broadcom Inc. and/or its subsidiaries.
 **********************************************************************/
#ifndef _SAIUT_H
#define _SAIUT_H

sai_object_id_t portGetId(uint32_t lane);
bool portGetAdminState(sai_object_id_t port);
sai_status_t portSetAdminState(sai_object_id_t port, bool enable);
uint32_t portGetSpeed(sai_object_id_t port);
sai_status_t portSetSpeed(sai_object_id_t port, uint32_t speed);
uint64_t portGetCounter(sai_object_id_t port, sai_stat_id_t id);
sai_status_t portClearCounter(sai_object_id_t port, sai_stat_id_t id);
sai_status_t portSetAttribute(sai_object_id_t port, sai_attribute_t *attr);
sai_status_t portGetAttribute(sai_object_id_t port, sai_attribute_t *attr);

uint32_t switchGetPortNumber(void);
sai_status_t switchShell(bool enable);
sai_status_t switchInitialize(const char *mac);
sai_status_t switchShutdown(void);

/* Broadcom SAI */
sai_status_t bcmDiagCommand(char *cmd);
sai_status_t bcmSendPackets(sai_object_id_t port, uint32_t num, uint32_t len);

#endif
