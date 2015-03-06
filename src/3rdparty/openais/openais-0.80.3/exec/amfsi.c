/** @file amfsi.c
 * 
 * Copyright (c) 2006 Ericsson AB.
 *  Author: Hans Feldt
 * - Refactoring of code into several AMF files
 *  Author: Anders Eriksson, Lars Holm
 *  - Component/SU restart, SU failover
 *
 * All rights reserved.
 *
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
 * 
 * AMF Workload related classes Implementation
 * 
 * This file contains functions for handling :
 *	- AMF service instances(SI)
 *	- AMF SI Dependency
 *	- AMF SI Ranked SU
 *	- AMF SI Assignment
 *	- AMF component service instances (CSI)
 *	- AMF CSI Assignment
 *	- AMF CSI Type
 *	- AMF CSI Attribute
 * The file can be viewed as the implementation of the classes listed above
 * as described in SAI-Overview-B.02.01. The SA Forum specification 
 * SAI-AIS-AMF-B.02.01 has been used as specification of the behaviour
 * and is referred to as 'the spec' below.
 * 
 * The functions in this file are responsible for:
 * - calculating and storing an SI_dependency_level integer per SI
 * - calculating and storing a csi_dependency_level integer per CSI
 * - on request change HA state of an SI or CSI in such a way that the
 *   requirements regarding SI -> SI dependencies (paragraphs 3.9.1.1 and
 *   3.9.1.2) and CSI -> CSI dependencies (paragraph 3.9.1.3) are fully
 *   respected
 *
 * The si_dependency_level is an attribute calculated at init (in the future
 * also at reconfiguration) which indicates dependencies between SIs as
 * an integer. The si_dependency level indicates to which extent an SI depends
 * on other SIs such that an SI that depends on no other SI is on 
 * si_dependecy_level == 1, an SI that depends only on an SI on
 * si_dependency_level == 1 is on si_dependency-level == 2. 
 * An SI that depends on several SIs gets a si_dependency_level that is one
 * unit higher than the SI with the highest si_dependency_level it depends on.
 *
 * The csi_dependency_level attribute works the same way.
 *
 * According to paragraph 3.9.1 of the spec, a change to or from the ACTIVE
 * HA state is not always allowed without first deactivate dependent SI and CSI
 * assignments. Dependencies between CSIs are absolute while an SI that depends
 * on another SI may tolerate that the SI on which it depends is inactive for a
 * configurable time (the tolerance time). The consequence of this is that a
 * request to change the SI state may require a sequence of requests to
 * components to assume a new HA state for a CSI-assignment and to guarantee
 * the dependency rules, the active response from the component has to be
 * awaited before next HA state can be set.
 * 
 * This file implements an SI state machine that fully implements these rules.
 * This state machine is called SI Dependency Control State Machine (dcsm)
 * and has the following states:
 *	- DEACTIVATED (there is no SI-assignment with active HA state)
 *	- ACTIVATING (a request to set the ACTIVE HA state has been received and
 *				  setting ACTIVE HA states to the appropriate components are
 *				  in progress)
 *	- ACTIVATED	(there is at least one SI-assignment with the ACTIVE HA-state)
 *	- DEACTIVATING (a request to de-activate an SI or only a specific CSI
 *					within an SI has been received and setting the QUISCED
 *				    HA states to the appropriate components are in progress)
 *	- DEPENDENCY_DEACTIVATING (the SI-SI dependency tolerance timer has expired
 *                             and setting the QUISCED HA states to the
 *							   appropriate components are in progress)
 *	- DEPENDENCY_DEACTIVATED (as state DEACTIVATED but will automatically
 *							  transition to state ACTIVATING when the
 *							  dependency problem is solved, i.e. the SI on
 *							  which it depends has re-assumed the ACTIVE HA
 *							  state)
 *	- SETTING (a request to change the HA state when neither the existing
 *			   nor the requested state is ACTIVE)
 * 
 * This file also implements:
 *	- SI:             Assignment state (for report purposes)
 *	- SI Assignment:  HA state
 *	- CSI Assignment: HA state
 * 
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "amf.h"
#include "print.h"
#include "util.h"
#include "aispoll.h"
#include "main.h"

/**
 * Check if any CSI assignment belonging to SU has the requested
 * state.
 * @param su
 * @param hastate
 * 
 * @return int
 */
