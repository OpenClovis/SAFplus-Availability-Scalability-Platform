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
 * ModuleName  : event
 * $File: //depot/dev/main/Andromeda/ASP/components/event/test/unit-test/emAutomatedTestSuite/emCont/emTestCases.c $
 * $Author: bkpavan $
 * $Date: 2006/09/13 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#include "string.h"
#include "clDebugApi.h"
#include "clCksmApi.h"
#include "clTimerApi.h"
#include "clOsalApi.h"
#include <clIocApi.h>
#include <clCpmApi.h>
#include <clEventApi.h>
#include <clEventExtApi.h>
#include <clEventErrors.h>
#include "emTestTemplate.h"
#include "../common/emAutoCommon.h"
#include "../common/emTestPatterns.h"
#include "emCont.h"

#if 0
# define INTERRUPTIBLE
#endif

#if 0
# define CKPT_TEST
#endif

#define TC0 "TC0"
#define TC1 "TC1"
#define TC2 "TC2"
#define TC3 "TC3"
#define TC4 "TC4"
#define TC5 "TC5"
#define TC6 "TC6"
#define TC7 "TC7"
#define TC8 "TC8"
#define TC9 "TC9"

#define TC12 "TC12"

#define APP_A "TestNode2_SU0_Comp0"
#define APP_B "TestNode3_SU0_Comp0"
#define APP_C "TestNode4_SU0_Comp0"


/*** Time Out value ***/
#define CL_EVT_TEST_1_SEC 1000*1
#define CL_EVT_TEST_2_SEC 1000*2
#define CL_EVT_TEST_3_SEC 1000*3
#define CL_EVT_TEST_4_SEC 1000*4
#define CL_EVT_TEST_5_SEC 1000*5


#define APP_A_COM 0x0000        /* 0 since dynamically generated */
#define APP_B_COM 0x0000        /* 0 since dynamically generated */
#define APP_C_COM 0x0000        /* 0 since dynamically generated */

#define APP_A_IOC 0x3
#define APP_B_IOC 0x4
#define APP_C_IOC 0x5

ClEvtContAppToIocAddrT gEvtAppToIoc[] = {
    {
     CL_NAME_SET(APP_A), {APP_A_IOC, APP_A_COM}, 0,
     },
    {
     CL_NAME_SET(APP_B), {APP_B_IOC, APP_B_COM}, 0,
     },
    {
     CL_NAME_SET(APP_C), {APP_C_IOC, APP_C_COM}, 0,
     },
};


ClRcT clEvtContAppAddressGet(void)
{
    ClRcT rc = CL_OK;

    ClIocAddressT compAddress;

    ClUint8T i = 0;

    for (i = 0; i < 3; i++)
    {
        rc = clCpmComponentAddressGet(gEvtAppToIoc[i].iocPhyAddr.nodeAddress,
                                      &gEvtAppToIoc[i].appName, &compAddress);
        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                           ("Component [%.*s] Address Get Failed [0x%X]\r\n",
                            gEvtAppToIoc[i].appName.length,
                            gEvtAppToIoc[i].appName.value, rc));
            return rc;
        }
        gEvtAppToIoc[i].iocPhyAddr = compAddress.iocPhyAddress;

        CL_DEBUG_PRINT(CL_DEBUG_TRACE,
                       ("The IOC Address of %.*s is Node %lX Port %lX\n\r\n",
                        gEvtAppToIoc[i].appName.length,
                        gEvtAppToIoc[i].appName.value,
                        (unsigned long) gEvtAppToIoc[i].iocPhyAddr.nodeAddress,
                        (unsigned long) gEvtAppToIoc[i].iocPhyAddr.portId));
    }

    return CL_OK;
}

#define INIT   "App_Init"
ClNameT gTCInit = CL_NAME_SET(INIT);

ClNameT gTCNode[] = {
    CL_NAME_SET("TestNode2"),
    CL_NAME_SET("TestNode3"),
    CL_NAME_SET("TestNode4"),
};



#if 0
/*** Test Case 0 ***/

/*
 ** The -ve test cases - Passing invalid arguments to the various API.
 */

ClEvtContChOpenT gTC0Open[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET(TC0),
     CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_SUBSCRIBER |
     CL_EVENT_CHANNEL_PUBLISHER, -1},
};

ClEvtContChCloseT gTC0Close[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET(TC0), CL_EVENT_GLOBAL_CHANNEL}
};

ClEvtContSubT gTC0Sub[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET(TC0), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_DEFAULT_PATTERN, 1, (void *) 0x100},
    {CL_NAME_SET(INIT), CL_NAME_SET(TC0), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_DEFAULT_PATTERN, 2, (void *) 0x200},
};

ClEvtContAllocT gTC0Alloc[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET(TC0), CL_EVENT_GLOBAL_CHANNEL},
};

ClEvtContAttrSetT gTC0AttrSet[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET(TC0), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_DEFAULT_PATTERN, 1, 0, CL_NAME_SET(APP_B)},
};

ClEvtContPubT gTC0Pub[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET(TC0), CL_EVENT_GLOBAL_CHANNEL, sizeof(TC0),
     TC0, CL_EVT_TEST_2_SEC}
    ,
};
#endif



/*** Test Case 1 ***/

ClEvtContChOpenT gTC1Open[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET(TC1),
     CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_SUBSCRIBER |
     CL_EVENT_CHANNEL_PUBLISHER, -1},
};

ClEvtContChCloseT gTC1Close[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET(TC1), CL_EVENT_GLOBAL_CHANNEL}
};

ClEvtContSubT gTC1Sub[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET(TC1), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_STRUCT_PATTERN, 1, (void *) 0x100},
    {CL_NAME_SET(INIT), CL_NAME_SET(TC1), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_STR_PATTERN, 2, (void *) 0x200},
};

ClEvtContAllocT gTC1Alloc[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET(TC1), CL_EVENT_GLOBAL_CHANNEL},
};

ClEvtContAttrSetT gTC1AttrSet[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET(TC1), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_STR_PATTERN, 1, 0, CL_NAME_SET(APP_B)},
};

ClEvtContPubT gTC1Pub[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET(TC1), CL_EVENT_GLOBAL_CHANNEL, sizeof(TC1),
     TC1, CL_EVT_TEST_2_SEC}
    ,
};

ClEvtContChOpenT gTC12Open[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET(TC12),
     CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_SUBSCRIBER |
     CL_EVENT_CHANNEL_PUBLISHER, -1},
};

