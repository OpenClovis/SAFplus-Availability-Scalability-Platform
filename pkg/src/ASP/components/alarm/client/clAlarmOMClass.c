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
 * ModuleName  : alarm                                                         
 * File        : clAlarmOMClass.c
 *******************************************************************************/

/**************************************************************************
 * Description :                                                           
 * This module contains definitions for the alarmOMClass.                 
 **************************************************************************/

/* standard includes */
#include <stdlib.h>
#include <string.h>

/* ASP includes */
#include <clCommon.h>
#include <clDebugApi.h>
#include <clCorApi.h>
#include <clCorUtilityApi.h>
#include <clOmApi.h>

/* alarm includes */
#include <clAlarmOMIpi.h>
#include <clAlarmErrors.h>
#include "clAlarmOmClass.h"

#define CL_ALARM_OM_CLASS_NAME           "alarmOMClass"
#define CL_ALARM_OM_CLASS_VERSION        0x001
#define CL_ALARM_OM_CLASS_METHOD_TABLE   (tFunc**)&CL_OM_ALARM_CLASSMethodsMapping
#define CL_ALARM_OM_CLASS_MAX_INSTANCES  100
#define CL_ALARM_OM_CLASS_MAX_SLOTS      1
#define CL_ALARM_MAX_OM_OBJ 256


ClRcT
clAlarmOmClassConstructor( void *pThis ,
                                void  *pUsrData,
                                ClUint32T usrDataLen )
{
        return CL_OK;
}
                                                                                   
ClRcT
clAlarmOmClassDestructor( void *pThis ,
                              void  *pUsrData,
                              ClUint32T usrDataLen )
{
        return CL_OK;
}



ClOmClassControlBlockT omAlarmClassTbl[] =
{
    {
      CL_ALARM_OM_CLASS_NAME,        /* object */
      sizeof(CL_OM_ALARM_CLASS),       /* size */
      CL_OM_BASE_CLASS_TYPE,       /* extends from */
      clAlarmOmClassConstructor,   /* constructor */
      clAlarmOmClassDestructor,    /* destructor */
      NULL,                     /* pointer to methods struct */
      CL_ALARM_OM_CLASS_VERSION,     /* version */
      0,                          /* Instance table ptr */
      CL_ALARM_MAX_OM_OBJ,     /* Maximum number of classes */
      0,                          /* cur instance count */
      CL_ALARM_OM_CLASS_MAX_SLOTS,   /* max slots */
      CL_OM_ALARM_CLASS_TYPE/* my class type */
     }
};

/**@#-*/

/* FORWARD DECLARATION */


/*externs*/


/**@#+*/

/*
 *
 *
 */

