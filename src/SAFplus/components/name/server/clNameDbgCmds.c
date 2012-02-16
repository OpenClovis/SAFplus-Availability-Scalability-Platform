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
 * ModuleName  : name
 * File        : clNameDbgCmds.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains Name Service related DBG CLI commands
 * implementation
 *****************************************************************************/

#include "stdio.h"
#include "string.h"
#include "clCommon.h"
#include "clEoApi.h"
#include "clNameApi.h"
#include "clTimerApi.h"
#include "clRmdApi.h"
#include "clDebugApi.h"
#include "clNameErrors.h"
#include "clCommonErrors.h"
#include "xdrClNameSvcInfoIDLT.h"
#include "clNameCommon.h"
#include "clNameIpi.h"
#include "xdrClNameSvcAttrEntryWithSizeIDLT.h"
#include "xdrClNameVersionT.h"
#include "clVersionApi.h"

/*#include "clNameIpi.h" */
extern ClRcT _nameSvcPerContextInfo();
extern ClRcT _nameSvcAttributeQuery();
extern ClRcT nameSvcLAQuery();
extern ClCntHandleT gNSHashTable;
#define CL_NAME_CLI_STR_LEN 1024*65
ClCharT gNameCliStr[CL_NAME_CLI_STR_LEN];

ClRcT _cliNSGetEntryInBuffer(ClCharT* pBuff, ClNameSvcEntryT* pNSInfo);

/**
 *  Name:  clNameCliStrPrint
 *
 *  Copies srting to return string of DBG CLI
 *  @param  str: String to be copied
 *          retStr: Return string of DBG CLI
 *
 *  @returns
 *    none
 */
                                                                                                                             
void clNameCliStrPrint(ClCharT* str, ClCharT**retStr)
{
                                                                                                                             
    *retStr = clHeapAllocate(strlen(str)+1);
    if(NULL == *retStr)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Malloc Failed \r\n"));
        return;
    }
    sprintf(*retStr, str);
    return;
}



/* cliNSInitialize
 *
 * Cli for initializing the name service library 
 *
 * @param none
 *
 *  @returns
 *    CL_OK
 */
ClRcT cliNSInitialize(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    ClRcT          rc = CL_OK;
#if 0
    ClVersionT version;    
    CL_NAME_VERSION_SET(version);
#endif
    gNameCliStr[0]='\0';
    rc = clNameLibInitialize();
    if(rc != CL_OK)
    {
        sprintf(gNameCliStr, "\n NSLibInitialize failed, rc = 0x%x \n", rc);
        rc = CL_OK;
    }
    clNameCliStrPrint(gNameCliStr, retStr);
      
    return rc;
}     


/* cliNSFinalize
 *
 * Cli for finalizing the name service library 
 *
 * @param none
 *
 *  @returns
 *    CL_OK
 */
ClRcT cliNSFinalize(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
     clNameLibFinalize();
     return CL_OK;
}

/**
 *  Name:  cliNSRegister
 *
 *  Cli for registration with NS
 * 
 *  @param  argc: No of args
 *          argv: Command line args
 *          retStr: Return string of DBG CLI
 *
 *  @returns
 *    CL_OK
 */