ClEvtContChCloseT gTC12Close[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET(TC12), CL_EVENT_GLOBAL_CHANNEL}
};

ClEvtContSubT gTC12Sub1[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET(TC12), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_STRUCT_PATTERN, 1, (void *) 0x100},
};


ClEvtContAllocT gTC12Alloc[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET(TC12), CL_EVENT_GLOBAL_CHANNEL},
};

ClEvtContAttrSetT gTC12AttrSet[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET(TC12), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_STR_PATTERN, 1, 0, CL_NAME_SET(APP_B)},
};

ClEvtContPubT gTC12Pub[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET(TC12), CL_EVENT_GLOBAL_CHANNEL,
     sizeof(TC12), TC12, CL_EVT_TEST_2_SEC}
    ,
};

ClEvtContTestHeadT gTC1Head[] = {
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_INIT, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC1Open,
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC1Sub[0],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC1Sub[1],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_INIT, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC1Open,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_ALLOC, CL_OK, (void *) &gTC1Alloc,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SET_ATTR, CL_OK, (void *) &gTC1AttrSet,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC1Pub,
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC12Open,
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC12Sub1,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC12Open,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_ALLOC, CL_OK, (void *) &gTC12Alloc,
     },
#if 0                           /* testing Default publish */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SET_ATTR, CL_OK, (void *) &gTC12AttrSet,
     },
#endif
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC12Pub,
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_FIN, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_FIN, CL_OK, (void *) &gTCInit,
     },
};


/*** Test Case 2 - Test if local pulbish goes to others ***/

ClEvtContChOpenT gTC2Open[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"),
     CL_EVENT_LOCAL_CHANNEL | CL_EVENT_CHANNEL_SUBSCRIBER, -1},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"),
     CL_EVENT_LOCAL_CHANNEL | CL_EVENT_CHANNEL_PUBLISHER |
     CL_EVENT_CHANNEL_SUBSCRIBER, -1}
};

ClEvtContChCloseT gTC2Close[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_LOCAL_CHANNEL}
};

ClEvtContSubT gTC2Sub[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_LOCAL_CHANNEL,
     CL_EVT_DEFAULT_PATTERN, 0xA00, (void *) 0xA00},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_LOCAL_CHANNEL,
     CL_EVT_DEFAULT_PATTERN, 0xB00, (void *) 0xB00},
};

ClEvtContAllocT gTC2Alloc[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_LOCAL_CHANNEL},
};

ClEvtContAttrSetT gTC2AttrSet[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_LOCAL_CHANNEL,
     CL_EVT_DEFAULT_PATTERN, 1, 0, CL_NAME_SET(APP_B)},
};

ClEvtContPubT gTC2Pub[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_LOCAL_CHANNEL,
     sizeof(TC2), TC2, CL_EVT_TEST_2_SEC}
    ,
};


ClEvtContTestHeadT gTC2Head[] = {
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_INIT, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_INIT, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC2Open[0],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC2Open[1],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC2Sub[0],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC2Sub[1],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_ALLOC, CL_OK, (void *) &gTC2Alloc,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SET_ATTR, CL_OK, (void *) &gTC2AttrSet,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC2Pub,
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_FIN, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_FIN, CL_OK, (void *) &gTCInit,
     },
};

/*** Test Case 3 ***/
/*
 * 1.  Initialize on A & B. 2.  Open chanAsBp0. 3.  Open chanAsBp1. 4.  Open
 * chanAsBp2. 5.  Subscribe to Default Pattern on AsBp1 0xA00. 6.  Subscribe to 
 * Default Pattern on AsBp1 0xA02. 7.  Subscribe to Default Pattern on AsBp1
 * 0xA03. 8.  Subscribe to Struct Pattern on AsBp2 0xA10. 9.  Subscribe to
 * Struct Pattern on AsBp2 0xA11. 10. Subscribe to Struct Pattern on AsBp2
 * 0xA12. 11. Subscribe to Default Pattern on AsBp3 0xA20. FIXME - put another
 * pattern 12. Subscribe to Default Pattern on AsBp3 0xA21. 13. Subscribe to
 * Default Pattern on AsBp3 0xA22. 14. Unsubscribe 0xA00. 15. Unsubscribe
 * 0xA11. 16. Unsubscribe 0xA22. 17. Publish Default Pattern on AB0. 18. Only
 * 0xA12 & 0xA02 should be notified. 19. Publish Struct Pattern on AsBp1. 20.
 * Only 0xA10 & 0xA12 should be notified. 21. Close AsBp1 on A. 22. Publish
 * Struct Pattern on AsBp1. 23. Nobody gets notified. 24. Publish Default
 * Pattern on AsBp2. 25. Only 0xA20 & 0xA21 should be notified. 26. Finalize on 
 * A & B. 
 */

ClEvtContChOpenT gTC3Open4Sub[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp0"),
     CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_SUBSCRIBER, -1},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp1"),
     CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_SUBSCRIBER, -1},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp2"),
     CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_SUBSCRIBER, -1},
};

ClEvtContChOpenT gTC3Open4Pub[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp0"),
     CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_PUBLISHER, -1},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp1"),
     CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_PUBLISHER, -1},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp2"),
     CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_PUBLISHER, -1},
};

ClEvtContChCloseT gTC3Close[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp0"), CL_EVENT_GLOBAL_CHANNEL},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp1"), CL_EVENT_GLOBAL_CHANNEL},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp2"), CL_EVENT_GLOBAL_CHANNEL},
};

ClEvtContSubT gTC3Sub[3][3] = {
    {
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp0"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_DEFAULT_PATTERN, 0xA00, (void *) 0xA00},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp0"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_DEFAULT_PATTERN, 0xA01, (void *) 0xA01},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp0"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_DEFAULT_PATTERN, 0xA02, (void *) 0xA02},
     },
    {
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp1"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_STRUCT_PATTERN, 0xA10, (void *) 0xA10},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp1"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_STRUCT_PATTERN, 0xA11, (void *) 0xA11},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp1"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_STRUCT_PATTERN, 0xA12, (void *) 0xA12},
     },
    {
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp2"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_DEFAULT_PATTERN, 0xA20, (void *) 0xA20},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp2"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_DEFAULT_PATTERN, 0xA21, (void *) 0xA21},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp2"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_DEFAULT_PATTERN, 0xA22, (void *) 0xA22},
     },
};

