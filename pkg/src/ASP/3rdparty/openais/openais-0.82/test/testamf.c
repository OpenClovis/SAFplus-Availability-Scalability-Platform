/*
 * Copyright (c) 2002-2003 MontaVista Software, Inc.
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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>

#include "ais_types.h"
#include "ais_amf.h"
#include "saClm.h"

void printSaClmNodeAddressT (SaClmNodeAddressT *nodeAddress) {
	int i;

	for (i = 0; i < nodeAddress->length; i++) {
		printf ("%d.", nodeAddress->value[i]);
	}
}

void printSaNameT (SaNameT *name)
{
	int i;

	for (i = 0; i < name->length; i++) {
		printf ("%c", name->value[i]);
	}
}

void setSanameT (SaNameT *name, char *str) {
	name->length = strlen (str);
	memcpy (name->value, str, name->length);
}

static int health_flag = -1;
static unsigned int healthcheck_count = 0;
static unsigned int healthcheck_no = 0;
void HealthcheckCallback (SaInvocationT invocation,
	const SaNameT *compName,
	SaAmfHealthcheckT checkType)
{
	SaErrorT res;

	healthcheck_no ++;
	if (health_flag == -1 || healthcheck_no%healthcheck_count == 0) {
		printf ("%u HealthcheckCallback have occured for component: ",healthcheck_no);
		printSaNameT ((SaNameT *)compName);
		printf ("\n");
	}
	res = saAmfResponse (invocation, SA_OK);
	if (res != SA_OK) {
		printf ("response res is %d\n", res);
	}
}

void ReadinessStateSetCallback (SaInvocationT invocation,
	const SaNameT *compName,
	SaAmfReadinessStateT readinessState)
{
	switch (readinessState) {
	case SA_AMF_IN_SERVICE:
		printf ("ReadinessStateSetCallback: '");
		printSaNameT ((SaNameT *)compName);
		printf ("' requested to enter operational state SA_AMF_IN_SERVICE.\n");
		saAmfResponse (invocation, SA_OK);
		break;
	case SA_AMF_OUT_OF_SERVICE:
		printf ("ReadinessStateSetCallback: '");
		printSaNameT ((SaNameT *)compName);
		printf ("' requested to enter operational state SA_AMF_OUT_OF_SERVICE.\n");
		saAmfResponse (invocation, SA_OK);
		break;
	case SA_AMF_STOPPING:
		printf ("ReadinessStateSetCallback: '");
		printSaNameT ((SaNameT *)compName);
		printf ("' requested to stop, stopping.\n");
		saAmfStoppingComplete (invocation, SA_OK);
		break;
	}
}

void ComponentTerminateCallback (
	SaInvocationT invocation,
	const SaNameT *compName)
{
	printf ("ComponentTerminateCallback\n");
}

void CSISetCallback (
	SaInvocationT invocation,
	const SaNameT *compName,
	const SaNameT *csiName,
	SaAmfCSIFlagsT csiFlags,
	SaAmfHAStateT *haState,
	SaNameT *activeCompName,
	SaAmfCSITransitionDescriptorT transitionDescriptor)
{
	switch (*haState) {
	case SA_AMF_ACTIVE:
		printf ("CSISetCallback: '"); 
		printSaNameT ((SaNameT *)compName);
		printf ("' for CSI '");
		printSaNameT ((SaNameT *)compName);
		printf ("'");
 		printf (" requested to enter hastate SA_AMF_ACTIVE.\n");
		saAmfResponse (invocation, SA_OK);
		break;
	case SA_AMF_STANDBY:
		printf ("CSISetCallback: '"); 
		printSaNameT ((SaNameT *)compName);
		printf ("' for CSI '");
		printSaNameT ((SaNameT *)compName);
		printf ("'");
		printf (" requested to enter hastate SA_AMF_STANDBY.\n");
		saAmfResponse (invocation, SA_OK);
		break;
	case SA_AMF_QUIESCED:
		printf ("CSISetCallback: '"); 
		printSaNameT ((SaNameT *)compName);
		printf ("' for CSI '");
		printSaNameT ((SaNameT *)compName);
		printf ("'");
		printf (" requested to enter hastate SA_AMF_QUIESCED.\n");
		saAmfResponse (invocation, SA_OK);
		break;
	}
}

void CSIRemoveCallback (
	SaInvocationT invocation,
	const SaNameT *compName,
	const SaNameT *csiName,
	const SaAmfCSIFlagsT *csiFlags)
{
	printf ("CSIRemoveCallback invocation id %llu compName ",
		(unsigned long long)invocation);
	printSaNameT ((SaNameT *)compName);
	printf (" csiName ");
	printSaNameT ((SaNameT *)csiName);
	printf ("\n");
	saAmfResponse (invocation, SA_OK);
}

void ProtectionGroupTrackCallback (
	const SaNameT *csiName,
	SaAmfProtectionGroupNotificationT *notificationBuffer,
	SaUint32T numberOfItems,
	SaUint32T numberOfMembers,
	SaErrorT error)
{
	int i;

	printf ("ProtectionGroupTrackCallback items %d members %d\n", (int)numberOfItems, (int)numberOfMembers);
	printf ("buffer is %p\n", notificationBuffer);
	for (i = 0; i < numberOfItems; i++) {
		printf ("component name");
		printSaNameT (&notificationBuffer[i].member.compName);
		printf ("\n");
		printf ("\treadiness state is %d\n",  notificationBuffer[i].member.readinessState);
		printf ("\thastate %d\n",  notificationBuffer[i].member.haState);
		printf ("\tchange is %d\n",  notificationBuffer[i].change);

	}
}

void ExternalComponentRestartCallback (
	const SaInvocationT invocation,
	const SaNameT *externalCompName)
{
	printf ("ExternalComponentRestartCallback\n");
}

void ExternalComponentControlCallback (
	const SaInvocationT invocation,
	const SaNameT *externalCompName,
	SaAmfExternalComponentActionT controlAction)
{
	printf ("ExternalComponentControlCallback\n");
}

void PendingOperationConfirmCallback (
	const SaInvocationT invocation,
	const SaNameT *compName,
	SaAmfPendingOperationFlagsT pendingOperationFlags)
{
	printf ("PendingOperationConfirmCallback\n");
}

void PendingOperationExpiredCallback (
	const SaNameT *compName,
	SaAmfPendingOperationFlagsT pendingOperationFlags)
{
	printf ("PendingOperationExpiredCallback\n");
}

SaAmfCallbacksT amfCallbacks = {
	HealthcheckCallback,
	ReadinessStateSetCallback,
	ComponentTerminateCallback,
	CSISetCallback,
	CSIRemoveCallback,
	ProtectionGroupTrackCallback,
	ExternalComponentRestartCallback,
	ExternalComponentControlCallback,
	PendingOperationConfirmCallback,
	PendingOperationExpiredCallback
};

SaVersionT version = { 'A', 1, 1 };

void sigintr_handler (int signum) {
        exit (0);
}

int main (int argc, char **argv) {
	SaAmfHandleT handle;
	SaAmfHandleT handleproxy;
	int result;
	SaSelectionObjectT select_fd;
	fd_set read_fds;
	SaNameT compName;
	SaNameT csiName;
	SaNameT compname_get_name;
	SaAmfReadinessStateT readinessState;
	SaAmfHAStateT HAState;
	SaAmfComponentCapabilityModelT componentCapabilityModel;
	SaAmfProtectionGroupNotificationT protectionGroupNotificationBuffer[64];
	SaAmfPendingOperationFlagsT pending_operation;
	extern char *optarg;
	extern int optind;
	int c;

        signal (SIGINT, sigintr_handler);

	for (;;) {
		c = getopt(argc,argv,"h:");
		if (c==-1) {
			break;
		}
		switch (c) {
		case 0 :
			break;
		case 'h':
			health_flag = 0;
			sscanf (optarg,"%ud" ,&healthcheck_count);
			break;
		default :
			break;
		}
	}

	result = saAmfInitialize (&handle, &amfCallbacks, &version);
	if (result != SA_OK) {
	printf ("initialize result is %d\n", result);
	exit (1);
	}
	result = saAmfInitialize (&handleproxy, &amfCallbacks, &version);
	if (result != SA_OK) {
	printf ("initialize result is %d\n", result);
	exit (1);
	}

	FD_ZERO (&read_fds);
	saAmfSelectionObjectGet (&handle, &select_fd);
	FD_SET (select_fd, &read_fds);
	saAmfSelectionObjectGet (&handleproxy, &select_fd);
	FD_SET (select_fd, &read_fds);

	setSanameT (&compName, "comp_a_in_su_x");
	setSanameT (&csiName, "pgA");

    result = saAmfComponentRegister (&handle, &compName, NULL);
	printf ("saAmfComponentRegister with result %d (should be 1) is:", result);

	result = saAmfCompNameGet (&handle, &compname_get_name);
	printf ("saAmfCompNameGet with result %d (should be 1) is:", result);
	printSaNameT (&compname_get_name);
	printf ("\n");

	result = saAmfPendingOperationGet (&compName, &pending_operation);
	printf ("saAmfPendingOperationGet with value %d result %d (should be 1) is:",
		pending_operation, result);

	result = saAmfComponentCapabilityModelGet (&compName, &componentCapabilityModel);
	printf ("component capability model get is %d (should be 1)\n", result);

	result = saAmfProtectionGroupTrack (&handle, &csiName, SA_TRACK_CURRENT | SA_TRACK_CHANGES, protectionGroupNotificationBuffer, 64);
	printf ("track start result is %d (should be 1)\n", result);

	result = saAmfProtectionGroupTrackStop (&handle, &csiName);
	printf ("track stop result is %d (should be 1)\n", result);

	result = saAmfReadinessStateGet (&compName, &readinessState);
	printf ("saAmfReadinessStateGet (%d) result %d (should be 1)\n",
		readinessState, result);

	result = saAmfHAStateGet (&compName, &csiName, &HAState);
	printf ("saAmfHAStateGet (%d) result %d (should be 1)\n",
		HAState, result);

	do {
        FD_SET (STDIN_FILENO, &read_fds);
		select (select_fd + 1, &read_fds, 0, 0, 0);
        if (FD_ISSET (STDIN_FILENO, &read_fds)) {
            break;
        }
		saAmfDispatch (&handle, SA_DISPATCH_ALL);
	} while (result);

	result = saAmfProtectionGroupTrackStop (&handle, &csiName);
	saAmfFinalize (&handle);

	return (0);
}