ClRcT cliNSRegister(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    ClRcT          rc         = CL_OK;
    ClNameSvcRegisterPtrT pRegInfo;
    ClUint32T      context;
    ClUint32T      attrCount;
    ClNameSvcAttrEntryT attrList[CL_NS_MAX_NO_ATTR];
    ClUint32T i=0,j=1;
    ClUint32T len = 0;
    ClUint64T objRef;
    ClUint32T arr[2];
    ClUint32T obj[2];

    if (argc < 8 )
    {
        clNameCliStrPrint("\nUsage: NSRegister <context> <name> <objRefHigh>"
            "<objRefLow> <compId> <compPriority> <attrCount> [<attrType attrVal>.. attrCount times]"
                "\n context      [INT]   - Context in which to register"
                "\n name        [STRING] - Service name"
                "\n objRef       [INT]   - Object reference" 
                "\n compId       [INT]   - Id of the component providing the service"
                "\n compPriority [INT]   - Priority of the component providing the service"
                "\n attrCount    [INT]   - Attribute count"
            "\n If object reference is not known use 0x0FFFFFFF for both <objRefHigh> and <objRefLow>"
            "\n For adding to default global context use 0x0FFFFFFF "
            "(CL_NS_DEFT_GLOBAL_CONTEXT)"
            "\n For adding to default local  context use 0x0FFFFFFE "
            "(CL_NS_DEFT_LOCAL_CONTEXT)", retStr);
        return CL_OK;
    }

    if ((argv[7][1] == 'x') || (argv[7][1] == 'X'))
        attrCount = (ClUint32T)strtol (argv[7], NULL, 16);
    else
        attrCount = (ClUint32T)strtol (argv[7], NULL, 10);
    if((attrCount > 0) && (argc < (8+(2*attrCount))))
    {
        clNameCliStrPrint("\nUsage: NSRegister <context> <name> <objRefHigh> "
            "<objRefLow> <compId> <compPriority> <attrCount> [<attrType attrVal>.. attrCount times]"
                "\n context      [INT]   - Context in which to register"
                "\n name        [STRING] - Service name"
                "\n objRef       [INT]   - Object reference" 
                "\n compId       [INT]   - Id of the component providing the service"
                "\n compPriority [INT]   - Priority of the component providing the service"
                "\n attrCount    [INT]   - Attribute count"
            "\n If object reference is not known use 0x0FFFFFFF for both <objRefHigh> and <objRefLow>"
            "\n For adding to default global context use 0x0FFFFFFF "
            "(CL_NS_DEFT_GLOBAL_CONTEXT)"
            "\n For adding to default local  context use 0x0FFFFFFE "
            "(CL_NS_DEFT_LOCAL_CONTEXT)", retStr);
        return CL_OK;
    }
    else
    {
        pRegInfo = (ClNameSvcRegisterT*)clHeapAllocate(sizeof(ClNameSvcRegisterT)+
                   (attrCount*sizeof(ClNameSvcAttrEntryT)));
        if ((argv[1][1] == 'x') || (argv[1][1] == 'X'))
            context = (ClUint32T)strtol (argv[1], NULL, 16);
        else
            context = (ClUint32T)strtol (argv[1], NULL, 10);

        strcpy(pRegInfo->name.value, argv[2]);
        pRegInfo->name.length = strlen(pRegInfo->name.value);

        if ((argv[3][1] == 'x') || (argv[3][1] == 'X'))
            obj[1] = (ClUint32T)strtol (argv[3], NULL, 16);
        else
            obj[1] = (ClUint32T)strtol (argv[3], NULL, 10);

        if ((argv[4][1] == 'x') || (argv[4][1] == 'X'))
            obj[0] = (ClUint32T)strtol (argv[4], NULL, 16);
        else
            obj[0] = (ClUint32T)strtol (argv[4], NULL, 10);

        if((obj[0] == 0x0FFFFFFF) && (obj[1] == 0x0FFFFFFF))
            objRef = 0x0FFFFFFF;
        else
        {
            objRef = obj[1];
            objRef = (objRef<<32)|obj[0];
        }
        if ((argv[5][1] == 'x') || (argv[5][1] == 'X'))
            pRegInfo->compId = (ClUint32T)strtol (argv[5], NULL, 16);
        else
            pRegInfo->compId = (ClUint32T)strtol (argv[5], NULL, 10);

        if ((argv[6][1] == 'x') || (argv[6][1] == 'X'))
            pRegInfo->priority = (ClUint32T)strtol (argv[6], NULL, 16);
        else
            pRegInfo->priority = (ClUint32T)strtol (argv[6], NULL, 10);

        memset(attrList, 0, attrCount*sizeof(ClNameSvcAttrEntryT));
        for(i=0; i<attrCount; i++)
        {
            len = strlen(argv[7+j]);
            memcpy(attrList[i].type, argv[7+j], 20);
            memset(attrList[i].type+len, 0, 20-len);
            j++;
            len = strlen(argv[7+j]);
            memcpy(attrList[i].value, argv[7+j], 20);
            memset(attrList[i].value+len, 0, 20-len);
            j++;
        }
 
        memcpy(pRegInfo->attr, attrList,
                 (attrCount*sizeof(ClNameSvcAttrEntryT)));
        pRegInfo->attrCount = attrCount;
                    

        gNameCliStr[0]='\0';
        rc =  clNameRegister(context, pRegInfo, &objRef);
        if(rc != CL_OK)
        {
            sprintf(gNameCliStr, "\n NSRegister failed, rc = 0x%x \n",
                rc);
        }
        else
        {
            arr[1] = objRef>>32;
            arr[0] = objRef & 0xFFFFFFFF;

/*            memcpy(arr, &objRef, sizeof(ClUint64T));                */
            sprintf(gNameCliStr, "\n objReference registered is %d:%d \n",
                                 arr[1], arr[0]);
        }
        clNameCliStrPrint(gNameCliStr, retStr);
        rc = CL_OK;
        clHeapFree(pRegInfo);
    }
        
    return rc;
}
   
         

/**
 *  Name:  cliNSComponentDeregister
 *
 *  Cli for Component Deregister
 * 
 *  @param  argc: No of args
 *          argv: Command line args
 *          retStr: Return string of DBG CLI
 *
 *  @returns
 *    CL_OK
 */

ClRcT cliNSComponentDeregister(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    ClRcT          rc         = CL_OK;
    ClUint32T      compId;
    if (argc < 2 )
    {
        clNameCliStrPrint("\nUsage:  NSComponentDeregister <compId>"
                          "\n compId [INT]  - Id of the component providing the service",
            retStr);
        return CL_OK;
    }
    else
    {
        if ((argv[1][1] == 'x') || (argv[1][1] == 'X'))
            compId = (ClUint32T)strtol (argv[1], NULL, 16);
        else
            compId = (ClUint32T)strtol (argv[1], NULL, 10);

        rc = clNameComponentDeregister(compId);
        if(rc != CL_OK)
        {
            gNameCliStr[0]='\0';
            sprintf(gNameCliStr, "\n NSComponentDeregister failed, rc = 0x%x \n",
                rc);
            clNameCliStrPrint(gNameCliStr, retStr);
            rc = CL_OK;
        }
    }
      
    return rc;
}



/**
 *  Name:  cliNSServiceDeregister
 *
 *  Cli for Service Deregister
 * 
 *  @param  argc: No of args
 *          argv: Command line args
 *          retStr: Return string of DBG CLI
 *
 *  @returns
 *    CL_OK
 */

ClRcT cliNSServiceDeregister(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    ClRcT          rc         = CL_OK;
    ClUint32T      compId, contextId;
    ClNameT            name;
    if (argc < 4 )
    {
        clNameCliStrPrint("\nUsage:  NSServiceDeregister <contextId> "
                                "<compId> <serviceName>"
                "\n contextId    [INT]   - Context where service exists"
                "\n compId       [INT]   - Id of the component providing the service"
                "\n serviceName [STRING] - Service name"
          "\n For default global context use 0x0FFFFFFF"
          " (CL_NS_DEFT_GLOBAL_CONTEXT)"
          "\n For default local  context use 0x0FFFFFFE"
          " (CL_NS_DEFT_LOCAL_CONTEXT)",
            retStr);
        return CL_OK;
    }
    else
    {
        if ((argv[1][1] == 'x') || (argv[1][1] == 'X'))
            contextId = (ClUint32T)strtol (argv[1], NULL, 16);
        else
            contextId = (ClUint32T)strtol (argv[1], NULL, 10);

        if ((argv[2][1] == 'x') || (argv[2][1] == 'X'))
            compId = (ClUint32T)strtol (argv[2], NULL, 16);
        else
            compId = (ClUint32T)strtol (argv[2], NULL, 10);

        strcpy(name.value, argv[3]);
        name.length = strlen(name.value);
     
        rc = clNameServiceDeregister(contextId, compId, &name);
        if(rc != CL_OK)
        {
            gNameCliStr[0]='\0';
            sprintf(gNameCliStr, "\n NSServiceDeregister failed, rc = 0x%x \n",
                rc);
            clNameCliStrPrint(gNameCliStr, retStr);
            rc = CL_OK;
        }
    }
      
    return rc;
}