ClEvtContUnsubT gTC3Unsub[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp0"), CL_EVENT_GLOBAL_CHANNEL,
     0xA00},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp1"), CL_EVENT_GLOBAL_CHANNEL,
     0xA11},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp2"), CL_EVENT_GLOBAL_CHANNEL,
     0xA22},
};

ClEvtContAllocT gTC3Alloc[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp0"), CL_EVENT_GLOBAL_CHANNEL},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp1"), CL_EVENT_GLOBAL_CHANNEL},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp2"), CL_EVENT_GLOBAL_CHANNEL},
};

ClEvtContAttrSetT gTC3AttrSet[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp0"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_DEFAULT_PATTERN, 1, 0, CL_NAME_SET(APP_B)},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp1"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_STRUCT_PATTERN, 2, 0, CL_NAME_SET(APP_B)},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp2"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_DEFAULT_PATTERN, 3, 0, CL_NAME_SET(APP_B)},
};

ClEvtContPubT gTC3Pub[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp0"), CL_EVENT_GLOBAL_CHANNEL,
     sizeof(TC3), TC3, CL_EVT_TEST_2_SEC}
    ,
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp1"), CL_EVENT_GLOBAL_CHANNEL,
     sizeof(TC3), TC3, CL_EVT_TEST_2_SEC}
    ,
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp2"), CL_EVENT_GLOBAL_CHANNEL,
     sizeof(TC3), TC3, CL_EVT_TEST_2_SEC}
    ,
};

ClEvtContTestHeadT gTC3Head[] = {
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_INIT, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_INIT, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC3Open4Sub[0],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC3Open4Sub[1],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC3Open4Sub[2],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC3Open4Pub[0],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC3Open4Pub[1],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC3Open4Pub[2],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC3Sub[0][0],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC3Sub[0][1],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC3Sub[0][2],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC3Sub[1][0],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC3Sub[1][1],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC3Sub[1][2],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC3Sub[2][0],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC3Sub[2][1],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC3Sub[2][2],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_UNSUB, CL_OK, (void *) &gTC3Unsub[0],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_UNSUB, CL_OK, (void *) &gTC3Unsub[1],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_UNSUB, CL_OK, (void *) &gTC3Unsub[2],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_ALLOC, CL_OK, (void *) &gTC3Alloc[0],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SET_ATTR, CL_OK, (void *) &gTC3AttrSet[0],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC3Pub[0],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_ALLOC, CL_OK, (void *) &gTC3Alloc[1],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SET_ATTR, CL_OK, (void *) &gTC3AttrSet[1],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC3Pub[1],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_CLOSE, CL_OK, (void *) &gTC3Close[1],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC3Pub[1],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_ALLOC, CL_OK, (void *) &gTC3Alloc[2],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SET_ATTR, CL_OK, (void *) &gTC3AttrSet[2],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC3Pub[2],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_FIN, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_FIN, CL_OK, (void *) &gTCInit,
     },
};

/*** Test Case 4 ***/

ClEvtContChOpenT gTC4Open4Sub[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBbCb"),
     CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_SUBSCRIBER, -1},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"),
     CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_SUBSCRIBER, -1}
};

ClEvtContChOpenT gTC4Open4Both[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBbCb"),
     CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_SUBSCRIBER |
     CL_EVENT_CHANNEL_PUBLISHER, -1},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"),
     CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_SUBSCRIBER |
     CL_EVENT_CHANNEL_PUBLISHER, -1}
};

ClEvtContSubT gTC4SubABC[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBbCb"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_STRUCT_PATTERN, 0xA00, (void *) 0xA00},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBbCb"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_STRUCT_PATTERN, 0xA01, (void *) 0xA01},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBbCb"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_STRUCT_PATTERN, 0xA02, (void *) 0xA02},
};

ClEvtContSubT gTC4SubAB[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_DEFAULT_PATTERN, 0xA10, (void *) 0xA10},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_DEFAULT_PATTERN, 0xA11, (void *) 0xA11},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_DEFAULT_PATTERN, 0xA12, (void *) 0xA12},
};

ClEvtContChCloseT gTC4Close[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBbCb"), CL_EVENT_GLOBAL_CHANNEL},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_GLOBAL_CHANNEL},
};

ClEvtContChOpenT gTC4Open4SubBC[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp0"),
     CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_SUBSCRIBER, -1},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp1"),
     CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_SUBSCRIBER, -1},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp2"),
     CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_SUBSCRIBER, -1},
};

ClEvtContChOpenT gTC4Open4PubBC[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp0"),
     CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_PUBLISHER, -1},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp1"),
     CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_PUBLISHER, -1},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp2"),
     CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_PUBLISHER, -1},
};

ClEvtContChCloseT gTC4CloseBC[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp0"), CL_EVENT_GLOBAL_CHANNEL},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp1"), CL_EVENT_GLOBAL_CHANNEL},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp2"), CL_EVENT_GLOBAL_CHANNEL},
};

ClEvtContSubT gTC4SubBC[3][3] = {
    {
     {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp0"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_DEFAULT_PATTERN, 0xB00, (void *) 0xB00},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp0"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_DEFAULT_PATTERN, 0xB01, (void *) 0xB01},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp0"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_DEFAULT_PATTERN, 0xB02, (void *) 0xB02},
     },
    {
     {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp1"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_STRUCT_PATTERN, 0xB10, (void *) 0xB10},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp1"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_STRUCT_PATTERN, 0xB11, (void *) 0xB11},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp1"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_STRUCT_PATTERN, 0xB12, (void *) 0xB12},
     },
    {
     {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp2"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_DEFAULT_PATTERN, 0xB20, (void *) 0xB20},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp2"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_DEFAULT_PATTERN, 0xB21, (void *) 0xB21},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp2"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_DEFAULT_PATTERN, 0xB22, (void *) 0xB22},
     },
};

ClEvtContUnsubT gTC4UnsubBC[3][3] = {
    {
     {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp0"), CL_EVENT_GLOBAL_CHANNEL,
      0xB00},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp0"), CL_EVENT_GLOBAL_CHANNEL,
      0xB01},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp0"), CL_EVENT_GLOBAL_CHANNEL,
      0xB02},
     },
    {
     {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp1"), CL_EVENT_GLOBAL_CHANNEL,
      0xB10},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp1"), CL_EVENT_GLOBAL_CHANNEL,
      0xB11},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp1"), CL_EVENT_GLOBAL_CHANNEL,
      0xB12},
     },
    {
     {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp2"), CL_EVENT_GLOBAL_CHANNEL,
      0xB20},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp2"), CL_EVENT_GLOBAL_CHANNEL,
      0xB21},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp2"), CL_EVENT_GLOBAL_CHANNEL,
      0xB22},
     },
};