ClRcT clAlarmOmObjectOperation(ClCorMOIdPtrT pMoId, 
                                        ClUint32T attrId, 
                                        ClUint32T idx, 
                                        void* val, 
                                        ClAlmOMOperationT oprId)
{
    ClRcT rc = CL_OK;
    CL_OM_ALARM_CLASS*        objPtr = NULL;

    rc = clOmMoIdToOmObjectReferenceGet(pMoId, (void*)&objPtr);
    if (rc != CL_OK)
    {
        clLogError("ALM", "OME", "Failed to get the OM object reference for the "
                             " attrId [%d]. rc[0x%0x] ", attrId, rc);
        rc = CL_ALARM_RC(CL_ERR_INVALID_PARAMETER);
        return rc;
    }


    if ((oprId != CL_ALARM_OM_OPERATION_SET) && (oprId != CL_ALARM_OM_OPERATION_GET))
    {
        rc = CL_ALARM_RC(CL_ERR_INVALID_PARAMETER);
        clLogNotice("ALM", "OME",  "Invalid attrid specified, attrId [%d]", attrId);
        return (rc);
    }
    if (idx >= CL_ALARM_MAX_ALARMS) 
    {
        rc = CL_ALARM_RC(CL_ERR_INVALID_PARAMETER);
        clLogNotice ("ALM", "OME",  " Invalid alarm index specified, index [%d]", idx);
        return (rc);    
    }        

    /* lock the mutex and set the value */ 
    if ((rc = clOsalMutexLock(objPtr->cllock))!= CL_OK )
    {
        clLogError("ALM", "OME", "Unable to lock the mutex. rc [0x%x]", rc);
        return rc;
    }

    switch (attrId)
    {
        case CL_ALARM_ELAPSED_POLLING_INTERVAL:
            if (oprId == CL_ALARM_OM_OPERATION_SET)
            {
                memcpy (&(objPtr->elapsedPollingInterval), val, 
                        sizeof (ClUint32T));
            }
            else
            {
                memcpy (val, &(objPtr->elapsedPollingInterval), 
                        sizeof (ClUint32T));
            }
            break;
        case CL_ALARM_CURRENT_ALMS_BITMAP:
            if (oprId == CL_ALARM_OM_OPERATION_SET)
            {
                memcpy (&(objPtr->clcurrentAlmsBitMap), val, sizeof (ClUint64T));
            }
            else
            {
                memcpy(val, &(objPtr->clcurrentAlmsBitMap), sizeof(ClUint64T));
            }
            break;
        case CL_ALARM_ALMS_UNDER_SOAKING:
            if (oprId == CL_ALARM_OM_OPERATION_SET)
            {
                memcpy (&(objPtr->clalmsUnderSoakingBitMap), val, 
                        sizeof (ClUint64T));
            }
            else
            {
                memcpy (val, &(objPtr->clalmsUnderSoakingBitMap), 
                        sizeof (ClUint64T));
            }
            break;
        case CL_ALARM_ELAPSED_ASSERT_SOAK_TIME:
            if (oprId == CL_ALARM_OM_OPERATION_SET)
            {
                memcpy (&(objPtr->alarmTransInfo[idx].elapsedAssertSoakingTime), val,
                        sizeof (ClUint32T));    
            }
            else
            {
                memcpy (val, &(objPtr->alarmTransInfo[idx].elapsedAssertSoakingTime),
                        sizeof (ClUint32T));
            }                
            break;
        case CL_ALARM_ELAPSED_CLEAR_SOAK_TIME:
            if (oprId == CL_ALARM_OM_OPERATION_SET)
            {
                memcpy (&(objPtr->alarmTransInfo[idx].elapsedClearSoakingTime), val, 
                        sizeof (ClUint32T));
            }
            else
            {
                memcpy (val, &(objPtr->alarmTransInfo[idx].elapsedClearSoakingTime), 
                        sizeof (ClUint32T));
            }
            break;
        case CL_ALARM_SOAKING_START_TIME:
            if (oprId == CL_ALARM_OM_OPERATION_SET)
            {
                memcpy (&(objPtr->alarmTransInfo[idx].SoakingStartTime), val, 
                        sizeof (struct timeval));
            }
            else
            {
                memcpy (val, &(objPtr->alarmTransInfo[idx].SoakingStartTime), 
                        sizeof (struct timeval));
            }
            break;            
        case CL_ALARM_EVE_HANDLE:
            if (oprId == CL_ALARM_OM_OPERATION_SET)
            {
                memcpy (&(objPtr->alarmTransInfo[idx].alarmEveH), val, 
                        sizeof (ClUint32T));
            }
            else
            {
                memcpy (val,&(objPtr->alarmTransInfo[idx].alarmEveH), 
                        sizeof (ClUint32T));
            }
            break;
        
            /* set the payload len */ 
                case CL_ALARM_EVENT_PAY_LOAD_LEN: 
            if (oprId == CL_ALARM_OM_OPERATION_SET)
            {
                
                memcpy (&(objPtr->alarmTransInfo[idx].payloadLen ), val, 
                        sizeof (ClUint32T ));
            }
            else
            {
                memcpy (val, &(objPtr->alarmTransInfo[idx].payloadLen ), 
                        sizeof (ClUint32T ));
               
            }
            break; 
            /* payload len end */   
                
                /* copy payload to or from */
                case CL_ALARM_EVENT_PAY_LOAD: 
            if (oprId == CL_ALARM_OM_OPERATION_SET)
            {
                
                (objPtr->alarmTransInfo[idx]).payload = (void *)
                clHeapAllocate(objPtr->alarmTransInfo[idx].payloadLen);
                if( objPtr->alarmTransInfo[idx].payload == NULL )
                {
                    clLogError("ALM", "OME", "Unable to allocate memory for userpayload ");

                    if ((rc = clOsalMutexUnlock(objPtr->cllock))!= CL_OK )
                    {
                        clLogError("ALM", "OME", "Unable to unlock the mutex. rc [0x%x]", rc);
                       return rc;
                    }

                    return CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
                }
                memcpy ((objPtr->alarmTransInfo[idx].payload ), val, 
                        objPtr->alarmTransInfo[idx].payloadLen );
            }
            else
            {
                if( objPtr->alarmTransInfo[idx].payload == NULL )
                {
                    clLogError("ALM", "OME", " The user data is not present for the MO. ");

                    if ((rc = clOsalMutexUnlock(objPtr->cllock))!= CL_OK )
                    {
                         clLogError("ALM", "OME", "Unable to unlock the mutex. rc [0x%x]",rc);
                        return rc;
                    }

                    return CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
                }

                memcpy (val, (objPtr->alarmTransInfo[idx].payload ), 
                       objPtr->alarmTransInfo[idx].payloadLen);
            }
                break; 
              
        
        default:
        {
            clLogNotice("ALM", "OME", "Invalid attr Id received, attrId [0x%x]", attrId);

            if ((rc = clOsalMutexUnlock(objPtr->cllock))!= CL_OK )
            {
                clLogError("ALM", "OME", "Unable to unlock the mutex rc 0x%x ",rc);
                return rc;
            }

            rc = CL_ALARM_RC(CL_ERR_INVALID_PARAMETER);
        }    
    }

    if ((rc = clOsalMutexUnlock(objPtr->cllock))!= CL_OK )
    {
        clLogError("ALM", "OME", "Unable to unlock the mutex. rc [0x%x]", rc);
        return rc;
    }

    return rc;
}