/**
 *  Name:  cliNSContextCreate
 *
 *  Cli for context creation
 * 
 *  @param  argc: No of args
 *          argv: Command line args
 *          retStr: Return string of DBG CLI
 *
 *  @returns
 *    CL_OK
 */

ClRcT cliNSContextCreate(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    ClRcT          rc         = CL_OK;
    ClUint32T      type;
    ClUint32T      context, cookie;
    if (argc < 3 )
    {
        clNameCliStrPrint("\nUsage:  NSContextCreate <contextType>"
                                " <contextMapCookie>"
                "\n contextType      [INT]   - node local/global "
                "\n contextMapCookie [INT]   - context cookie"
          "\ncontextType: 1 - user defined node local"
          "\n             2 - user defined global ",
            retStr);
        return CL_OK;
    }
    else
    {
        if ((argv[1][1] == 'x') || (argv[1][1] == 'X'))
            type = (ClUint32T)strtol (argv[1], NULL, 16);
        else
            type = (ClUint32T)strtol (argv[1], NULL, 10);

        if ((argv[2][1] == 'x') || (argv[2][1] == 'X'))
            cookie = (ClUint32T)strtol (argv[2], NULL, 16);
        else
            cookie = (ClUint32T)strtol (argv[2], NULL, 10);
        switch(type)
        {
            case 1:
                type = CL_NS_USER_NODELOCAL;
            break;
            case 2:
                type = CL_NS_USER_GLOBAL;
            break;
            default:
                clNameCliStrPrint("\nUsage:  NSContextCreate <contextType>"
                                        " <contextMapCookie>"
                  "\n contextType      [INT]   - node local/global "
                  "\n contextMapCookie [INT]   - context cookie"
                  "\ncontextType: 1 - user defined node local"
                  "\n             2 - user defined global ",
                   retStr);
                return rc; 
        }
  
        rc = clNameContextCreate(type, cookie, &context);
        if(rc != CL_OK)
        {
            gNameCliStr[0]='\0';
            sprintf(gNameCliStr, "\n NSContextCreate failed, rc = 0x%x \n",
                rc);
            clNameCliStrPrint(gNameCliStr, retStr);
            rc = CL_OK;
        }
        else
        {
            gNameCliStr[0]='\0';
            sprintf(gNameCliStr, "\n Context created with Id = 0x%x \n",
                    context);
            clNameCliStrPrint(gNameCliStr, retStr);
        }
    }

    return rc;
}



/**
 *  Name:  cliNSContextDelete
 *
 *  Cli for context deletion
 * 
 *  @param  argc: No of args
 *          argv: Command line args
 *          retStr: Return string of DBG CLI
 *
 *  @returns
 *    CL_OK
 */

ClRcT cliNSContextDelete(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    ClRcT          rc       = CL_OK;
    ClUint32T      contextId;
    if (argc < 2 )
    {
        clNameCliStrPrint("\nUsage: NSContextDelete <contextId>"
                "\n contextId      [INT]   - context to be deleted ",
            retStr);
        return CL_OK;
    }
    else
    {
        if ((argv[1][1] == 'x') || (argv[1][1] == 'X'))
            contextId = (ClUint32T)strtol (argv[1], NULL, 16);
        else
            contextId = (ClUint32T)strtol (argv[1], NULL, 10);
        rc = clNameContextDelete(contextId);
        if(rc != CL_OK)
        {
            gNameCliStr[0]='\0';
            sprintf(gNameCliStr, "\n NSContextDelete failed, rc = 0x%x \n",
                rc);
            clNameCliStrPrint(gNameCliStr, retStr);
            rc = CL_OK;
        }
    }
      
    return rc;
}



/**
 *  Name:  cliNSListEntries
 *
 *  Cli for listing entries in a context
 * 
 *  @param  argc: No of args
 *          argv: Command line args
 *          retStr: Return string of DBG CLI
 *
 *  @returns
 *    CL_OK
 */

ClRcT cliNSListEntries(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    ClRcT          rc         = CL_OK;
    ClUint32T      context;
    ClUint8T       *pData     = NULL;
    ClBufferHandleT outMsgHandle;
    ClUint32T      size;
    if (argc < 2 )
    {
        clNameCliStrPrint("\nUsage: NSList <contextCookie>"
                "\n contextMapCookie [INT]   - context cookie"
            "\n For default global context use 0x0FFFFFFF"
            " (CL_NS_DEFT_GLOBAL_MAP_COOKIE)"
            "\n For default local  context use 0x0FFFFFFE"
            " (CL_NS_DEFT_LOCAL_MAP_COOKIE)",
            retStr);
        return CL_OK;
    }
    else
    {
        if ((argv[1][1] == 'x') || (argv[1][1] == 'X'))
            context = (ClUint32T)strtol (argv[1], NULL, 16);
        else
            context = (ClUint32T)strtol (argv[1], NULL, 10);

        rc = clBufferCreate (&outMsgHandle);
        rc = _nameSvcPerContextInfo(context, outMsgHandle);
        if(rc != CL_OK)
        {
            gNameCliStr[0]='\0';
            sprintf(gNameCliStr, "\n NSList failed, rc = 0x%x \n",
                rc);
            clNameCliStrPrint(gNameCliStr, retStr);
            rc = CL_OK;
        }
        else
        {
            rc = clBufferLengthGet(outMsgHandle, &size);
            if(rc == CL_OK)
            {
               pData = (ClUint8T*) clHeapCalloc(1,size+1); /* 1 extra for '\0' */
               rc = clBufferNBytesRead(outMsgHandle,
                                       (ClUint8T*)pData, &size);
            }
            clNameCliStrPrint((ClCharT*)pData, retStr);
            clHeapFree(pData);
        }
        clBufferDelete(&outMsgHandle);
    }

    return rc;
}



