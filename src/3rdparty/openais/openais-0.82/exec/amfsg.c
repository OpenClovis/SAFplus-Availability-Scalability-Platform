/** @file amfsg.c
 * 
 * Copyright (c) 2002-2006 MontaVista Software, Inc.
 * Author: Steven Dake (sdake@mvista.com)
 *
 * Copyright (c) 2006 Ericsson AB.
 * Author: Hans Feldt, Anders Eriksson, Lars Holm
 * - Introduced AMF B.02 information model
 * - Use DN in API and multicast messages
 * - (Re-)Introduction of event based multicast messages
 * - Refactoring of code into several AMF files
 * - Component/SU restart, SU failover
 * - Constructors/destructors
 * - Serializers/deserializers
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
 * AMF Service Group Class Implementation
 * 
 * This file contains functions for handling AMF-service groups(SGs). It can be 
 * viewed as the implementation of the AMF Service Group class (called SG)
 * as described in SAI-Overview-B.02.01. The SA Forum specification 
 * SAI-AIS-AMF-B.02.01 has been used as specification of the behaviour
 * and is referred to as 'the spec' below.
 * 
 * The functions in this file are responsible for:
 *	-on request start the service group by instantiating the contained SUs
 *	-on request assign the service instances it protects to the in-service
 *   service units it contains respecting as many as possible of the configured
 *   requirements for the group
 *	-create and delete an SI-assignment object for each relation between
 *	 an SI and an SU
 *	-order each contained SU to create and delete CSI-assignments
 *	-request the Service Instance class (SI) to execute the transfer of the
 *	  HA-state set/remove requests to each component involved
 *	-fully control the execution of component failover and SU failover
 *	-on request control the execution of the initial steps of node switchover
 *	 and node failover
 *	-fully handle the auto adjust procedure
 *
 * Currently only the 'n+m' redundancy model is implemented. It is the 
 * ambition to identify n+m specific variables and functions and add the suffix
 * '_nplusm' to them so that they can be easily recognized.
 * 
 * When SG is requested to assign workload to all SUs or all SUs hosted on
 * a specific node, a procedure containing several steps is executed:
 *	<1> An algorithm is executed which assigns SIs to SUs respecting the rules
 *		that has been configured for SG. The algorithm also has to consider 
 *	    if assignments between som SIs and SUs already exist. The scope of this
 *	    algorithm is to create SI-assignments and set up requested HA-state for
 *	    each assignment but not to transfer those HA-states to the components.
 *	<2> All SI-assignments with a requested HA state == ACTIVE are transferred
 *	    to the components concerned before any STANDBY assignments are 
 *      transferred. All components have to acknowledge the setting of the 
 *      ACTIVE HA state before the transfer of any STANDBY assignment is 
 *      initiated.
 *	<3> All active assignments can not be transferred at the same time to the
 *      different components because the rules for dependencies between SI and
 *      SI cluster wide and CSI and CSI within one SI, has to be respected.
 *
 * SG is fully responsible for step <1> but not fully responsible for handling
 * step <2> and <3>. However, SG uses an attribute called 'dependency level'
 * when requsted to assign workload. This parameter refers to an integer that
 * has been calculated initially for each SI. The 'dependency level' indicates
 * to which extent an SI depends on other SIs such that an SI that depends on
 * no other SI is on dependecy_level == 1, an SI that depends only on an SI on
 * dependency_level == 1 is on dependency-level == 2. 
 * An SI that depends on several SIs gets a 
 * dependency_level that is one unit higher than the SI with the highest 
 * dependency_level it depends on. When SG is requested to assign the workload
 * on a certain dependency level, it requests all SI objects on that level to
 * activate (all) SI-assignments that during step <1> has been requested to
 * assume the active HA state.
 *
 * SG contains the following state machines:
 *	- administrative state machine (ADSM) (NOT IN THIS RELEASE)
 *	- availability control state machine (ACSM)
 *
 * The availability control state machine contains three states and one of them
 * is composite. Being a composite state means that it contains substates.
 * The states are:
 * - IDLE (non composite state)
 * - INSTANTIATING_SERVICE_UNITS
 * - MANAGING_SG (composite state)
 * MANAGING_SG is entered at several different events which has in common
 * the need to set up or change the assignment of SIs to SUs. Only one such
 * event can be handled at the time. If new events occur while one event is
 * being handled then the new event is saved and will be handled after the
 * handling of the first event is ready (return to IDLE state has been done).
 * MANAGING_SG handles the following events:
 * - start (requests SG to order SU to instantiate all SUs in SG and waits
 *			for SU to indicate presence state change reports from the SUs and
 *			finally responds 'started' to the requester)
 * - assign_si (requests SG to assign SIs to SUs according to pre-configured 
 *			   rules (if not already done) and transfer the HA state of
 *			   the SIs on the requested SI dependency level. Then SG waits for 
 *			   confirmation that the HA state has been succesfully set and 
 *			   finally responds 'assigned' to the reqeuster)
 * - auto_adjust (this event indicates that the auto-adjust probation timer has
 *				  expired and that SG should evaluate current assignments of
 *				  SIs to SUs and if needed remove current assignments and 
 *				  create new according to what is specified in paragraph
 *				  3.7.1.2) 
 * - failover_comp (requests SG to failover a specific component according to
 *					the procedure described in paragraph 3.12.1.3)
 * - failover_su (requests SG to failover a specific SU according to the 
 *				  procedure described in paragraph 3.12.1.3 and 3.12.1.4)
 * - switchover_node (requests SG to execute the recovery actions described
 *					  in 3.12.1.3 and respond to the requester when recovery 
 *					  is completed)
 * - failover_node (requests SG to execute the recovery actions described
 *				   in 3.12.1.3 and respond to the requester when recovery is
 *                 completed)
 * 
* 1. SG Availability Control State Machine
 * ==========================================
 * 
 * 1.1  State Transition Table
 * 
 * State:              Event:                Action:          New state:
 * ============================================================================
 * IDLE				   start			     A48,A28          INSTANTIATING_SUs
 * IDLE				   assign_si    	     A48,A31          ASSIGNING_ON_REQ
 * IDLE            	   failover_su           A48,[C22]A10,A11 DEACTIVATING_DEP
 * IDLE                failover_su           A48,[!C22]A12    TERMINATING_SUSP
 * IDLE                failover_node         A48,[!C22]A12    TERMINATING_SUSP
 * IDLE				   failover_node         A48,[C22]A10,A11 DEACTIVATING_DEP
 * IDLE                failover_node         A48,[C100]A34	  IDLE
 * INSTANTIATING_SUs   start			     A48,A28          INSTANTIATING_SUs
 * INSTANTIATING_SUs   su_state_chg          [C101]A26,A53    IDLE		  
 * INSTANTIATING_SUs   su_state_chg          [C102]A26,A53    IDLE
 * INSTANTIATING_SUs   assign_si    	     A31              ASSIGNING_ON_REQ
 * INSTANTIATING_SUs   failover_su           A52			  INSTANTIATING_SUs
 * INSTANTIATING_SUs   failover_node         A52			  INSTANTIATING_SUs
 * ASSIGNING_ON_REQ    ha_state_assumed		 [C15]A54		  IDLE
 * ASSIGNING_ON_REQ    failover_su           A52              ASSIGNING_ON_REQ
 * ASSIGNING_ON_REQ    failover_node         A52              ASSIGNING_ON_REQ
 * DEACTIVATING_DEP    si_deactivated		 [C20]            REMOVING_S-BY_ASS
 * DEACTIVATING_DEP    si_deactivated		 [!C20]A12        TERMINATING_SUSP
 * TERMINATING_SUSP    su_state_chg          [C103]A24,A20    ASS_S-BY_TO_SPARE
 * TERMINATING_SUSP    su_state_chg          [C104]A24,A50    REMOVING_S-BY_ASS
 * TERMINATING_SUSP    su_state_chg          [C105]A16,A17    ACTIVATING_S-BY
 * TERMINATING_SUSP    su_state_chg          [C106]A20        ASS_S-BY_TO_SPARE
 * TERMINATING_SUSP    su_state_chg          [C108]A23        REPAIRING_SU
 * TERMINATING_SUSP    su_state_chg          [C109]           IDLE
 * TERMINATING_SUSP    failover_su           A52              TERMINATING_SUSP
 * TERMINATING_SUSP    failover_node         A52              TERMINATING_SUSP
 * REMOVING_S-BY_ASS   assignment_removed    A51              REMOVING_S-BY_ASS
 * REMOVING_S-BY_ASS   assignment_removed    [C27]&[C24]      ACTIVATING_S-BY
 * REMOVING_S-BY_ASS   assignment_removed    [C110]A20        ASS_S-BY_TO_SPARE
 * REMOVING_S-BY_ASS   assignment_removed    [C111]A23        REPAIRING_SU
 * REMOVING_S-BY_ASS   assignment_removed    [C112]           IDLE
 * REMOVING_S-BY_ASS   failover_su           A52              REMOVING_S-BY_ASS
 * REMOVING_S-BY_ASS   failover_node         A52              REMOVING_S-BY_ASS
 * ACTIVATING_S-BY     su_activated          [C2]&[C11]A20    ASS_S-BY_TO_SPARE
 * ACTIVATING_S-BY     su_activated          [C113]A23        REPAIRING_SU
 * ACTIVATING_S-BY     su_activated          [C114]           IDLE
 * ACTIVATING_S-BY     failover_su           A52              ACTIVATING_S-BY
 * ACTIVATING_S-BY     failover_node         A52              ACTIVATING_S-BY
 * ASS_S-BY_TO_SPARE   ha_state_assumed      [C115]A23        REPAIRING_SU
 * ASS_S-BY_TO_SPARE   ha_state_assumed      [C116]           IDLE
 * ASS_S-BY_TO_SPARE   failover_su           A52              ASS_S-BY_TO_SPARE	
 * ASS_S-BY_TO_SPARE   failover_node         A52              ASS_S-BY_TO_SPARE
 * REPAIRING_SU        su_state_chg          [C28]A36,A37,A31 ASSIGNING_WL
 * REPAIRING_SU        su_state_chg          [C28][C31]       IDLE
 * REPAIRING_SU        failover_su           A52              REPAIRING_SU
 * REPAIRING_SU        failover_node         A52              REPAIRING_SU
 * ASSIGNING_WL        ha_state_assumed      [C15]            IDLE
 * ASSIGNING_WL        failover_su           A52              ASSIGNING_WL
 * ASSIGNING_WL        failover_node         A52              ASSIGNING_WL
 * 
 *  1.2 State Description
 *  =====================
 * IDLE 			 -  SG is synchronized and idle. When IDLE state is set,
 *					  the oldest deferred event (if any) is recalled. 
 * INSTANTIATING_SUs - INSTANTIATING_SERVICE_UNITS
 *                    SG has ordered all contained SUs to instantiate and
 *                    waits for the SUs to report a change of their
 *                    presence state. SG is also prepared to accept an
 *                    order to assign workload in this state.
 * ASSIGNING_ON_REQ  - ASSIGNING_ON_REQUEST
 *					  SG has on request assigned workload to all service units
 *					  on the requested dependency level and waits for SIs to
 *					  indicate that the requested HA-state have been set to the
 *                    appropriate components.
 * TERMINATING_SUSP  - TERMINATING_SUSPECTED
 *					  SG has cleaned up all components suspected to be
 *                    erroneous and waits for the concerned SUs to report a
 *                    change of their presence states.
 * REMOVING_S-BY_ASS - REMOVING_STANDBY_ASSIGNMENTS
 *                    This state is only applicable to the n-plus-m redundancy
 *                    model. In this state, SG has initiated the removal of
 *                    those assignments from standby SUs that do not match the
 *                    assignments of error suspected SUs. The reason for this
 *                    removal is a preparation for not violating the rule which
 *                    says an SU can not have both active and standby assign-
 *					  ments simultaneously in the n-plus-m redundancy model.
 * ACTIVATING_S-BY   - ACTIVATING_STANDBY
 *                    SG has located all standby SI-assignments in the recovery
 *                    scope and ordered the corresponding SI to set the active
 *                    HA-state and waits for SI to indicate that the requested
 *                    HA-state has been set to the appropriate components.
 * ASS_S-BY_TO_SPARE - ASSIGNING_STANDBY_TO_SPARE
 *					  Current SG is configured with a spare SU. In this state,
 *                    SG has requested all SIs to assign the standby HA-state
 *					  to the spare SU and waits for the SIs to indicate that
 *					  the standby HA-state have been set.
 * REPAIRING_SU      - REPAIRING_SU
 *                    In this state SG has initiated instantiation of all SUs
 *                    in current recovery scope until the configured preference
 *                    of number of instantiated SUs is fulfiled. SG then waits
 *                    for the concerned SUs to report a change of presence
 *					  state.
 * ASSIGNING_WL      - ASSIGNING_WORKLOAD
 *                    In this state SG has initiated the assignment of workload
 *					  to all or a subset of its contained SUs and waits for the
 *                    concerned SIs to indicated that the requested HA-state
 *                    has been assumed.
 *
 * 1.3 Actions
 * ===========
 * A1  - 
 * A2  - 
 * A3  - 
 * A4  - 
 * A5  - 
 * A6  - 
 * A7  - 
 * A8  - 
 * A9  - 
 * A10 - [foreach SI in the scope]&[foreach SI-assignment with
 *       confirmed-HA-state == ACTIVE]/set requested-ha-state = QUIESCED
 * A11 - [foreach SI in the scope]/deactivate SI
 * A12 - [foreach suspected SU]/terminate all components
 * A13 - 
 * A14 - 
 * A15 - 
 * A16 - [foreach SI in the scope]&[foreach SI-assignment with
 *       confirmed-HA-state == STANDBY]/set requested-ha-state = ACTIVE
 * A17 - [foreach SI in the scope]/activate SI
 * A18 - 
 * A19 - 
 * A20 -
 * A21 -
 * A22 -
 * A23 -
 * A24 -
 * A25 -
 * A26 -
 * A27 -
 * A28 -
 * A29 -
 * A30 -
 * A31 -
 * A32 -
 * A33 -
 * A34 -
 * A35 -
 * A36 -
 * A37 -
 * A38 -
 * A39 -
 * A40 -
 * A41 -
 * A42 -
 * A43 -
 * A44 -
 * A45 -
 * A46 -
 * A47 -
 * A48 -
 * A49 -

 * 
 * 1.4 Composite Guards
 * ====================
 * The meaning with these guards is just to save space in the state transition
 * table above.
 * C100 - [C7]&[!C22]&[C20]
 * C101 - [C12]&[C28]
 * C102 - [C13]&[C28]
 * C103 - [C6]&[C21]&[C11]
 * C104 - [C6]&[C25]&[C26]
 * C105 - [C6]&(!([C25]|[C26]))&[C24]
 * C106 - [C6]&(!([C25]|[C26]))&[!C24]&[C11]
 * C107 -
 * C108 - [C6]&(!([C25]|[C26]))&[!C24]&[!C11]&[C9]&[C10]&[!C30]
 * C109 - [C6]&(!([C25]|[C26]))&[!C24]&[!C11]&[C9]&[C10]&[C30]
 * C110 - [C27]&[!C24]&[C11]
 * C111 - [C27]&[!C24]&[!C11]&[C9]&[C10]&[!C30]
 * C112 - [C27]&[!C24]&[!C11]&[C9]&[C10]&[C30]
 * C113 - [C2]&[!C11]&[C9]&[C10]&[!C30]
 * C114 - [C2]&[!C11]&[C9]&[C10]&[C30]
 * C115 - [C9]&[C10]&[!C30]
 * C116 - [C9]&[C10]&[C30]
 * 
 * 1.4 Guards
 * ==========
 * C1  -
 * C2  - all SI in the recovery scope
 * C3  - 
 * C4  -
 * C5  -
 * C6  - all supected SUs or components have presence state == UNINSTANTIATED
 *       or presence state == INSTANTIATION_FAILED
 * C7  - original event == failover node
 * C8  -
 * C9  - original event == failover su
 * C10 - SaAmfSGAutoRepair == true
 * C11 - spare SUs exist
 * C12 - original event == start(all SUs)
 * C13 - original event == start(node)
 * C14 -
 * C15 - all SI-assignments on current dependency-level have requested-ha-state
         == confirmed-ha-state or operation failed
 * C16 -
 * C17 -
 * C18 -
 * C19 -
 * C20 - all suspected SUs have presence state == UNINSTANTIATED or
         presence state == xx_FAILED
 * C21 - no SI in the scope has an SI-assignment with HA-state == STANDBY
 * C22 - the concerned entity has ACTIVE HA-state
 * C23 -
 * C24 - at least one SI-assignment related to an SI in the scope has HA-state
         == STANDBY
 * C25 - redundancy model == N plus M
 * C26 - at least one SU has STANDBY assignments for more SIs than those SIs
         that are within the recovery scope
 * C27 - no SI-assignment related to SI protected by current SG has requested
         HA-state == REMOVED
 * C28 - no SU has presence state == INSTANTIATING
 * C29 - 
 * C30 - more SUs not needed or the node hosting the SU to repair is disabled.
 * C31 - no new additional assignments needed or possible
 */

