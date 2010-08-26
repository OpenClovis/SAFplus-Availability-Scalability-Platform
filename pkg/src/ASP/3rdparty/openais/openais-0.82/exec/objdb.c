/*
 * Copyright (c) 2006 MontaVista Software, Inc.
 *
 * All rights reserved.
 *
 * Author: Steven Dake (sdake@mvista.com)
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

#include <stdio.h>
#include <errno.h>
#include "objdb.h"
#include "../lcr/lcr_comp.h"
#include "../include/hdb.h"
#include "../include/list.h"

struct object_key {
	void *key_name;
	int key_len;
	void *value;
	int value_len;
	struct list_head list;
};

struct object_instance {
	void *object_name;
	int object_name_len;
	unsigned int object_handle;
	struct list_head key_head;
	struct list_head child_head;
	struct list_head child_list;
	struct list_head *find_child_list;
	void *priv;
	struct object_valid *object_valid_list;
	int object_valid_list_entries;
	struct object_key_valid *object_key_valid_list;
	int object_key_valid_list_entries;
};

static struct hdb_handle_database object_instance_database = {
	.handle_count	= 0,
	.handles	= 0,
	.iterator	= 0,
	.mutex		= PTHREAD_MUTEX_INITIALIZER
};

static int objdb_init (void)
{
	unsigned int handle;
	struct object_instance *instance;
	unsigned int res;

	res = hdb_handle_create (&object_instance_database,
		sizeof (struct object_instance), &handle);
	if (res != 0) {
		goto error_exit;
	}
	res = hdb_handle_get (&object_instance_database,
		handle, (void *)&instance);
	if (res != 0) {
		goto error_destroy;
	}
	instance->find_child_list = &instance->child_head;
	instance->object_name = "parent";
	instance->object_name_len = strlen ("parent");
	instance->object_handle = handle;
	instance->priv = NULL;
	instance->object_valid_list = NULL;
	instance->object_valid_list_entries = 0;
	list_init (&instance->key_head);
	list_init (&instance->child_head);
	list_init (&instance->child_list);

	hdb_handle_put (&object_instance_database, handle);
	return (0);

error_destroy:
	hdb_handle_destroy (&object_instance_database, handle);

error_exit:
	return (-1);
}

/*
 * object db create/destroy/set
 */
static int object_create (
	unsigned int parent_object_handle,
	unsigned int *object_handle,
	void *object_name,
	unsigned int object_name_len)
{
	struct object_instance *object_instance;
	struct object_instance *parent_instance;
	unsigned int res;
	int found = 0;
	int i;

	res = hdb_handle_get (&object_instance_database,
		parent_object_handle, (void *)&parent_instance);
	if (res != 0) {
		goto error_exit;
	}

	/*
	 * Do validation check if validation is configured for the parent object
	 */
	if (parent_instance->object_valid_list_entries) {
		for (i = 0; i < parent_instance->object_valid_list_entries; i++) {
			if ((object_name_len ==
					parent_instance->object_valid_list[i].object_len) &&
				(memcmp (object_name,
					parent_instance->object_valid_list[i].object_name,
					object_name_len) == 0)) {

				found = 1;
				break;
			}
		}

		/*
		 * Item not found in validation list
		 */
		if (found == 0) {
			goto error_object_put;
		}
	}


	res = hdb_handle_create (&object_instance_database,
		sizeof (struct object_instance), object_handle);
	if (res != 0) {
		goto error_object_put;
	}

	res = hdb_handle_get (&object_instance_database,
		*object_handle, (void *)&object_instance);
	if (res != 0) {
		goto error_destroy;
	}
	list_init (&object_instance->key_head);
	list_init (&object_instance->child_head);
	list_init (&object_instance->child_list);
	object_instance->object_name = malloc (object_name_len);
	if (object_instance->object_name == 0) {
		goto error_put_destroy;
	}
	memcpy (object_instance->object_name, object_name, object_name_len);

	object_instance->object_name_len = object_name_len;

	list_add (&object_instance->child_list, &parent_instance->child_head);

	object_instance->object_handle = *object_handle;
	object_instance->find_child_list = &object_instance->child_head;
	object_instance->priv = NULL;
	object_instance->object_valid_list = NULL;
	object_instance->object_valid_list_entries = 0;

	hdb_handle_put (&object_instance_database, *object_handle);

	hdb_handle_put (&object_instance_database, parent_object_handle);

	return (0);

error_put_destroy:
	hdb_handle_put (&object_instance_database, *object_handle);

error_destroy:
	hdb_handle_destroy (&object_instance_database, *object_handle);

error_object_put:
	hdb_handle_put (&object_instance_database, parent_object_handle);

error_exit:
	return (-1);
}