/**
 *  Name:  cliNSObjectReferenceQuery
 *
 *  Cli for obj reference query
 * 
 *  @param  argc: No of args
 *          argv: Command line args
 *          retStr: Return string of DBG CLI
 *
 *  @returns
 *    CL_OK
 */

ClRcT cliNSObjectReferenceQuery(ClUint32T argc, ClCharT **argv,
                                ClCharT** retStr)
{
    ClRcT               rc  = CL_OK;
    ClUint32T           contextMapCookie;
    ClNameT             name;
    ClUint64T          objRef;
    ClNameSvcAttrEntryT attrList[CL_NS_MAX_NO_ATTR];
    ClUint32T           attrCount = 0;
    ClUint32T           i=0,j=1;
    ClUint32T           len = 0;
    ClUint32T           tempCnt = 0;
    ClUint32T           arr[2];

    if (argc < 4 )
    {
        clNameCliStrPrint("\nUsage: NSObjRefQuery <name> <contextMapCookie>"
            " <attrCount> [<attrType attrVal>.. attrCount times]"
                "\n name        [STRING] - Service name"
                "\n contextMapCookie [INT]   - context cookie"
                "\n attrCount    [INT]   - Attribute count"
            "\n For default global context use 0x0FFFFFFF"
            " (CL_NS_DEFT_GLOBAL_MAP_COOKIE)"
            "\n For default local  context use 0x0FFFFFFE"
            " (CL_NS_DEFT_LOCAL_MAP_COOKIE)"
            "\n If you dont know the attributes "
            " use attrCount = 0xF (CL_NS_DEFT_ATTR_LIST)",
            retStr);
        return CL_OK;
    }
    if ((argv[3][1] == 'x') || (argv[3][1] == 'X'))
        attrCount = (ClUint32T)strtol (argv[3], NULL, 16);
    else
        attrCount = (ClUint32T)strtol (argv[3], NULL, 10);

    if(attrCount == 0xF)
    {
        tempCnt   = 1;
        attrCount = 0;
    }

    if((attrCount > 0) && 
       (argc < (4+(2*attrCount))))
    {
        clNameCliStrPrint("\nUsage: NSObjRefQuery <name> <contextMapCookie>"
            " <attrCount> [<attrType attrVal>.. attrCount times]"
                "\n name        [STRING] - Service name"
                "\n contextMapCookie [INT]   - context cookie"
                "\n attrCount    [INT]   - Attribute count"
            "\n For default global context use 0x0FFFFFFF"
            " (CL_NS_DEFT_GLOBAL_MAP_COOKIE)"
            "\n For default local  context use 0x0FFFFFFE"
            " (CL_NS_DEFT_LOCAL_MAP_COOKIE)"
            "\n If you dont know the attributes "
            " use attrCount = 0x0FFFFFFF (CL_NS_DEFT_ATTR_LIST)",
            retStr);
        return CL_OK;
    }
    else
    {
        if ((argv[2][1] == 'x') || (argv[2][1] == 'X'))
            contextMapCookie = (ClUint32T)strtol (argv[2], NULL, 16);
        else
            contextMapCookie = (ClUint32T)strtol (argv[2], NULL, 10);

        strcpy(name.value, argv[1]);
        name.length = strlen(name.value);
       
        memset(attrList, 0, attrCount*sizeof(ClNameSvcAttrEntryT));
        for(i=0; i<attrCount; i++)
        {
            len = strlen(argv[3+j]);
            memcpy(attrList[i].type, argv[3+j], 20);
            memset(attrList[i].type+len, 0, 20-len);
           
            j++;
            len = strlen(argv[3+j]);
            memcpy(attrList[i].value, argv[3+j], 20);
            memset(attrList[i].value+len, 0, 20-len);
            j++;
        }
     

        if(tempCnt == 1)
            attrCount = 0xF;

        rc = clNameToObjectReferenceGet(&name, attrCount, attrList, 
                                            contextMapCookie, &objRef);
        if (rc == CL_OK)
        {
            gNameCliStr[0]='\0';
            arr[1] = objRef>>32;
            arr[0] = objRef & 0xFFFFFFFF;

        /*    memcpy(arr, &objRef, sizeof(ClUint64T));*/
            sprintf(gNameCliStr, "\n Object Reference is %d:%d \n",
                 arr[1], arr[0]);
            clNameCliStrPrint(gNameCliStr, retStr);
        }
        else
        {
            gNameCliStr[0]='\0';
            sprintf(gNameCliStr, "\n NSObjRefQuery failed, rc = 0x%x \n",
                rc);
            clNameCliStrPrint(gNameCliStr, retStr);
            rc = CL_OK;
        }
    }

    return rc;
}


/**
 *  Name: cliNSAllObjectMapsQuery 
 *
 *  Cli for all objs mapping query
 * 
 *  @param  argc: No of args
 *          argv: Command line args
 *          retStr: Return string of DBG CLI
 *
 *  @returns
 *    CL_OK
 */
                                                                                                                             
