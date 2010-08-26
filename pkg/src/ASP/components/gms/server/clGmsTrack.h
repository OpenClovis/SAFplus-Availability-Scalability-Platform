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
 * ModuleName  : gms
 * File        : clGmsTrack.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Contains the data structures and function protypes for clGmsTrack.c
 *****************************************************************************/

#ifndef _CL_GMS_TRACK_H_
#define _CL_GMS_TRACK_H_

# ifdef __cplusplus
extern "C" {
# endif

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clGmsView.h>

typedef struct {
    ClGmsHandleT    handle;
    ClIocAddressT   address;
} ClGmsTrackNodeKeyT;

/* Node Track Info structure. 
 */

typedef struct {
    ClGmsHandleT    handle;
    ClIocAddressT   address;
    ClUint8T        trackFlags;
	ClUint32T		dsId;
} ClGmsTrackNodeT;

/* Track Notify Info structure */

typedef struct {
    void        *buffer;    /* ptr. to changelist notification buffer */
    ClUint64T   entries;

} ClGmsTrackNotifyT;


ClRcT   _clGmsTrackInitialize();
ClRcT   _clGmsTrackFinalize();

/* Methods of the track element.
 * FIXME: make the prototypes consistant across files.
 */

ClRcT   _clGmsTrackCliPrint(
                CL_IN   ClGmsGroupIdT       groupId, 
                CL_IN   ClCharT             **ret);

ClRcT   _clGmsTrackAddNode(
                CL_IN   ClGmsGroupIdT       groupId, 
                CL_IN   ClGmsTrackNodeKeyT  key,
                CL_IN   ClGmsTrackNodeT    *node,
                CL_INOUT ClUint32T*         dsId);

ClRcT   _clGmsTrackDeleteNode(
                CL_IN   ClGmsGroupIdT       groupId, 
                CL_IN   ClGmsTrackNodeKeyT  key);

ClRcT   _clGmsTrackFindNode(
                CL_IN  ClGmsGroupIdT        groupId,
                CL_IN  ClGmsTrackNodeKeyT   key,
                CL_OUT ClGmsTrackNodeT      **node);
 
ClRcT   _clGmsTrackNotify(
                CL_IN   ClGmsGroupIdT       groupId);

/* Hash table callbacks */

ClUint32T  _clGmsTrackHashCallback(
                CL_IN   ClCntKeyHandleT);

ClInt32T   _clGmsTrackKeyCompareCallback(
                CL_IN   ClCntKeyHandleT, 
                CL_IN   ClCntKeyHandleT);

void       _clGmsTrackDeleteCallback(
                CL_IN   ClCntKeyHandleT,
                CL_IN   ClCntDataHandleT);

void       _clGmsTrackDestroyCallback(
                CL_IN   ClCntKeyHandleT, 
                CL_IN   ClCntDataHandleT);


#ifdef  __cplusplus
}
#endif
            
#endif /* _CL_GMS_TRACK_H_ */
