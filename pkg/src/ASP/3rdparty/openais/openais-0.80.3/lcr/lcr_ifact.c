/*
 * Copyright (C) 2006-2007 Red Hat, Inc.
 * Copyright (c) 2006 Sun Microsystems, Inc.
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

#include <stdio.h>
#include <dlfcn.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#ifndef VXWORKS_BUILD
#include <fnmatch.h>
#endif
#ifdef POSIX_BUILD
#include <sys/types.h>
#ifndef VXWORKS_BUILD
#include <sys/dir.h>
#endif
#endif
#include "lcr_comp.h"
#include "lcr_ifact.h"
#include "../include/hdb.h"

#ifdef VXWORKS_BUILD
extern int alphasort(const void *a, const void *b);
extern int scandir(const char *path, struct dirent ***dlist, int (*filter)(struct dirent *d),
                   int (*cmp)(const void *, const void *));

#endif

struct lcr_component_instance {
	struct lcr_iface *ifaces;
	int iface_count;
	unsigned int comp_handle;
	void *dl_handle;
	int refcount;
	char library_name[256];
};

struct lcr_iface_instance {
	unsigned int component_handle;
	void *context;
	void (*destructor) (void *context);
};
	
static struct hdb_handle_database lcr_component_instance_database = {
	.handle_count	= 0,
	.handles	= 0,
	.iterator	= 0
};

static struct hdb_handle_database lcr_iface_instance_database = {
	.handle_count	= 0,
	.handles	= 0,
	.iterator	= 0
};

static unsigned int g_component_handle = 0xFFFFFFFF;

#ifdef OPENAIS_LINUX
static int lcr_select_so (const struct dirent *dirent)
#else
static int lcr_select_so (struct dirent *dirent)
#endif
{
	unsigned int len;

	len = strlen (dirent->d_name);

	if (len > 6) {
		if (strcmp (".lcrso", dirent->d_name + len - 6) == 0) {
			return (1);
		}
	}
	return (0);
}

#ifdef VXWORKS_BUILD
static int pathlist_select (struct dirent *dirent)
{
    int len = strlen(dirent->d_name);
    if(len > 5)
    {
        if (strcmp (".conf", dirent->d_name + len - 5) == 0) {
            return (1);
        }
    }
	return (0);
}

#else

#ifdef OPENAIS_LINUX
static int pathlist_select (const struct dirent *dirent)
#else
static int pathlist_select (struct dirent *dirent)
#endif
{
	if (fnmatch ("*.conf", dirent->d_name, 0) == 0) {
		return (1);
	}

	return (0);
}
#endif


static inline struct lcr_component_instance *lcr_comp_find (
	char *iface_name,
	unsigned int version,
	unsigned int *iface_number)
{
	struct lcr_component_instance *instance;
	void *instance_p = NULL;
	unsigned int component_handle = 0;
	int i;

	/*
	 * Try to find interface in already loaded component
	 */
	hdb_iterator_reset (&lcr_component_instance_database);
	while (hdb_iterator_next (&lcr_component_instance_database,
		&instance_p, &component_handle) == 0) {

		instance = (struct lcr_component_instance *)instance_p;

		for (i = 0; i < instance->iface_count; i++) {
			if ((strcmp (instance->ifaces[i].name, iface_name) == 0) &&
				instance->ifaces[i].version == version) {

				*iface_number = i;
				return (instance);
			}
		}
		hdb_handle_put (&lcr_component_instance_database, component_handle);
	}

	return (NULL);
}

static inline int lcr_lib_loaded (
	char *library_name)
{
	struct lcr_component_instance *instance;
	void *instance_p = NULL;
	unsigned int component_handle = 0;

	/*
	 * Try to find interface in already loaded component
	 */
	hdb_iterator_reset (&lcr_component_instance_database);
	while (hdb_iterator_next (&lcr_component_instance_database,
		(void *)&instance_p, &component_handle) == 0) {

		instance = (struct lcr_component_instance *)instance_p;

		if (strcmp (instance->library_name, library_name) == 0) {
			return (1);
		}

		hdb_handle_put (&lcr_component_instance_database, component_handle);
	}

	return (0);
}