ClRcT cliNSAllObjectMapsQuery(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    ClRcT                rc      = CL_OK;
    ClUint32T            contextMapCookie, i = 0, j = 1;
    ClNameT              name;
    ClNameSvcInfoIDLT*   pNSInfo = NULL;
    ClNameSvcAttrEntryT  attrList[CL_NS_MAX_NO_ATTR];
    ClUint32T            attrCount = 0, size =0;
    ClUint32T            len = 0;
    ClCharT              buff[500];
    ClBufferHandleT outMsgHandle;
    ClNameSvcEntryT*     pEntry   = NULL;
    ClUint32T            attrSize    = sizeof(ClNameSvcAttrEntryT);
    ClUint32T            listSize    = sizeof(ClNameSvcCompListT);
    ClUint32T            totalSize   = 0;
    ClNameSvcCompListT*  pList       = NULL;
    ClUint8T*            pBuff       = NULL;
    ClUint32T            offset      = 0, index = 0;
    ClUint32T            refCount    = 0;
    ClCharT              data[CL_NAME_CLI_STR_LEN];


    if (argc < 4 )
    {
        sprintf(buff, "\nUsage: %s <name> <contextMapCookie>"
            " <attrCount> [<attrType attrVal>.. attrCount times]"
                "\n name        [STRING] - Service name"
                "\n contextMapCookie [INT]   - context cookie"
                "\n attrCount    [INT]   - Attribute count"
            "\n For default global context use 0x0FFFFFFF"
            " (CL_NS_DEFT_GLOBAL_MAP_COOKIE)"
            "\n For default local  context use 0x0FFFFFFE"
            " (CL_NS_DEFT_LOCAL_MAP_COOKIE)",
            argv[0]);
        clNameCliStrPrint(buff, retStr);
        return CL_OK;
    }
    if ((argv[3][1] == 'x') || (argv[3][1] == 'X'))
        attrCount = (ClUint32T)strtol (argv[3], NULL, 16);
    else
        attrCount = (ClUint32T)strtol (argv[3], NULL, 10);
                                                                                                                             
    if((attrCount > 0) &&
       (argc < (4+(2*attrCount))))
    {
        sprintf(buff, "\nUsage: %s <name> <contextMapCookie>"
            " <attrCount> [<attrType attrVal>.. attrCount times]"
                "\n name        [STRING] - Service name"
                "\n contextMapCookie [INT]   - context cookie"
                "\n attrCount    [INT]   - Attribute count"
            "\n For default global context use 0x0FFFFFFF"
            " (CL_NS_DEFT_GLOBAL_MAP_COOKIE)"
            "\n For default local  context use 0x0FFFFFFE"
            " (CL_NS_DEFT_LOCAL_MAP_COOKIE)",
            argv[0]);
        clNameCliStrPrint(buff, retStr);
        return CL_OK;
    }
    else
    {
        clBufferCreate (&outMsgHandle);

        if ((argv[2][1] == 'x') || (argv[2][1] == 'X'))
            contextMapCookie = (ClUint32T)strtol (argv[2], NULL, 16);
        else
            contextMapCookie = (ClUint32T)strtol (argv[2], NULL, 10);
                                                                                                                             
        strcpy(name.value, argv[1]);
        name.length = strlen(name.value);
                                                                                                                             

        memset(attrList, 0, attrCount*sizeof(ClNameSvcAttrEntryT));
        for(i=0; i<attrCount; i++)
        {
            len = strlen(argv[3+j]);
            memcpy(attrList[i].type, argv[3+j], 20);
            memset(attrList[i].type+len, 0, 20-len);
            j++;
            len = strlen(argv[3+j]);
            memcpy(attrList[i].value, argv[3+j], 20);
            memset(attrList[i].value+len, 0, 20-len);
            j++;
        }
                                                                                                                             
        size = sizeof(ClNameSvcInfoIDLT);
        pNSInfo = (ClNameSvcInfoIDLT*) clHeapAllocate(size);
        if(pNSInfo == NULL)
        {
            rc = CL_NS_RC(CL_ERR_NO_MEMORY);
            clBufferDelete(&outMsgHandle);
            return rc;
        }

                                                                                                                             
        memset(pNSInfo, 0, sizeof(ClNameSvcInfoIDLT));
        pNSInfo->version          = CL_NS_VERSION_NO;
        memcpy (&pNSInfo->name, &name, sizeof(ClNameT));
        pNSInfo->contextMapCookie = contextMapCookie;
        pNSInfo->op               = CL_NS_QUERY_ALL_MAPPINGS;
        pNSInfo->attrCount = attrCount;
                                                                                                                             
        if(attrCount>0)
        {
            pNSInfo->attrLen = attrCount*sizeof(ClNameSvcAttrEntryIDLT);
            pNSInfo->attr = (ClNameSvcAttrEntryIDLT *)
               clHeapAllocate(pNSInfo->attrLen);
            memcpy(pNSInfo->attr, &attrList, pNSInfo->attrLen);
            size = size + pNSInfo->attrLen;
        }


        rc = nameSvcLAQuery(pNSInfo, size, outMsgHandle);
        if (rc != CL_OK)
        {
            gNameCliStr[0]='\0';
            sprintf(gNameCliStr, "\n %s failed, rc = 0x%x \n", argv[0],
                rc);
            clNameCliStrPrint(gNameCliStr, retStr);
            clBufferDelete(&outMsgHandle);
            clHeapFree(pNSInfo->attr);
            clHeapFree(pNSInfo);
            rc = CL_OK;
            return rc;
        }
        clBufferLengthGet(outMsgHandle, &size);
        {
            pBuff = (ClUint8T*) clHeapAllocate(size);
            memset(&data, 0, size);
            rc = clBufferNBytesRead(outMsgHandle,
                               (ClUint8T*)pBuff, &size);
            while(index<size)
            {
                attrCount = ((ClNameSvcEntryT*)(pBuff+index))->attrCount;
                refCount  = ((ClNameSvcEntryT*)(pBuff+index))->refCount;
                                                                                                                             
                totalSize = sizeof(ClNameSvcEntryT)+ ((attrCount)* attrSize);
                                                                                                                             
                pEntry = (ClNameSvcEntryT*) clHeapCalloc(1,totalSize);
                memcpy(pEntry, pBuff+index, totalSize);
                index = index + totalSize;
                pEntry->compId.pNext = NULL;
                while(refCount > 0)
                {
                    pList = (ClNameSvcCompListT*) clHeapAllocate(listSize);
                    pList->pNext = pEntry->compId.pNext;
                    pEntry->compId.pNext = pList;
                    memcpy(&pList->compId, pBuff+index, sizeof(ClUint32T));
                    refCount --;
                    index = index + sizeof(ClUint32T);
                    memcpy(&pList->priority, pBuff+index, sizeof(ClUint32T));
                    index = index + sizeof(ClUint32T);
                }
                _cliNSGetEntryInBuffer(data+offset, pEntry);
                offset = strlen(data);
                clHeapFree(pEntry);
            }
            clNameCliStrPrint(data, retStr);
            clHeapFree(pBuff);
        }
        clBufferDelete(&outMsgHandle);
    }                                                                                                                  
    clHeapFree(pNSInfo->attr);
    clHeapFree(pNSInfo);
    return rc;
}


