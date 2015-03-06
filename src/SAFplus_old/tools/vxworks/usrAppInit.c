/* usrAppInit.c - stub application initialization routine */

/* Copyright (c) 1998,2006 Wind River Systems, Inc. 
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01b,16mar06,jmt  Add header file to find USER_APPL_INIT define
01a,02jun98,ms   written
*/

/*
DESCRIPTION
Initialize user application code.
*/ 
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <vxWorks.h>
#if defined(PRJ_BUILD)
#include "prjParams.h"
#endif /* defined PRJ_BUILD */
#include <drv/bre/breExtras.h>
#include <nfsDrv.h>
#include <taskLib.h>
#include <rtpLib.h>
#define ASP_RAMDISK_SIZE (64U<<20) /*total ramdisk size*/
#define ASP_RAMDISK_DEVICE "/ram0"
#define ASP_NFS_HOST "192.168.0.14"
#define ASP_NFS_MOUNT_POINT "/vault/honeywell/vxworks"
#define ASP_RUNTIME_DIR "/romfs"
#define ASP_LOADER_IMAGE "asp.vxe"
#define ASP_SLOT_ID  "5"
#define ASP_NODE_NAME "ControllerI0"

/******************************************************************************
*
* usrAppInit - initialize the users application
*/ 

void usrAppInit (void)
{
    int status;
    char srcPath[0xff+1];
    /*
     * Step 1: create ramdisk
     */
    status = addRamDisk(ASP_RAMDISK_SIZE, ASP_RAMDISK_DEVICE);
    if(status == ERROR)
    {
        printf("Unable to create ramdisk [%s] with [%d] blocks. Error [%s]\n", 
               ASP_RAMDISK_DEVICE, ASP_RAMDISK_SIZE, strerror(errno));
        return;
    }
    /*
     * Step 2: nfs mount
     */
    status = nfsMount(ASP_NFS_HOST, ASP_NFS_MOUNT_POINT, ASP_RUNTIME_DIR);
    if(status == ERROR)
    {
        printf("Unable to mount [%s] from nfs server [%s]. Error [%s]\n", 
               ASP_NFS_MOUNT_POINT, ASP_NFS_HOST, strerror(errno));
        return ;
    }
    /*
     * Step 3: copy asp loader to ramdisk and change directory to ramdisk. before spawning ASP loader
     */
    snprintf(srcPath, sizeof(srcPath), "%s/%s", ASP_RUNTIME_DIR, ASP_LOADER_IMAGE);
    if(cp(srcPath, ASP_RAMDISK_DEVICE) == ERROR)
    {
        printf("Unable to copy [%s] to [%s]. Error [%s]\n", 
               srcPath, ASP_RAMDISK_DEVICE, strerror(errno));
        return;
    }
    /*
     * Change directory to ramdisk. 
     */
    printf("Changing directory to %s\n\n", ASP_RAMDISK_DEVICE);
    if(chdir(ASP_RAMDISK_DEVICE) == ERROR)
    {
        printf("Unable to change directory to [%s]. Error [%s]\n\n", 
               ASP_RAMDISK_DEVICE, strerror(errno));
    }
    else
    {
        int rtpId = 0;
        char aspImage[0xff+1];
        const char *args[] = { 
            ASP_LOADER_IMAGE, "-c", "0", "-l", ASP_SLOT_ID, "-n", ASP_NODE_NAME, 
            "-s", ASP_RAMDISK_DEVICE, ASP_RUNTIME_DIR, NULL 
        };
        snprintf(aspImage, sizeof(aspImage), "%s/%s", ASP_RAMDISK_DEVICE, ASP_LOADER_IMAGE);
        printf("Starting ASP...\n\n");
        rtpId = rtpSpawn(aspImage, (const char **)args, NULL, 100, 256<<10, 0, VX_FP_TASK);
        if(rtpId == ERROR)
        {
            printf("Unable to spawn image [%s]. Error [%s]\n", aspImage, strerror(errno));
        }
        else
            printf("rtpSpawn for image [%s] success\n", aspImage);
    }
}