char *path_list[128];
unsigned int path_list_entries = 0;

static void defaults_path_build (void)
{
	char cwd[1024] = "";
	char *res = NULL;

	res = getcwd (cwd, sizeof (cwd));
	if (res != NULL) {
		strncat (cwd, "/", sizeof(cwd)-1);
		path_list[0] = strdup (cwd);
		path_list_entries++;
	}
	path_list[path_list_entries++] = "/usr/libexec/lcrso";
}

static void ld_library_path_build (void)
{
	char *ld_library_path;
	char *my_ld_library_path;
	char *p_s, *ptrptr;

	ld_library_path = getenv ("LD_LIBRARY_PATH");
	if (ld_library_path == NULL) {
		return;
	}
	my_ld_library_path = strdup (ld_library_path);
	if (my_ld_library_path == NULL) {
		return;
	}

	p_s = strtok_r (my_ld_library_path, ":", &ptrptr);
	while (p_s != NULL) {
		path_list[path_list_entries++] = strdup (p_s);
		p_s = strtok_r (NULL, ":", &ptrptr);
	}

	free (my_ld_library_path);
}


static int ldso_path_build (char *path, char *filename)
{
	FILE *fp;
	char string[1024];
	char filename_cat[1024];
	char newpath[1024] = {0};
	char *newpath_tmp;
	char *new_filename;
	int j;
#ifdef QNX_BUILD
    struct direct **scandir_list;
#else
	struct dirent **scandir_list;
#endif
	unsigned int scandir_entries;

	snprintf (filename_cat,sizeof(filename_cat), "%s/%s", path, filename);
	if (filename[0] == '*') {
		scandir_entries = scandir (
			path,
			&scandir_list,
			pathlist_select, alphasort);
		if (scandir_entries == 0) {
			return 0;
		} else if (scandir_entries == -1) {
			return -1;
		} else {
			for (j = 0; j < scandir_entries; j++) {
				ldso_path_build (path, scandir_list[j]->d_name);
			}
		}
	}

	fp = fopen (filename_cat, "r");
	if (fp == NULL) {
		return (-1);
	}

	while (fgets (string, sizeof (string), fp)) {
		string[strlen(string) - 1] = '\0';
		if (strncmp (string, "include", strlen ("include")) == 0) {
			newpath_tmp = string + strlen ("include") + 1;
			for (j = strlen (string);
				string[j] != ' ' &&
					string[j] != '/' &&
					j > 0;
				j--) {
			}
			string[j] = '\0';
			new_filename = &string[j] + 1;
			strncpy (newpath, path, sizeof(newpath)-1);
			strncat (newpath, "/", (sizeof(newpath)-strlen(newpath)-1));
			strncat (newpath, newpath_tmp, (sizeof(newpath)-strlen(newpath)-1));
			ldso_path_build (newpath, new_filename);
			continue;
		}
		path_list[path_list_entries++] = strdup (string);
	}
	fclose(fp);
	return (0);
}

