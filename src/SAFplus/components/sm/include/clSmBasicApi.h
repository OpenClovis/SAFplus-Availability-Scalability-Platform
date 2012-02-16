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
/*******************************************************************************
 * ModuleName  : sm
 * File        : clSmBasicApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains State Machine definitions          
 *************************************************************************/

#ifndef _CL_SM_BASIC_API_H
#define _CL_SM_BASIC_API_H

#ifdef __cplusplus
extern "C" {
#endif

/** @pkg cl.sm.fsm */

/* INCLUDES */
#include <clSmTemplateApi.h>

/* DEFINES */


/* ENUM */

/**
 * State Machine Library.
 *
 * <h1>FSM Overview</h1>
 * 
 * Clovis State Machine library contains API's to create state
 * machines and their run time instances.  
 *
 * A FiniteStateMachine (FSM) is a representation of an event driven
 * system (a system that is driven by external events that are input
 * into the system). In an event-driven system, the system makes a
 * transition from one state (previous state) to another prescribed
 * new state (new state), provided that the condition that defines the
 * change is satisfied.  In other words, a Finite-state machine is a
 * pre-determined network which has a finite number of states that are
 * interconnected based on the external events that trigger the state
 * transition.
 *
 * A Finite-State machine contains a finite number of
 * states, events and user call-back routines. Simple state machines
 * can be represented by networking a set of states and transition
 * events, and we refer to it as Finite-State Machine (FSM) in this
 * document. Alternatively, complex state machines with a large number
 * of states and transition events are easier to represent and
 * implement using Hierarchical Finite-State Machines. A Hierarchical
 * Finite-State Machine (HSM) can be viewed as a layered state
 * machine, and each state in the layer above containing a
 * finite-state machine within itself. A detailed description follows
 * later in this document. First, lets describe the various building
 * blocks that are utilized in constructing the FSM, next walk through
 * the Start transitions, and finally present a pseudo call graph.
 *
 * Following picture captures a sample state machine:
 *
 * <img src="images/StateMachine.gif"/>
 * 
 * init state: s1 <p>
 * final state: s4  <p>
 * States: s1,s2,s3,s4 <p>
 * At State (s1): Events 'a' and 'b' are handled (using transitions t1,t2) <p>
 * At State (s2): Event c is handled (using transition t3) <p>
 * At State (s3): Event d is handled (using transition t4) <p>
 * Events: a,b,c,d <p>
 * Transitions: t1,t2,t3,t2 <p>
 *
 * <h1>Interaction with other components</h1>
 * none
 *
 * <h1>Configuration</h1>
 * none
 *
 * <h1>Usage Scenario(s)</h1>
 *
 * Following code block shall create the above depicted sample state
 * machine
 * <blockquote>
 * <pre>
 *@@  
 *@@  #include <sm.h>
 *@@  
 *@@  /@ code snippet begins here @/
 *@@  ClRcT  ret;
 *@@  ret = clSmTypeCreate(MAX_STATES, &sm);
 *@@  if(ret==CL_OK) 
 *@@    {
 *@@      smTypeNameSet(sm, "SampleSimpleStateMachine");
 *@@      /@ calling a convenience macro to create a state and add to state
 *@@         machine type created above @/
 *@@      SM_TYPE_STATE_CREATE(ret, sm, 5, tmp, s1_state, s1_entry, s1_exit);
 *@@      if(ret==CL_OK) 
 *@@        smStateNameSet(tmp, "s1_state", "s1_entry", "s1_exit");
 *@@      /@ -- create state s2 -- @/
 *@@      SM_TYPE_STATE_CREATE(ret, sm, 5, tmp, s2_state, s2_entry, s2_exit);
 *@@      if(ret==CL_OK) 
 *@@        smStateNameSet(tmp, "s2_state", "s2_entry", "s2_exit");
 *@@      /@ -- create state s3 -- @/
 *@@      SM_TYPE_STATE_CREATE(ret, sm, 5, tmp, s3_state, s3_entry, s3_exit);
 *@@      if(ret==CL_OK) 
 *@@        smStateNameSet(tmp, "s3_state", "s3_entry", "s3_exit");
 *@@      /@ -- create state s4 -- @/
 *@@      SM_TYPE_STATE_CREATE(ret, sm, 5, tmp, s4_state, s4_entry, s4_exit);
 *@@      if(ret==CL_OK) 
 *@@        smStateNameSet(tmp, "s4_state", "s4_entry", "s4_exit");
 *@@    }
 *@@   
 *@@  /@ add transition at state s1, where s2 is the next state and
 *@@     t1_action is the transition action
 *@@  @/
 *@@  SM_STATE_TRANSITION_CREATE(ret, sm, s1_state, a_event, s2_state, t1_action, to);
 *@@  if(ret==CL_OK && to) {
 *@@    smTransitionNameSet(to, "t1_action");
 *@@  }
 *@@  SM_STATE_TRANSITION_CREATE(ret, sm, s1_state, b_event, s3_state, t2_action, to);
 *@@  if(ret==CL_OK && to) {
 *@@    smTransitionNameSet(to, "t2_action");
 *@@  }
 *@@  SM_STATE_TRANSITION_CREATE(ret, sm, s2_state, c_event, s3_state, t3_action, to);
 *@@  if(ret==CL_OK && to) {
 *@@    smTransitionNameSet(to, "t3_action");
 *@@  }
 *@@  SM_STATE_TRANSITION_CREATE(ret, sm, s3_state, d_event, s4_state, t4_action, to);
 *@@  if(ret==CL_OK && to) {
 *@@    smTransitionNameSet(to, "t4_action");
 *@@  }
 * </pre>
 * </blockquote>
 * Following code block would create an instance of the state machine created above.
 * <blockquote>
 * <pre>
 *@@    /@ API to create state machine instance @/
 *@@    ret = clSmInstanceCreate(sm, &instance);
 * </pre>
 * </blockquote>
 * Following code block would handle events for the instantiated state machine.
 * <blockquote>
 * <pre>
 *@@    /@ API to process events at Simple State Machine Instance @/
 *@@    ret = clSmInstanceOnEvent(sm, msg);
 * </pre>
 * </blockquote>
 * </dd>
 *
 * @pkgdoc  cl.sm.fsm
 * @pkgdoctid State Machine Library
 */

/**
 * State Machine Instance Object.  
 *
 * Describes the State Machine run time instance object. 
 */
typedef struct SMInstance
{
  ClSmTemplatePtrT            sm;                    /**< State machine */
  ClSmStatePtrT           current;               /**< current state */

#ifdef DEBUG
  char                name[MAX_STR_NAME];    /**< Instance Name */
#endif


} ClSmInstanceT;

typedef ClSmInstanceT* ClSmInstancePtrT;


/* Macros defined here */




/**@#-*/

/* Function Prototypes */

/* constructor/destructor functions */

ClRcT                clSmInstanceCreate(CL_IN ClSmTemplatePtrT sm, 
                                        CL_OUT ClSmInstancePtrT* instance);
ClRcT                clSmInstanceDelete(CL_IN ClSmInstancePtrT thisInst);

/* public functions */
ClRcT                clSmInstanceOnEvent(CL_IN ClSmInstancePtrT thisInst, CL_IN ClSmEventPtrT msg);
ClRcT                clSmInstanceCurrentStateGet(CL_IN ClSmInstancePtrT thisInst,
                                                CL_OUT ClSmStatePtrT* state);

ClRcT                clSmInstanceStart(CL_IN ClSmInstancePtrT thisInst);
ClRcT                clSmInstanceRestart(CL_IN ClSmInstancePtrT thisInst);

void                  smInstanceShow(CL_IN ClSmInstancePtrT thisInst);

#ifdef DEBUG
void                  smInstanceNameSet(CL_IN ClSmInstancePtrT thisInst, char* name);
#endif

/**@#+*/

#ifdef __cplusplus
}
#endif

#endif  /*  _CL_SM_BASIC_API_H */