static int any_csi_has_hastate_in_su (struct amf_su *su, SaAmfHAStateT hastate)
{
	struct amf_comp *component;
	struct amf_csi_assignment *csi_assignment;
	int exist = 0;

	for (component = su->comp_head; component != NULL;
		component = component->next) {

		csi_assignment = amf_comp_get_next_csi_assignment (component, NULL);
		while (csi_assignment != NULL) {
			if (csi_assignment->saAmfCSICompHAState == hastate) {
				exist = 1;
				goto done;
			}
			csi_assignment =
				amf_comp_get_next_csi_assignment (component, csi_assignment);
		}
	}

	done:
	return exist;
}

/**
 * Check if all CSI assignments belonging to a
 * an SI assignemnt has the requested state.
 * @param su
 * @param hastate
 * 
 * @return int
 */
static int all_csi_has_hastate_for_si (
	struct amf_si_assignment *si_assignment, SaAmfHAStateT hastate)
{
	struct amf_comp *component;
	struct amf_csi_assignment *tmp_csi_assignment;
	int all = 1;

	for (component = si_assignment->su->comp_head; component != NULL;
		component = component->next) {

		tmp_csi_assignment = amf_comp_get_next_csi_assignment (component, NULL);
		while (tmp_csi_assignment != NULL) {
			if ((tmp_csi_assignment->si_assignment == si_assignment) &&
				(tmp_csi_assignment->saAmfCSICompHAState != hastate)) {

				all = 0;
				goto done;
			}
			tmp_csi_assignment =
				amf_comp_get_next_csi_assignment (component, tmp_csi_assignment);
		}
	}

	done:
	return all;
}

/**
 * Implements table 6 in 3.3.2.4
 * TODO: active & standby is not correct calculated acc. to
 * table. This knowledge is e.g. used in assign_si_assumed_cbfn
 * (sg.c)
 * @param csi_assignment
 */
static void set_si_ha_state (struct amf_csi_assignment *csi_assignment)
{
	SaAmfHAStateT old_ha_state =
		csi_assignment->si_assignment->saAmfSISUHAState;
	SaAmfAssignmentStateT old_assigment_state =
		amf_si_get_saAmfSIAssignmentState (csi_assignment->csi->si);

	if (all_csi_has_hastate_for_si (
		csi_assignment->si_assignment, SA_AMF_HA_ACTIVE)) {

		csi_assignment->si_assignment->saAmfSISUHAState = SA_AMF_HA_ACTIVE;
	}

	if (all_csi_has_hastate_for_si (
		csi_assignment->si_assignment, SA_AMF_HA_STANDBY)) {

		csi_assignment->si_assignment->saAmfSISUHAState = SA_AMF_HA_STANDBY;
	}

	if (any_csi_has_hastate_in_su (
		csi_assignment->comp->su, SA_AMF_HA_QUIESCING)) {

		csi_assignment->si_assignment->saAmfSISUHAState = SA_AMF_HA_QUIESCING;
	}
	if (any_csi_has_hastate_in_su (
		csi_assignment->comp->su, SA_AMF_HA_QUIESCED)) {

		csi_assignment->si_assignment->saAmfSISUHAState = SA_AMF_HA_QUIESCED;
	}

	/* log changes to HA state */
	if (old_ha_state != csi_assignment->si_assignment->saAmfSISUHAState) {
		log_printf (LOG_NOTICE, "SU HA state changed to '%s' for:\n"
			"\t\tSI '%s', SU '%s'",
			amf_ha_state (csi_assignment->si_assignment->saAmfSISUHAState),
			csi_assignment->si_assignment->si->name.value,
			csi_assignment->si_assignment->name.value);
	}

	/* log changes to assignment state */
	if (old_assigment_state !=
		amf_si_get_saAmfSIAssignmentState (csi_assignment->csi->si)) {
		log_printf (LOG_NOTICE, "SI Assignment state changed to '%s' for:\n"
			"\t\tSI '%s', SU '%s'",
			amf_assignment_state (
			amf_si_get_saAmfSIAssignmentState (csi_assignment->csi->si)),
			csi_assignment->si_assignment->si->name.value,
			csi_assignment->si_assignment->name.value);
	}
}

