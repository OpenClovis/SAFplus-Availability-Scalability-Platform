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

#ifndef _CL_AMS_TEST_ENTITIES_H_
#define _CL_AMS_TEST_ENTITIES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "clAmsTestApi.h"


    typedef struct ClAmsTestEntitySG
    {
#define CL_AMS_TEST_SG_SI_LIST (1<<0)
#define CL_AMS_TEST_SG_SU_LIST (1<<1)
#define CL_AMS_TEST_SG_SU_ASSIGNED_LIST (1<<2)
#define CL_AMS_TEST_SG_SU_INSTANTIABLE_LIST (1<<3)
#define CL_AMS_TEST_SG_SU_INSTANTIATED_LIST (1<<4)
#define CL_AMS_TEST_SG_SU_INSERVICE_SPARE_LIST (1<<5)
#define CL_AMS_TEST_SG_SU_FAULTY_LIST (1<<6)
#define CL_AMS_TEST_SG_CONFIG (1<<7)
#define CL_AMS_TEST_SG_STATUS (1<<8)
#define CL_AMS_TEST_SG_MAP_BITS (9)

        ClAmsSGConfigT sgConfig;
        ClAmsSGStatusT sgStatus;
        ClAmsEntityBufferT sgSUList;
        ClAmsEntityBufferT sgSUInstantiableList;
        ClAmsEntityBufferT sgSUInstantiatedList;
        ClAmsEntityBufferT sgSUInserviceSpareList;
        ClAmsEntityBufferT sgSUAssignedList;
        ClAmsEntityBufferT sgSUFaultyList;
        ClAmsEntityBufferT sgSIList;
    }ClAmsTestEntitySGT;

    typedef struct ClAmsTestEntitySI
    {
#define CL_AMS_TEST_SI_MAP_START (CL_AMS_TEST_SG_MAP_BITS)
#define CL_AMS_TEST_SI_SU_RANK_LIST (1<< (CL_AMS_TEST_SI_MAP_START))
#define CL_AMS_TEST_SI_SU_LIST (1<<(CL_AMS_TEST_SI_MAP_START+1))
#define CL_AMS_TEST_SI_DEPENDENCIES_LIST (1<<(CL_AMS_TEST_SI_MAP_START+2))
#define CL_AMS_TEST_SI_CSI_LIST (1<<(CL_AMS_TEST_SI_MAP_START+3))
#define CL_AMS_TEST_SI_CONFIG (1<<(CL_AMS_TEST_SI_MAP_START+4))
#define CL_AMS_TEST_SI_STATUS (1<<(CL_AMS_TEST_SI_MAP_START+5))
#define CL_AMS_TEST_SI_MAP_BITS (6)

        ClAmsSIConfigT siConfig;
        ClAmsSIStatusT siStatus;
        ClAmsEntityBufferT siSURankList;
        ClAmsSISURefBufferT siSUList;
        ClAmsEntityBufferT siDependenciesList;
        ClAmsEntityBufferT siCSIList;
    }ClAmsTestEntitySIT;

    typedef struct ClAmsTestEntitySU
    {
#define CL_AMS_TEST_SU_MAP_START (CL_AMS_TEST_SI_MAP_START+CL_AMS_TEST_SI_MAP_BITS)
#define CL_AMS_TEST_SU_COMP_LIST (1<<(CL_AMS_TEST_SU_MAP_START))
#define CL_AMS_TEST_SU_SI_LIST (1<<(CL_AMS_TEST_SU_MAP_START+1))
#define CL_AMS_TEST_SU_CONFIG (1<<(CL_AMS_TEST_SU_MAP_START+2))
#define CL_AMS_TEST_SU_STATUS (1<<(CL_AMS_TEST_SU_MAP_START+3))
#define CL_AMS_TEST_SU_MAP_BITS (4)

        ClAmsSUConfigT suConfig;
        ClAmsSUStatusT suStatus;
        ClAmsEntityBufferT suCompList;
        ClAmsSUSIRefBufferT suSIList;
    }ClAmsTestEntitySUT;

    typedef struct ClAmsTestEntityNode
    {
#define CL_AMS_TEST_NODE_MAP_START (CL_AMS_TEST_SU_MAP_START+CL_AMS_TEST_SU_MAP_BITS)
#define CL_AMS_TEST_NODE_DEPENDENCIES_LIST (1<<(CL_AMS_TEST_NODE_MAP_START))
#define CL_AMS_TEST_NODE_SU_LIST (1<<(CL_AMS_TEST_NODE_MAP_START+1))
#define CL_AMS_TEST_NODE_CONFIG (1<<(CL_AMS_TEST_NODE_MAP_START+2))
#define CL_AMS_TEST_NODE_STATUS (1<<(CL_AMS_TEST_NODE_MAP_START+3))
#define CL_AMS_TEST_NODE_MAP_BITS (4)

        ClAmsNodeConfigT nodeConfig;
        ClAmsNodeStatusT nodeStatus;
        ClAmsEntityBufferT nodeDependenciesList;
        ClAmsEntityBufferT nodeSUList;
    }ClAmsTestEntityNodeT;

    typedef struct ClAmsTestEntityComp
    {
#define CL_AMS_TEST_COMP_MAP_START (CL_AMS_TEST_NODE_MAP_START+CL_AMS_TEST_NODE_MAP_BITS)
#define CL_AMS_TEST_COMP_CSI_LIST (1<<(CL_AMS_TEST_COMP_MAP_START))
#define CL_AMS_TEST_COMP_CONFIG (1<<(CL_AMS_TEST_COMP_MAP_START+1))
#define CL_AMS_TEST_COMP_STATUS (1<<(CL_AMS_TEST_COMP_MAP_START+2))
#define CL_AMS_TEST_COMP_MAP_BITS (3)

        ClAmsCompConfigT compConfig;
        ClAmsCompStatusT compStatus;
        ClAmsCompCSIRefBufferT compCSIList;
    }ClAmsTestEntityCompT;

    typedef struct clAmsTestEntityCSI
    {
#define CL_AMS_TEST_CSI_MAP_START (CL_AMS_TEST_COMP_MAP_START+CL_AMS_TEST_COMP_MAP_BITS)
#define CL_AMS_TEST_CSI_NVP_LIST (1<<(CL_AMS_TEST_CSI_MAP_START))
#define CL_AMS_TEST_CSI_CONFIG (1 << (CL_AMS_TEST_CSI_MAP_START+1))
#define CL_AMS_TEST_CSI_STATUS (1<< (CL_AMS_TEST_CSI_MAP_START+2))
#define CL_AMS_TEST_CSI_MAP_BITS (3)

        ClAmsCSIConfigT csiConfig;
        ClAmsCSIStatusT csiStatus;
        ClAmsCSINVPBufferT csiNVPList;
    }ClAmsTestEntityCSIT;

    typedef struct ClAmsTestEntities
    {
        ClAmsTestEntitySGT sg;
        ClAmsTestEntitySIT si;
        ClAmsTestEntitySUT su;
        ClAmsTestEntityNodeT node;
        ClAmsTestEntityCompT comp;
        ClAmsTestEntityCSIT csi;
    }ClAmsTestEntitiesT;

    extern ClAmsTestEntitiesT gClAmsTestEntities;

    extern ClRcT clAmsTestEntityGetConfig(ClAmsMgmtHandleT ,
                                          ClAmsEntityT*,
                                          ClAmsEntityConfigT **ppConfig);

    extern ClRcT clAmsTestEntityGetStatus(ClAmsMgmtHandleT,
                                          ClAmsEntityT*,
                                          ClAmsEntityStatusT **ppStatus);

    extern ClRcT clAmsTestEntityGetSGSUList(ClAmsMgmtHandleT,ClAmsEntityT *pSG,ClAmsEntityBufferT **ppRetBuffer);

    extern ClRcT clAmsTestEntityGetSGSIList(ClAmsMgmtHandleT,ClAmsEntityT *pSG,ClAmsEntityBufferT **ppRetBuffer);

    extern ClRcT clAmsTestEntityGetSGSUInstantiableList(ClAmsMgmtHandleT,
                                                        ClAmsEntityT *pSG,
                                                        ClAmsEntityBufferT **ppRetBuffer);
    extern ClRcT clAmsTestEntityGetSGSUInstantiatedList(ClAmsMgmtHandleT,
                                                        ClAmsEntityT *pSG,
                                                        ClAmsEntityBufferT **ppRetBuffer);
    extern ClRcT clAmsTestEntityGetSGSUInserviceSpareList(ClAmsMgmtHandleT,
                                                          ClAmsEntityT *pSG,
                                                          ClAmsEntityBufferT**ppRetBuffer);

    extern ClRcT clAmsTestEntityGetSGSUAssignedList(ClAmsMgmtHandleT,
                                                    ClAmsEntityT *pSG,
                                                    ClAmsEntityBufferT **ppRetBuffer);
    extern ClRcT clAmsTestEntityGetSGSUFaultyList(ClAmsMgmtHandleT,
                                                  ClAmsEntityT *pSG,
                                                  ClAmsEntityBufferT **ppRetBuffer);

    extern ClRcT clAmsTestEntityGetSISURankList(ClAmsMgmtHandleT,
                                                ClAmsEntityT*pSI,
                                                ClAmsEntityBufferT**ppRetBuffer);

    extern ClRcT clAmsTestEntityGetSIDependenciesList(ClAmsMgmtHandleT ,
                                                      ClAmsEntityT *pSI,
                                                      ClAmsEntityBufferT**ppRetBuffer);

    extern ClRcT clAmsTestEntityGetSICSIList(ClAmsMgmtHandleT,
                                             ClAmsEntityT *pSI,
                                             ClAmsEntityBufferT**ppRetBuffer);

    extern ClRcT clAmsTestEntityGetSISUList(ClAmsMgmtHandleT,
                                            ClAmsEntityT *pSI,
                                            ClAmsSISURefBufferT **ppRetBuffer);

    extern ClRcT clAmsTestEntityGetSUCompList(ClAmsMgmtHandleT,
                                              ClAmsEntityT*pSU,
                                              ClAmsEntityBufferT**ppRetBuffer);

    extern ClRcT clAmsTestEntityGetSUSIList(ClAmsMgmtHandleT,
                                            ClAmsEntityT*pSU,
                                            ClAmsSUSIRefBufferT**ppRetBuffer);

    extern ClRcT clAmsTestEntityGetNodeDependenciesList(ClAmsMgmtHandleT,
                                                        ClAmsEntityT*pNode,
                                                        ClAmsEntityBufferT**ppRetBuffer);

    extern ClRcT clAmsTestEntityGetNodeSUList(ClAmsMgmtHandleT,
                                              ClAmsEntityT*pNode,
                                              ClAmsEntityBufferT**ppRetBuffer);

    extern ClRcT clAmsTestEntityGetCompCSIList(ClAmsMgmtHandleT,
                                               ClAmsEntityT*pComp,
                                               ClAmsCompCSIRefBufferT **ppRetBuffer);

    extern ClRcT clAmsTestEntityGetCSINVPList(ClAmsMgmtHandleT,
                                              ClAmsEntityT*pCSI,
                                              ClAmsCSINVPBufferT **ppRetBuffer);


    extern ClRcT clAmsTestEntityLockAssignment(ClAmsMgmtHandleT,
                                               ClAmsEntityT*);

    extern ClRcT clAmsTestEntityLockInstantiation(ClAmsMgmtHandleT,
                                                  ClAmsEntityT*);

    extern ClRcT clAmsTestEntityUnlock(ClAmsMgmtHandleT,
                                       ClAmsEntityT*);

    extern ClRcT clAmsTestEntityRestart(ClAmsMgmtHandleT,
                                        ClAmsEntityT*);

    extern ClRcT clAmsTestEntityShutdown(ClAmsMgmtHandleT,
                                         ClAmsEntityT*);

    extern ClRcT clAmsTestEntityRepaired(ClAmsMgmtHandleT,
                                         ClAmsEntityT*);

    extern void clAmsTestEntityMark(void);

    extern void clAmsTestEntityUnmark(void);

    extern ClRcT clAmsTestEntityGet(ClAmsMgmtHandleT handle,
                                    ClUint32T info,
                                    ClAmsEntityT *pEntity,
                                    ClBoolT insideTest);
                                   
#ifdef __cplusplus
}
#endif

#endif
