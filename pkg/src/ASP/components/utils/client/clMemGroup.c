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


#include <assert.h>
#include <stdio.h>

#include <clHeapApi.h>
#include <clMemGroup.h>

void clMemGroupInit(ClMemGroupT* obj,ClWordT baseSize)
{
    ClWordT i;
    
    for (i =0;i<CL_MEM_GRP_ARRAY_SIZE;i++)
    {
        obj->buffers[i] = 0;
        obj->bufPos[i] = 0;
    }
    obj->fundamentalSize = baseSize;
    obj->curBuffer = -1;    
}

void* clMemGroupAlloc(ClMemGroupT* obj,int amt)
{
    void* ret;
    int i;

    while(obj->curBuffer<CL_MEM_GRP_ARRAY_SIZE)
    {
        
        ClWordT bSize = obj->fundamentalSize<<obj->curBuffer;
        /* Look through the allocated buffers to see if there is room for what
           the caller wants at the end of one of them */
        for (i = obj->curBuffer;i>=0 && bSize >= amt; i--,bSize>>=1)
        {
            if (bSize-obj->bufPos[i]>=amt)
            {
                ret = obj->buffers[i]+obj->bufPos[i];
                obj->bufPos[i] += amt;
                return ret;
            }
        }
        /*
         * Check for the max limit.
         */
        if(obj->curBuffer + 1 == CL_MEM_GRP_ARRAY_SIZE)
            return NULL;
        /* Nothing found so we have to allocate a new buffer */
        obj->curBuffer+=1;
        obj->bufPos[obj->curBuffer] = 0;
        /* Each time thru I allocate 2x the memory */
        obj->buffers[obj->curBuffer] = clHeapAllocate(obj->fundamentalSize<<obj->curBuffer);
        if (obj->buffers[obj->curBuffer]==0) return NULL; /* no memory to be had */   
    }
    /* Memory Group is completely full */
    return NULL; /* no memory to be had */    
}

void clMemGroupFreeAll(ClMemGroupT* obj)
{
    ClWordT i;
    
    for (i = obj->curBuffer;i>=0;i--)
    {
        clHeapFree(obj->buffers[i]);
        obj->buffers[i] = 0;        
        obj->bufPos[i] = 0;            
    }
    obj->curBuffer = -1;
}

void clMemGroupDelete(ClMemGroupT* obj)
{
    clMemGroupFreeAll(obj);
    obj->fundamentalSize = 0;
}