/**
 *  Name:  cliNSObjectMapQuery
 *
 *  Cli for obj mapping query
 * 
 *  @param  argc: No of args
 *          argv: Command line args
 *          retStr: Return string of DBG CLI
 *
 *  @returns
 *    CL_OK
 */

ClRcT cliNSObjectMapQuery(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    ClRcT                rc      = CL_OK;
    ClUint32T            contextMapCookie, i = 0, j = 1;
    ClNameT              name;
    ClNameSvcEntryT*     pNSInfo = NULL;
    ClUint32T            tempCnt = 0;
    ClNameSvcAttrEntryT  attrList[CL_NS_MAX_NO_ATTR];
    ClUint32T            attrCount = 0;
    ClUint32T            len = 0;
    ClCharT              buff[500];

    if (argc < 4 )
    {
        sprintf(buff, "\nUsage: %s <name> <contextMapCookie>"
            " <attrCount> [<attrType attrVal>.. attrCount times]"
                "\n name        [STRING] - Service name"
                "\n contextMapCookie [INT]   - context cookie"
                "\n attrCount    [INT]   - Attribute count"
            "\n For default global context use 0x0FFFFFFF"
            " (CL_NS_DEFT_GLOBAL_MAP_COOKIE)"
            "\n For default local  context use 0x0FFFFFFE"
            " (CL_NS_DEFT_LOCAL_MAP_COOKIE)"
            "\n If you dont know the attributes "
            " use attrCount = 0xF (CL_NS_DEFT_ATTR_LIST)",
            argv[0]);
        clNameCliStrPrint(buff, retStr);
        return CL_OK;
    }
    if ((argv[3][1] == 'x') || (argv[3][1] == 'X'))
        attrCount = (ClUint32T)strtol (argv[3], NULL, 16);
    else
        attrCount = (ClUint32T)strtol (argv[3], NULL, 10);

    if(attrCount == 0xF)
    {
        tempCnt   = 1;
        attrCount = 0;
    }

    if((attrCount > 0) && 
       (argc < (4+(2*attrCount))))
    {
        sprintf(buff, "\nUsage: %s <name> <contextMapCookie>"
            " <attrCount> [<attrType attrVal>.. attrCount times]"
                "\n name        [STRING] - Service name"
                "\n contextMapCookie [INT]   - context cookie"
                "\n attrCount    [INT]   - Attribute count"
            "\n For default global context use 0x0FFFFFFF"
            " (CL_NS_DEFT_GLOBAL_MAP_COOKIE)"
            "\n For default local  context use 0x0FFFFFFE"
            " (CL_NS_DEFT_LOCAL_MAP_COOKIE)"
            "\n If you dont know the attributes "
            " use attrCount = 0xF (CL_NS_DEFT_ATTR_LIST)",
            argv[0]);
        clNameCliStrPrint(buff, retStr);
        return CL_OK;
    }
    else
    {
        if ((argv[2][1] == 'x') || (argv[2][1] == 'X'))
            contextMapCookie = (ClUint32T)strtol (argv[2], NULL, 16);
        else
            contextMapCookie = (ClUint32T)strtol (argv[2], NULL, 10);

        strcpy(name.value, argv[1]);
        name.length = strlen(name.value);

        memset(attrList, 0, attrCount*sizeof(ClNameSvcAttrEntryT));
        for(i=0; i<attrCount; i++)
        {
            len = strlen(argv[3+j]);
            memcpy(attrList[i].type, argv[3+j], 20);
            memset(attrList[i].type+len, 0, 20-len);
            j++;
            len = strlen(argv[3+j]);
            memcpy(attrList[i].value, argv[3+j], 20);
            memset(attrList[i].value+len, 0, 20-len);
            j++;
        }
     
        if(tempCnt == 1)
            attrCount = 0xF;

        rc = clNameToObjectMappingGet(&name, attrCount, attrList,
                                      contextMapCookie, &pNSInfo);
        if (rc != CL_OK)
        {
            gNameCliStr[0]='\0';
            memset(gNameCliStr, 0, sizeof(gNameCliStr));
            sprintf(gNameCliStr, "\n %s failed, rc = 0x%x \n", argv[0],
                rc);
            clNameCliStrPrint(gNameCliStr, retStr);
            rc = CL_OK;
            return rc;
        }
    
        gNameCliStr[0]='\0';
        
        _cliNSGetEntryInBuffer(gNameCliStr, pNSInfo);
        clNameCliStrPrint(gNameCliStr, retStr);
        clNameObjectMappingCleanup(pNSInfo);
    }
   
    return rc;
}