ClEvtContAllocT gTC4Alloc[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp0"), CL_EVENT_GLOBAL_CHANNEL},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp1"), CL_EVENT_GLOBAL_CHANNEL},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp2"), CL_EVENT_GLOBAL_CHANNEL},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBbCb"), CL_EVENT_GLOBAL_CHANNEL},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_GLOBAL_CHANNEL},
};

ClEvtContAttrSetT gTC4AttrSet[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp0"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_DEFAULT_PATTERN, 1, 0, CL_NAME_SET(APP_B)},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp1"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_STRUCT_PATTERN, 2, 0, CL_NAME_SET(APP_B)},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp2"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_DEFAULT_PATTERN, 3, 0, CL_NAME_SET(APP_B)},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBbCb"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_STRUCT_PATTERN, 1, 0, CL_NAME_SET(APP_C)},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_DEFAULT_PATTERN, 1, 0, CL_NAME_SET(APP_B)},
};

ClEvtContPubT gTC4Pub[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp0"), CL_EVENT_GLOBAL_CHANNEL,
     sizeof(TC4), TC4, CL_EVT_TEST_2_SEC}
    ,
    {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp1"), CL_EVENT_GLOBAL_CHANNEL,
     sizeof(TC4), TC4, CL_EVT_TEST_2_SEC}
    ,
    {CL_NAME_SET(INIT), CL_NAME_SET("chanBsCp2"), CL_EVENT_GLOBAL_CHANNEL,
     sizeof(TC4), TC4, CL_EVT_TEST_2_SEC}
    ,
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBbCb"), CL_EVENT_GLOBAL_CHANNEL,
     sizeof(TC4), TC4, CL_EVT_TEST_2_SEC}
    ,
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_GLOBAL_CHANNEL,
     sizeof(TC4), TC4, CL_EVT_TEST_2_SEC}
    ,
};

ClEvtContTestHeadT gTC4Head[] = {
    /*
     * 1. Initialize A, B & C 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_INIT, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_INIT, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_C), CL_EVT_CONT_INIT, CL_OK, (void *) &gTCInit,
     },

    /*
     * 2. Open chanAsBbCb 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC4Open4Sub[0],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC4Open4Both[0],
     },
    {
     CL_NAME_SET(APP_C), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC4Open4Both[0],
     },

    /*
     * 3. Open chanAsBb 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC4Open4Sub[1],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC4Open4Both[1],
     },

    /*
     * 4. Open ChanBsCb0 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC4Open4SubBC[0],
     },
    {
     CL_NAME_SET(APP_C), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC4Open4PubBC[0],
     },

    /*
     * 5. Open ChanBsCb1 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC4Open4SubBC[1],
     },
    {
     CL_NAME_SET(APP_C), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC4Open4PubBC[1],
     },

    /*
     * 6. Open ChanBsCb2 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC4Open4SubBC[2],
     },
    {
     CL_NAME_SET(APP_C), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC4Open4PubBC[2],
     },

    /*
     * 7. Subscribe to on ABC - 0xA00, 0xA01, 0xA02 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC4SubABC[0],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC4SubABC[1],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC4SubABC[2],
     },

    /*
     * 8. Subscribe to on AB - 0xA10, 0xA11, 0xA12 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC4SubAB[0],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC4SubAB[1],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC4SubAB[2],
     },

    /*
     * 9. Subscribe to on BC0 thru BC2 - 0xA00 thru 0xA22 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC4SubBC[0][0],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC4SubBC[0][1],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC4SubBC[0][2],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC4SubBC[1][0],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC4SubBC[1][1],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC4SubBC[1][2],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC4SubBC[2][0],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC4SubBC[2][1],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC4SubBC[2][2],
     },


    /*
     * 10. B Publishes Default Pattern on AB. 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_ALLOC, CL_OK, (void *) &gTC4Alloc[4],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SET_ATTR, CL_OK, (void *) &gTC4AttrSet[4],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC4Pub[4],
     },


    /*
     * 11. C Publishes Struct Pattern on ABC. 
     */
    {
     CL_NAME_SET(APP_C), CL_EVT_CONT_ALLOC, CL_OK, (void *) &gTC4Alloc[3],
     },
    {
     CL_NAME_SET(APP_C), CL_EVT_CONT_SET_ATTR, CL_OK, (void *) &gTC4AttrSet[3],
     },
    {
     CL_NAME_SET(APP_C), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC4Pub[3],
     },

    /*
     * 12. Close AB. 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_CLOSE, CL_OK, (void *) &gTC4Close[1],
     },

    /*
     * 13. C Publishes Struct Pattern on BC0. 
     */
    {
     CL_NAME_SET(APP_C), CL_EVT_CONT_ALLOC, CL_OK, (void *) &gTC4Alloc[0],
     },
    {
     CL_NAME_SET(APP_C), CL_EVT_CONT_SET_ATTR, CL_OK, (void *) &gTC4AttrSet[0],
     },
    {
     CL_NAME_SET(APP_C), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC4Pub[0],
     },

    /*
     * 14. Unsubscribe 0xB10 & 0xB12 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_UNSUB, CL_OK, (void *) &gTC4UnsubBC[1][0],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_UNSUB, CL_OK, (void *) &gTC4UnsubBC[1][2],
     },

    /*
     * 15. C Publishes Struct Pattern on BC1. 
     */
    {
     CL_NAME_SET(APP_C), CL_EVT_CONT_ALLOC, CL_OK, (void *) &gTC4Alloc[1],
     },
    {
     CL_NAME_SET(APP_C), CL_EVT_CONT_SET_ATTR, CL_OK, (void *) &gTC4AttrSet[1],
     },
    {
     CL_NAME_SET(APP_C), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC4Pub[1],
     },

    /*
     * 16. Unsubscribe 0xB22. 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_UNSUB, CL_OK, (void *) &gTC4UnsubBC[2][2],
     },

    /*
     * 17. C Publishes Struct Pattern on BC2 
     */
    {
     CL_NAME_SET(APP_C), CL_EVT_CONT_ALLOC, CL_OK, (void *) &gTC4Alloc[2],
     },
    {
     CL_NAME_SET(APP_C), CL_EVT_CONT_SET_ATTR, CL_OK, (void *) &gTC4AttrSet[2],
     },
    {
     CL_NAME_SET(APP_C), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC4Pub[2],
     },

#ifdef CKPT_TEST
    /*
     * kill & restart 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_KILL, CL_OK, (void *) &gTCNode[1],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_RESTART, CL_OK, (void *) &gTCNode[1],
     },
#endif

    /*
     * C Publishes Struct Pattern BC0 
     */
    {
     CL_NAME_SET(APP_C), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC4Pub[0],
     },

    /*
     * Close BC0 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_CLOSE, CL_OK, (void *) &gTC4CloseBC[0],
     },
    {
     CL_NAME_SET(APP_C), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC4Pub[1],
     },

    /*
     * C Publishes Struct Pattern BC0 
     */
    {
     CL_NAME_SET(APP_C), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC4Pub[0],
     },

#ifdef CKPT_TEST
    /*
     * kill & restart 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_KILL, CL_OK, (void *) &gTCNode[1],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_RESTART, CL_OK, (void *) &gTCNode[1],
     },
#endif

    /*
     * B Publishes Default Pattern on AB. 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC4Pub[4],
     },

    /*
     * Finalize on A 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_FIN, CL_OK, (void *) &gTCInit,
     },

    /*
     * B Publishes Default Pattern on AB. 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC4Pub[4],
     },

#ifdef CKPT_TEST
    /*
     * kill & restart 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_KILL, CL_OK, (void *) &gTCNode[1],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_RESTART, CL_OK, (void *) &gTCNode[1],
     },
#endif


    /*
     * C Publishes Struct Pattern on ABC. 
     */
    {
     CL_NAME_SET(APP_C), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC4Pub[3],
     },

    /*
     * Unsubscribe 0xB20 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_UNSUB, CL_OK, (void *) &gTC4UnsubBC[2][0],
     },

    /*
     * C Publishes Struct Pattern on BC2 
     */
    {
     CL_NAME_SET(APP_C), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC4Pub[2],
     },

    /*
     * Finalize on B & C 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_FIN, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_C), CL_EVT_CONT_FIN, CL_OK, (void *) &gTCInit,
     },
};

/*** Test Case 5 - Superset & Subset patterns/filters ***/

ClEvtContChOpenT gTC5Open[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp"),
     CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_SUBSCRIBER |
     CL_EVENT_CHANNEL_PUBLISHER, -1},
};

