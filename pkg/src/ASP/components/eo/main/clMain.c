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
 * ModuleName  : eo 
 * File        : clMain.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This file contains the implementation of the main function.
 *
 *
 ****************************************************************************/

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clEoApi.h>
#include <clEoIpi.h>

/*
 * Main function for Non-SAF compliant components.
 */ 
ClInt32T main(ClInt32T argc, ClCharT *argv[])
{
    ClRcT rc = CL_OK;
    
    rc = clEoInitialize(argc, argv);

    return (CL_OK != rc);
}

