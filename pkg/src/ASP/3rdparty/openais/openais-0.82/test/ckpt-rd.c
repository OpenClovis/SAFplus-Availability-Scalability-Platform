#define _BSD_SOURCE
/*
 * Copyright (c) 2002-2004 MontaVista Software, Inc.
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
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <time.h>

#include "saAis.h"
#include "saCkpt.h"
#include "sa_error.h"

#define SECONDS_TO_EXPIRE 4

int ckptinv;
void printSaNameT (SaNameT *name)
{
	int i;

	for (i = 0; i < name->length; i++) {
		printf ("%c", name->value[i]);
	}
}

SaVersionT version = { 'B', 1, 1 };

SaNameT checkpointName = { 16, "checkpoint-sync\0" };

char readBuffer1[1025];


SaCkptIOVectorElementT ReadVectorElements[] = {
	{
		SA_CKPT_DEFAULT_SECTION_ID,
		readBuffer1,
		sizeof (readBuffer1),
		0, 
		0
	}
};

#define DATASIZE 127000
char data[DATASIZE];

SaCkptCallbacksT callbacks = {
 	0,
	0
};

int main (void) {
	SaCkptHandleT ckptHandle;
	SaCkptCheckpointHandleT checkpointHandle;
	SaAisErrorT error;
	SaUint32T erroroneousVectorIndex = 0;
	struct timespec delay;	
	delay.tv_sec = 1;
	delay.tv_nsec = 0;


	
	error = saCkptInitialize (&ckptHandle, &callbacks, &version);
	
	error = saCkptCheckpointOpen (ckptHandle,
		&checkpointName,
		0,
		SA_CKPT_CHECKPOINT_READ,
		0,
		&checkpointHandle);
	printf ("%s: initial open of checkpoint\n",
		get_test_output (error, SA_AIS_OK));
	
	while (1) {
		error = saCkptCheckpointRead (checkpointHandle,
			ReadVectorElements,
			1,
			&erroroneousVectorIndex);
		if (error != SA_AIS_OK) {
			printf ("%s: read checkpoint\n",
						get_test_output (error, SA_AIS_OK));
			return (0);
		}

		printf ("Checkpoint contains %s\n", 
			((((char*) ReadVectorElements->dataBuffer)[0] != '\0') ? (char *)ReadVectorElements->dataBuffer : "empty string."));
		nanosleep(&delay,0);
	}

	return (0);
}
