/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office
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


/************************************************************************
 * 
 * This file contains test cases for following APIs.
 *
 * clGmsGroupTrack() 
 * clGmsGroupTrackStop()
 *
 * Following test cases are coded in this file.
 * 
 *
 * NOTE: Future addition of test cases should be at the bottom
 *       and this metadata should be modified accordingly.
 ************************************************************************/
#include "main.h"

static ClHandleT   handle = 0;
static ClVersionT  correct_version = {'B',1,1};
static ClGmsGroupNameT  groupName1 = {
    .value = "group1",
    .length = 6
};
static  ClGmsGroupIdT  groupId = 0;
static  ClGmsMemberNameT memberName1 = {
    .value = "member1",
    .length = 7
};
static ClGmsMemberIdT   memberId1 = 1234;


static ClGmsCallbacksT callbacks_non_null = {
    .clGmsClusterTrackCallback      = NULL,
    .clGmsClusterMemberGetCallback  = NULL,
    .clGmsGroupTrackCallback        = NULL,
    .clGmsGroupMemberGetCallback    = NULL
};

struct msg {
    int i;
    char j;
    float k;
    char l[188];
};

void messageDeliveryCallback (ClGmsGroupIdT         groupId,
                              ClGmsMemberIdT        senderId,
                              ClUint32T             messageSize,
                              ClPtrT                message)
{
    printf("Group mcast message received on group id %d, from memberid %d\n",groupId, senderId);

    if (messageSize == sizeof(struct msg))
    {
        printf("Message size %d, message - i = %d, j = %c, k = %f, l = %s\n",messageSize,
                ((struct msg*)message)->i,((struct msg*)message)->j,((struct msg*)message)->k,((struct msg*)message)->l);
    }
    else 
        printf("Message size %d, message %s\n",messageSize, (char*)message);
}


START_TEST_EXTERN(mcast_send_char_array)
{
    ClRcT       rc = CL_OK;
    char        msg[120] = "Hello. This is a simple character mcast message";

    rc = clGmsInitialize(&handle,&callbacks_non_null,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with callback values null failed with rc 0x%x",rc);

    /* Create a group */
    rc = clGmsGroupCreate(handle, &groupName1, NULL, NULL, &groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupCreate failed with rc 0x%x\n",rc);

    /* Do a node join */
    rc = clGmsGroupJoin(handle, groupId, memberId1, &memberName1, 0,messageDeliveryCallback, 0);
    fail_unless(rc == CL_OK,
            "clGmsGroupJoin for memberId %d failed with rc 0x%x",memberId1,rc);

    rc = clGmsGroupMcastSend(handle, groupId, memberId1, 0, 120, msg);
    fail_unless(rc == CL_OK,
            "clGmsGroupMcastSend failed with rc 0x%x",rc);

    sleep(3);

    /* Do group leave */
    rc = clGmsGroupLeave(handle, groupId, memberId1, 0);
    fail_unless(rc == CL_OK,
            "clGmsGroupLeave failed with rc 0x%x",rc);

    /* Group Destroy */
    rc = clGmsGroupDestroy(handle, groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupDestroy for groupId %d failed with rc 0x%x\n",groupId,rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN

START_TEST_EXTERN(mcast_send_struct_msg)
{
    ClRcT       rc = CL_OK;
    struct msg msg1 = {0};

    rc = clGmsInitialize(&handle,&callbacks_non_null,&correct_version);
    fail_unless(rc == CL_OK,
            "clGmsInitialize with callback values null failed with rc 0x%x",rc);

    /* Create a group */
    rc = clGmsGroupCreate(handle, &groupName1, NULL, NULL, &groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupCreate failed with rc 0x%x\n",rc);

    /* Do a node join */
    rc = clGmsGroupJoin(handle, groupId, memberId1, &memberName1, 0,messageDeliveryCallback, 0);
    fail_unless(rc == CL_OK,
            "clGmsGroupJoin for memberId %d failed with rc 0x%x",memberId1,rc);

    msg1.i = 10;
    msg1.j = 'A';
    msg1.k = 1.2345f;
    strncpy(msg1.l, "Hello dear", 100);
        
    rc = clGmsGroupMcastSend(handle, groupId, memberId1, 0, sizeof(struct msg), &msg1);
    fail_unless(rc == CL_OK,
            "clGmsGroupMcastSend failed with rc 0x%x",rc);

    sleep(3);

    /* Do group leave */
    rc = clGmsGroupLeave(handle, groupId, memberId1, 0);
    fail_unless(rc == CL_OK,
            "clGmsGroupLeave failed with rc 0x%x",rc);

    /* Group Destroy */
    rc = clGmsGroupDestroy(handle, groupId);
    fail_unless(rc == CL_OK,
            "clGmsGroupDestroy for groupId %d failed with rc 0x%x\n",groupId,rc);

    /* Finalize */
    rc = clGmsFinalize(handle);
    fail_unless(rc == CL_OK,
            "clGmsFinalize failed with rc = 0x%x",rc);
} END_TEST_EXTERN
