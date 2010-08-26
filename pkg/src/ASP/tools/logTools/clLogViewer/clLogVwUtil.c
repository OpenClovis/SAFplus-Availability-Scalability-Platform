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
#include <stdlib.h>
#include <stdio.h>

#include "clLogVwUtil.h"
#include "clLogVwType.h"
#include "clLogVwConstants.h"
#include "clLogVwErrors.h"

/*
 * Reverses the original byte sequence of raw bytes
 * of log record for specified length 
 * 
 *@param -  bytes
 *          the byte sequence
 *@param -  index
 *          start index of byte seq
 *          to be reversed
 *@param -  len
 *          no. of bytes to be reversed
 *
 *@return - rc 
 *          CL_OK if everything is OK
 *          error code otherwise
 */
ClRcT
clLogVwGetRevSubBytes(void *ptr, ClUint32T index, ClUint32T len) 
{
    int i = 0 ;
    
    ClCharT tempChar = ' ';

    ClLogVwByteT *bytes = (ClLogVwByteT*) ptr;

    for( i = 0; i < len / 2 ; i++ )
    {
        tempChar = bytes[index + i]; 
        
        bytes[index + i] = bytes[index + len - 1 - i];

        bytes[index + len - 1 - i] = tempChar;
    }
    return CL_OK;
}


/*
 * Determines endianess of the machine
 *
 *@return - true when machine is little endian
            false otherwise
 */
ClBoolT 
clLogVwIsLittleEndian(void)
{
    ClInt32T i = 1;

    return (*((char *) &i) ? CL_TRUE : CL_FALSE);
}

/*
 * Checks if machine and record header
 * has same endianess
 *
 *@return - TRUE when machine and record header
 *          has same endianess
 */
ClBoolT
clLogVwIsSameEndianess( ClLogVwFlagT endianess)
{
    if(CL_TRUE == clLogVwIsLittleEndian())
    {
      if( CL_LOGVW_LITTLE_ENDIAN == endianess )
        return CL_TRUE;
      else
          return CL_FALSE;
    }
    else
    {
      if( CL_LOGVW_BIG_ENDIAN == endianess )
        return CL_TRUE;
      else
          return CL_FALSE;
    }
}

