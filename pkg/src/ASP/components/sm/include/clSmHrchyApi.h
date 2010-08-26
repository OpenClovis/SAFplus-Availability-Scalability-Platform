/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is  free software; you can redistribute it and / or
 * modify  it under  the  terms  of  the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * You  should  have  received  a  copy of  the  GNU General Public
 * License along  with  this program. If  not,  write  to  the 
 * Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : sm
 * File        : clSmHrchyApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains State Machine definitions          
 *************************************************************************/

#ifndef _CL_SM_HRCHY_API_H
#define _CL_SM_HRCHY_API_H

#ifdef __cplusplus
extern "C" {
#endif

/** @pkg cl.sm.hsm */

/* INCLUDES */
#include <clSmTemplateApi.h>

/* DEFINES */
#define MAX_DEPTH 16

/* ENUM */

/**
 * Hierarchical State Machine Library.
 *
 * <h1>HSM Overview</h1>
 * 
 * Clovis Hierarchical State Machine library contains API's to create
 * state machines and their run time instances.
 *
 * Hierarchical state machine (HSM) captures the commonality by
 * organizing the states as a hierarchy. The states at the higher
 * level in hierarchy perform the common message handling, while the
 * lower level states inherit the commonality from higher level ones
 * and perform the state specific functions.
 *
 * Following picture captures a sample HSM with nested states and sub
 * states.
 *
 * <img src="images/hsm.jpg"/>
 *
 * Description of the state machine is as follows:
 * <blockquote>
 * <pre>
 * - IS       In-service is a simple state
 * - OOS      (Out of Service) is a composite state 
 *      a.      Entry state is AU
 *      b.      Composed of states (MA/AU)
 * - MA       (Management) is a composite State
 *      a.      Entry state is MA
 *      b.      Composed of states (UAS)
 *      c.      Handles common Event (at composite level)
 *              i.      Lock
 *              ii.     Fail
 * - AU      (Autonomous) is a composite state 
 *      a.      Entry state is FLT
 *      b.      Composed of states (UEQ/FLT/MA1)
 * - MA1     (managed) is a composite state 
 *      a.      Entry state is FLT1
 *      b.      Composted of states (FLT1/UEQ1/UAS1,UEQ1)
 * - Rest of the states (UAS, FLT1, UEQ1, UAS1,UEQ1,UEQ, FLT) are
 * simple states and their events and transitions are as captured in
 * the diagram.
 * </pre>
 * </blockquote>
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
 *
 * @pkgdoc  cl.sm.hsm
 * @pkgdoctid Hierarchical State Machine Library
 */


/* Macros defined here */


/**@#-*/

/* Function Prototypes */

/* constructor/destructor functions */

/* public functions */

/**@#+*/

#ifdef __cplusplus
}
#endif

#endif  /*  _CL_SM_HRCHY_API_H */