#include <stdlib.h>
#include <errno.h>

#include "amf.h"
#include "logsys.h"
#include "main.h"
#include "util.h"

LOGSYS_DECLARE_SUBSYS ("AMF", LOG_INFO);

static int assign_si (struct amf_sg *sg, int dependency_level);
static void acsm_enter_activating_standby (struct amf_sg *sg);
static void delete_si_assignments_in_scope (struct amf_sg *sg);
static void acsm_enter_repairing_su (struct amf_sg *sg);
static void standby_su_activated_cbfn (
	struct amf_si_assignment *si_assignment, int result);
static void dependent_si_deactivated_cbfn (
	struct amf_si_assignment *si_assignment, int result);
static void dependent_si_deactivated_cbfn2 (struct amf_sg *sg);
static void assign_si_assumed_cbfn (
	struct amf_si_assignment *si_assignment, int result);
static void acsm_enter_removing_standby_assignments (amf_sg_t *sg);
static void acsm_enter_assigning_standby_to_spare (amf_sg_t *sg);

static const char *sg_event_type_text[] = {
	"Unknown",
	"Failover SU",
	"Failover node",
	"Failover comp",
	"Switchover node",
	"Start",
	"Autoadjust",
	"Assign SI"
};

typedef struct sg_event {
	amf_sg_event_type_t event_type;
	amf_sg_t *sg;
	amf_su_t *su;
	amf_comp_t *comp;
	amf_node_t *node;
} sg_event_t;

/******************************************************************************
 * Internal (static) utility functions
 *****************************************************************************/

static	int is_cluster_start(amf_node_t *node_to_start) 
{
	return node_to_start == NULL;
}

static void sg_set_event (amf_sg_event_type_t sg_event_type,
	amf_sg_t *sg, amf_su_t *su, amf_comp_t *comp, amf_node_t * node,
	sg_event_t *sg_event)
{
	sg_event->event_type = sg_event_type;
	sg_event->node = node;
	sg_event->su = su;
	sg_event->comp = comp;
	sg_event->sg = sg;
}

static void sg_defer_event (amf_sg_event_type_t event_type,
	sg_event_t *sg_event)
{
	ENTER("Defered event = %d", event_type);
	amf_fifo_put (event_type, &sg_event->sg->deferred_events,
		sizeof (sg_event_t),
		sg_event);
}

static void sg_recall_deferred_events (amf_sg_t *sg)
{
	sg_event_t sg_event;

	ENTER ("%s", sg->name.value);
	if (amf_fifo_get (&sg->deferred_events, &sg_event)) {
		switch (sg_event.event_type) {
			case SG_FAILOVER_SU_EV:
				amf_sg_failover_su_req (sg_event.sg, 
					sg_event.su, sg_event.node);
				break;
			case SG_FAILOVER_NODE_EV:
				amf_sg_failover_node_req (sg_event.sg, 
					sg_event.node);
				break;
			case SG_FAILOVER_COMP_EV:
			case SG_SWITCH_OVER_NODE_EV:
			case SG_START_EV:
			case SG_AUTO_ADJUST_EV:
			default:
				dprintf("event_type = %d", sg_event.event_type);
				break;
		}
	}
}

static void timer_function_sg_recall_deferred_events (void *data)
{
	amf_sg_t *sg = (amf_sg_t*)data;
	ENTER ("");

	sg_recall_deferred_events (sg);
}

static void acsm_enter_idle (amf_sg_t *sg)
{
	SaNameT dn;

	ENTER ("sg: %s state: %d", sg->name.value, sg->avail_state);

	sg->avail_state = SG_AC_Idle;
	if (sg->recovery_scope.event_type != 0) {
		switch (sg->recovery_scope.event_type) {
			case SG_FAILOVER_SU_EV:
				assert (sg->recovery_scope.sus[0] != NULL);
				amf_su_dn_make (sg->recovery_scope.sus[0], &dn);
				log_printf (
					LOG_NOTICE,
					"'%s' %s recovery action finished",
					dn.value,
					sg_event_type_text[sg->recovery_scope.event_type]);

				break;
			case SG_FAILOVER_NODE_EV:
				amf_node_sg_failed_over (
					sg->recovery_scope.node, sg);
				log_printf (
					LOG_NOTICE, 
					"'%s for %s' recovery action finished",
					sg_event_type_text[sg->recovery_scope.event_type],
					sg->name.value);
				break;
			case SG_START_EV:
				amf_application_sg_started (sg->application,
					sg, this_amf_node);
				break;
			case SG_ASSIGN_SI_EV:
				log_printf (LOG_NOTICE, "All SI assigned");
				break;
			default:
				log_printf (
					LOG_NOTICE, 
					"'%s' recovery action finished",
					sg_event_type_text[sg->recovery_scope.event_type]);
				break;
		}
	}

	if (sg->recovery_scope.sus != NULL) {
		free ((void *)sg->recovery_scope.sus);
	}
	if (sg->recovery_scope.sis != NULL) {
		free ((void *)sg->recovery_scope.sis);
	}
	memset (&sg->recovery_scope, 0, sizeof (struct sg_recovery_scope));
	sg->node_to_start = NULL;

	amf_call_function_asynchronous (
		timer_function_sg_recall_deferred_events, sg);
}

static int su_instantiated_count (struct amf_sg *sg)
{
	int cnt = 0;
	struct amf_su *su;

	for (su = sg->su_head; su != NULL; su = su->next) {
		if (su->saAmfSUPresenceState == SA_AMF_PRESENCE_INSTANTIATED)
			cnt++;
	}

	return cnt;
}

static int has_any_su_in_scope_active_workload (struct amf_sg *sg)
{
	struct amf_su **sus= sg->recovery_scope.sus;
	struct amf_si_assignment *si_assignment;

	while (*sus != NULL) {
		si_assignment = amf_su_get_next_si_assignment (*sus, NULL);
		while (si_assignment != NULL) {
			if (si_assignment->saAmfSISUHAState != 
				SA_AMF_HA_ACTIVE) {
				break;
			}
			si_assignment = amf_su_get_next_si_assignment (
				*sus, si_assignment);
		}
		if (si_assignment != NULL) {
			break;
		}
		sus++;
	}
	return(*sus == NULL);
}

static int is_any_si_in_scope_assigned_standby (struct amf_sg *sg)
{
	struct amf_si **sis= sg->recovery_scope.sis;
	struct amf_si_assignment *si_assignment;

	/*
     * Check if there is any si in the scope which has no
     * active assignment and at least one standby assignment.
	 */
	while (*sis != NULL) {
		si_assignment = (*sis)->assigned_sis;
		while (si_assignment != NULL) {
			if (si_assignment->saAmfSISUHAState == 
				SA_AMF_HA_ACTIVE) {
				break;
			}
			si_assignment = si_assignment->next;
		}
		if (si_assignment == NULL) {
			/* There is no ACTIVE assignment ..*/
			si_assignment = (*sis)->assigned_sis;
			while (si_assignment != NULL) {
				if (si_assignment->saAmfSISUHAState == 
					SA_AMF_HA_STANDBY) {
					break;
				}
				si_assignment = si_assignment->next;
			}
			if (si_assignment != NULL) {
				/* .. and one STANDBY assignment*/
				break;
			}
		}
		sis++;
	}
	return(*sis != NULL);
}


static void acsm_enter_terminating_suspected (struct amf_sg *sg)
{
	struct amf_su **sus= sg->recovery_scope.sus;
	ENTER("%s",sg->name.value);
	sg->avail_state = SG_AC_TerminatingSuspected;
	/* 
	* Terminate suspected SU(s)
	*/
	while (*sus != 0) {
		amf_su_terminate (*sus);
		sus++;
	}
}