/**
 *  Name: _cliNSGetEntryInBuffer 
 *
 *  Ipi for converting ClNameSvcEntryT into ClCharT buffer
 * 
 *  @param  pBuff: Will hold the info in ClCharT format
 *          pNSInfo: Info in ClNameSvcEntryT format
 *
 *  @returns
 *    CL_OK
 */

ClRcT _cliNSGetEntryInBuffer(ClCharT* pBuff, ClNameSvcEntryT* pNSInfo)
{
        ClNameSvcCompListT*  pCompId = NULL;
        ClNameSvcAttrEntryT* pAttr   = NULL;
        ClUint32T  i = 0, index = 0, j= 0;
        ClUint32T arr[2];


        pCompId = &pNSInfo->compId;
        pAttr   = pNSInfo->attr;  
    
        arr[1] = pNSInfo->objReference>>32;
        arr[0] = pNSInfo->objReference & 0xFFFFFFFF;
        /* memcpy(arr, &pNSInfo->objReference, sizeof(ClUint64T));*/
        sprintf(pBuff, "\n Name: %10s  ObjReference: %04d:%04d"
                 "  CompId: %04d  Priority: %d", pNSInfo->name.value,
                 (ClUint32T) arr[1],
                 (ClUint32T) arr[0],
                 pCompId->compId, pCompId->priority);
        index = index+sizeof(ClNameSvcEntryT)+ 
                    ((pNSInfo->attrCount)* sizeof(ClNameSvcAttrEntryT));
    
        if(pNSInfo->attrCount > 0)
        {
                for(i=0; i<pNSInfo->attrCount; i++)
                {
                   sprintf(pBuff+strlen(pBuff),"\t %10s:%10s", pAttr->type,
                             pAttr->value);
                   pAttr++;
                }
        }
        for(i=0; i<pNSInfo->refCount; i++)
        {
            pAttr = pNSInfo->attr;
            pCompId = pCompId->pNext;
    
            sprintf(pBuff+strlen(pBuff),
                "\n Name: %10s  ObjReference: %04d:%04d  CompId: %04d  Priority: %d",
                pNSInfo->name.value,
                (ClUint32T) arr[1],
                (ClUint32T) arr[0],
                pCompId->compId, pCompId->priority);

            index = index+sizeof(ClNameSvcCompListT);
                                                        
            if(pNSInfo->attrCount > 0)
            {
                for(j=0; j<pNSInfo->attrCount; j++)
                {
                    sprintf(pBuff+strlen(pBuff),"\t %10s:%10s",
                    pAttr->type, pAttr->value);
                    pAttr++;
                }
            }
        }
           
    return CL_OK;
}



/**
 *  Name:  cliNSAttributeLevelQuery
 *
 *  Cli for attrib level query
 * 
 *  @param  argc: No of args
 *          argv: Command line args
 *          retStr: Return string of DBG CLI
 *
 *  @returns
 *    CL_OK
 */

