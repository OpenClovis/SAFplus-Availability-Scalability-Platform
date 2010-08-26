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
/*General utility functions*/
#include <stdio.h>

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clBinSearchApi.h>

/*
  Generic Binary search for faster chunk index fetching.
  Return the value thats equal or just above the key.
*/
ClRcT clBinarySearch(ClPtrT pKey,ClPtrT pBase,ClInt32T numElements,ClInt32T sizeOfEachElement,ClInt32T (*pCmp)(const void *,const void*),ClPtrT *ppResult) {
    ClInt32T left = 0;
    ClInt32T right = numElements - 1;
    ClRcT rc ;
    ClPtrT pLastTurnLeft = NULL;
    
    rc = CL_ERR_INVALID_PARAMETER;
    if(ppResult == NULL)
    {
        goto out;
    }

    *ppResult = NULL;
    rc = CL_OK;
    while(left <= right) 
    { 
        ClInt32T mid = (left+right) >> 1;
        ClPtrT pElement = (ClPtrT)((ClUint8T*)pBase + sizeOfEachElement*mid);
        ClInt32T cmp ;

        if((cmp = pCmp(pKey,pElement)) < 0 ) 
        {
            /*
              Now the key is lesser than the element.
              Mark this node
            */
            pLastTurnLeft = pElement;
            right = mid-1;
        } 
        else if(cmp > 0 ) 
        {
            left = mid+1;
        } 
        else 
        {
            /*
              Bulls eye
             */
            pLastTurnLeft = pElement;
            goto out_set;
        }
    }
    if(pLastTurnLeft == NULL)
    {
        rc = CL_ERR_NOT_EXIST;
        goto out;
    }

    out_set:
    *ppResult = pLastTurnLeft;

    out:
    return rc;
}