static inline int su_presense_state_is_ored (amf_su_t *su, 
	SaAmfPresenceStateT state1,SaAmfPresenceStateT state2, 
	SaAmfPresenceStateT state3)
{
	return(su->saAmfSUPresenceState == state1 || su->saAmfSUPresenceState == 
		state2 || su->saAmfSUPresenceState ==  state3) ? 1 : 0;
}

static inline int su_presense_state_is_not (amf_su_t *su, 
	SaAmfPresenceStateT state1,SaAmfPresenceStateT state2, 
	SaAmfPresenceStateT state3)
{
	return(su->saAmfSUPresenceState != state1 && su->saAmfSUPresenceState != 
		state2 && su->saAmfSUPresenceState !=  state3) ? 1 : 0;
}


static void timer_function_dependent_si_deactivated2 (void *data)
{
	amf_sg_t *sg = (amf_sg_t *)data;

	ENTER ("");
	dependent_si_deactivated_cbfn2 (sg);
}


static struct amf_si *si_get_dependent (struct amf_si *si)
{
	struct amf_si *tmp_si = NULL;

	if (si->depends_on != NULL) {
		SaNameT res_arr[2];
		int is_match;

		if (si->depends_on->name.length < SA_MAX_NAME_LENGTH) {
			si->depends_on->name.value[si->depends_on->name.length] = '\0';  
		}

		is_match = sa_amf_grep ((char*)si->depends_on->name.value, 
			"safDepend=.*,safSi=(.*),safApp=.*", 
			2, res_arr);

		if (is_match) {
			tmp_si = amf_si_find (si->application, (char*)res_arr[1].value);
		} else {
			log_printf (LOG_LEVEL_ERROR, "distinguished name for "
				"amf_si_depedency failed\n");
			openais_exit_error (AIS_DONE_FATAL_ERR);
		}
	}

	return tmp_si;
}

static struct amf_si *amf_dependent_get_next (struct amf_si *si, 
	struct amf_si *si_iter)
{
	struct amf_si *tmp_si;
	struct amf_application *application;

	if (si_iter == NULL) {
		assert(amf_cluster != NULL);
		application = amf_cluster->application_head;
		assert(application != NULL);    
		tmp_si = application->si_head;
	} else {
		tmp_si = si_iter->next;
		if (tmp_si == NULL) {
			application = si->application->next;
			if (application == NULL) {
				goto out;
			}
		}
	}

	for (; tmp_si != NULL; tmp_si = tmp_si->next) {
		struct amf_si *depends_on_si = si_get_dependent (tmp_si);
		while (depends_on_si != NULL) {
			if (depends_on_si == si) {
				goto out;
			}
			depends_on_si = depends_on_si->next;
		}
	}

out:
	return tmp_si;
}

static void acsm_enter_deactivating_dependent_workload (amf_sg_t *sg)
{
	struct amf_si **sis= sg->recovery_scope.sis;
	struct amf_si_assignment *si_assignment;
	int callback_pending = 0;

	sg->avail_state = SG_AC_DeactivatingDependantWorkload;

	ENTER("'%s'",sg->name.value);
	/*                           
	 * For each SI in the recovery scope, find all active
	 * assignments and request them to be deactivated.
	 */
	while (*sis != NULL) {
		struct amf_si *dependent_si;
		struct amf_si *si = *sis;
		si_assignment = si->assigned_sis;
		dependent_si = amf_dependent_get_next (si, NULL);   

		while (dependent_si != NULL) {
			si_assignment = dependent_si->assigned_sis;

			while (si_assignment != NULL) {

				if (si_assignment->saAmfSISUHAState == 
					SA_AMF_HA_ACTIVE) {
					si_assignment->requested_ha_state = 
						SA_AMF_HA_QUIESCED;
					callback_pending = 1;
					amf_si_ha_state_assume (
						si_assignment, 
						dependent_si_deactivated_cbfn);
				}
				si_assignment = si_assignment->next;
			}
			dependent_si = amf_dependent_get_next (si, dependent_si);   
		}
		sis++;
	}

	if (callback_pending == 0) {
		static poll_timer_handle dependent_si_deactivated_handle;
		ENTER("");
		poll_timer_add (aisexec_poll_handle, 0, sg,
			timer_function_dependent_si_deactivated2, 
			&dependent_si_deactivated_handle);
	}
}
/**
 * Enter function for state SG_AC_ActivatingStandby. It activates
 * one STANDBY assignment for each SI in the recovery scope.
 * @param sg
 */
static void acsm_enter_activating_standby (struct amf_sg *sg)
{
	struct amf_si **sis= sg->recovery_scope.sis;
	struct amf_si_assignment *si_assignment;
	int is_no_standby_activated = 1;

	ENTER("'%s'",sg->name.value); 
	sg->avail_state = SG_AC_ActivatingStandby;

	/*                                                              
	 * For each SI in the recovery scope, find one standby
	 * SI assignment and activate it.
	 */
	while (*sis != NULL) {
		si_assignment = (*sis)->assigned_sis;
		while (si_assignment != NULL) {
			if (si_assignment->saAmfSISUHAState == 
				SA_AMF_HA_STANDBY) {
				si_assignment->requested_ha_state = 
					SA_AMF_HA_ACTIVE;
				amf_si_ha_state_assume (
					si_assignment, standby_su_activated_cbfn);
				is_no_standby_activated = 0;
				break;
			}
			si_assignment = si_assignment->next;
		}
		sis++;
	}

	if (is_no_standby_activated) {

		acsm_enter_assigning_standby_to_spare (sg);
	}
}

static void acsm_enter_repairing_su (struct amf_sg *sg)
{
	struct amf_su **sus= sg->recovery_scope.sus;
	int is_any_su_instantiated = 0;
	const int PERFORMS_INSTANTIATING = 1;

	ENTER("'%s'",sg->name.value);
	sg->avail_state = SG_AC_ReparingSu;
	/* 
	 * Instantiate SUs in current recovery scope until the configured
	 * preference is fulfiled.
	 */
	while (*sus != NULL) {
		if (su_instantiated_count ((*sus)->sg) <
			(*sus)->sg->saAmfSGNumPrefInserviceSUs) {
			struct amf_node *node = 
				amf_node_find(&((*sus)->saAmfSUHostedByNode));
			if (node == NULL) {
				log_printf (LOG_LEVEL_ERROR, 
					"Su to recover not hosted on any node\n");
				openais_exit_error (AIS_DONE_FATAL_ERR);
			}
			if (node->saAmfNodeOperState == SA_AMF_OPERATIONAL_ENABLED) {
				/* node is synchronized */
				
				if (amf_su_instantiate ((*sus)) == PERFORMS_INSTANTIATING) {
					is_any_su_instantiated = 1;
				}
			}
		}
		sus++;
	}

	if (is_any_su_instantiated == 0) {
		acsm_enter_idle (sg);
	}
}

static inline void remove_all_suspected_sus (amf_sg_t *sg)
{
	amf_su_t *su;
	ENTER("");
	for (su = sg->su_head; su != NULL; su =su->next) {

		amf_comp_t *component;

		for (component = su->comp_head; component != NULL; 
			component = component->next) {

				amf_comp_error_suspected_clear (component);
		}
	}
}
static int is_all_si_assigned (amf_sg_t *sg)
{
	struct amf_si_assignment *si_assignment;
	int si_assignment_cnt = 0;
	int confirmed_assignments = 0;
	amf_si_t *si;

	for (si = sg->application->si_head; si != NULL; si = si->next) {
		if (name_match (&si->saAmfSIProtectedbySG, &sg->name)) {

			for (si_assignment = si->assigned_sis;
				si_assignment != NULL;
				si_assignment = si_assignment->next) {

				si_assignment_cnt++;
				if (si_assignment->requested_ha_state ==
					si_assignment->saAmfSISUHAState) {

					confirmed_assignments++;
				}
			}
		}
	}
	return (confirmed_assignments == si_assignment_cnt);
}

/**
 * Inquire if SI is assigned to SU
 * @param si
 * @param su
 * 
 * @return int
 */
static int is_si_assigned_to_su (amf_si_t *si, amf_su_t *su)
{
	amf_si_assignment_t *si_assignment = 0;
	int si_assignment_assigned_to_su = 0;

	for (si_assignment = si->assigned_sis; si_assignment != NULL;
		si_assignment = si_assignment->next) {

		if (si_assignment->su == su) {
			si_assignment_assigned_to_su = 1;
			break;
		}
	}
	return si_assignment_assigned_to_su;
}

/**
 * Inquire if SU is a spare.
 * @param sg
 * @param su
 * 
 * @return int
 */
static int is_spare_su (amf_sg_t *sg, amf_su_t *su)
{
	amf_si_t *si;
	int spare_su = 1;
	for (si = sg->application->si_head; si != NULL; si = si->next) {
		if(name_match(&sg->name, &si->saAmfSIProtectedbySG)) {
			if (is_si_assigned_to_su (si, su)) {

				spare_su = 0;
				break;
			}
		}
	}
	return (spare_su && su->saAmfSUPresenceState == 
		SA_AMF_PRESENCE_INSTANTIATED);
}
/**
 * Inqure if it is any spare SUs covered by SG
 * @param sg
 * 
 * @return int
 */
static int is_spare_sus (amf_sg_t *sg)
{
	amf_su_t *su = NULL;
	int spare_sus = 0;

	for (su = sg->su_head; su != NULL; su = su->next) {

		if (is_spare_su(sg, su)) {
			spare_sus = 1;
			break;
		}
	}
	return spare_sus;
}

/**
 * Provide standby assignments for the spare SUs in SG
 * @param sg
 */
static void assume_standby_si_assignment_for_spare_sus (amf_sg_t *sg)
{
	ENTER("");

	assign_si (sg, 0);
}

/**
 * Enter the AssigningStandbyToSpare state.
 * @param sg
 */
static void acsm_enter_assigning_standby_to_spare (amf_sg_t *sg)
{
	ENTER("%s", sg->name.value);

	sg->avail_state = SG_AC_AssigningStandbyToSpare;
	if (is_spare_sus (sg)) {
		assume_standby_si_assignment_for_spare_sus (sg);
	} else {
		switch (sg->recovery_scope.event_type) {
			case SG_FAILOVER_NODE_EV:
				acsm_enter_idle (sg);
				break;
			case SG_FAILOVER_SU_EV:
				acsm_enter_repairing_su (sg);
				break;
			default:
				dprintf("event_type %d",sg->recovery_scope.event_type);
				assert (0);
				break;
		}
	}
}

/**
 * Checks if the si pointed out is already in the scope.
 * @param sg
 * @param si
 */
static int is_si_in_scope(struct amf_sg *sg, struct amf_si *si)
{
	struct amf_si **tmp_sis= sg->recovery_scope.sis;

	while (*tmp_sis != NULL) {
		if (*tmp_sis == si) {
			break;
		}
		tmp_sis++;
	}
	return(*tmp_sis == si);
}

/**
 * Adds the si pointed out to the scope.
 * @param sg
 * @param si
 */
static void add_si_to_scope ( struct amf_sg *sg, struct amf_si *si)
{
	int number_of_si = 2; /* It shall be at least two */
	struct amf_si **tmp_sis= sg->recovery_scope.sis;

	ENTER ("'%s'", si->name.value);

	while (*tmp_sis != NULL) {
		number_of_si++;
		tmp_sis++;
	}

	sg->recovery_scope.sis = (struct amf_si **)
	realloc((void *)sg->recovery_scope.sis, 
		sizeof (struct amf_si *)*number_of_si);
	assert (sg->recovery_scope.sis != NULL);

	tmp_sis= sg->recovery_scope.sis;
	while (*tmp_sis != NULL) {
		tmp_sis++;
	}

	*tmp_sis = si;
	*(++tmp_sis) = NULL;
}

/**
 * Adds the ssu pointed out to the scope.
 * @param sg
 * @param su
 */