ClEvtContChCloseT gTC5Close[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp"), CL_EVENT_GLOBAL_CHANNEL}
};

ClEvtContSubT gTC5Sub[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_NORMAL_PATTERN, 0xA00, (void *) 0xA00},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_SUPERSET_PATTERN, 0xA01, (void *) 0xA01},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_SUBSET_PATTERN, 0xA02, (void *) 0xA02},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_NORMAL_PASS_ALL_PATTERN, 0xA03, (void *) 0xA03},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_SUBSET_PASS_ALL_PATTERN, 0xA04, (void *) 0xA04},
};

ClEvtContAllocT gTC5Alloc[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp"), CL_EVENT_GLOBAL_CHANNEL},
};

ClEvtContAttrSetT gTC5AttrSet[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_NORMAL_PATTERN, 1, 0, CL_NAME_SET(APP_B)},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_SUPERSET_PATTERN, 1, 0, CL_NAME_SET(APP_B)},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_SUBSET_PATTERN, 1, 0, CL_NAME_SET(APP_B)},
};

ClEvtContPubT gTC5Pub[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBp"), CL_EVENT_GLOBAL_CHANNEL,
     sizeof(TC5), TC5, CL_EVT_TEST_2_SEC}
    ,
};

ClEvtContTestHeadT gTC5Head[] = {
    /*
     * 1. Initialize A & B. 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_INIT, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_INIT, CL_OK, (void *) &gTCInit,
     },

    /*
     * 2. Open chanAsBp. 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC5Open,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC5Open,
     },

    /*
     * 3. Subscribe to Normal Event. 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC5Sub[0],
     },

    /*
     * 4. Subscribe to Superset Event. 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC5Sub[1],
     },

    /*
     * 5. Subscribe to Subset Event. 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC5Sub[2],
     },

    /*
     * 6. Subscribe to Normal Pass ALL Event. 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC5Sub[3],
     },

    /*
     * 7. Subscribe to Subset Pass ALL Event. 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC5Sub[4],
     },

    /*
     * 8. B publishes Normal Event. 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_ALLOC, CL_OK, (void *) &gTC5Alloc,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SET_ATTR, CL_OK, (void *) &gTC5AttrSet[0],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC5Pub,
     },

#ifdef CKPT_TEST
    /*
     * kill & restart 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_KILL, CL_OK, (void *) &gTCNode[1],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_RESTART, CL_OK, (void *) &gTCNode[1],
     },

    /*
     * B publishes Normal Event 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC5Pub,
     },
#endif

    /*
     * B publishes Superset Event 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SET_ATTR, CL_OK, (void *) &gTC5AttrSet[1],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC5Pub,
     },

    /*
     * B publishes Subset Event 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SET_ATTR, CL_OK, (void *) &gTC5AttrSet[2],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC5Pub,
     },
    /*
     * Finalize on A & B 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_FIN, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_FIN, CL_OK, (void *) &gTCInit,
     },
};


/*** Test Case 6 - Duplicate Subscription per Channel (Bug 3177) ***/

ClEvtContChOpenT gTC6Open[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"),
     CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_SUBSCRIBER |
     CL_EVENT_CHANNEL_PUBLISHER, -1},
};

ClEvtContChCloseT gTC6Close[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL}
};

ClEvtContSubT gTC6Sub[3][6] = {
    {
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_DEFAULT_PATTERN, 0xA00, (void *) 0xA00},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_DEFAULT_PATTERN, 0xA01, (void *) 0xA01},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_DEFAULT_PATTERN, 0xA02, (void *) 0xA02},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_DEFAULT_PATTERN, 0xA17, (void *) 0xA03},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_DEFAULT_PATTERN, 0xA25, (void *) 0xA04},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_DEFAULT_PATTERN, 0xA02, (void *) 0xA05},
     },
    {
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_STRUCT_PATTERN, 0xA17, (void *) 0xA10},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_STRUCT_PATTERN, 0xA18, (void *) 0xA11},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_STRUCT_PATTERN, 0xA19, (void *) 0xA12},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_STRUCT_PATTERN, 0xA01, (void *) 0xA13},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_STRUCT_PATTERN, 0xA24, (void *) 0xA14},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_STRUCT_PATTERN, 0xA19, (void *) 0xA15},
     },
    {
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_STR_PATTERN, 0xA23, (void *) 0xA20},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_STR_PATTERN, 0xA24, (void *) 0xA21},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_STR_PATTERN, 0xA25, (void *) 0xA22},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_STR_PATTERN, 0xA00, (void *) 0xA23},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_STR_PATTERN, 0xA18, (void *) 0xA24},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
      CL_EVT_STR_PATTERN, 0xA23, (void *) 0xA25},
     },
};

