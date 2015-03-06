/*
 * Copyright (c) 2005 MontaVista Software, Inc.
 * Copyright (c) 2006-2007 Red Hat, Inc.
 *
 * All rights reserved.
 *
 * Author: Steven Dake (sdake@redhat.com)
 *
 * This software licensed under BSD license, the text of which follows:
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of the MontaVista Software, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef AIS_IPC_CFG_H_DEFINED
#define AIS_IPC_CFG_H_DEFINED

#include <netinet/in.h>
#include "ipc_gen.h"
#include "saAis.h"
#include "cfg.h"

enum req_lib_cfg_types {
	MESSAGE_REQ_CFG_RINGSTATUSGET = 0,
	MESSAGE_REQ_CFG_RINGREENABLE = 1,
        MESSAGE_REQ_CFG_STATETRACKSTART = 2,
        MESSAGE_REQ_CFG_STATETRACKSTOP = 3,
        MESSAGE_REQ_CFG_ADMINISTRATIVESTATESET = 4,
        MESSAGE_REQ_CFG_ADMINISTRATIVESTATEGET = 5,
};

enum res_lib_cfg_types {
        MESSAGE_RES_CFG_RINGSTATUSGET = 0,
        MESSAGE_RES_CFG_RINGREENABLE = 1,
        MESSAGE_RES_CFG_STATETRACKSTART = 2,
        MESSAGE_RES_CFG_STATETRACKSTOP = 3,
        MESSAGE_RES_CFG_ADMINISTRATIVESTATESET = 4,
        MESSAGE_RES_CFG_ADMINISTRATIVESTATEGET = 5,
};

struct req_lib_cfg_statetrack {
	mar_req_header_t header;
	SaUint8T trackFlags;
	OpenaisCfgStateNotificationT *notificationBufferAddress;
};

struct res_lib_cfg_statetrack {
	mar_res_header_t header;
};

struct req_lib_cfg_statetrackstop {
	mar_req_header_t header;
};

struct res_lib_cfg_statetrackstop {
	mar_res_header_t header;
};

struct req_lib_cfg_administrativestateset {
	mar_req_header_t header;
	SaNameT compName;
	OpenaisCfgAdministrativeTargetT administrativeTarget;
	OpenaisCfgAdministrativeStateT administrativeState;
};

struct res_lib_cfg_administrativestateset {
	mar_res_header_t header;
};

struct req_lib_cfg_administrativestateget {
	mar_req_header_t header;
	SaNameT compName;
	OpenaisCfgAdministrativeTargetT administrativeTarget;
	OpenaisCfgAdministrativeStateT administrativeState;
};

struct res_lib_cfg_administrativestateget {
	mar_res_header_t header __attribute__((aligned(8)));
};

struct req_lib_cfg_ringstatusget {
	mar_req_header_t header __attribute__((aligned(8)));
};

struct res_lib_cfg_ringstatusget {
	mar_res_header_t header __attribute__((aligned(8)));
	mar_uint32_t interface_count __attribute__((aligned(8)));
	char interface_name[16][128] __attribute__((aligned(8)));
	char interface_status[16][512] __attribute__((aligned(8)));
};

struct req_lib_cfg_ringreenable {
	mar_req_header_t header __attribute__((aligned(8)));
};

struct res_lib_cfg_ringreenable {
	mar_res_header_t header __attribute__((aligned(8)));
};

typedef enum {
	AIS_AMF_ADMINISTRATIVETARGET_SERVICEUNIT = 0,
	AIS_AMF_ADMINISTRATIVETARGET_SERVICEGROUP = 1,
	AIS_AMF_ADMINISTRATIVETARGET_COMPONENTSERVICEINSTANCE = 2,
	AIS_AMF_ADMINISTRATIVETARGET_NODE = 3
} openaisAdministrativeTarget;

typedef enum {
	AIS_AMF_ADMINISTRATIVESTATE_UNLOCKED = 0,
	AIS_AMF_ADMINISTRATIVESTATE_LOCKED = 1,
	AIS_AMF_ADMINISTRATIVESTATE_STOPPING = 2
} openaisAdministrativeState;

#endif /* AIS_IPC_CFG_H_DEFINED */