static void add_su_to_scope (struct amf_sg *sg, struct amf_su *su)
{
	int number_of_su = 2; /* It shall be at least two */
	struct amf_su **tmp_sus= sg->recovery_scope.sus;

	ENTER ("'%s'", su->name.value);
	while (*tmp_sus != NULL) {
		number_of_su++;
		tmp_sus++;
	}
	sg->recovery_scope.sus = (struct amf_su **)
	realloc((void *)sg->recovery_scope.sus, 
		sizeof (struct amf_su *)*number_of_su);
	assert (sg->recovery_scope.sus != NULL);

	tmp_sus= sg->recovery_scope.sus;
	while (*tmp_sus != NULL) {
		tmp_sus++;
	}

	*tmp_sus = su;
	*(++tmp_sus) = NULL; 
}

/**
 * Set recovery scope for failover SU.
 * @param sg
 * @param su
 */

static void set_scope_for_failover_su (struct amf_sg *sg, struct amf_su *su)
{
	struct amf_si_assignment *si_assignment;
	struct amf_si **sis;
	struct amf_su **sus;
	SaNameT dn;

	sg->recovery_scope.event_type = SG_FAILOVER_SU_EV;
	sg->recovery_scope.node = NULL;
	sg->recovery_scope.comp = NULL;
	sg->recovery_scope.sus = (struct amf_su **)
	calloc (2, sizeof (struct amf_su *));
	sg->recovery_scope.sis = (struct amf_si **)
	calloc (1, sizeof (struct amf_si *));

	assert ((sg->recovery_scope.sus != NULL) &&
		(sg->recovery_scope.sis != NULL));
	sg->recovery_scope.sus[0] = su;

	amf_su_dn_make (sg->recovery_scope.sus[0], &dn);
	log_printf (
		LOG_NOTICE, "'%s' for %s recovery action started",
		sg_event_type_text[sg->recovery_scope.event_type],
		dn.value);

	si_assignment = amf_su_get_next_si_assignment (su, NULL);
	while (si_assignment != NULL) {
		if (is_si_in_scope(sg, si_assignment->si) == 0) {
			add_si_to_scope(sg,si_assignment->si );
		}
		si_assignment = amf_su_get_next_si_assignment (su, si_assignment);
	}

	sus = sg->recovery_scope.sus;
	dprintf("The following sus are within the scope:\n");
	while (*sus != NULL) {
		dprintf("%s\n", (*sus)->name.value);
		sus++;
	}
	sis= sg->recovery_scope.sis;
	dprintf("The following sis are within the scope:\n");
	while (*sis != NULL) {
		dprintf("%s\n", (*sis)->name.value);
		sis++;
	}
}

static void set_scope_for_failover_node (struct amf_sg *sg, struct amf_node *node)
{
	struct amf_si_assignment *si_assignment;
	struct amf_si **sis;
	struct amf_su **sus;
	struct amf_su *su;

	ENTER ("'%s'", node->name.value);
	sg->recovery_scope.event_type = SG_FAILOVER_NODE_EV;
	sg->recovery_scope.node = node;
	sg->recovery_scope.comp = NULL;
	sg->recovery_scope.sus = (struct amf_su **)
	calloc (1, sizeof (struct amf_su *));
	sg->recovery_scope.sis = (struct amf_si **)
	calloc (1, sizeof (struct amf_si *));

	log_printf (
		LOG_NOTICE, "'%s' for node %s recovery action started",
		sg_event_type_text[sg->recovery_scope.event_type],
		node->name.value);

	assert ((sg->recovery_scope.sus != NULL) &&
		(sg->recovery_scope.sis != NULL));
	for (su = sg->su_head; su != NULL; su = su->next) {
		if (name_match (&node->name, &su->saAmfSUHostedByNode)) {
			add_su_to_scope (sg, su);
		}
	}

	sus = sg->recovery_scope.sus;
	while (*sus != 0) {
		su  = *sus;
		si_assignment = amf_su_get_next_si_assignment (su, NULL);
		while (si_assignment != NULL) {
			if (is_si_in_scope(sg, si_assignment->si) == 0) {
				add_si_to_scope(sg, si_assignment->si );
			}
			si_assignment = amf_su_get_next_si_assignment (
				su, si_assignment);
		}
		sus++;
	}

	sus = sg->recovery_scope.sus;
	dprintf("The following sus are within the scope:\n");
	while (*sus != NULL) {
		dprintf("%s\n", (*sus)->name.value);
		sus++;
	}
	sis = sg->recovery_scope.sis;
	dprintf("The following sis are within the scope:\n");
	while (*sis != NULL) {
		dprintf("%s\n", (*sis)->name.value);
		sis++;
	}
}

static void delete_si_assignment (amf_si_assignment_t *si_assignment)
{
	amf_csi_t *csi;
	amf_si_assignment_t *si_assignment_tmp;
	amf_si_assignment_t **prev = &si_assignment->si->assigned_sis;    

	for (csi = si_assignment->si->csi_head; csi != NULL; csi = csi->next) {
		amf_csi_delete_assignments (csi, si_assignment->su);
	}

	for (si_assignment_tmp = si_assignment->si->assigned_sis;
		si_assignment_tmp != NULL; 
		si_assignment_tmp = si_assignment_tmp->next) {

		if (si_assignment_tmp == si_assignment) {

			amf_si_assignment_t *to_be_removed = si_assignment_tmp;
			*prev = si_assignment_tmp->next;
			dprintf ("SI assignment %s unlinked", 
				to_be_removed->name.value);
			free (to_be_removed);
		} else {
			prev = &si_assignment_tmp->next;
		}
	}
}

/**
 * Delete all SI assignments and all CSI assignments
 * by requesting all contained components.
 * @param su
 */
static void delete_si_assignments (struct amf_su *su)
{
	struct amf_csi *csi;
	struct amf_si *si;
	struct amf_si_assignment *si_assignment;
	struct amf_si_assignment **prev;

	ENTER ("'%s'", su->name.value);

	for (si = su->sg->application->si_head; si != NULL; si = si->next) {

		prev = &si->assigned_sis;

		if (!name_match (&si->saAmfSIProtectedbySG, &su->sg->name)) {
			continue;
		}

		for (csi = si->csi_head; csi != NULL; csi = csi->next) {
			amf_csi_delete_assignments (csi, su);
		}

		for (si_assignment = si->assigned_sis; si_assignment != NULL;
			si_assignment = si_assignment->next) {
			if (si_assignment->su == su) {
				struct amf_si_assignment *tmp = si_assignment;
				*prev = si_assignment->next;
				dprintf ("SI assignment %s unlinked", tmp->name.value);
				free (tmp);
			} else {
				prev = &si_assignment->next;
			}
		}
	}
}

/**
 * Delete all SI assignments and all CSI assignments in current
 * recovery scope.
 * @param sg
 */
static void delete_si_assignments_in_scope (struct amf_sg *sg) 
{
	struct amf_su **sus= sg->recovery_scope.sus;

	while (*sus != NULL) {
		delete_si_assignments (*sus);
		sus++;
	}
}

/**
 * Given an SI, find and return the SU assigned as standby
 * @param si
 * 
 * @return amf_su_t*
 */
static amf_su_t *find_standby_su (amf_si_t *si)
{
	amf_si_assignment_t *si_assignment;
	amf_su_t *standby_su = NULL;

	si_assignment = si->assigned_sis;
	while (si_assignment != NULL) {
		if (si_assignment->saAmfSISUHAState == SA_AMF_HA_STANDBY) {
			standby_su = si_assignment->su;
			break;
		}
		si_assignment = si_assignment->next;
	}

	return standby_su;
}

static int no_si_assignment_is_requested_to_be_removed (amf_sg_t *sg)
{
	amf_si_t *si;
	int no_to_be_removed = 1;

	for (si = sg->application->si_head; si != NULL; si = si->next) {

		if (name_match (&si->saAmfSIProtectedbySG, &sg->name)) {
			amf_si_assignment_t *si_assignment = 0;
			for (si_assignment = si->assigned_sis; si_assignment != NULL;
				si_assignment = si_assignment->next) {
				if (si_assignment->requested_ha_state == 
					USR_AMF_HA_STATE_REMOVED) {

					no_to_be_removed = 0;
					goto out;
				}
			}
		}
	}
out:
	return no_to_be_removed;
}

static void removed_si_assignment_callback_fn (void *si_assignment_in)
{
	amf_si_assignment_t *si_assignment = si_assignment_in;

	ENTER("");
	delete_si_assignment (si_assignment);

	/*                                                              
	 * if all si assignments are remove then change state
	 */
	if (no_si_assignment_is_requested_to_be_removed (si_assignment->su->sg)) {
		acsm_enter_activating_standby (si_assignment->su->sg);
	}
}

/**
 * 
 * @param sg
 * 
 * @return int, number of removed SI assignments
 */
static int remove_standby_si_assignments (amf_sg_t *sg)
{
	struct amf_si **sis = sg->recovery_scope.sis;
	struct amf_si_assignment *si_assignment;
	amf_su_t *standby_su;
	int removed = 0;

	ENTER("'%s'", sg->name.value); 

	/*                                                              
	 * For each SI in the recovery scope, find a standby
     * SU, then remove all 'standby SI assignment' not in
	 * the recovery scope.
	 */
	while (*sis != NULL) {
		standby_su = find_standby_su (*sis);
		if (standby_su != NULL) {
			si_assignment = amf_su_get_next_si_assignment (standby_su, NULL);
			while (si_assignment != NULL) {

				amf_si_t **sia;
                int in_recovery_scope;	

				for (sia = sg->recovery_scope.sis, in_recovery_scope = 0; 
					*sia != NULL; sia++) {
					if (name_match (&si_assignment->si->name, &(*sia)->name)) {
						in_recovery_scope = 1;
					}
				}

				/*
                 * The si_assignment found with standby hastate is not in the
                 * recovery scope. The found si_assignment will then be
                 * requested to be removed once.
                 */
				if (!in_recovery_scope && 
					si_assignment->requested_ha_state != 
					USR_AMF_HA_STATE_REMOVED) {

					amf_si_assignment_remove (si_assignment,
							removed_si_assignment_callback_fn);
					removed++;
				}
				si_assignment = amf_su_get_next_si_assignment (standby_su,
					si_assignment);
			}
		}
		sis++;
	}

	return removed;
}

/**
 * Entry function for state 'removing standby assignments'
 * @param sg
 */
static void acsm_enter_removing_standby_assignments (amf_sg_t *sg)
{
	ENTER("SG: %s", sg->name.value);
	sg->avail_state = SG_AC_RemovingStandbyAssignments;

	if (sg->saAmfSGRedundancyModel == SA_AMF_NPM_REDUNDANCY_MODEL) {
		if (!remove_standby_si_assignments (sg)) {
			acsm_enter_activating_standby (sg);
		}
	}
}

static inline int div_round (int a, int b)
{
	int res;

	assert (b != 0);
	res = a / b;
	if ((a % b) != 0)
		res++;
	return res;
}

static int no_su_has_presence_state (
	struct amf_sg *sg, struct amf_node *node_to_start, 
	SaAmfPresenceStateT state)
{
	struct amf_su *su;
	int no_su_has_presence_state = 1;

	for (su = sg->su_head; su != NULL; su = su->next) {

		if (su->saAmfSUPresenceState == state) {
			if (node_to_start == NULL) {
				no_su_has_presence_state = 0;
				break;
			} else {
				if (name_match(&node_to_start->name,
					&su->saAmfSUHostedByNode)) {
					no_su_has_presence_state = 0;
					break;
				}
			}
		}
	}

	return no_su_has_presence_state;
}

#if COMPILE_OUT
static int all_su_in_scope_has_presence_state (
	struct amf_sg *sg, SaAmfPresenceStateT state)
{
	struct amf_su **sus= sg->recovery_scope.sus;

	while (*sus != NULL) {
		if ((*sus)->saAmfSUPresenceState != state) {
			break;
		}
		sus++;
	}
	return(*sus == NULL);
}
#endif
static int all_su_in_scope_has_either_two_presence_state (
	amf_sg_t *sg, 
	SaAmfPresenceStateT state1, 
	SaAmfPresenceStateT state2)
{
	struct amf_su **sus = sg->recovery_scope.sus;

	while (*sus != NULL) {
		if (!((*sus)->saAmfSUPresenceState == state1 || 
			(*sus)->saAmfSUPresenceState == state2)) {
			break;
		} 
		sus++;
	}
	return (*sus == NULL);
}