ClEvtContUnsubT gTC6Unsub[3][3] = {
    {
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
      0xA00},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
      0xA01},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
      0xA02},
     },
    {
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
      0xA17},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
      0xA18},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
      0xA19},
     },
    {
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
      0xA23},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
      0xA24},
     {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
      0xA25},
     },
};

ClEvtContAllocT gTC6Alloc[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL},
};

ClEvtContAttrSetT gTC6AttrSet[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_DEFAULT_PATTERN, 1, 0, CL_NAME_SET(APP_B)},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_STRUCT_PATTERN, 1, 0, CL_NAME_SET(APP_B)},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_STR_PATTERN, 1, 0, CL_NAME_SET(APP_B)},
};

ClEvtContPubT gTC6Pub[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAbBb"), CL_EVENT_GLOBAL_CHANNEL,
     sizeof(TC6), TC6, CL_EVT_TEST_2_SEC}
    ,
};

ClEvtContTestHeadT gTC6Head[] = {
    /*
     * Initialize A & B. 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_INIT, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_INIT, CL_OK, (void *) &gTCInit,
     },

    /*
     * Open chanAbBb. 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC6Open,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC6Open,
     },

    /*
     * Subscribe to Default Event - 0xA00, 0xA01 & 0xA02. Should succeed. 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC6Sub[0][0],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC6Sub[0][1],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC6Sub[0][2],
     },

    /*
     * Subscribe to Struct Event - 0xA17, 0xA18 & 0xA19. Should succeed. 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC6Sub[1][0],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC6Sub[1][1],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC6Sub[1][2],
     },

    /*
     * Subscribe to Str Event - 0xA23, 0xA24 & 0xA25. Should succeed. 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC6Sub[2][0],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC6Sub[2][1],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC6Sub[2][2],
     },


    /*
     * Subscribe to Default Event. Should return CL_EVENT_ERR_EXIST. 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_EVENT_ERR_EXIST,
     (void *) &gTC6Sub[0][3],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_EVENT_ERR_EXIST,
     (void *) &gTC6Sub[0][4],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_EVENT_ERR_EXIST,
     (void *) &gTC6Sub[0][5],
     },

    /*
     * Subscribe to Struct Event. Should return CL_EVENT_ERR_EXIST. 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_EVENT_ERR_EXIST,
     (void *) &gTC6Sub[1][3],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_EVENT_ERR_EXIST,
     (void *) &gTC6Sub[1][4],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_EVENT_ERR_EXIST,
     (void *) &gTC6Sub[1][5],
     },

    /*
     * Subscribe to Str Event. Should return CL_EVENT_ERR_EXIST. 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_EVENT_ERR_EXIST,
     (void *) &gTC6Sub[2][3],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_EVENT_ERR_EXIST,
     (void *) &gTC6Sub[2][4],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_EVENT_ERR_EXIST,
     (void *) &gTC6Sub[2][5],
     },


    /*
     * Allocate Event for Publish 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_ALLOC, CL_OK, (void *) &gTC6Alloc,
     },

    /*
     * B publishes Default Event. 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SET_ATTR, CL_OK, (void *) &gTC6AttrSet[0],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC6Pub,
     },


    /*
     * B publishes Struct Event 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SET_ATTR, CL_OK, (void *) &gTC6AttrSet[1],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC6Pub,
     },

    /*
     * B publishes Str Event 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SET_ATTR, CL_OK, (void *) &gTC6AttrSet[2],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC6Pub,
     },

    /*
     * Unsubscribe the succesful subscriptions for Default Event 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_UNSUB, CL_OK, (void *) &gTC6Unsub[0][0],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_UNSUB, CL_OK, (void *) &gTC6Unsub[0][1],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_UNSUB, CL_OK, (void *) &gTC6Unsub[0][2],
     },

    /*
     * Unsubscribe the succesful subscriptions for Struct Event 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_UNSUB, CL_OK, (void *) &gTC6Unsub[1][0],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_UNSUB, CL_OK, (void *) &gTC6Unsub[1][1],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_UNSUB, CL_OK, (void *) &gTC6Unsub[1][2],
     },

    /*
     * Unsubscribe the succesful subscriptions for Str Event 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_UNSUB, CL_OK, (void *) &gTC6Unsub[2][0],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_UNSUB, CL_OK, (void *) &gTC6Unsub[2][1],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_UNSUB, CL_OK, (void *) &gTC6Unsub[2][2],
     },


    /*
     * Subscribe to Default Event. Should succeed. 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC6Sub[0][3],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC6Sub[0][4],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC6Sub[0][5],
     },

    /*
     * Subscribe to Struct Event. Should return CL_OK. 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC6Sub[1][3],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC6Sub[1][4],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC6Sub[1][5],
     },

    /*
     * Subscribe to Str Event. Should return CL_OK. 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC6Sub[2][3],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC6Sub[2][4],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC6Sub[2][5],
     },

    /*
     * B publishes Default Event. 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SET_ATTR, CL_OK, (void *) &gTC6AttrSet[0],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC6Pub,
     },


    /*
     * B publishes Struct Event 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SET_ATTR, CL_OK, (void *) &gTC6AttrSet[1],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC6Pub,
     },

    /*
     * B publishes Str Event 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SET_ATTR, CL_OK, (void *) &gTC6AttrSet[2],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC6Pub,
     },

    /*
     * Finalize on A & B 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_FIN, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_FIN, CL_OK, (void *) &gTCInit,
     },
};


/*** Test Case 7 - Test Drive for CKPT ***/

ClEvtContChOpenT gTC7Open[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"),
     CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_SUBSCRIBER, -1},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"),
     CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_PUBLISHER |
     CL_EVENT_CHANNEL_SUBSCRIBER, -1}
};