static int interface_find_and_load (
	char *path,
	char *iface_name,
	int version,
	struct lcr_component_instance **instance_ret,
	unsigned int *iface_number)
{
	struct lcr_component_instance *instance;
	void *dl_handle;
	struct dirent **scandir_list;
	int scandir_entries;
	unsigned int libs_to_scan;
	char dl_name[1024];

	scandir_entries = scandir (path,  &scandir_list, lcr_select_so, alphasort);
	if (scandir_entries > 0)
	/*
	 * no error so load the object
	 */
	for (libs_to_scan = 0; libs_to_scan < scandir_entries; libs_to_scan++) {
#ifndef VXWORKS_BUILD
		/*
		 * Load objects, scan them, unload them if they are not a match
		 */
		snprintf (dl_name, sizeof(dl_name), "%s/%s", path, scandir_list[libs_to_scan]->d_name);
		/*
	 	 * Don't reload already loaded libraries
		 */
		if (lcr_lib_loaded (dl_name)) {
			continue;
		}
		dl_handle = dlopen (dl_name, RTLD_LAZY);
		if (dl_handle == NULL) {
			fprintf (stderr, "LCR error loading plugin: %s\n",
				dlerror());
			continue;
		}
		instance = lcr_comp_find (iface_name, version, iface_number);
		if (instance) {
			instance->dl_handle = dl_handle;
			strncpy (instance->library_name, dl_name,sizeof(instance->library_name)-1);
			goto found;
		}

		/*
		 * No matching interfaces found, try next shared object
		 */
		if (g_component_handle != 0xFFFFFFFF) {
			hdb_handle_destroy (&lcr_component_instance_database,
				g_component_handle);
			g_component_handle = 0xFFFFFFFF;
		}
		dlclose (dl_handle);
#endif
	} /* scanning for lcrso loop */
	return (-1);

found:
	*instance_ret = instance;
	return (0);
}

static unsigned int lcr_initialized = 0;

int lcr_ifact_reference (
	unsigned int *iface_handle,
	char *iface_name,
	int version,
	void **iface,
	void *context)
{
	struct lcr_iface_instance *iface_instance;
	struct lcr_component_instance *instance;
	unsigned int iface_number;
	unsigned int res;
	unsigned int i;

	/*
	 * Determine if the component is already loaded
	 */
	instance = lcr_comp_find (iface_name, version, &iface_number);
	if (instance) {
		goto found;
	}

	if (lcr_initialized == 0) {
		lcr_initialized = 1;
		defaults_path_build ();
		ld_library_path_build ();
		ldso_path_build ("/etc", "ld.so.conf");
	}

// TODO error checking in this code is weak
	/*
	 * Find all *.lcrso files in search paths
	 */
	for (i = 0; i < path_list_entries; i++) {
	res = interface_find_and_load (
		path_list[i],
		iface_name,
		version,
		&instance,
		&iface_number);

	if (res == 0) {
		goto found;
	}
	}

	/*
	 * No matching interfaces found in all shared objects
	 */
	return (-1);

found:
	*iface = instance->ifaces[iface_number].interfaces;
	if (instance->ifaces[iface_number].constructor) {
		instance->ifaces[iface_number].constructor (context);
	}
	hdb_handle_create (&lcr_iface_instance_database,
		sizeof (struct lcr_iface_instance),
		iface_handle);
	hdb_handle_get (&lcr_iface_instance_database,
		*iface_handle, (void *)&iface_instance);
	iface_instance->component_handle = instance->comp_handle;
	iface_instance->context = context;
	iface_instance->destructor = instance->ifaces[iface_number].destructor;
	hdb_handle_put (&lcr_iface_instance_database, *iface_handle);
	return (0);
}

int lcr_ifact_release (unsigned int handle)
{
	struct lcr_iface_instance *iface_instance;
	int res = 0;

	res = hdb_handle_get (&lcr_iface_instance_database,
		handle, (void *)&iface_instance);

	if (iface_instance->destructor) {
		iface_instance->destructor (iface_instance->context);
	}

	hdb_handle_put (&lcr_component_instance_database,
		iface_instance->component_handle);
	hdb_handle_put (&lcr_iface_instance_database, handle);
	hdb_handle_destroy (&lcr_iface_instance_database, handle);

	return (res);
}

void lcr_component_register (struct lcr_comp *comp)
{
	struct lcr_component_instance *instance;
	static unsigned int comp_handle;

	hdb_handle_create (&lcr_component_instance_database,
		sizeof (struct lcr_component_instance),
		&comp_handle);
	hdb_handle_get (&lcr_component_instance_database,
		comp_handle, (void *)&instance);

	instance->ifaces = comp->ifaces;
	instance->iface_count = comp->iface_count;
	instance->comp_handle = comp_handle;
	instance->dl_handle = NULL;

	hdb_handle_put (&lcr_component_instance_database,
		comp_handle);

	g_component_handle = comp_handle;
}