static int all_su_in_scope_has_either_of_three_presence_state (amf_sg_t *sg, 
	SaAmfPresenceStateT state1, SaAmfPresenceStateT state2, 
	SaAmfPresenceStateT state3)
{
	struct amf_su **sus = sg->recovery_scope.sus;

	while (*sus != NULL) {
		if (!((*sus)->saAmfSUPresenceState ==  state1  || 
			(*sus)->saAmfSUPresenceState   ==  state2  || 
			(*sus)->saAmfSUPresenceState   ==  state3)) {
			break;
		} 
		sus++;
	}
	return (*sus == NULL);
}



/**
 * Get number of SIs protected by the specified SG.
 * @param sg
 * 
 * @return int
 */
static int sg_si_count_get (struct amf_sg *sg)
{
	struct amf_si *si;
	int cnt = 0;

	for (si = sg->application->si_head; si != NULL; si = si->next) {
		if (name_match (&si->saAmfSIProtectedbySG, &sg->name)) {
			cnt += 1;
		}
	}
	return(cnt);
}

static int amf_si_get_saAmfSINumReqActiveAssignments(struct amf_si *si) 
{
	struct amf_si_assignment *si_assignment = si->assigned_sis;
	int number_of_req_active_assignments = 0;

	for (; si_assignment != NULL; si_assignment = si_assignment->next) {

		if (si_assignment->requested_ha_state == SA_AMF_HA_ACTIVE) {
			number_of_req_active_assignments++;
		}
	}
	return number_of_req_active_assignments;
}

static int amf_si_get_saAmfSINumReqStandbyAssignments(struct amf_si *si) 
{
	struct amf_si_assignment *si_assignment = si->assigned_sis;
	int number_of_req_active_assignments = 0;

	for (; si_assignment != NULL; si_assignment = si_assignment->next) {
		if (si_assignment->requested_ha_state == SA_AMF_HA_STANDBY) {
			number_of_req_active_assignments++;
		}
	}
	return number_of_req_active_assignments;
}

static int sg_assign_active_nplusm (struct amf_sg *sg, int su_active_assign)
{
	struct amf_su *su;
	struct amf_si *si;
	int assigned = 0;
	int assign_to_su = 0;
	int total_assigned = 0;
	int si_left;
	int si_total;
	int su_left_to_assign = su_active_assign;
	ENTER("SG: %s", sg->name.value);

	si_total = sg_si_count_get (sg);
	si_left = si_total;
	assign_to_su = div_round (si_left, su_active_assign);
	if (assign_to_su > sg->saAmfSGMaxActiveSIsperSUs) {
		assign_to_su = sg->saAmfSGMaxActiveSIsperSUs;
	}

	su = sg->su_head;
	while (su != NULL && su_left_to_assign > 0) {
		if (amf_su_get_saAmfSUReadinessState (su) !=
			SA_AMF_READINESS_IN_SERVICE ||
			amf_su_get_saAmfSUNumCurrActiveSIs (su) ==  assign_to_su ||
			amf_su_get_saAmfSUNumCurrStandbySIs (su) > 0) {

			su = su->next;
			continue; /* Not in service */
		}

		si = sg->application->si_head;
		assigned = 0;
		assign_to_su = div_round (si_left, su_left_to_assign);
		if (assign_to_su > sg->saAmfSGMaxActiveSIsperSUs) {
			assign_to_su = sg->saAmfSGMaxActiveSIsperSUs;
		}
		while (si != NULL) {
			if (name_match (&si->saAmfSIProtectedbySG, &sg->name) &&
				assigned < assign_to_su && 
				amf_si_get_saAmfSINumReqActiveAssignments(si) == 0) {
				assigned += 1;
				total_assigned += 1;
				amf_su_assign_si (su, si, SA_AMF_HA_ACTIVE);
			}

			si = si->next;
		}
		su = su->next;
		su_left_to_assign -= 1;
		si_left -= assigned;
		dprintf (" su_left_to_assign =%d, si_left=%d\n",
			su_left_to_assign, si_left);
	}

	assert (total_assigned <= si_total);
	if (total_assigned == 0) {
		dprintf ("Info: No SIs assigned");
	}

	return total_assigned;
}

static int sg_assign_standby_nplusm (struct amf_sg *sg, int su_standby_assign)
{
	struct amf_su *su;
	struct amf_si *si;
	int assigned = 0;
	int assign_to_su = 0;
	int total_assigned = 0;
	int si_left;
	int si_total;
	int su_left_to_assign = su_standby_assign;

	ENTER ("'%s'", sg->name.value);

	if (su_standby_assign == 0) {
		return 0;
	}
	si_total = sg_si_count_get (sg);
	si_left = si_total;
	assign_to_su = div_round (si_left, su_standby_assign);
	if (assign_to_su > sg->saAmfSGMaxStandbySIsperSUs) {
		assign_to_su = sg->saAmfSGMaxStandbySIsperSUs;
	}

	su = sg->su_head;
	while (su != NULL && su_left_to_assign > 0) {
		if (amf_su_get_saAmfSUReadinessState (su) !=
			SA_AMF_READINESS_IN_SERVICE ||
			amf_su_get_saAmfSUNumCurrActiveSIs (su) > 0 ||
			amf_su_get_saAmfSUNumCurrStandbySIs (su) ==
			assign_to_su) {

			su = su->next;
			continue; /* Not available for assignment */
		}

		si = sg->application->si_head;
		assigned = 0;
		assign_to_su = div_round (si_left, su_left_to_assign);
		if (assign_to_su > sg->saAmfSGMaxStandbySIsperSUs) {
			assign_to_su = sg->saAmfSGMaxStandbySIsperSUs;
		}
		while (si != NULL) {
			if (name_match (&si->saAmfSIProtectedbySG, &sg->name) &&
				assigned < assign_to_su && 
				amf_si_get_saAmfSINumReqStandbyAssignments (si) == 0) {
				assigned += 1;
				total_assigned += 1;
				amf_su_assign_si (su, si, SA_AMF_HA_STANDBY);
			}
			si = si->next;
		}
		su_left_to_assign -= 1;
		si_left -= assigned;
		dprintf (" su_left_to_assign =%d, si_left=%d\n",
			su_left_to_assign, si_left);

		su = su->next;
	}

	assert (total_assigned <= si_total);
	if (total_assigned == 0) {
		dprintf ("Info: No SIs assigned!");
	}

	return total_assigned;
}

static int su_inservice_count_get (struct amf_sg *sg)
{
	struct amf_su *su;
	int answer = 0;

	for (su = sg->su_head; su != NULL; su = su->next) {
		if (amf_su_get_saAmfSUReadinessState (su) ==
			SA_AMF_READINESS_IN_SERVICE) {

			answer += 1;
		}
	}
	return(answer);
}

static int su_active_out_of_service_count_get (amf_sg_t *sg)
{
	int active_out_of_service_count = 0;
	amf_su_t *su;
	for (su = sg->su_head; su != NULL; su = su->next) {
		amf_si_assignment_t *si_assignment;
		si_assignment = amf_su_get_next_si_assignment (su, NULL);
		while (si_assignment != NULL) {
			if ((si_assignment->saAmfSISUHAState == SA_AMF_HA_ACTIVE) &&
				(amf_su_get_saAmfSUReadinessState (su) == 
				SA_AMF_READINESS_OUT_OF_SERVICE)) {
				active_out_of_service_count += 1;
			}
			si_assignment = amf_su_get_next_si_assignment (su, si_assignment);
		}
	}
	return active_out_of_service_count;
}


static int su_standby_out_of_service_count_get (amf_sg_t *sg)
{
	int active_out_of_service_count = 0;
	amf_su_t *su;
	for (su = sg->su_head; su != NULL; su = su->next) {
		amf_si_assignment_t *si_assignment;
		si_assignment = amf_su_get_next_si_assignment (su, NULL);
		while (si_assignment != NULL) {
			if ((si_assignment->saAmfSISUHAState == SA_AMF_HA_STANDBY) &&
				(amf_su_get_saAmfSUReadinessState (su) == 
				SA_AMF_READINESS_OUT_OF_SERVICE)) {
				active_out_of_service_count += 1;
			}
			si_assignment = amf_su_get_next_si_assignment (su, si_assignment);
		}
	}
	return active_out_of_service_count;
}

/**
 * This function calculates the number of active and standby assignments that
 * shall be done according to what is configured and the number of in-service
 * SUs available. This function leaves possible existing assignments as they are
 * but possibly adds new assignments. This function also initiates the transfer
 * of the calculated assignments to the SUs that shall execute them.
 * 
 * TODO: dependency_level not used, hard coded
 * @param sg
 * @param dependency_level
 * @return - the sum of assignments initiated
 */
static int assign_si (struct amf_sg *sg, int dependency_level)
{
	int active_sus_needed = 0;
	int standby_sus_needed = 0;
	int inservice_count;
	int su_active_assign;
	int su_standby_assign;
	int su_spare_assign;
	int assigned = 0;
	int active_out_of_service = 0;
	int standby_out_of_service = 0;
	ENTER ("'%s'", sg->name.value);

	/**
	 * Phase 1: Calculate assignments and create all runtime objects in
	 * information model. Do not do the actual assignment, done in
	 * phase 2.
	 */

	/**
	 * Calculate number of SUs to assign to active or standby state
	 */
	inservice_count = su_inservice_count_get (sg);
	active_out_of_service = su_active_out_of_service_count_get(sg);
	standby_out_of_service = su_standby_out_of_service_count_get(sg);
	if (sg->saAmfSGNumPrefActiveSUs > 0) {

		active_sus_needed = div_round (
			sg_si_count_get (sg),
			sg->saAmfSGMaxActiveSIsperSUs);
	} else {
		log_printf (LOG_LEVEL_ERROR, "ERROR: saAmfSGNumPrefActiveSUs == 0 !!");
		openais_exit_error (AIS_DONE_FATAL_ERR);
	}

	if (sg->saAmfSGNumPrefStandbySUs > 0) {
		standby_sus_needed = div_round (
			sg_si_count_get (sg),
			sg->saAmfSGMaxStandbySIsperSUs);
	} else {
		log_printf (LOG_LEVEL_ERROR, "ERROR: saAmfSGNumPrefStandbySUs == 0 !!");
		openais_exit_error (AIS_DONE_FATAL_ERR);

	}

	dprintf ("(inservice=%d) (active_sus_needed=%d) (standby_sus_needed=%d)"
		"\n",
		inservice_count, active_sus_needed, standby_sus_needed);

	/* Determine number of active and standby service units
	 * to assign based upon reduction procedure
	 */
	if ((inservice_count < active_sus_needed - active_out_of_service)) {
		dprintf ("assignment VI - partial assignment with SIs drop outs\n");

		su_active_assign = inservice_count;
		su_standby_assign = 0;
		su_spare_assign = 0;
	} else
		if ((inservice_count < active_sus_needed - active_out_of_service + 
			 standby_sus_needed)) {
		dprintf ("assignment V - partial assignment with reduction of"
			" standby units\n");

		su_active_assign = active_sus_needed;
		su_standby_assign = inservice_count - active_sus_needed - active_out_of_service;
		su_spare_assign = 0;
	} else
		if ((inservice_count < sg->saAmfSGNumPrefActiveSUs + standby_sus_needed)) {
		dprintf ("IV: full assignment with reduction of active service"
			" units\n");
		su_active_assign = inservice_count - standby_sus_needed;
		su_standby_assign = standby_sus_needed;
		su_spare_assign = 0;
	} else
		if ((inservice_count < 
		sg->saAmfSGNumPrefActiveSUs + sg->saAmfSGNumPrefStandbySUs)) {
		dprintf ("III: full assignment with reduction of standby service"
			" units\n");
		su_active_assign = sg->saAmfSGNumPrefActiveSUs;
		su_standby_assign = inservice_count - sg->saAmfSGNumPrefActiveSUs;
		su_spare_assign = 0;
	} else
		if ((inservice_count == 
		sg->saAmfSGNumPrefActiveSUs + sg->saAmfSGNumPrefStandbySUs)) {
		if (sg->saAmfSGNumPrefInserviceSUs > inservice_count) {
			dprintf ("II: full assignment with spare reduction\n");
		} else {
			dprintf ("II: full assignment without spares\n");
		}

		su_active_assign = sg->saAmfSGNumPrefActiveSUs;
		su_standby_assign = sg->saAmfSGNumPrefStandbySUs;
		su_spare_assign = 0;
	} else {
		dprintf ("I: full assignment with spares\n");
		su_active_assign = sg->saAmfSGNumPrefActiveSUs;
		su_standby_assign = sg->saAmfSGNumPrefStandbySUs;
		su_spare_assign = inservice_count - 
			sg->saAmfSGNumPrefActiveSUs - sg->saAmfSGNumPrefStandbySUs;
	}

	dprintf ("(inservice=%d) (assigning active=%d) (assigning standby=%d)"
		" (assigning spares=%d)\n",
		inservice_count, su_active_assign, su_standby_assign, su_spare_assign);

	if (inservice_count > 0) {
		assigned = sg_assign_active_nplusm (sg, su_active_assign);
		assigned += sg_assign_standby_nplusm (sg, su_standby_assign);
		sg->saAmfSGNumCurrAssignedSUs = inservice_count;

		/**
		 * Phase 2: do the actual assignment to the component
		 * TODO: first do active, then standby
		 */
		{
			struct amf_si *si;
			struct amf_si_assignment *si_assignment;

			for (si = sg->application->si_head; si != NULL; si = si->next) {
				if (name_match (&si->saAmfSIProtectedbySG, &sg->name)) {
					for (si_assignment = si->assigned_sis; 
						si_assignment != NULL;
						si_assignment = si_assignment->next) {

						if (si_assignment->requested_ha_state !=
							si_assignment->saAmfSISUHAState) {
							amf_si_ha_state_assume (
								si_assignment, assign_si_assumed_cbfn);
						}
					}
				}
			}
		}
	}

	LEAVE ("'%s'", sg->name.value);
	return assigned;
}