ClEvtContChCloseT gTC7Close[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_GLOBAL_CHANNEL},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_LOCAL_CHANNEL},
};

ClEvtContSubT gTC7Sub[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_DEFAULT_PATTERN, 0xA00, (void *) 0xA00},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_DEFAULT_PATTERN, 0xB00, (void *) 0xB00},
};

ClEvtContAllocT gTC7Alloc[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_GLOBAL_CHANNEL},
};

ClEvtContAttrSetT gTC7AttrSet[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_DEFAULT_PATTERN, 1, 0, CL_NAME_SET(APP_B)},
};

ClEvtContPubT gTC7Pub[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_GLOBAL_CHANNEL,
     sizeof(TC7), TC7, CL_EVT_TEST_2_SEC}
    ,
};

ClEvtContTestHeadT gTC7Head[] = {
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_INIT, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_INIT, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_FIN, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_INIT, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_FIN, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_INIT, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_FIN, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_INIT, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_FIN, CL_OK, (void *) &gTCInit,
     },
#if 1                           /* To test the threshold */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_INIT, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_FIN, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_INIT, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_FIN, CL_OK, (void *) &gTCInit,
     },
#endif
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_INIT, CL_OK, (void *) &gTCInit,
     },
    /*
     * Open the channels 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC7Open[0],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC7Open[1],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC7Sub[0],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC7Sub[1],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_ALLOC, CL_OK, (void *) &gTC7Alloc,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SET_ATTR, CL_OK, (void *) &gTC7AttrSet,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC7Pub,
     },
#ifdef CKPT_TEST
    /*
     * Kill & Restart 
     */
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_KILL, CL_OK, (void *) &gTCNode[1],
     },
    /*
     * FIXME 
     */
# if 0
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_RESTART, CL_OK, (void *) &gTCNode[1],
     },
# endif
#endif
    /*
     * Test if publish works 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC7Pub,
     },
#if 0                           /* To allow publis after recovery disable
                                 * Finalize */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_FIN, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_INIT, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC7Open[1],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC7Sub[1],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_ALLOC, CL_OK, (void *) &gTC7Alloc,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SET_ATTR, CL_OK, (void *) &gTC7AttrSet,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC7Pub,
     },
#endif
    /*
     * Test kill & publish again 
     */

    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_FIN, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_FIN, CL_OK, (void *) &gTCInit,
     },
};

/*** Test Case 8 - For Multiple ***/
ClEvtContChOpenT gTC8Open[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"),
     CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_SUBSCRIBER, -1},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"),
     CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_PUBLISHER |
     CL_EVENT_CHANNEL_SUBSCRIBER, -1}
};

ClEvtContChCloseT gTC8Close[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_GLOBAL_CHANNEL}
};

ClEvtContSubT gTC8Sub[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_DEFAULT_PASS_ALL_PATTERN, 0xA00, (void *) 0xA00},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_DEFAULT_PATTERN, 0xA01, (void *) 0xA01},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_DEFAULT_PASS_ALL_PATTERN, 0xB00, (void *) 0xB00},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_DEFAULT_PATTERN, 0xB01, (void *) 0xB01},
};

ClEvtContAllocT gTC8Alloc[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_GLOBAL_CHANNEL},
};

ClEvtContAttrSetT gTC8AttrSet[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_DEFAULT_PASS_ALL_PATTERN, 1, 0, CL_NAME_SET(APP_B)},
};

ClEvtContPubT gTC8Pub[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_GLOBAL_CHANNEL,
     sizeof(TC8), TC8, CL_EVT_TEST_2_SEC}
    ,
};


ClEvtContTestHeadT gTC8Head[] = {
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_INIT, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_INIT, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC8Open[0],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC8Open[1],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC8Sub[1],  /* First 
                                                                         * subscription 
                                                                         * on
                                                                         * specific 
                                                                         * wards 
                                                                         * generic 
                                                                         */
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC8Sub[0],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC8Sub[2],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC8Sub[3],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_ALLOC, CL_OK, (void *) &gTC8Alloc,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SET_ATTR, CL_OK, (void *) &gTC8AttrSet,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC8Pub,
     },
#if 1
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_FIN, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_FIN, CL_OK, (void *) &gTCInit,
     },
#endif
};


/*** Test Case 9 - Default Values for Event ***/
ClEvtContChOpenT gTC9Open[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"),
     CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_SUBSCRIBER, -1},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"),
     CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_PUBLISHER |
     CL_EVENT_CHANNEL_SUBSCRIBER, -1}
};

ClEvtContChCloseT gTC9Close[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_GLOBAL_CHANNEL}
};

ClEvtContSubT gTC9Sub[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_DEFAULT_PATTERN, 0xA00, (void *) 0xA00},
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_DEFAULT_PATTERN, 0xB00, (void *) 0xB00},
};

ClEvtContAllocT gTC9Alloc[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_GLOBAL_CHANNEL},
};

ClEvtContAttrSetT gTC9AttrSet[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_GLOBAL_CHANNEL,
     CL_EVT_DEFAULT_PATTERN, 1, 0, CL_NAME_SET(APP_B)},
};

ClEvtContPubT gTC9Pub[] = {
    {CL_NAME_SET(INIT), CL_NAME_SET("chanAsBb"), CL_EVENT_GLOBAL_CHANNEL,
     sizeof(TC9), TC9, CL_EVT_TEST_2_SEC}
    ,
};


ClEvtContTestHeadT gTC9Head[] = {
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_INIT, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_INIT, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC9Open[0],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_OPEN, CL_OK, (void *) &gTC9Open[1],
     },
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC9Sub[0],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SUB, CL_OK, (void *) &gTC9Sub[1],
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_ALLOC, CL_OK, (void *) &gTC9Alloc,
     },
#if 1
    /*
     * Publish With Default Values of Event 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC9Pub,
     },
#endif
    /*
     * Now set the Attributes 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_SET_ATTR, CL_OK, (void *) &gTC9AttrSet,
     },
    /*
     * Publish the Event with Attributes Set 
     */
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_PUB, CL_OK, (void *) &gTC9Pub,
     },
#if 1
    {
     CL_NAME_SET(APP_A), CL_EVT_CONT_FIN, CL_OK, (void *) &gTCInit,
     },
    {
     CL_NAME_SET(APP_B), CL_EVT_CONT_FIN, CL_OK, (void *) &gTCInit,
     },
#endif
};


