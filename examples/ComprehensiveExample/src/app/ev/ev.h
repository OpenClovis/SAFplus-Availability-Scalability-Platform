/*
 * Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
 *
 * This file is available  under  a  commercial  license  from  the
 * copyright  holder or the GNU General Public License Version 2.0.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * For more  information, see  the file  COPYING provided with this
 * material.
 */
/* ev.h
 * Clovis Evaluation Kit Console Library
 */
#ifndef _EV_H_
#define _EV_H_
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
/* int ev_init(int argc, char *argv, char *instance_name)
 *
 * Redirects stdout and stderr to a pseudo terminal.
 * The pseudo terminal device is in a file in /tmp
 * which is named as a function of the app instance,
 * /tmp/clinstancename.tty
 *
 * Takes: argc, argv - standard cli parameters
 *        char *instance_name - the instance name of
 *        the calling app.
 *
 * Returns: 0 if success
 */
int ev_init(int argc, char *argv[], char *instance_name); 

#endif