static int object_priv_set (
	unsigned int object_handle,
	void *priv)
{
	int res;
	struct object_instance *object_instance;

	res = hdb_handle_get (&object_instance_database,
		object_handle, (void *)&object_instance);
	if (res != 0) {
		goto error_exit;
	}

	object_instance->priv = priv;

	hdb_handle_put (&object_instance_database, object_handle);
	return (0);

error_exit:
	return (-1);
}

static int object_key_create (
	unsigned int object_handle,
	void *key_name,
	int key_len,
	void *value,
	int value_len)
{
	struct object_instance *instance;
	struct object_key *object_key;
	unsigned int res;
	int found = 0;
	int i;
	unsigned int val;

	res = hdb_handle_get (&object_instance_database,
		object_handle, (void *)&instance);
	if (res != 0) {
		goto error_exit;
	}

	/*
	 * Do validation check if validation is configured for the parent object
	 */
	if (instance->object_key_valid_list_entries) {
		for (i = 0; i < instance->object_key_valid_list_entries; i++) {
			if ((key_len ==
					instance->object_key_valid_list[i].key_len) &&
				(memcmp (key_name,
					instance->object_key_valid_list[i].key_name,
					key_len) == 0)) {

				found = 1;
				break;
			}
		}

		/*
		 * Item not found in validation list
		 */
		if (found == 0) {
			goto error_put;
		} else {
			if (instance->object_key_valid_list[i].validate_callback) {
				res = instance->object_key_valid_list[i].validate_callback (
					key_name, key_len, value, value_len);
				if (res != 0) {
					goto error_put;
				}
			}
		}
	}

	object_key = malloc (sizeof (struct object_key));
	if (object_key == 0) {
		goto error_put;
	}
	object_key->key_name = malloc (key_len);
	if (object_key->key_name == 0) {
		goto error_put_object;
	}
	memcpy (&val, value, 4);
	object_key->value = malloc (value_len);
	if (object_key->value == 0) {
		goto error_put_key;
	}
	memcpy (object_key->key_name, key_name, key_len);
	memcpy (object_key->value, value, value_len);

	object_key->key_len = key_len;
	object_key->value_len = value_len;

	list_init (&object_key->list);
	list_add (&object_key->list, &instance->key_head);

	return (0);

error_put_key:
	free (object_key->key_name);

error_put_object:
	free (object_key);

error_put:
	hdb_handle_put (&object_instance_database, object_handle);

error_exit:
	return (-1);
}

static int object_destroy (
	unsigned int object_handle)
{
	return (0);
}

static int object_valid_set (
	unsigned int object_handle,
	struct object_valid *object_valid_list,
	unsigned int object_valid_list_entries)
{
	struct object_instance *instance;
	unsigned int res;

	res = hdb_handle_get (&object_instance_database,
		object_handle, (void *)&instance);
	if (res != 0) {
		goto error_exit;
	}

	instance->object_valid_list = object_valid_list;
	instance->object_valid_list_entries = object_valid_list_entries;

	hdb_handle_put (&object_instance_database, object_handle);

	return (0);

error_exit:
	return (-1);
}

static int object_key_valid_set (
		unsigned int object_handle,
		struct object_key_valid *object_key_valid_list,
		unsigned int object_key_valid_list_entries)
{
	struct object_instance *instance;
	unsigned int res;

	res = hdb_handle_get (&object_instance_database,
		object_handle, (void *)&instance);
	if (res != 0) {
		goto error_exit;
	}

	instance->object_key_valid_list = object_key_valid_list;
	instance->object_key_valid_list_entries = object_key_valid_list_entries;

	hdb_handle_put (&object_instance_database, object_handle);

	return (0);

error_exit:
	return (-1);
}