#ifdef COMPILE_OUT
static void remove_si_in_scope (amf_sg_t *sg, amf_si_t *si)
{
	int i; 
	int j;
	amf_si_t **sis = sg->recovery_scope.sis;
	amf_si_t **new_sis = amf_calloc (1, sizeof (amf_si_t*));

	for (i = 0,j = 0; sis[i] != NULL; i++) {
		if (sis[i] == si) {
			continue;
		}
		new_sis[j] = sis[i];
		new_sis = amf_realloc (new_sis, j + sizeof (amf_si_t *));
		j++;
	}

	sg->recovery_scope.sis = new_sis;
}
#endif

#ifdef COMPILE_OUT
static void remove_sis_for_term_failed_su_from_scope (amf_sg_t *sg, 
	amf_su_t *su)
{
	amf_comp_t *component;

	/*
	 * foreach component with presense state termiantion failed in su
	 */
	
	for (component = su->comp_head; component != NULL; 
		component = component->next) {

		amf_csi_assignment_t *csi_assignment;

		if (component->saAmfCompPresenceState != 
			SA_AMF_PRESENCE_INSTANTIATION_FAILED) {
			continue;
		}

		csi_assignment = amf_comp_get_next_csi_assignment (component, NULL);

		while (csi_assignment != NULL) {

			remove_si_in_scope (sg, csi_assignment->csi->si);
			csi_assignment = amf_comp_get_next_csi_assignment (component, NULL);
		}
	}
}
#endif

/**
 * This function returns 1 if the redundancy model is N plus M and at least one
 * component of the specified SU has an active HA-state, else the function
 * returns 0.
 * @param sg
 * @param su
 * @return int
 */
static int is_comp_in_active_ha_state_nplusm (
	amf_sg_t *sg, amf_su_t *su)
{
	amf_comp_t *component;
	amf_csi_assignment_t *csi_assignment;
	int comp_is_in_active_ha_state = 0;

	if(sg->saAmfSGRedundancyModel == SA_AMF_NPM_REDUNDANCY_MODEL) {
		for (component = su->comp_head; component != NULL; 
			  component = component->next) {
			csi_assignment = amf_comp_get_next_csi_assignment(component, NULL);
			while (csi_assignment != NULL) {
				if (csi_assignment->saAmfCSICompHAState == SA_AMF_HA_ACTIVE) {
					comp_is_in_active_ha_state = 1;
					goto out;
				}
				csi_assignment = amf_comp_get_next_csi_assignment(component, 
					csi_assignment);
			}
		}
	}
out:
	return comp_is_in_active_ha_state;
}

/**
 * This function handles a change of presence state reported by an SU contained
 * in specified SG. The new presence state is INSTANTIATED.
 * @param sg
 * @param su
 */
static void sg_su_state_changed_to_instantiated (struct amf_sg *sg, struct amf_su *su)
{
	ENTER("%s %s",sg->name.value, su->name.value);
	switch (sg->avail_state) {
		case SG_AC_InstantiatingServiceUnits:
			if (no_su_has_presence_state(sg, sg->node_to_start, 
				SA_AMF_PRESENCE_INSTANTIATING)) {
				acsm_enter_idle (sg);
			}
			break;
		case SG_AC_ReparingSu:
			if (no_su_has_presence_state(sg, sg->node_to_start, 
				SA_AMF_PRESENCE_INSTANTIATING)) {
				if (all_su_in_scope_has_either_of_three_presence_state(
					su->sg,
					SA_AMF_PRESENCE_INSTANTIATED,
					SA_AMF_PRESENCE_INSTANTIATION_FAILED,
					SA_AMF_PRESENCE_UNINSTANTIATED)) {
					su->sg->avail_state = SG_AC_AssigningWorkload;
					if (assign_si (sg, 0) == 0) {
						acsm_enter_idle (sg);
					}
				} else {
					dprintf ("avail-state: %u", sg->avail_state);
					assert (0);
				}
			}
			break;
		default:
			dprintf ("avail-state: %u", sg->avail_state);
			assert (0);
			break;
	}
}

/**
 * This function handles a change of presence state reported by an SU contained
 * in specified SG. The new presence state is UNINSTANTIATED.
 * @param sg
 * @param su
 */
static void amf_sg_su_state_changed_to_uninstantiated (amf_sg_t *sg, 
	amf_su_t *su)
{
	ENTER("%s %s",sg->name.value, su->name.value);
	switch (sg->avail_state) {
		case SG_AC_TerminatingSuspected:
			if (no_su_has_presence_state(sg, sg->node_to_start, 
				SA_AMF_PRESENCE_TERMINATING)) {
				if (all_su_in_scope_has_either_two_presence_state (sg,
					SA_AMF_PRESENCE_UNINSTANTIATED,
					SA_AMF_PRESENCE_TERMINATION_FAILED)) {
					
					delete_si_assignments_in_scope (sg);
					
					if (is_any_si_in_scope_assigned_standby (sg)) {
						remove_all_suspected_sus (sg);
						acsm_enter_removing_standby_assignments (sg);
					} else { /*is_no_si_in_scope_assigned_standby*/
						remove_all_suspected_sus (sg);
						acsm_enter_assigning_standby_to_spare (sg);
					}
				}
			}
			break;
		case SG_AC_ReparingSu:
			if (no_su_has_presence_state(sg, sg->node_to_start, 
				SA_AMF_PRESENCE_TERMINATING)) {
				if (all_su_in_scope_has_either_of_three_presence_state(
					su->sg,
					SA_AMF_PRESENCE_INSTANTIATED,
					SA_AMF_PRESENCE_INSTANTIATION_FAILED,
					SA_AMF_PRESENCE_UNINSTANTIATED)) {
					su->sg->avail_state = SG_AC_AssigningWorkload;
					if (assign_si (sg, 0) == 0) {
						acsm_enter_idle (sg);
					}
				}
			}
			break;
		default:
			log_printf (LOG_ERR, "sg avail_state = %d", sg->avail_state);
			assert (0);
			break;
	}
}

/**
 * This function handles a change of presence state reported by an SU contained
 * in specified SG. The new presence state is TERMINATION_FAILED.
 * @param sg
 * @param su
 */
static void amf_sg_su_state_changed_to_termination_failed (amf_sg_t *sg,
	amf_su_t *su)
{
	ENTER("%s %s",sg->name.value, su->name.value);
	if (no_su_has_presence_state(sg, sg->node_to_start, 
		SA_AMF_PRESENCE_TERMINATING)) {
		if (is_comp_in_active_ha_state_nplusm (sg, su)) {
			acsm_enter_idle (sg);
			goto out;
		}

		if (all_su_in_scope_has_either_two_presence_state (sg,
			SA_AMF_PRESENCE_UNINSTANTIATED,
			SA_AMF_PRESENCE_TERMINATION_FAILED)) {
			
			delete_si_assignments_in_scope (sg);
			
			if (is_any_si_in_scope_assigned_standby (sg)) {
				remove_all_suspected_sus (sg);
				acsm_enter_removing_standby_assignments (sg);
			} else { /*is_no_si_in_scope_assigned_standby*/
				remove_all_suspected_sus (sg);
				acsm_enter_assigning_standby_to_spare (sg);
			}
		}
	}
out:
	return;
}

/**
 * This function handles a change of presence state reported by an SU contained
 * in specified SG. The new presence state is INSTANTIATION_FAILED.
 * @param sg
 * @param su
 */
static void amf_sg_su_state_changed_to_instantiation_failed (amf_sg_t *sg,  
	amf_su_t *su)
{
	ENTER("%s %s",sg->name.value, su->name.value);
	switch (sg->avail_state) {
		case SG_AC_InstantiatingServiceUnits:
			if (no_su_has_presence_state(sg, sg->node_to_start, 
				SA_AMF_PRESENCE_INSTANTIATING)) {
				acsm_enter_idle (sg);
			}
			break;
		case SG_AC_ReparingSu:
			if (no_su_has_presence_state(sg, sg->node_to_start, 
				SA_AMF_PRESENCE_INSTANTIATING)) {
				if (all_su_in_scope_has_either_of_three_presence_state(
					su->sg,
					SA_AMF_PRESENCE_INSTANTIATED,
					SA_AMF_PRESENCE_INSTANTIATION_FAILED,
					SA_AMF_PRESENCE_UNINSTANTIATED)) {
					su->sg->avail_state = SG_AC_AssigningWorkload;
					if (assign_si (sg, 0) == 0) {
						acsm_enter_idle (sg);
					}
				}
			}
			break;
		default:
			/* TODO: Insert the assert (0) until solving defers in SU   */
			dprintf("sg->avail_state = %d", sg->avail_state);
			break;
	}
}

/******************************************************************************
 * Event methods
 *****************************************************************************/

/**
 * This function starts all SUs in the SG or all SUs on a specified node.
 * @param sg
 * @param node - Node on which all SUs shall be started or
 *               NULL indicating that all SUs on all nodes shall be started.
 * @return - No of SUs that has been attempted to start
 */
int amf_sg_start (struct amf_sg *sg, struct amf_node *node)
{

	sg->recovery_scope.event_type = SG_START_EV;
	ENTER ("'%s'", sg->name.value);
	int instantiated_sus = 0;

	switch (sg->avail_state) {
		case SG_AC_InstantiatingServiceUnits:
		case SG_AC_Idle: { 

				amf_su_t *su;
				sg_avail_control_state_t old_avail_state = sg->avail_state;

				ENTER ("'%s'", sg->name.value);

				sg->node_to_start = node;

				sg->avail_state = SG_AC_InstantiatingServiceUnits;

				for (su = sg->su_head;
					(su != NULL) && 
					(instantiated_sus < sg->saAmfSGNumPrefInserviceSUs);
					su = su->next) {

					if (is_cluster_start (node)) {

						amf_su_instantiate (su);
						instantiated_sus++;

					} else { /*is_not_cluster_start*/

						/*
						 * Node start, match if SU is hosted on the
						 * specified node
						 */

						if (name_match (&node->name, 
							&su->saAmfSUHostedByNode)) {
							amf_su_instantiate (su);
							instantiated_sus++; 
						}
					}
				}

				if (instantiated_sus == 0) {
					sg->avail_state = old_avail_state;
				}
				break;
			}
		case SG_AC_DeactivatingDependantWorkload:
		case SG_AC_TerminatingSuspected:
		case SG_AC_ActivatingStandby:
		case SG_AC_AssigningStandbyToSpare:
		case SG_AC_ReparingComponent:
		case SG_AC_ReparingSu:
		case SG_AC_AssigningOnRequest:
		case SG_AC_RemovingAssignment:
		case SG_AC_AssigningActiveworkload:
		case SG_AC_AssigningAutoAdjust:
		case SG_AC_AssigningWorkload:
		case SG_AC_WaitingAfterOperationFailed:
		case SG_AC_RemovingStandbyAssignments:
		default:
			assert (0);
			break;
	}
	return instantiated_sus;
}