char *amf_csi_dn_make (struct amf_csi *csi, SaNameT *name)
{
	int i = snprintf((char*) name->value, SA_MAX_NAME_LENGTH,
		"safCsi=%s,safSi=%s,safApp=%s",
		csi->name.value, csi->si->name.value,
		csi->si->application->name.value);
	assert (i <= SA_MAX_NAME_LENGTH);
	name->length = i;

	return(char *)name->value;
}

void amf_si_init (void)
{
	log_init ("AMF");
}

void amf_si_comp_set_ha_state_done (
	struct amf_si *si, struct amf_csi_assignment *csi_assignment)
{
	ENTER ("'%s', '%s'", si->name.value, csi_assignment->csi->name.value);

	set_si_ha_state (csi_assignment);

	assert (csi_assignment->si_assignment->assumed_callback_fn != NULL);

	/*                                                              
	 * Report to caller when the requested SI assignment state is
	 * confirmed.
	 */
	if (csi_assignment->si_assignment->requested_ha_state ==
		csi_assignment->si_assignment->saAmfSISUHAState) {

		csi_assignment->si_assignment->assumed_callback_fn (
			csi_assignment->si_assignment, 0);
		csi_assignment->si_assignment->assumed_callback_fn = NULL;
	}
}

void amf_si_activate (
	struct amf_si *si,
	void (*activated_callback_fn)(struct amf_si *si, int result))
{
	struct amf_csi *csi;

	ENTER ("'%s'", si->name.value);

	for (csi = si->csi_head; csi != NULL; csi = csi->next) {
		struct amf_csi_assignment *csi_assignment;

		for (csi_assignment = csi->assigned_csis; csi_assignment != NULL;
			csi_assignment = csi_assignment->next) {

			csi_assignment->si_assignment->requested_ha_state =
				SA_AMF_HA_ACTIVE;

			/*                                                              
			 * TODO: only active assignments should be set when dependency
			 * levels are used.
			 */
			csi_assignment->requested_ha_state = SA_AMF_HA_ACTIVE;
			amf_comp_hastate_set (csi_assignment->comp, csi_assignment);
		}
	}
}

void amf_si_comp_set_ha_state_failed (
	struct amf_si *si, struct amf_csi_assignment *csi_assignment)
{
	ENTER ("");
	assert (0);
}

static void timer_function_ha_state_assumed (void *_si_assignment)
{
	struct amf_si_assignment *si_assignment = _si_assignment;

	ENTER ("");
	si_assignment->saAmfSISUHAState = si_assignment->requested_ha_state;
	si_assignment->assumed_callback_fn (si_assignment, 0);
}