/*
 * object db reading
 */
static int object_find_reset (
	unsigned int object_handle)
{
	unsigned int res;
	struct object_instance *instance;

	res = hdb_handle_get (&object_instance_database,
		object_handle, (void *)&instance);
	if (res != 0) {
		goto error_exit;
	}
	instance->find_child_list = &instance->child_head;

	hdb_handle_put (&object_instance_database, object_handle);
	return (0);

error_exit:
	return (-1);
}

static int object_find (
	unsigned int parent_object_handle,
	void *object_name,
	int object_name_len,
	unsigned int *object_handle)
{
	unsigned int res;
	struct object_instance *instance;
	struct object_instance *find_instance = NULL;
	struct list_head *list;
	unsigned int found = 0;

	res = hdb_handle_get (&object_instance_database,
		parent_object_handle, (void *)&instance);
	if (res != 0) {
		goto error_exit;
	}
	res = -ENOENT;
	for (list = instance->find_child_list->next;
		list != &instance->child_head; list = list->next) {

                find_instance = list_entry (list, struct object_instance,
			child_list);

		if ((find_instance->object_name_len == object_name_len) &&
			(memcmp (find_instance->object_name, object_name,
			object_name_len) == 0)) {
			found = 1;
			break;
		}
	}
	instance->find_child_list = list;
	hdb_handle_put (&object_instance_database, parent_object_handle);
	if (found) {
		*object_handle = find_instance->object_handle;
		res = 0;
	}
	return (res);

error_exit:
	return (-1);
}

static int object_key_get (
	unsigned int object_handle,
	void *key_name,
	int key_len,
	void **value,
	int *value_len)
{
	unsigned int res;
	struct object_instance *instance;
	struct object_key *object_key = NULL;
	struct list_head *list;
	int found = 0;

	res = hdb_handle_get (&object_instance_database,
		object_handle, (void *)&instance);
	if (res != 0) {
		goto error_exit;
	}
	for (list = instance->key_head.next;
		list != &instance->key_head; list = list->next) {

		object_key = list_entry (list, struct object_key, list);

		if ((object_key->key_len == key_len) &&
			(memcmp (object_key->key_name, key_name, key_len) == 0)) {
			found = 1;
			break;
		}
	}
	if (found) {
		*value = object_key->value;
		if (value_len) {
			*value_len = object_key->value_len;
		}
	}

	hdb_handle_put (&object_instance_database, object_handle);
	return (0);

error_exit:
	return (-1);
}

static int object_priv_get (
	unsigned int object_handle,
	void **priv)
{
	int res;
	struct object_instance *object_instance;

	res = hdb_handle_get (&object_instance_database,
		object_handle, (void *)&object_instance);
	if (res != 0) {
		goto error_exit;
	}

	*priv = object_instance->priv;

	hdb_handle_put (&object_instance_database, object_handle);
	return (0);

error_exit:
	return (-1);
}

struct objdb_iface_ver0 objdb_iface = {
	.objdb_init		= objdb_init,
	.object_create		= object_create,
	.object_priv_set	= object_priv_set,
	.object_key_create	= object_key_create,
	.object_destroy		= object_destroy,
	.object_valid_set	= object_valid_set,
	.object_key_valid_set	= object_key_valid_set,
	.object_find_reset	= object_find_reset,
	.object_find		= object_find,
	.object_key_get		= object_key_get,
	.object_priv_get	= object_priv_get
};

struct lcr_iface objdb_iface_ver0[1] = {
	{
		.name			= "objdb",
		.version		= 0,
		.versions_replace	= 0,
		.versions_replace_count = 0,
		.dependencies		= 0,
		.dependency_count	= 0,
		.constructor		= NULL,
		.destructor		= NULL,
		.interfaces		= NULL,
	}
};

struct lcr_comp objdb_comp_ver0 = {
	.iface_count			= 1,
	.ifaces				= objdb_iface_ver0
};



__attribute__ ((constructor)) static void objdb_comp_register (void) {
        lcr_interfaces_set (&objdb_iface_ver0[0], &objdb_iface);

	lcr_component_register (&objdb_comp_ver0);
}
