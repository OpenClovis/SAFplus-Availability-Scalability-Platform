/*
 * 
 *   Copyright (C) 2002-2009 by OpenClovis Inc. All Rights  Reserved.
 * 
 *   The source code for this program is not published or otherwise divested
 *   of its trade secrets, irrespective of what has been deposited with  the
 *   U.S. Copyright office.
 * 
 *   No part of the source code  for this  program may  be use,  reproduced,
 *   modified, transmitted, transcribed, stored  in a retrieval  system,  or
 *   translated, in any form or by  any  means,  without  the prior  written
 *   permission of OpenClovis Inc
 */
/*
 * Build: 4.2.0
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