ClEvtContTestCaseT gEmTestCases[] = {
#if 0
#endif
    {
     CL_NAME_SET(TC1), gTC1Head, sizeof(gTC1Head) / sizeof(ClEvtContTestHeadT)},
    {
     CL_NAME_SET(TC2), gTC2Head, sizeof(gTC2Head) / sizeof(ClEvtContTestHeadT)},
    {
     CL_NAME_SET(TC3), gTC3Head, sizeof(gTC3Head) / sizeof(ClEvtContTestHeadT)},
    {
     CL_NAME_SET(TC4), gTC4Head, sizeof(gTC4Head) / sizeof(ClEvtContTestHeadT)},
    {
     CL_NAME_SET(TC5), gTC5Head, sizeof(gTC5Head) / sizeof(ClEvtContTestHeadT)},
    {
     CL_NAME_SET(TC6), gTC6Head, sizeof(gTC6Head) / sizeof(ClEvtContTestHeadT)},
    {
     CL_NAME_SET(TC7), gTC7Head, sizeof(gTC7Head) / sizeof(ClEvtContTestHeadT)},
    {
     CL_NAME_SET(TC8), gTC8Head, sizeof(gTC8Head) / sizeof(ClEvtContTestHeadT)},
    {
     CL_NAME_SET(TC9), gTC9Head, sizeof(gTC9Head) / sizeof(ClEvtContTestHeadT)},
#if 0
#endif
};

ClRcT clEvtContValidateResult()
{
    ClUint32T noOfApp;
    ClUint32T i = 0;
    ClIocAddressT destAddr;

    clEvtContGetApp(&noOfApp);
    for (i = 0; i < noOfApp; i++)
    {
        destAddr.iocPhyAddress = gEvtAppToIoc[i].iocPhyAddr;
        clEvtContSubResultGet(destAddr, gEvtAppToIoc[i].noOfSubs);
    }

    return CL_OK;
}

void clEvtContGetApp(ClUint32T *pNoOfApp)
{

    *pNoOfApp = sizeof(gEvtAppToIoc) / sizeof(ClEvtContAppToIocAddrT);
}

void clEvtContSubInfoInc(ClNameT *appName)
{

    ClInt32T result;
    ClUint32T noOfApp;
    ClUint32T i;

    noOfApp = sizeof(gEvtAppToIoc) / sizeof(ClEvtContAppToIocAddrT);

    for (i = 0; i < noOfApp; i++)
    {
        result = clEvtContUtilsNameCmp(appName, &gEvtAppToIoc[i].appName);
        if (0 == result)
        {
            gEvtAppToIoc[i].noOfSubs++;
            break;
        }
    }


}

void clEvtContAppInfoReset()
{

    ClUint32T noOfApp;
    ClUint32T i;
    ClIocAddressT destAddr;

    noOfApp = sizeof(gEvtAppToIoc) / sizeof(ClEvtContAppToIocAddrT);

    for (i = 0; i < noOfApp; i++)
    {
        destAddr.iocPhyAddress = gEvtAppToIoc[i].iocPhyAddr;
        clEvtContRest(destAddr);
        gEvtAppToIoc[i].noOfSubs = 0;
    }
}

ClRcT clEvtContParseTestInfo()
{
    ClUint32T noOfTestCase = 0;
    ClUint32T noOfSteps = 0;
    ClEvtContTestHeadT *pTestHead = NULL;
    ClUint32T i = 0;
    ClUint32T j = 0;
    ClRcT retCode;


    noOfTestCase = sizeof(gEmTestCases) / sizeof(ClEvtContTestCaseT);

    for (i = 0; i < noOfTestCase; i++)
    {
        noOfSteps = gEmTestCases[i].noOfSteps;
        pTestHead = gEmTestCases[i].pTestHead;

        clOsalPrintf(" <<<<<<------------ %s ---------->>>>>  \r\n",
                     gEmTestCases[i].testCaseName.value);

        for (j = 0; j < noOfSteps; j++)
        {
            switch (pTestHead[j].operation)
            {
                case CL_EVT_CONT_INIT:
                    clEvtContInit(&pTestHead[j], &retCode);
                    break;
                case CL_EVT_CONT_FIN:
                    clEvtContFin(&pTestHead[j], &retCode);
                    break;
                case CL_EVT_CONT_OPEN:
                    clEvtContOpen(&pTestHead[j], &retCode);
                    break;
                case CL_EVT_CONT_CLOSE:
                    clEvntContClose(&pTestHead[j], &retCode);
                    break;
                case CL_EVT_CONT_SUB:
                    clEvntContSub(&pTestHead[j], &retCode);
                    break;
                case CL_EVT_CONT_UNSUB:
                    clEvntContUnsub(&pTestHead[j], &retCode);
                    break;
                case CL_EVT_CONT_PUB:
                    clEvntContPub(&pTestHead[j], &retCode);
                    break;
                case CL_EVT_CONT_ALLOC:
                    clEvntContAlloc(&pTestHead[j], &retCode);
                    break;
                case CL_EVT_CONT_SET_ATTR:
                    clEvntContSetAttr(&pTestHead[j], &retCode);
                    break;
                case CL_EVT_CONT_GET_ATTR:
                    clEvntContGetAttr(&pTestHead[j], &retCode);
                    break;
                case CL_EVT_CONT_KILL:
                    clEvntContKill(&pTestHead[j], &retCode);
                    break;
                case CL_EVT_CONT_RESTART:
                    clEvntContRestart(&pTestHead[j], &retCode);
                    break;
            }

            clEvtContResultPrint(retCode, &pTestHead[j]);

        }
#ifdef INTERRUPTIBLE
        getchar();
#endif
    }

    return CL_OK;
}

ClRcT clEvtContIocAddreGet(ClNameT *appName, ClIocPhysicalAddressT *pIocAddress)
{
    ClUint32T noOfEntry = 0;
    ClUint32T i = 0;
    ClRcT rc = CL_OK;

    noOfEntry = sizeof(gEvtAppToIoc) / sizeof(ClEvtContAppToIocAddrT);

    for (i = 0; i < noOfEntry; i++)
    {
        rc = clEvtContUtilsNameCmp(&gEvtAppToIoc[i].appName, appName);
        if (CL_OK == rc)
        {
            memcpy(pIocAddress, &gEvtAppToIoc[i].iocPhyAddr,
                   sizeof(ClIocPhysicalAddressT));
            return CL_OK;
        }
    }
    return CL_ERR_NOT_EXIST;;
}