ClRcT cliNSAttributeLevelQuery(ClUint32T argc, ClCharT **argv,
                               ClCharT** retStr)
{
    ClRcT          rc         = CL_OK;
    ClUint32T      contextCookie, attrCount;
    ClNameSvcAttrSearchT attrList;
    ClUint32T i=0,j=1, size = 0;
    ClUint8T*       pData      = NULL;
    ClBufferHandleT outMsgHandle;
   
    if (argc < 3 )
    {
        clNameCliStrPrint("\nUsage: NSAttribQuery <contextMapCookie>"
            " <attrCount>"
            "[<attrType attrVal> <AND/OR>.. attrCount times] "
                "\n contextMapCookie [INT]   - context cookie"
                "\n attrCount    [INT]   - Attribute count"
            "\n For default global context use 0x0FFFFFFF"
            " (CL_NS_DEFT_GLOBAL_MAP_COOKIE)"
            "\n For default local  context use 0x0FFFFFFE"
            " (CL_NS_DEFT_LOCAL_MAP_COOKIE)",
            retStr);
        return CL_OK;
    }
    if ((argv[2][1] == 'x') || (argv[2][1] == 'X'))
        attrCount = (ClUint32T)strtol (argv[2], NULL, 16);
    else
        attrCount = (ClUint32T)strtol (argv[2], NULL, 10);


    if((attrCount > 0) && (argc < (3+(3*attrCount)-1)))
    {
        clNameCliStrPrint("\nUsage: NSAttribQuery <contextMapCookie>"
            " <attrCount>"
            "[<attrType attrVal> <AND/OR>.. attrCount times] "
                "\n contextMapCookie [INT]   - context cookie"
                "\n attrCount    [INT]   - Attribute count"
            "\n For default global context use 0x0FFFFFFF"
            " (CL_NS_DEFT_GLOBAL_MAP_COOKIE)"
            "\n For default local  context use 0x0FFFFFFE"
            " (CL_NS_DEFT_LOCAL_MAP_COOKIE)",
            retStr);
        return CL_OK;
    }

    else
    {
        if ((argv[1][1] == 'x') || (argv[1][1] == 'X'))
            contextCookie = (ClUint32T)strtol (argv[1], NULL, 16);
        else
            contextCookie = (ClUint32T)strtol (argv[1], NULL, 10);
                                                                                                                             
        for(i=0; i<attrCount; i++)
        {
            memcpy(attrList.attrList[i].type, argv[2+j], 20);
            j++;
            memcpy(attrList.attrList[i].value, argv[2+j], 20);
            j++;
            if(i < (attrCount-1))
            {
                if((strcmp(argv[2+j],"AND") == 0))
                    attrList.opCode[i] = 1;
                else
                    attrList.opCode[i] = 0;
                j++; 
            }
        }


        attrList.attrCount = attrCount;
        rc = clBufferCreate (&outMsgHandle);
                                                                                                                             
        rc = _nameSvcAttributeQuery(contextCookie, &attrList, outMsgHandle);

        if(rc != CL_OK)
        {
            gNameCliStr[0]='\0';
            sprintf(gNameCliStr, "\n NSAttribQuery failed, rc = 0x%x \n",
                rc);
            clNameCliStrPrint(gNameCliStr, retStr);
            rc = CL_OK;
        }
        else
        {
            gNameCliStr[0]='\0';
            rc = clBufferLengthGet(outMsgHandle, &size);
            if(rc == CL_OK)
            {
               pData = (ClUint8T*) clHeapCalloc(1,size+1); /* 1 extra for '\0' */
               rc = clBufferNBytesRead(outMsgHandle,
                                       (ClUint8T*)pData, &size);
            }

            strcpy(gNameCliStr, (ClCharT*)pData);
            clNameCliStrPrint(gNameCliStr, retStr);
            clHeapFree(pData);
        }
        clBufferDelete(&outMsgHandle);
    }
                                                                                                                             
    return rc;
}
ClRcT
clNameSvcBDTblWalk(ClCntHandleT       key,
                    ClCntDataHandleT  data,
                    ClCntArgHandleT   arg,
                    ClUint32T         size)
{
    ClBufferHandleT           msg   = (ClBufferHandleT)arg;
    ClNameSvcBindingDetailsT *pData = (ClNameSvcBindingDetailsT *)data;
    ClNameSvcCompListT       *pComp = NULL;
     
    clDebugPrint(msg, "Name      : %lld\n", pData->objReference); 
    clDebugPrint(msg, "RefCount  : %d\n", pData->refCount); 
    clDebugPrint(msg, "dsId      : %d\n", pData->dsId); 
    clDebugPrint(msg, "attrCount : %d\n", pData->attrCount);
    clDebugPrint(msg, "attrLen   : %d\n", pData->attrLen);
    clDebugPrint(msg, "-----------------------------------------\n");
    clDebugPrint(msg, "CompList Details:\n");
    pComp = &pData->compId;
    while(pComp != NULL)
    {
        clDebugPrint(msg, "dsId     : %d \n", pComp->dsId);
        clDebugPrint(msg, "compId   : %d \n", pComp->compId);
        clDebugPrint(msg, "priority : %d \n", pComp->priority);
        pComp = pComp->pNext;
    }
    return CL_OK;
}

ClRcT
clNameEntryTblPrint(ClCntKeyHandleT   key,
                    ClCntDataHandleT  data,
                    ClCntArgHandleT   arg,
                    ClUint32T         size)
{
    ClBufferHandleT   msg    = (ClBufferHandleT)arg;
    ClNameSvcBindingT *pData = (ClNameSvcBindingT *)data;
    ClRcT              rc    = CL_OK;
     
    clDebugPrint(msg, "Name      : %s\n", pData->name.value); 
    clDebugPrint(msg, "RefCount  : %d\n", pData->refCount); 
    clDebugPrint(msg, "dsId      : %d\n", pData->dsId); 
    clDebugPrint(msg, "priority  : %d\n", pData->priority);
    clDebugPrint(msg, "-----------------------------------------\n");
    clDebugPrint(msg, "Binding Details:\n");
    rc = clCntWalk(pData->hashId, clNameSvcBDTblWalk, 
                   (ClCntArgHandleT)msg, sizeof(msg));
    return CL_OK;
}

ClRcT
clNameNSTablePrint(ClCntKeyHandleT   key,
                   ClCntDataHandleT  data,
                   ClCntArgHandleT   arg,
                   ClUint32T         size)
{
    ClBufferHandleT  msg = (ClBufferHandleT)arg;
    ClNameSvcContextInfoT *pCtxData = (ClNameSvcContextInfoT *)data;
    ClRcT                  rc       = CL_OK;

    clDebugPrint(msg, "ContextId: %p \n", key);
    clDebugPrint(msg, "ContextData:\n");
    clDebugPrint(msg, "entryCount: %d \n", pCtxData->entryCount); 
    clDebugPrint(msg, "dsIdCnt   : %d \n", pCtxData->dsIdCnt);
    clDebugPrint(msg, "cookie    : %d \n", pCtxData->contextMapCookie);
    clDebugPrint(msg, "-----------------------------------------------\n");
    clDebugPrint(msg, "Svc Entry:"); 
    rc = clCntWalk(pCtxData->hashId, clNameEntryTblPrint, 
                   (ClCntArgHandleT)msg, sizeof(msg));
    return CL_OK;
    
}   

ClRcT
clNameSvcNSTableDump(ClBufferHandleT msg)
{
    ClRcT  rc = CL_OK;

    clDebugPrint(msg, "NSTABLE:\n");
    rc = clCntWalk(gNSHashTable, clNameNSTablePrint, 
                   (ClCntArgHandleT)msg, sizeof(msg));
    return CL_OK;
}    
                                        
ClRcT cliNSDataDump(ClUint32T argc,
                    ClCharT **argv,
                    ClCharT** retStr)
{
    ClRcT  rc = CL_OK;
    ClBufferHandleT msg = 0;

    if( argc != 1 )
    {
        clNameCliStrPrint("Usage: NameDbShow\n", retStr);
        return CL_OK;
    }    
    clDebugPrintInitialize(&msg);
    rc = clNameSvcNSTableDump(msg);

    clDebugPrintFinalize(&msg, retStr);
    return CL_OK;
}    

