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
/*
 * common.h
 *
 * Clovis sample app common header file
 *
 */

#define EVENT_CHANNEL_NAME "csa11[23]TestEvents"
#define PUBLISHER_NAME "csa11[23]_Publisher"
#define EVENT_TYPE 5432

//
// Define IDs for our sample applications
typedef enum {
    MY_EO_CLIENT_ID = CL_EO_CLOVIS_RESERVED_CLIENTID_END,
    MY_MAX_EO_CLIENT_ID
} MyEoClientIds;