/**
 * This function initiates assignment of the subset of the workload which
 * matches the specified workload dependency level, to all SUs contained in the
 * SG according to the requirements specified in the configuration.
 * @param sg -
 * @param dependency_level - Dependency level to assign
 * @return - No of SUs that has been attempted to start
 */
int amf_sg_assign_si_req (struct amf_sg *sg, int dependency_level)
{
/* TODO: Introduce state control in this function 
 */
	int posible_to_assign_si;
	sg->recovery_scope.event_type = SG_ASSIGN_SI_EV;
	sg->avail_state = SG_AC_AssigningOnRequest;

	if ((posible_to_assign_si = assign_si (sg, dependency_level)) == 0) {
		acsm_enter_idle (sg);
	}
	return posible_to_assign_si;
}

/**
 * This function is called because an error has been detected and the analysis
 * (done elsewhere) indicated that this error shall be recovered by a Node
 * failover. This function initiates the recovery action 'Node failover'.
 * @param sg
 * @param su - SU to failover
 * @param node -
 */
void amf_sg_failover_node_req (struct amf_sg *sg, struct amf_node *node) 
{
	ENTER("'%s, %s'",node->name.value, sg->name.value);
	sg_event_t sg_event;

	switch (sg->avail_state) {
		case SG_AC_Idle:
			set_scope_for_failover_node(sg, node);
			if (has_any_su_in_scope_active_workload (sg)) {
				acsm_enter_deactivating_dependent_workload (sg);
			} else {
				amf_su_t **sus = sg->recovery_scope.sus;

				/*
				 * Select next state depending on if some
				 * SU in the scope needs to be terminated.
				 */
				while (*sus != NULL) {

					amf_su_t *su = *sus;
					ENTER("SU %s pr_state='%d'",su->name.value,
						su->saAmfSUPresenceState);

					if (su_presense_state_is_ored (su,
						SA_AMF_PRESENCE_UNINSTANTIATED,
						SA_AMF_PRESENCE_TERMINATION_FAILED,
						SA_AMF_PRESENCE_INSTANTIATION_FAILED)) {
						sus++;
						continue;
					}
					break;
				}

				if (*sus != NULL) {
					acsm_enter_terminating_suspected (sg);
				} else {
					delete_si_assignments_in_scope (sg);
					acsm_enter_idle (sg);         
				}

			} 
			break;
		case SG_AC_DeactivatingDependantWorkload:
		case SG_AC_TerminatingSuspected:
		case SG_AC_ActivatingStandby:
		case SG_AC_AssigningStandbyToSpare:
		case SG_AC_ReparingComponent:
		case SG_AC_ReparingSu:
		case SG_AC_AssigningOnRequest:
		case SG_AC_InstantiatingServiceUnits:
		case SG_AC_RemovingAssignment:
		case SG_AC_AssigningActiveworkload:
		case SG_AC_AssigningAutoAdjust:
		case SG_AC_AssigningWorkload:
		case SG_AC_WaitingAfterOperationFailed:
		case SG_AC_RemovingStandbyAssignments:
			sg_set_event (SG_FAILOVER_NODE_EV, sg, 0, 0, node, &sg_event); 
			sg_defer_event (SG_FAILOVER_NODE_EV, &sg_event); 
			break;
		default:
			assert (0);
			break;

	}
}

/**
 * This function is called because an error has been detected and the analysis
 * (done elsewhere) indicated that this error shall be recovered by an SU
 * failover. This function initiates the recovery action 'SU failover'.
 * @param sg
 * @param su - SU to failover
 * @param node -
 */
void amf_sg_failover_su_req (struct amf_sg *sg, struct amf_su *su, 
	struct amf_node *node)
{
	ENTER ("%s", su->name.value);
	sg_event_t sg_event;

	switch (sg->avail_state) {
		case SG_AC_Idle:
			su->su_failover_cnt += 1;
			set_scope_for_failover_su (sg, su);
			if (has_any_su_in_scope_active_workload (sg)) {
				acsm_enter_deactivating_dependent_workload (sg);
			} else {
				acsm_enter_terminating_suspected (sg);
			}
			break;
		case SG_AC_DeactivatingDependantWorkload:
		case SG_AC_TerminatingSuspected:
		case SG_AC_ActivatingStandby:
		case SG_AC_AssigningStandbyToSpare:
		case SG_AC_ReparingComponent:
		case SG_AC_ReparingSu:
		case SG_AC_AssigningOnRequest:
		case SG_AC_InstantiatingServiceUnits:
		case SG_AC_RemovingAssignment:
		case SG_AC_AssigningActiveworkload:
		case SG_AC_AssigningAutoAdjust:
		case SG_AC_AssigningWorkload:
		case SG_AC_WaitingAfterOperationFailed:
		case SG_AC_RemovingStandbyAssignments:
			sg_set_event (SG_FAILOVER_SU_EV, sg, su, 0, 0, &sg_event); 
			sg_defer_event (SG_FAILOVER_SU_EV, &sg_event); 
			break;
		default:
			assert (0);
			break;
	}
}

/******************************************************************************
 * Event response methods
 *****************************************************************************/

#ifdef COMPILE_OUT
void amf_sg_su_state_changed_2 (struct amf_sg *sg, 
	struct amf_su *su, SaAmfStateT type, int state)
{
	ENTER ("'%s' SU '%s' state %s",
		sg->name.value, su->name.value, amf_presence_state(state));

	if (type == SA_AMF_PRESENCE_STATE) {
		if (state == SA_AMF_PRESENCE_INSTANTIATED) {
			if (sg->avail_state == SG_AC_InstantiatingServiceUnits) {
				if (no_su_has_presence_state(sg, sg->node_to_start, 
					SA_AMF_PRESENCE_INSTANTIATING)) {
					acsm_enter_idle (sg);
				}
			} else if (sg->avail_state == SG_AC_ReparingSu) {
				if (all_su_in_scope_has_either_of_three_presence_state(
					su->sg,
					SA_AMF_PRESENCE_INSTANTIATED,
					SA_AMF_PRESENCE_INSTANTIATION_FAILED,
					SA_AMF_PRESENCE_UNINSTANTIATED)) {
					su->sg->avail_state = SG_AC_AssigningWorkload;
					if (assign_si (sg, 0) == 0) {
						acsm_enter_idle (sg);
					}

				} else {
					dprintf ("avail-state: %u", sg->avail_state);
					assert (0);
				}
			} else {
				dprintf ("avail-state: %u", sg->avail_state);
				assert (0);
			}
		} else if (state == SA_AMF_PRESENCE_UNINSTANTIATED) {
			if (sg->avail_state == SG_AC_TerminatingSuspected) {
				if (all_su_in_scope_has_either_two_presence_state (sg,
					SA_AMF_PRESENCE_UNINSTANTIATED,
					SA_AMF_PRESENCE_TERMINATION_FAILED)) {

					delete_si_assignments_in_scope (sg);

					if (is_any_si_in_scope_assigned_standby (sg)) {
						remove_all_suspected_sus (sg);
						acsm_enter_removing_standby_assignments (sg);
					} else { /*is_no_si_in_scope_assigned_standby*/
						remove_all_suspected_sus (sg);
						acsm_enter_assigning_standby_to_spare (sg);
					}
				}
			} else if (sg->avail_state == SG_AC_ReparingSu) {
				if (all_su_in_scope_has_either_of_three_presence_state(
					su->sg,
					SA_AMF_PRESENCE_INSTANTIATED,
					SA_AMF_PRESENCE_INSTANTIATION_FAILED,
					SA_AMF_PRESENCE_UNINSTANTIATED)) {
					su->sg->avail_state = SG_AC_AssigningWorkload;
					if (assign_si (sg, 0) == 0) {
						acsm_enter_idle (sg);
					}
				} else {
					dprintf("%d",sg->avail_state);
					assert (0);
				}
			}
		} else if (state == SA_AMF_PRESENCE_TERMINATION_FAILED) {

			if (all_su_in_scope_has_either_two_presence_state (sg,
				SA_AMF_PRESENCE_UNINSTANTIATED,
				SA_AMF_PRESENCE_TERMINATION_FAILED) && 
				is_any_si_in_scope_assigned_standby (sg)) {
				remove_all_suspected_sus (sg);

				acsm_enter_removing_standby_assignments (sg);

			} else if (all_su_in_scope_has_either_two_presence_state (sg,
				SA_AMF_PRESENCE_UNINSTANTIATED,
				SA_AMF_PRESENCE_TERMINATION_FAILED) && 
				!is_any_si_in_scope_assigned_standby (sg)) {

				remove_all_suspected_sus (sg);
				acsm_enter_assigning_standby_to_spare (sg);

			} else {

				remove_sis_for_term_failed_su_from_scope (sg, su);
			}
		} else if (state == SA_AMF_PRESENCE_INSTANTIATING) {
			; /* nop */
		} else if (state == SA_AMF_PRESENCE_INSTANTIATION_FAILED) {
			if (sg->avail_state == SG_AC_InstantiatingServiceUnits) {
				if (no_su_has_presence_state(sg, sg->node_to_start, 
					SA_AMF_PRESENCE_INSTANTIATING)) {
					acsm_enter_idle (sg);
				}

			} else if (sg->avail_state == SG_AC_ReparingSu) {
				if (all_su_in_scope_has_either_of_three_presence_state(
					su->sg,
					SA_AMF_PRESENCE_INSTANTIATED,
					SA_AMF_PRESENCE_INSTANTIATION_FAILED,
					SA_AMF_PRESENCE_UNINSTANTIATED)) {
					su->sg->avail_state = SG_AC_AssigningWorkload;
					if (assign_si (sg, 0) == 0) {
						acsm_enter_idle (sg);
					}
				}
			} else {
				/* TODO: Insert the assert (0) until solving defers in SU   */
				dprintf("sg->avail_state = %d, su instantiation state = %d",
					sg->avail_state, state);
			}
		} else {
			dprintf("sg->avail_state = %d, su instantiation state = %d",
					sg->avail_state, state);
			assert (0);
		}
	}
}
#endif

/**
 * SU indicates to SG that one of its state machines has changed state.
 * @param sg - SG which contains the SU that has changed state
 * @param su - SU which has changed state
 * @param type - Indicates which state machine that has changed state
 * @param state - The new state that has been assumed.
 * 
 */
void amf_sg_su_state_changed (struct amf_sg *sg, struct amf_su *su, 
	SaAmfStateT type, int state)
{
	ENTER ("'%s' SU '%s' state %s",
		sg->name.value, su->name.value, amf_presence_state(state));

	if (sg->avail_state != SG_AC_Idle) {
		if (type == SA_AMF_PRESENCE_STATE) {
			switch (state) {
				case SA_AMF_PRESENCE_INSTANTIATED:
					sg_su_state_changed_to_instantiated(sg, su);
					break;
				case SA_AMF_PRESENCE_UNINSTANTIATED:
					amf_sg_su_state_changed_to_uninstantiated(sg, su);
					break;
				case SA_AMF_PRESENCE_TERMINATION_FAILED:
					amf_sg_su_state_changed_to_termination_failed(sg, su);
					break;
				case SA_AMF_PRESENCE_INSTANTIATING:
					; /* nop */
					break;
				case SA_AMF_PRESENCE_INSTANTIATION_FAILED:
					amf_sg_su_state_changed_to_instantiation_failed(sg, su);
					break;
				case SA_AMF_PRESENCE_TERMINATING:
					; /* nop */
					break;
				default :
					dprintf("sg->avail_state = %d, su instantiation state = %d",
						sg->avail_state, state);
					assert (0);
					break;
			}
		}
	}
}

/**
 * Callback function used by SI when there is no dependent SI to
 * deactivate.
 * @param sg
 */
static void dependent_si_deactivated_cbfn2 (struct amf_sg *sg)
{
	struct amf_su **sus = sg->recovery_scope.sus;

	ENTER("'%s'", sg->name.value);

	/*
	 * Select next state depending on if some
	 * SU in the scope needs to be terminated.
	 */

	while (*sus != NULL) {
		amf_su_t *su = *sus;

		ENTER("SU %s pr_state='%d'",su->name.value, 
			su->saAmfSUPresenceState);

		if (su_presense_state_is_ored (su,
			SA_AMF_PRESENCE_UNINSTANTIATED,
			SA_AMF_PRESENCE_TERMINATION_FAILED,
			SA_AMF_PRESENCE_INSTANTIATION_FAILED)) {
			sus++;
			continue;
		}
		break;
	}

	if (*sus != NULL) {
		acsm_enter_terminating_suspected (sg);
	} else {
		delete_si_assignments_in_scope(sg);         
		acsm_enter_removing_standby_assignments (sg);
	}
}

