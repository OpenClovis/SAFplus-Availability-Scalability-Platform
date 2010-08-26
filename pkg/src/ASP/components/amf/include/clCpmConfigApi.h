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

/**
 *  \file
 *  \brief Header file for definitions of configuration parameters
 *         used by the CPM
 *  \ingroup cpm_apis
 */

/**
 *  \addtogroup cpm_apis
 *  \{
 */

#ifndef _CL_CPM_CONFIG_API_H_
#define _CL_CPM_CONFIG_API_H_

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Default frequency at which Component Manager performs the
 * heartbeats of the component. This value is in milliseconds. This
 * value can be overwritten in the Component Manager configuration
 * file.
 */
#define CL_CPM_DEFAULT_MIN_FREQ     2000

/**
 * Maximum frequency at which Component Manager will check the
 * heartbeats of the component. This value is in miliseconds. This
 * value can be overwritten in the Component Manager configuration
 * file.
 */
#define CL_CPM_DEFAULT_MAX_FREQ     32000

/**
 * This definition of enum indicates the relationship between the
 * component and the process.
 */
typedef enum
{
    /**
     * This indicates the component does not have any execution
     * context.
     */
    CL_CPM_COMP_NONE = 0,

    /**
     * This indicates the component consists of multiple processes.
     */
    CL_CPM_COMP_MULTI_PROCESS = 1,

    /**
     * This indicates the component consists of a single process.
     */
    CL_CPM_COMP_SINGLE_PROCESS = 2,

    /**
     * This indicates that the component consists of multiple threads,
     * but the process does not belong to the component.
     */
    CL_CPM_COMP_THREADED = 3
} ClCpmCompProcessRelT;

/**
 * This is the definition of the class of the node.
 */
typedef struct
{
    /**
     *  Node name.
     */
    ClNameT     name;
    /**
     * Node identifier, an opaque string to the CPM.
     */
    ClNameT     identifier;
} ClCpmNodeClassTypeT;

/**
 *  This structure is used by the clCpmCardMatch() API.
 */
       
typedef struct {
    /**
     * Indicates the number of items in the \e nodeClassTypes.
     */
    ClUint32T               numItems;
    /**
     * Array of \e nodeClass and the identifier pair
     */
    ClCpmNodeClassTypeT     *nodeClassTypes;
} ClCpmSlotClassTypesT;

#ifdef __cplusplus
}
#endif

#endif                          /* _CL_CPM_CONFIG_API_H_ */

/** \} */
