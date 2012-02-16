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
 * ModuleName  : cor
 * File        : clCorDmHooks.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Handles DM Hook routines, that user can change
 *****************************************************************************/

/* FILES INCLUDED */

#include <clCommon.h>
#include <clDebugApi.h>

/*Internal Headers*/
#include "clCorDmDefs.h"
#include "clCorDmProtoType.h"
#include "clCorLlist.h"

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

/* ----- Hash table hooks are implemented here ----- */


/** 
 * compare keys.
 *
 * API to
 *
 *  @param this       object handle
 *  @param 
 * 
 *  @returns 
 *    int 
 *
 *  @todo      
 *
 */
int
corKeyCompare(ClCntKeyHandleT key1, 
              ClCntKeyHandleT key2
              )
{
    CL_FUNC_ENTER();
    CL_FUNC_EXIT(); 
    return ((ClWordT)key1 - (ClWordT)key2);
}


/** 
 * Hash function.
 *
 * API to corHashFunction 
 *
 *  @param this       object handle
 *  @param 
 * 
 *  @returns 
 *    int   hash value for the key.
 *
 *  @todo      
 *
 */
unsigned int
corHashFunction(ClCntKeyHandleT key
                )
{
    CL_FUNC_ENTER();
    CL_FUNC_EXIT(); 
    return ((ClWordT)key % NUMBER_OF_BUCKETS);
}

ClInt32T clCorLlistKeyCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    return ((ClWordT)key1 - (ClWordT)key2);
}

/* ----- DM Utilities implemented here ----- */

/** 
 * dumps a buffer in binary dump format (more readable).
 *
 * API to dmBinaryShow <Deailed desc>. 
 *
 *  @param title    title of the dump (can be NULL)
 *  @param buf      buffer to dump (can be NULL)
 * 
 *  @returns 
 *    none.
 *
 *  @todo      
 *
 */
void
dmBinaryShow(Byte_h title, Byte_h buf, ClUint32T len0)
{
}
