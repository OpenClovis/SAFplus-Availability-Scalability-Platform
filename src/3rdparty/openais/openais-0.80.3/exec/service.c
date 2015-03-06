/*
 * Copyright (c) 2006 MontaVista Software, Inc.
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../lcr/lcr_ifact.h"
#include "service.h"
#include "mainconfig.h"
#include "util.h"
#include "print.h"

struct default_service {
	char *name;
	int ver;
};

static struct default_service default_services[] = {
	{
		.name			 = "openais_evs",
		.ver			 = 0,
	},
	{
		.name			 = "openais_clm",
		.ver			 = 0,
	},
	{
		.name			 = "openais_amf",
		.ver			 = 0,
	},
	{
		.name			 = "openais_ckpt",
		.ver			 = 0,
	},
	{
		.name			 = "openais_evt",
		.ver			 = 0,
	},
	{
		.name			 = "openais_lck",
		.ver			 = 0,
	},
	{
		.name			 = "openais_msg",
		.ver			 = 0,
	},
	{
		.name			 = "openais_cfg",
		.ver			 = 0,
	},
	{
		.name			 = "openais_cpg",
		.ver			 = 0,
	}
};

struct openais_service_handler *ais_service[SERVICE_HANDLER_MAXIMUM_COUNT];

/*
 * Adds a service handler to the object database
 */
int openais_service_objdb_add (
	struct objdb_iface_ver0 *objdb,
	char *name,
	int version)
{
	unsigned int object_handle;

	objdb->object_create (OBJECT_PARENT_HANDLE, &object_handle,
		"service", strlen ("service"));
	objdb->object_key_create (object_handle, "name", strlen ("name"),
		name, strlen (name) + 1);
	objdb->object_key_create (object_handle, "ver", strlen ("ver"),
		&version, sizeof (version));

	return (0);
}

static int service_handler_config (
	struct openais_service_handler *handler,
	struct objdb_iface_ver0 *objdb)
{
	int res = 0;

	/* Already loaded? */
	if (ais_service[handler->id] != NULL)
		return 0;

	log_printf (LOG_LEVEL_NOTICE, "Registering service handler '%s'\n", handler->name);
	ais_service[handler->id] = handler;
	if (ais_service[handler->id]->config_init_fn) {
		res = ais_service[handler->id]->config_init_fn (objdb);
	}
	return (res);
}

/*
 * adds the default services to the object database
 */
int openais_service_default_objdb_set (struct objdb_iface_ver0 *objdb)
{
	int i;
	unsigned int object_service_handle;
	char *value = NULL;

	/* Load default services unless they have been explicitly disabled */
	objdb->object_find_reset (OBJECT_PARENT_HANDLE);
	if (objdb->object_find (
		OBJECT_PARENT_HANDLE,
		"aisexec",
		strlen ("aisexec"),
		&object_service_handle) == 0) {

		if ( ! objdb->object_key_get (object_service_handle,
					      "defaultservices",
					      strlen ("defaultservices"),
					      (void *)&value,
					      NULL)) {

			if (value && strcmp (value, "no") == 0) {
				return 0;
			}
		}
	}

	log_init ("SERV");

	for (i = 0; i < sizeof (default_services) / sizeof (struct default_service); i++) {
		openais_service_objdb_add (objdb, default_services[i].name, default_services[i].ver);
	}
	return (0);
}

/*
 * Links dynamic services into the executive
 */
int openais_service_link_all (struct objdb_iface_ver0 *objdb)
{
	char *service_name;
	char *service_ver;
	unsigned int object_service_handle;
	int ret;
	unsigned int handle;
	struct openais_service_handler_iface_ver0 *iface_ver0;
	void *iface_ver0_p;
	unsigned int ver_int;

	objdb->object_find_reset (OBJECT_PARENT_HANDLE);
	while (objdb->object_find (
		OBJECT_PARENT_HANDLE,
		"service",
		strlen ("service"),
		&object_service_handle) == 0) {

		objdb->object_key_get (object_service_handle,
			"name",
			strlen ("name"),
			(void *)&service_name,
			NULL);

		ret = objdb->object_key_get (object_service_handle,
			"ver",
			strlen ("ver"),
			(void *)&service_ver,
			NULL);

		ver_int = atoi (service_ver);

		/*
		 * reference the interface and register it
		 */
		lcr_ifact_reference (
			&handle,
			service_name,
			ver_int,
			&iface_ver0_p,
			(void *)0);

		iface_ver0 = (struct openais_service_handler_iface_ver0 *)iface_ver0_p;

		if (iface_ver0 == 0) {
			log_printf(LOG_LEVEL_ERROR, "openais component %s did not load.\n", service_name);
			openais_exit_error (AIS_DONE_DYNAMICLOAD);
		} else {
			log_printf(LOG_LEVEL_NOTICE, "openais component %s loaded.\n", service_name);
		}

		service_handler_config (
			iface_ver0->openais_get_service_handler_ver0(), objdb);
	}
	return (0);
}

int openais_service_init_all (int service_count,
			      struct objdb_iface_ver0 *objdb)
{
	int i;
	int res=0;

	for (i = 0; i < service_count; i++) {
		if (ais_service[i] && ais_service[i]->exec_init_fn) {
			log_printf (LOG_LEVEL_NOTICE, "Initialising service handler '%s'\n", ais_service[i]->name);
			res = ais_service[i]->exec_init_fn (objdb);
			if (res != 0) {
				break;
			}
		}
	}
	return (res);
}