/**
 * Callback function used by SI when an SI has been deactivated, i.e.
 * transitioned from active HA-state to any other state.
 * @param si_assignment
 * @param result - Indicates the result of the operation.
 */
static void dependent_si_deactivated_cbfn (
	struct amf_si_assignment *si_assignment, int result)
{
	struct amf_sg *sg = si_assignment->su->sg;
	struct amf_su **sus = sg->recovery_scope.sus;
	struct amf_su *su;

	ENTER ("'%s', %d", si_assignment->si->name.value, result);

	/*
	 * If all SI assignments for all SUs in the SG are not pending,
	 * goto next state (TerminatingSuspected).
	 */
	for (su = sg->su_head ; su != NULL; su = su->next) {
		struct amf_si_assignment *si_assignment;
		si_assignment = amf_su_get_next_si_assignment(su, NULL);

		while (si_assignment != NULL) {
			if (si_assignment->saAmfSISUHAState != 
				si_assignment->requested_ha_state) {
				goto still_wating;
			}
			si_assignment = amf_su_get_next_si_assignment(su, 
				si_assignment);
		}
	}

still_wating:

	if (su == NULL) {
		sus = si_assignment->su->sg->recovery_scope.sus;

		/*
		 * Select next state depending on if some
		 * SU in the scope is needs to be terminated.
		 */

		while (*sus != NULL) {
			if (su_presense_state_is_not (*sus,
				SA_AMF_PRESENCE_UNINSTANTIATED,
				SA_AMF_PRESENCE_TERMINATION_FAILED,
				SA_AMF_PRESENCE_INSTANTIATION_FAILED)) {
				break;
			}
			sus++;
		}
		if (*sus != NULL) {
			acsm_enter_terminating_suspected (sg);
		} else {
			acsm_enter_removing_standby_assignments (sg);
		}
	}
	LEAVE("");
}

/**
 * Callback function used by SI to indicate an SI has assumed a new HA-state or
 * that the attempt to do so failed.
 * @param si_assignment
 * @param result - Indicates the result of the operation.
 */
static void assign_si_assumed_cbfn (
	struct amf_si_assignment *si_assignment, int result)
{
	struct amf_sg *sg = si_assignment->su->sg;
	int si_assignment_cnt = 0;
	int confirmed_assignments = 0;

	ENTER ("'%s', %d", si_assignment->si->name.value, result);


	switch (sg->avail_state) {
		case SG_AC_AssigningOnRequest:
			if (is_all_si_assigned (sg)) {
				acsm_enter_idle (sg);
				amf_application_sg_assigned (sg->application, sg);
			} else {
				dprintf ("%d, %d", si_assignment_cnt, confirmed_assignments);
			}
			break;
		case SG_AC_AssigningWorkload:
			{
				if (is_all_si_assigned(sg)) {
					acsm_enter_idle (sg);
				}
				break;
			}
		case SG_AC_AssigningStandbyToSpare: 
			{
				if(is_all_si_assigned (sg)) {
					/*
					 * All si_assignments has asumed
					 * Prescense state SA_AMF_HA_STANDBY
					 */
					switch (sg->recovery_scope.event_type) {
						case SG_FAILOVER_NODE_EV:
							acsm_enter_idle (sg);
							break;
						case SG_FAILOVER_SU_EV:
							if (sg->saAmfSGAutoRepair == SA_TRUE) {
								acsm_enter_repairing_su (sg);
							} 
							break;
						default:
							assert (0);
							break;
					}
				} else {
					si_assignment->saAmfSISUHAState = SA_AMF_HA_STANDBY;
				}
			}
			break;
		default:
			dprintf ("%d, %d, %d", sg->avail_state, si_assignment_cnt,
				confirmed_assignments);
			amf_runtime_attributes_print (amf_cluster);
			assert (0);
			break;
	}
}

/**
 * Callback function used by SI when an SI has been activated, i.e. transitioned
 * from any HA-state to an active HA-state.
 * @param si_assignment
 * @param result - Indicates the result of the operation.
 */
static void standby_su_activated_cbfn (
	struct amf_si_assignment *si_assignment, int result)
{
	struct amf_su **sus = si_assignment->su->sg->recovery_scope.sus;
	struct amf_si **sis = si_assignment->su->sg->recovery_scope.sis;

	ENTER ("'%s', %d", si_assignment->si->name.value, result);

	/*
	 * If all SI assignments for all SIs in the scope are activated, goto next
	 * state.
	 */

	while (*sis != NULL) {
		if ((*sis)->assigned_sis != NULL &&
			(*sis)->assigned_sis->saAmfSISUHAState != SA_AMF_HA_ACTIVE) {
			break;
		}
		sis++;
	}

	if (*sis == NULL) {

		acsm_enter_assigning_standby_to_spare ((*sus)->sg);
	}
}

/******************************************************************************
 * General methods
 *****************************************************************************/

/**
 * Constructor for SG objects. Adds SG to the list owned by
 * the specified application. Always returns a valid SG
 * object, out-of-memory problems are handled here. Default
 * values are initialized.
 * @param sg
 * @param name
 * 
 * @return struct amf_sg*
 */

struct amf_sg *amf_sg_new (struct amf_application *app, char *name) 
{
	struct amf_sg *sg = amf_calloc (1, sizeof (struct amf_sg));

	setSaNameT (&sg->name, name);
	sg->saAmfSGAdminState = SA_AMF_ADMIN_UNLOCKED;
	sg->saAmfSGNumPrefActiveSUs = 1;
	sg->saAmfSGNumPrefStandbySUs = 1;
	sg->saAmfSGNumPrefInserviceSUs = ~0;
	sg->saAmfSGNumPrefAssignedSUs = ~0;
	sg->saAmfSGCompRestartProb = -1;
	sg->saAmfSGCompRestartMax = ~0;
	sg->saAmfSGSuRestartProb = -1;
	sg->saAmfSGSuRestartMax = ~0;
	sg->saAmfSGAutoAdjustProb = -1;
	sg->saAmfSGAutoRepair = SA_TRUE;
	sg->application = app;
	sg->next = app->sg_head;
	app->sg_head = sg;
	sg->deferred_events = NULL;

	return sg;
}

void amf_sg_delete (struct amf_sg *sg)
{
	struct amf_su *su;

	for (su = sg->su_head; su != NULL;) {
		struct amf_su *tmp = su;
		su = su->next;
		amf_su_delete (tmp);
	}

	free (sg);
}

void *amf_sg_serialize (struct amf_sg *sg, int *len)
{
	char *buf = NULL;
	int offset = 0, size = 0;

	TRACE8 ("%s", sg->name.value);

	buf = amf_serialize_SaNameT (buf, &size, &offset, &sg->name);
	buf = amf_serialize_SaUint32T (buf, &size, &offset, sg->saAmfSGRedundancyModel);
	buf = amf_serialize_SaUint32T (
		buf, &size, &offset, sg->saAmfSGAutoAdjust);
	buf = amf_serialize_SaUint32T (
		buf, &size, &offset, sg->saAmfSGNumPrefActiveSUs);
	buf = amf_serialize_SaUint32T (
		buf, &size, &offset, sg->saAmfSGNumPrefStandbySUs);
	buf = amf_serialize_SaUint32T (
		buf, &size, &offset, sg->saAmfSGNumPrefInserviceSUs);
	buf = amf_serialize_SaUint32T (
		buf, &size, &offset, sg->saAmfSGNumPrefAssignedSUs);
	buf = amf_serialize_SaUint32T (
		buf, &size, &offset, sg->saAmfSGMaxActiveSIsperSUs);
	buf = amf_serialize_SaUint32T (
		buf, &size, &offset, sg->saAmfSGMaxStandbySIsperSUs);
	buf = amf_serialize_SaUint32T (
		buf, &size, &offset, sg->saAmfSGCompRestartProb);
	buf = amf_serialize_SaUint32T (
		buf, &size, &offset, sg->saAmfSGCompRestartMax);
	buf = amf_serialize_SaUint32T (
		buf, &size, &offset, sg->saAmfSGSuRestartProb);
	buf = amf_serialize_SaUint32T (
		buf, &size, &offset, sg->saAmfSGSuRestartMax);
	buf = amf_serialize_SaUint32T (
		buf, &size, &offset, sg->saAmfSGAutoAdjustProb);
	buf = amf_serialize_SaUint32T (
		buf, &size, &offset, sg->saAmfSGAutoRepair);
	buf = amf_serialize_SaUint32T (
		buf, &size, &offset, sg->saAmfSGAdminState);
	buf = amf_serialize_SaUint32T (
		buf, &size, &offset, sg->saAmfSGNumCurrAssignedSUs);
	buf = amf_serialize_SaUint32T (
		buf, &size, &offset, sg->saAmfSGNumCurrNonInstantiatedSpareSUs);
	buf = amf_serialize_SaUint32T (
		buf, &size, &offset, sg->saAmfSGNumCurrInstantiatedSpareSUs);
	buf = amf_serialize_SaStringT (
		buf, &size, &offset, sg->clccli_path);
	buf = amf_serialize_SaUint32T (
		buf, &size, &offset, sg->avail_state);
	buf = amf_serialize_SaUint32T (
		buf, &size, &offset, sg->recovery_scope.event_type);

	*len = offset;

	return buf;
}

struct amf_sg *amf_sg_deserialize (struct amf_application *app, char *buf)
{
	char *tmp = buf;
	struct amf_sg *sg = amf_sg_new (app, "");

	tmp = amf_deserialize_SaNameT (tmp, &sg->name);
	tmp = amf_deserialize_SaUint32T (tmp, &sg->saAmfSGRedundancyModel);
	tmp = amf_deserialize_SaUint32T (tmp, &sg->saAmfSGAutoAdjust);
	tmp = amf_deserialize_SaUint32T (tmp, &sg->saAmfSGNumPrefActiveSUs);
	tmp = amf_deserialize_SaUint32T (tmp, &sg->saAmfSGNumPrefStandbySUs);
	tmp = amf_deserialize_SaUint32T (tmp, &sg->saAmfSGNumPrefInserviceSUs);
	tmp = amf_deserialize_SaUint32T (tmp, &sg->saAmfSGNumPrefAssignedSUs);
	tmp = amf_deserialize_SaUint32T (tmp, &sg->saAmfSGMaxActiveSIsperSUs);
	tmp = amf_deserialize_SaUint32T (tmp, &sg->saAmfSGMaxStandbySIsperSUs);
	tmp = amf_deserialize_SaUint32T (tmp, &sg->saAmfSGCompRestartProb);
	tmp = amf_deserialize_SaUint32T (tmp, &sg->saAmfSGCompRestartMax);
	tmp = amf_deserialize_SaUint32T (tmp, &sg->saAmfSGSuRestartProb);
	tmp = amf_deserialize_SaUint32T (tmp, &sg->saAmfSGSuRestartMax);
	tmp = amf_deserialize_SaUint32T (tmp, &sg->saAmfSGAutoAdjustProb);
	tmp = amf_deserialize_SaUint32T (tmp, &sg->saAmfSGAutoRepair);
	tmp = amf_deserialize_SaUint32T (tmp, &sg->saAmfSGAdminState);
	tmp = amf_deserialize_SaUint32T (tmp, &sg->saAmfSGNumCurrAssignedSUs);
	tmp = amf_deserialize_SaUint32T (tmp, &sg->saAmfSGNumCurrNonInstantiatedSpareSUs);
	tmp = amf_deserialize_SaUint32T (tmp, &sg->saAmfSGNumCurrInstantiatedSpareSUs);
	tmp = amf_deserialize_SaStringT (tmp, &sg->clccli_path);
	tmp = amf_deserialize_SaUint32T (tmp, &sg->avail_state);
	tmp = amf_deserialize_SaUint32T (tmp, &sg->recovery_scope.event_type);

	return sg;
}

struct amf_sg *amf_sg_find (struct amf_application *app, char *name) 
{
	struct amf_sg *sg;

	for (sg = app->sg_head; sg != NULL; sg = sg->next) {
		if (sg->name.length == strlen(name) && 
			strncmp (name, (char*)sg->name.value, sg->name.length) == 0) {
			break;
		}
	}

	return sg;
}