void amf_si_ha_state_assume (
	struct amf_si_assignment *si_assignment,
	void (*assumed_ha_state_callback_fn)(struct amf_si_assignment *si_assignment,
	int result))
{
	struct amf_csi_assignment *csi_assignment;
	struct amf_csi *csi;
	int csi_assignment_cnt = 0;
	int hastate_set_done_cnt = 0;

	ENTER ("SI '%s' SU '%s' state %s", si_assignment->si->name.value,
		si_assignment->su->name.value,
		amf_ha_state (si_assignment->requested_ha_state));

	si_assignment->assumed_callback_fn = assumed_ha_state_callback_fn;

	for (csi = si_assignment->si->csi_head; csi != NULL; csi = csi->next) {
		for (csi_assignment = csi->assigned_csis; csi_assignment != NULL;
			csi_assignment = csi_assignment->next) {

			/*                                                              
			 * If the CSI assignment and the SI assignment belongs to the
			 * same SU, we have a match and can request the component to
			 * change HA state.
			 */
			if (name_match (&csi_assignment->comp->su->name,
				&si_assignment->su->name) &&
				(csi_assignment->saAmfCSICompHAState !=
				si_assignment->requested_ha_state)) {

				csi_assignment_cnt++;
				csi_assignment->requested_ha_state =
					si_assignment->requested_ha_state;
				amf_comp_hastate_set (csi_assignment->comp, csi_assignment);

				if (csi_assignment->saAmfCSICompHAState ==
					csi_assignment->requested_ha_state) {

					hastate_set_done_cnt++;
				}
			}
		}
	}

	/*                                                              
	 * If the SU has only one component which is the faulty one, we
	 * will not get an asynchronous response from the component.
	 * This response (amf_si_comp_set_ha_state_done) is used to do
	 * the next state transition. The asynchronous response is
	 * simulated using a timeout instead.
	 */
	if (csi_assignment_cnt == hastate_set_done_cnt) {
		poll_timer_handle handle;
		poll_timer_add (aisexec_poll_handle,
			0,
			si_assignment,
			timer_function_ha_state_assumed,
			&handle);
	}
}

/**
 * Get number of active assignments for the specified SI
 * @param si
 * 
 * @return int
 */
int amf_si_get_saAmfSINumCurrActiveAssignments (struct amf_si *si)
{
	int cnt = 0;
	struct amf_si_assignment *si_assignment;

	for (si_assignment = si->assigned_sis; si_assignment != NULL;
		si_assignment = si_assignment->next) {

		if (si_assignment->saAmfSISUHAState == SA_AMF_HA_ACTIVE) {
			cnt++;
		}
	}

	return cnt;
}

int amf_si_get_saAmfSINumCurrStandbyAssignments (struct amf_si *si)
{
	int cnt = 0;
	struct amf_si_assignment *si_assignment;

	for (si_assignment = si->assigned_sis; si_assignment != NULL;
		si_assignment = si_assignment->next) {

		if (si_assignment->saAmfSISUHAState == SA_AMF_HA_STANDBY) {
			cnt++;
		}
	}

	return cnt;
}

SaAmfAssignmentStateT amf_si_get_saAmfSIAssignmentState (struct amf_si *si)
{
	if ((amf_si_get_saAmfSINumCurrActiveAssignments (si) ==
		si->saAmfSIPrefActiveAssignments) &&
		(amf_si_get_saAmfSINumCurrStandbyAssignments (si) ==
		si->saAmfSIPrefStandbyAssignments)) {

		return SA_AMF_ASSIGNMENT_FULLY_ASSIGNED;
	} else if (amf_si_get_saAmfSINumCurrActiveAssignments (si) == 0) {
		return SA_AMF_ASSIGNMENT_UNASSIGNED;
	} else {
		return SA_AMF_ASSIGNMENT_PARTIALLY_ASSIGNED;
	}
}

void amf_csi_delete_assignments (struct amf_csi *csi, struct amf_su *su)
{
	struct amf_csi_assignment *csi_assignment;

	ENTER ("'%s'", su->name.value);

	/*                                                              
	* TODO: this only works for n+m where each CSI list has only
	* two assignments, one active and one standby.
	* TODO: use DN instead
	*/
	if (csi->assigned_csis->comp->su == su) {
		csi_assignment = csi->assigned_csis;
		csi->assigned_csis = csi_assignment->next;
	} else {
		csi_assignment = csi->assigned_csis->next;
		csi->assigned_csis->next = NULL;
		assert (csi_assignment != NULL && csi_assignment->comp->su == su);
	}
	assert (csi_assignment != NULL);
	free (csi_assignment);
}

