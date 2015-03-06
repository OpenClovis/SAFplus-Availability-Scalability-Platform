#include <libgen.h>

#include <clCommon.h>
#include <clCommonErrors.h>

#include <clDebugApi.h>
#include <clList.h>
#include <clLogApi.h>
#include <clParserApi.h>

#include <clSnmpErrors.h>
#include <clSnmpSubagentClient.h>

#include <clSnmpDefs.h>
#include <clSnmpOp.h>
#include <clSnmpDataDefinition.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

ClSnmpSubagentInfoT gSubAgentInfo;

ClSnmpOidErrMapTableItemT oidErrMapTable[] = {
    { CL_SNMP_ANY_MODULE, CL_OK, CL_SNMP_ERR_NOERROR },
    { CL_SNMP_ANY_MODULE, CL_ERR_NO_MEMORY, CL_SNMP_ERR_NOCREATION },
    { CL_SNMP_ANY_MODULE, CL_ERR_INVALID_PARAMETER, CL_SNMP_ERR_NOSUCHNAME },
    { CL_SNMP_ANY_MODULE, CL_ERR_NULL_POINTER, CL_SNMP_ERR_NOCREATION },
    { CL_SNMP_ANY_MODULE, CL_ERR_NOT_EXIST, CL_SNMP_ERR_NOSUCHNAME },
    { CL_SNMP_ANY_MODULE, CL_ERR_INVALID_HANDLE, CL_SNMP_ERR_INCONSISTENTVALUE },
    { CL_SNMP_ANY_MODULE, CL_ERR_INVALID_BUFFER, CL_SNMP_ERR_INCONSISTENTVALUE }, 
    { CL_SNMP_ANY_MODULE, CL_ERR_NOT_IMPLEMENTED, CL_SNMP_ERR_NOCREATION },
    { CL_SNMP_ANY_MODULE, CL_ERR_DUPLICATE, CL_SNMP_ERR_BADVALUE },
    { CL_SNMP_ANY_MODULE, CL_ERR_OUT_OF_RANGE, CL_SNMP_ERR_WRONGVALUE },
    { CL_SNMP_ANY_MODULE, CL_ERR_NO_RESOURCE, CL_SNMP_ERR_RESOURCEUNAVAILABLE },
    { CL_SNMP_ANY_MODULE, CL_ERR_INITIALIZED, CL_SNMP_ERR_NOACCESS },
    { CL_SNMP_ANY_MODULE, CL_ERR_BAD_OPERATION, CL_SNMP_ERR_GENERR },
    { CL_SNMP_ANY_MODULE, CL_COR_ERR_INVALID_SIZE, CL_SNMP_ERR_WRONGLENGTH },
    { CL_SNMP_ANY_MODULE, CL_COR_ERR_OI_NOT_REGISTERED, CL_SNMP_ERR_NOACCESS},
    { CL_SNMP_ANY_MODULE, CL_SNMP_ANY_OTHER_ERROR, CL_SNMP_ERR_GENERR }
};

ClUint8T snmpGetASNType(const ClCharT *s)
{
    ClUint8T asnType = 0;
    struct asnStrType 
    {
        ClCharT *s;
        ClUint8T type;
    } asnTypeList[] = 
      {
          { "ASN_INTEGER", ASN_INTEGER },
          { "ASN_UNSIGNED", ASN_UNSIGNED },
          { "ASN_OCTET_STR", ASN_OCTET_STR },
          { "ASN_GAUGE", ASN_GAUGE },
          { "ASN_COUNTER64", ASN_COUNTER64 },
          { "ASN_COUNTER", ASN_COUNTER },
          { "ASN_TIMETICKS", ASN_TIMETICKS },
          { "ASN_IPADDRESS", ASN_IPADDRESS },
          { "ASN_OBJECT_ID", ASN_OBJECT_ID },
          { "ASN_OPAQUE", ASN_OPAQUE },
      };

    ClUint32T ntypes = sizeof(asnTypeList)/sizeof(asnTypeList[0]);
    ClUint32T i = 0;
    
    clLogDebug("SNMP", "SUB", "In snmpGetASNType type is %s", s);
    
    for (i = 0; i < ntypes; ++i)
    {
        if (!strncmp(s, asnTypeList[i].s, strlen(asnTypeList[i].s)))
        {
            asnType = asnTypeList[i].type;
            break;
        }
        
    }
    
    return asnType;
}

ClSnmpScalarInfoT *snmpColumnScalarGet(ClSnmpTableInfoT *t, ClUint32T colnum)
{
    ClListHeadT *iter = NULL;
    ClListHeadT *item = &t->columns;

    CL_LIST_FOR_EACH(iter, item)
    {
        ClSnmpColumnInfoT *c = CL_LIST_ENTRY(iter, ClSnmpColumnInfoT, list);

        clLogDebug("SNMP", "SUB", "colnum is %d, column is %d", colnum, c->column);
        

        if (c->column == colnum)
        {
            return c->s;
        }
    }

    return NULL;
}

ClRcT snmpDataInit(const ClCharT *file)
{
    ClRcT rc = CL_OK;
    ClUint32T i = 0;

    ClSnmpMibInfoT *mibInfo = &gSubAgentInfo.mibInfo;

    ClParserPtrT mibs = NULL;
    ClParserPtrT mib = NULL;
    ClParserPtrT scalars = NULL;
    ClParserPtrT scalar = NULL;
    ClParserPtrT tables = NULL;
    ClParserPtrT table = NULL;
    ClParserPtrT notifs = NULL;
    ClParserPtrT notif = NULL;

    const ClCharT *lastParent = NULL;
    const ClCharT *curParent = NULL;
    ClSnmpScalarInfoT *lastScalar = NULL;

    ClUint32T ntraps = 0;

    strncpy(gSubAgentInfo.file, file, CL_MAX_NAME_LENGTH-1);
    gSubAgentInfo.moCounter = 4;
    gSubAgentInfo.attrCounter = 1;
    gSubAgentInfo.tableTypeCounter = 0;

    CL_LIST_HEAD_INIT(&mibInfo->scalars);
    CL_LIST_HEAD_INIT(&mibInfo->tables);
    CL_LIST_HEAD_INIT(&mibInfo->traps);
    
    mibs = clParserOpenFile(dirname(strdup(file)),
                            basename(strdup(file)));
    if (!mibs)
    {
        goto failure;
    }

    mib = clParserChild(mibs, "mib");
    if (!mib)
    {
        clLogError("SNMP", "SUB", "The 'mib' tag not found.");
        goto failure;
    }

    strncpy(mibInfo->name,
            clParserAttr(mib, "module_name"),
            CL_MAX_NAME_LENGTH-1);
    
    /* Assume only one MIB for now. */
    clLogInfo("SNMP", "SUB", "MIB module name is %s",
              mibInfo->name);

    mibInfo->nEntries = atoi(clParserAttr(mib, "entries"));
    mibInfo->nOidEntries = atoi(clParserAttr(mib, "oid_entries"));

    rc = CL_SNMP_RC(CL_ERR_NO_MEMORY);
    scalars = clParserChild(mib, "scalars");
    if (!scalars)
    {
        clLogError("SNMP", "SUB", "The 'scalars' tag not found.");
        goto failure;
    }

    scalar = clParserChild(scalars, "scalar");
    while (scalar)
    {
        ClSnmpScalarInfoT *s = clHeapAllocate(sizeof(ClSnmpScalarInfoT));
        if (!s)
        {
            goto failure;
        }

        CL_LIST_HEAD_INIT(&s->list);
        strncpy(s->name, clParserAttr(scalar, "name"), CL_MAX_NAME_LENGTH-1);
        strncpy(s->oid, clParserAttr(scalar, "oid")+1, CL_MAX_NAME_LENGTH-1);
        s->settable = strncmp(clParserAttr(scalar, "settable"), "yes", 3) ?
                      CL_NO : CL_YES;
        s->type = snmpGetASNType(clParserAttr(scalar, "type"));
        s->attr = ++gSubAgentInfo.attrCounter;

        if (lastParent)
        {
            clLogDebug("SNMP", "SUB", "last parent is [%s], current parent is [%s]", lastParent, curParent);
            curParent = clParserAttr(scalar, "parent");
            if (!strncmp(curParent, lastParent, strlen(lastParent)))
            {
                s->parentmo = lastScalar->parentmo;
            }
            else
            {
                gSubAgentInfo.moCounter += 2;
                s->parentmo = gSubAgentInfo.moCounter;
            }
        }
        else
        {
            s->parentmo = gSubAgentInfo.moCounter;
            curParent = clParserAttr(scalar, "parent");
            lastParent = curParent;
            lastScalar = s;
        }

        clLogDebug("SNMP", "SUB", "parent mo of scalar [%s] is [%d]", s->name, s->parentmo);

        lastParent = curParent;
        lastScalar = s;
                
        s->tableType = 0;
        clListAddTail(&s->list, &mibInfo->scalars);

        scalar = scalar->next;
    }

    tables = clParserChild(mib, "tables");
    if (!tables)
    {
        clLogError("SNMP", "SUB", "The 'tables' tag not found.");
        goto failure;
    }

    table = clParserChild(tables, "table");
    while (table)
    {
        ClParserPtrT indexes = NULL;
        ClParserPtrT index = NULL;
        ClParserPtrT nonindexes = NULL;
        ClParserPtrT nonindex = NULL;
        ClUint32T nindexes = 0;
        ClUint32T column = 0;
        
        ClSnmpTableInfoT *t = clHeapAllocate(sizeof(ClSnmpTableInfoT));

        if (!t)
        {
            goto failure;
        }

        CL_LIST_HEAD_INIT(&t->list);
        strncpy(t->name, clParserAttr(table, "name"), CL_MAX_NAME_LENGTH-1);
        strncpy(t->oid, clParserAttr(table, "oid")+1, CL_MAX_NAME_LENGTH-1);
        t->settable = strncmp(clParserAttr(table, "settable"), "yes", 3) ?
                      CL_NO : CL_YES;
        t->minColumn = atoi(clParserAttr(table, "min_column"));
        t->maxColumn = atoi(clParserAttr(table, "max_column"));
        gSubAgentInfo.moCounter += 2;
        t->mo = gSubAgentInfo.moCounter;
        clLogDebug("SNMP", "SUB", "mo of table [%s] is [%d]", t->name, t->mo);
        t->tableType = ++gSubAgentInfo.tableTypeCounter;
        clListAddTail(&t->list, &mibInfo->tables);

        CL_LIST_HEAD_INIT(&t->indexes);
        CL_LIST_HEAD_INIT(&t->nonindexes);
        CL_LIST_HEAD_INIT(&t->columns);

        indexes = clParserChild(table, "indexes");
        index = clParserChild(indexes, "index");
        while (index)
        {
            ClSnmpScalarInfoT *s = NULL;
            ClSnmpColumnInfoT *c = NULL;
            
            s = clHeapAllocate(sizeof(ClSnmpScalarInfoT));
            if (!s)
            {
                goto failure;
            }

            CL_LIST_HEAD_INIT(&s->list);
            strncpy(s->name, clParserAttr(index, "name"), CL_MAX_NAME_LENGTH-1);
            strncpy(s->oid, clParserAttr(index, "oid")+1, CL_MAX_NAME_LENGTH-1);
            s->settable = CL_NO;
            s->type = snmpGetASNType(clParserAttr(index, "type"));
            s->attr = ++gSubAgentInfo.attrCounter;
            s->parentmo = t->mo;
            clListAddTail(&s->list, &t->indexes);

            c = clHeapAllocate(sizeof(ClSnmpColumnInfoT));
            if (!c)
            {
                goto failure;
            }

            CL_LIST_HEAD_INIT(&c->list);
            c->column = ++column;
            c->s = s;
            clListAddTail(&c->list, &t->columns);
            
            ++nindexes;
            index = index->next;
        }

        t->nIndexes = nindexes;
        t->attrIdList = clHeapAllocate((t->nIndexes+1) *
                                       sizeof(ClSnmpTableIndexCorAttrIdInfoT));
        if (!t->attrIdList)
        {
            goto failure;
        }

        for (i = 0; i < t->nIndexes; ++i)
        {
            t->attrIdList[i].pAttrPath = NULL;
            t->attrIdList[i].attrId = 0;
        }
        t->attrIdList[i].pAttrPath = NULL;
        t->attrIdList[i].attrId = -1;

        nonindexes = clParserChild(table, "nonindexes");
        nonindex = clParserChild(nonindexes, "nonindex");
        while (nonindex)
        {
            ClSnmpScalarInfoT *s = NULL;
            ClSnmpColumnInfoT *c = NULL;
            
            s = clHeapAllocate(sizeof(ClSnmpScalarInfoT));
            if (!s)
            {
                goto failure;
            }

            CL_LIST_HEAD_INIT(&s->list);
            strncpy(s->name, clParserAttr(nonindex, "name"), CL_MAX_NAME_LENGTH-1);
            strncpy(s->oid, clParserAttr(nonindex, "oid")+1, CL_MAX_NAME_LENGTH-1);
            s->settable = strncmp(clParserAttr(nonindex, "settable"), "yes", 3) ?
                          CL_YES : CL_NO;
            s->type = snmpGetASNType(clParserAttr(nonindex, "type"));
            s->attr = ++gSubAgentInfo.attrCounter;
            s->parentmo = t->mo;
            clListAddTail(&s->list, &t->nonindexes);

            c = clHeapAllocate(sizeof(ClSnmpColumnInfoT));
            if (!c)
            {
                goto failure;
            }

            CL_LIST_HEAD_INIT(&c->list);
            c->column = ++column;
            c->s = s;
            clListAddTail(&c->list, &t->columns);
            
            nonindex = nonindex->next;
        }

        table = table->next;
    }

    notifs = clParserChild(mib, "notifications");
    notif = clParserChild(notifs, "notification");

    while (notif)
    {
        ClSnmpTrapInfoT *t = clHeapAllocate(sizeof(ClSnmpTrapInfoT));
        if (!t)
        {
            goto failure;
        }

        CL_LIST_HEAD_INIT(&t->list);
        strncpy(t->name, clParserAttr(notif, "name"), CL_MAX_NAME_LENGTH-1);
        strncpy(t->oid, clParserAttr(notif, "oid")+1, CL_MAX_NAME_LENGTH-1);
        clListAddTail(&t->list, &mibInfo->traps);

        ++ntraps;
        notif = notif->next;
    }

    mibInfo->nTraps = ntraps;

    return CL_OK;
failure:
    return rc;
}

#define CL_MED_COR_STR_MOID_PRINT(corMoId)                              \
do                                                                      \
{                                                                       \
    SaNameT strMoId;                                                    \
    clCorMoIdToMoIdNameGet(corMoId, &strMoId );                         \
    clLogInfo("SNMP", "SUB", "%.*s", strMoId.length, strMoId.value);    \
}while(0)

ClRcT snmpDefaultInstXlator(const struct ClMedAgentId *pAgntId,
                            ClCorMOIdPtrT hmoId,
                            ClCorAttrPathPtrT containedPath,
                            void **pRetInstData,
                            ClUint32T *pInstLen,
                            ClPtrT pCorAttrValueDescList,
                            ClMedInstXlationOpEnumT instXlnOp,
                            ClPtrT cookie)
{
    ClRcT rc = CL_SNMP_RC(CL_ERR_NULL_POINTER);
    ClUint32T i = 0;
    ClSnmpReqInfoT *pInst = NULL;
    ClSnmpScalarInfoT *s = cookie;

    clLogDebug("SNMP", "SUB", "Default instance translation for MOID");
    CL_MED_COR_STR_MOID_PRINT(hmoId);

    if (!pAgntId || !pRetInstData || !pInstLen)
    {
        goto failure;
    }

    pInst = (ClSnmpReqInfoT *)*pRetInstData;

    if (!pInst)
    {
        pInst = (ClSnmpReqInfoT *) clHeapAllocate(sizeof(ClSnmpReqInfoT));
        if (!pInst)
        {
            rc = CL_SNMP_RC(CL_ERR_NULL_POINTER);
            goto failure;
        }
        
        *pInstLen = sizeof(ClSnmpReqInfoT);
        memset(pInst, 0, *pInstLen);
        *pRetInstData = (void **)pInst;
    }

    for (i = 0; i < pAgntId->len; ++i)
    {
        pInst->oid[i] = pAgntId->id[i];
    }

    pInst->tableType = s->tableType; /* CL_OCTRAIN_SCALARS */

    return CL_OK;
 failure:
    return rc;
}

ClInt32T snmpDefaultInstCompare(ClCntKeyHandleT key1,
                                ClCntKeyHandleT key2)
{
    return 0;
}

ClRcT snmpGetIndexOidList(ClSnmpTableInfoT *t,
                          ClUint8T ***indexOidList,
                          ClUint32T *nIndexes)
{
    ClRcT rc = CL_SNMP_RC(CL_ERR_NO_MEMORY);
    ClListHeadT *iter = NULL;
    ClListHeadT *item = NULL;
    ClUint8T **index = NULL;
    ClUint32T i = 0;

    index = clHeapAllocate((t->nIndexes+1) * sizeof(ClUint8T *));
    if (!index)
    {
        goto failure;
    }
    
    item = &t->indexes;
    CL_LIST_FOR_EACH(iter, item)
    {
        ClSnmpScalarInfoT *s = CL_LIST_ENTRY(iter, ClSnmpScalarInfoT, list);
        
        index[i] = clHeapAllocate((strlen(s->oid)+1) * sizeof(ClUint8T));
        if (!index[i])
        {
            goto failure;
        }

        memcpy(index[i], s->oid, strlen(s->oid)+1);
        clLogInfo("SNMP", "SUB", "index OID is %s", s->oid);
        clLogInfo("SNMP", "SUB", "index OID after copying is %s", (ClCharT *)index[i]);

        ++i;
    }

    {
        index[i] = NULL;
    }

    *indexOidList = index;
    *nIndexes = t->nIndexes;

    return CL_OK;
 failure:
    return rc;
}

ClRcT snmpTableInstXlator(const struct ClMedAgentId *pAgntId,
                          ClCorMOIdPtrT hmoId,
                          ClCorAttrPathPtrT containedPath,
                          void **pRetInstData,
                          ClUint32T *pInstLen,
                          ClPtrT pCorAttrValueDescList,
                          ClMedInstXlationOpEnumT instXlnOp,
                          ClPtrT cookie)
{
    ClRcT rc = CL_OK;
    ClUint32T i = 0;
    ClSnmpReqInfoT *pInst = NULL;
    ClSnmpTableInfoT *t = cookie;

    clLogDebug("SNMP", "SUB", "Table instance translation for MOID");
    CL_MED_COR_STR_MOID_PRINT(hmoId);

    if (!pAgntId || !pRetInstData || !pInstLen)
    {
        rc = CL_SNMP_RC(CL_ERR_NULL_POINTER);
        goto failure;
    }

    pInst = (ClSnmpReqInfoT*)*pRetInstData;
    if (!pInst)
    {
        pInst = clHeapAllocate(sizeof(ClSnmpReqInfoT));
        if (!pInst)
        {
            rc = CL_SNMP_RC(CL_ERR_NO_MEMORY);
            goto failure;
        }
        *pInstLen = sizeof(ClSnmpReqInfoT);
        memset(pInst, 0, *pInstLen);
        *pRetInstData = (void**)pInst;
    }

    for (i = 0; i < pAgntId->len; ++i)
    {
        pInst->oid[i] = pAgntId->id[i];
    }

    pInst->tableType = t->tableType; /* CL_CLOCKTABLE; */
    if (instXlnOp == CL_MED_CREATION_BY_OI)
    {
        ClCorObjectHandleT objHdl;
        ClUint32T attrSize = 0;

        clLogInfo("SNMP", "SUB",
                  "Translating MOID to index for SNMP table [%s]",
                  t->name);
        rc = clCorMoIdServiceSet(hmoId, CL_COR_SVC_ID_PROVISIONING_MANAGEMENT);
        if (rc != CL_OK)
        {
            clLogError("SNMP", "SUB",
                       "Failed to set service ID for MOID, error [%#x]",
                       rc);
            goto failure;
        }

        rc = clCorObjectHandleGet(hmoId, &objHdl);
        if (rc != CL_OK)
        {
            clLogError("SNMP", "SUB",
                       "Failed to get COR object handle, error [%#x]",
                       rc);
            goto failure;
        }

        if (t->attrIdList[0].attrId <= 0)
        {
            ClUint8T **indexOidList = NULL;
            ClUint32T nindexes;
            
            rc = snmpGetIndexOidList(t, &indexOidList, &nindexes);
            if (rc != CL_OK)
            {
                clLogError("SNMP", "SUB",
                           "Getting index OID list failed, "
                           "error %#x",
                           rc);
                CL_ASSERT(0);
            }

            clLogInfo("SNMP", "SUB", "snmpGetIndexOidList returned number of indexes as %d", nindexes);

            for (i = 0; i < nindexes; ++i)
            {
                clLogInfo("SNMP", "SUB", "index OID after returning is %s", (ClCharT *)indexOidList[i]);
            }

            rc = clSnmpTableIndexCorAttrIdInit(nindexes,
                                               indexOidList,
                                               t->attrIdList);
            if (rc != CL_OK)
            {
                clLogError("SNMP", "SUB",
                           "table index cor attribute id initialization failed "
                           "error [%#x]",
                           rc);
            }
        }
        
        attrSize = sizeof(pInst->index.tableInfo.data);

        clLogDebug("SNMP", "SUB",
                   "Getting attribute value for attribute id [%d] "
                   "attribute length : [%d]",
                   t->attrIdList[0].attrId,
                   attrSize);

        rc = clCorObjectAttributeGet(objHdl,
                                     NULL,
                                     t->attrIdList[0].attrId,
                                     -1,
                                     &pInst->index.tableInfo.data,
                                     &attrSize);
        if (rc != CL_OK)
        {
            clLogError("SNMP", "SUB",
                       "COR object attribute get failed, error [%#x]",
                       rc);
            goto failure;
        }
    }

    *pRetInstData = (void**)pInst;

    return CL_OK;
failure:
    return rc;
}

ClInt32T snmpTableInstCompare(ClCntKeyHandleT key1,
                              ClCntKeyHandleT key2)
{
    ClSnmpReqInfoT *pTabIdx1 = NULL;
    ClSnmpReqInfoT *pTabIdx2 = NULL;
    
    /* return 0; */
    
    if (key1 != 0) pTabIdx1 = (ClSnmpReqInfoT *)key1;
    if (key2 != 0) pTabIdx2 = (ClSnmpReqInfoT *)key2;

    if(!pTabIdx1 || !pTabIdx2)
    {
        /* return non-zero value  in case of error condition. */
        return(-1);
    }

    clLogDebug("SNMP", "SUB", "Data1 : [%d] Data2 : [%d]",
               *(ClUint32T*)&pTabIdx1->index.tableInfo.data,
               *(ClUint32T*)&pTabIdx2->index.tableInfo.data);
    
    return memcmp(&(pTabIdx1->index.tableInfo), 
                  &(pTabIdx2->index.tableInfo), 
                  sizeof(pTabIdx1->index.tableInfo));
}

ClRcT snmpBuildAppOIDInfo(ClSnmpOidInfoT **appOidInfo)
{
    ClRcT rc = CL_SNMP_RC(CL_ERR_NULL_POINTER);
    ClSnmpMibInfoT *mibInfo = &gSubAgentInfo.mibInfo;
    ClUint32T nentries = 0;
    ClUint32T i = 0;
    ClSnmpOidInfoT *oidInfo = NULL;
    ClListHeadT *iter = NULL;
    ClListHeadT *items = NULL;

    nentries = mibInfo->nEntries+1;
    i = 0;

    rc = CL_SNMP_RC(CL_ERR_NO_MEMORY);
    oidInfo = clHeapAllocate(nentries * sizeof(ClSnmpOidInfoT));
    if (!oidInfo)
    {
        goto failure;
    }

    items = &mibInfo->scalars;
    CL_LIST_FOR_EACH(iter, items)
    {
        ClSnmpOidInfoT *oid = oidInfo+i;
        ClSnmpScalarInfoT *s = CL_LIST_ENTRY(iter, ClSnmpScalarInfoT, list);

        oid->tableType = s->tableType;
        oid->oid = s->oid; /* Need to copy later */
        oid->numAttr = 1;
        memset(oid->attr, 0, sizeof(oid->attr));
        oid->fpTblIndexCorAttrGet = NULL;
        oid->fpInstXlator = snmpDefaultInstXlator;
        oid->fpInstCompare = snmpDefaultInstCompare;
        oid->cookie = s;
        ++i;
    }

    items = &mibInfo->tables;
    CL_LIST_FOR_EACH(iter, items)
    {
        ClSnmpOidInfoT *oid = oidInfo+i;
        ClSnmpTableInfoT *t = CL_LIST_ENTRY(iter, ClSnmpTableInfoT, list);
        ClListHeadT *iter2 = NULL;
        ClListHeadT *items2 = NULL;
        ClUint32T j = 0;

        oid->tableType = t->tableType;
        oid->oid = t->oid; /* Need to copy later */
        oid->fpTblIndexCorAttrGet = NULL;
        oid->fpInstXlator = snmpTableInstXlator;
        oid->fpInstCompare = snmpTableInstCompare;
        oid->cookie = t;

        oid->numAttr = t->nIndexes;
        items2 = &t->indexes;
        CL_LIST_FOR_EACH(iter2, items2)
        {
            ClSnmpScalarInfoT *index = CL_LIST_ENTRY(iter2,
                                                     ClSnmpScalarInfoT,
                                                     list);
            if (index->type == ASN_OCTET_STR)
            {
                oid->attr[j].type = CL_SNMP_STRING_ATTR;
                oid->attr[j].size = CL_MAX_NAME_LENGTH;
            }
            else
            {
                /* Should we handle all SMI types here ? */
                oid->attr[j].type = CL_SNMP_NON_STRING_ATTR;
                oid->attr[j].size = 4; /* ?? */
            }
            ++j;
        }

        CL_ASSERT(j == oid->numAttr);
        
        ++i;
    }

    {
        ClSnmpOidInfoT *oid = oidInfo+i;

        oid->tableType = 0;
        oid->oid = NULL;
        oid->numAttr = 0;
        memset(oid->attr, 0, sizeof(oid->attr));
        oid->fpTblIndexCorAttrGet = NULL;
        oid->fpInstXlator = NULL;
        oid->fpInstCompare = NULL;
        ++i;
    }

    CL_ASSERT(i == nentries);

    *appOidInfo = oidInfo;
        
    return CL_OK;
failure:
    /* TODO: cleanup */
    return rc;
}

ClInt32T snmpOidCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    ClCharT *oid1 = NULL;
    ClCharT *oid2 = NULL;
    ClInt32T result = 0;

    if (key1 != 0) oid1 = (ClCharT *)key1;
    if (key2 != 0) oid2 = (ClCharT *)key2;

    /* Check the opcode first */
    result = strncmp(oid1, oid2, strlen(oid1));
    if (result == 0)
    {
        if ((oid2[strlen(oid1)] != '\0') && (oid2[strlen(oid1)] != '.'))
            result = 1;
    }

    return result;
}

void snmpOidNodeDelete(ClCntKeyHandleT key, ClCntDataHandleT pData)
{
    /* currently, we don't need anything here. */
    return;
}

ClRcT snmpOpCodeXlationInit(void)
{
    ClRcT rc = CL_OK;
    ClUint32T i = 0;

    ClSnmpOpCodeT snmpOpCodeList[] =
    {
        CL_SNMP_SET, 
        CL_SNMP_GET, 
        CL_SNMP_GET_NEXT,
        CL_SNMP_CREATE, 
        CL_SNMP_DELETE, 
        CL_SNMP_WALK,
        CL_SNMP_GET_FIRST,
        CL_SNMP_GET_SVCS,
        CL_SNMP_CONTAINMENT_SET
    };

    ClMedCorOpEnumT corOpCodeList[] =
    {
        CL_COR_SET, 
        CL_COR_GET, 
        CL_COR_GET_NEXT,
        CL_COR_CREATE, 
        CL_COR_DELETE, 
        CL_COR_WALK,
        CL_COR_GET_FIRST,
        CL_COR_GET_SVCS,
        CL_COR_CONTAINMENT_SET
    };

    for (i = 0; i < CL_SNMP_MAX-1; ++i)
    {
        ClMedAgntOpCodeT agntOpCode;
        ClMedTgtOpCodeT  tgtOpCode;

        tgtOpCode.type = CL_MED_XLN_TYPE_COR;

        agntOpCode.opCode = snmpOpCodeList[i];
        tgtOpCode.opCode = corOpCodeList[i];

        rc = clMedOperationCodeTranslationAdd(gSubAgentInfo.medHdl,
                                              agntOpCode,
                                              &tgtOpCode,
                                              1);
        if (rc != CL_OK)
        {
            clLogError("SNMP", "SUB", "opcode translation addition to "
                       "the mediation library failed, error %#x",
                       rc);
            goto failure;
        }
    }

    return CL_OK;
failure:
    return rc;
}

void snmpFillOidMapTableItem(ClSnmpOidMapTableItemT **destitem,
                             const ClSnmpScalarInfoT *s)
{
    ClSnmpOidMapTableItemT *item = *destitem;
    
    ClCorMOIdT fakeMoId =
    {
        {
            {3, /* e.g. CLASS_CHASSIS_MO */
             CL_COR_INSTANCE_WILD_CARD},
            {s->parentmo, /* e.g. CLASS_CLOCKTABLE_MO */
             CL_COR_INSTANCE_WILD_CARD}
        },
        CL_COR_SVC_ID_PROVISIONING_MANAGEMENT,
        2
    };

    clLogInfo("SNMP", "SUB", "attribute name is [%s] id is [%d]", s->name, s->attr);
    
    ClInt32T fakeCorAttrId[20] =
    {
        s->attr, /* Needs to be filled in */
        CL_MED_ATTR_VALUE_END
    };
        
    item->agntId.id = clHeapAllocate(strlen(s->oid)+1);
    if (!item->agntId.id)
    {
        goto failure;
    }
    memcpy(item->agntId.id, (ClUint8T *)s->oid, strlen(s->oid)+1);
    item->agntId.len = 1; /* ?? */

    memcpy(&item->moId, &fakeMoId, sizeof(ClCorMOIdT));
    memcpy(item->corAttrId, fakeCorAttrId, sizeof(fakeCorAttrId));

failure:
    return;
}

void snmpFillTrapOidMapTableItem(ClSnmpTrapOidMapTableItemT **destitem,
                                 const ClCharT *oid)
{
    ClSnmpTrapOidMapTableItemT *item = *destitem;
    
    ClCorMOIdT fakeMoId =
    {
        {
            {0, /* e.g. CLASS_CHASSIS_MO */
             CL_COR_INSTANCE_WILD_CARD},
            {0, /* e.g. CLASS_CLOCKTABLE_MO */
             CL_COR_INSTANCE_WILD_CARD}
        },
        CL_COR_SVC_ID_PROVISIONING_MANAGEMENT,
        2
    };

    ClInt32T fakeCorAttrId[20] =
    {
        0, /* Needs to be filled in */
        CL_MED_ATTR_VALUE_END
    };
        
    static ClUint32T i = 0;
    
    clLogInfo("SNMP", "SUB", "called %d times", ++i);
    
    item->agntId.id = clHeapAllocate(strlen(oid)+1);
    if (!item->agntId.id)
    {
        goto failure;
    }
    memcpy(item->agntId.id, (ClUint8T *)oid, strlen(oid)+1);
    item->agntId.len = 1; /* ?? */

    memcpy(&item->moId, &fakeMoId, sizeof(ClCorMOIdT));
    memcpy(item->corAttrId, fakeCorAttrId, sizeof(fakeCorAttrId));

    item->probableCause = 0;
    item->specificProblem = 0;
    item->pTrapDes = NULL;

failure:
    return;
}

ClRcT snmpBuildOidMapTable(ClSnmpOidMapTableItemT **oidMapTable)
{
    ClRcT rc = CL_OK;
    ClSnmpMibInfoT *mibInfo = &gSubAgentInfo.mibInfo;
    ClUint32T nentries = 0;
    ClUint32T i = 0;
    ClListHeadT *iter = NULL;
    ClListHeadT *items = NULL;
    ClSnmpOidMapTableItemT *oidMap = NULL;

    nentries = mibInfo->nOidEntries+1;
    i = 0;

    rc = CL_SNMP_RC(CL_ERR_NO_MEMORY);
    oidMap = clHeapAllocate(nentries * sizeof(ClSnmpOidMapTableItemT));
    if (!oidMap)
    {
        goto failure;
    }

    items = &mibInfo->scalars;
    CL_LIST_FOR_EACH(iter, items)
    {
        ClSnmpOidMapTableItemT *item = oidMap+i;
        ClSnmpScalarInfoT *s = CL_LIST_ENTRY(iter, ClSnmpScalarInfoT, list);

        snmpFillOidMapTableItem(&item, s);

        ++i;
    }

    /* items = &mibInfo->tables; */
    /* CL_LIST_FOR_EACH(iter, items) */
    /* { */
    /*     ClSnmpOidMapTableItemT *item = oidMap+i; */
    /*     ClSnmpTableInfoT *t = CL_LIST_ENTRY(iter, ClSnmpTableInfoT, list); */

    /*     snmpFillOidMapTableItem(&item, t->oid); */
        
    /*     ++i; */
    /* } */

    items = &mibInfo->tables;
    CL_LIST_FOR_EACH(iter, items)
    {
        ClSnmpTableInfoT *t = CL_LIST_ENTRY(iter, ClSnmpTableInfoT, list);
        ClListHeadT *iter2 = NULL;
        ClListHeadT *items2 = NULL;

        items2 = &t->indexes;
        CL_LIST_FOR_EACH(iter2, items2)
        {
            ClSnmpOidMapTableItemT *item2 = oidMap+i;
            ClSnmpScalarInfoT *s = CL_LIST_ENTRY(iter2, ClSnmpScalarInfoT, list);

            snmpFillOidMapTableItem(&item2, s);

            ++i;
        }

        items2 = &t->nonindexes;
        CL_LIST_FOR_EACH(iter2, items2)
        {
            ClSnmpOidMapTableItemT *item2 = oidMap+i;
            ClSnmpScalarInfoT *s = CL_LIST_ENTRY(iter2, ClSnmpScalarInfoT, list);

            snmpFillOidMapTableItem(&item2, s);

            ++i;
        }
    }

    {
        ClSnmpOidMapTableItemT *item = oidMap+i;
        ClSnmpOidMapTableItemT emptyItem =
        {
            {NULL, 0},
            {
                {
                    {0, 0}
                },
                0,
                0
            },
            {0}
        };

        memcpy(item, &emptyItem, sizeof(ClSnmpOidMapTableItemT));
        
        ++i;
    }

    CL_ASSERT(i == nentries);

    for (i = 0; oidMap[i].agntId.id; ++i)
    {
        clCorMoIdShow(&oidMap[i].moId);
    }

    *oidMapTable = oidMap;
    
    return CL_OK;
failure:
    return rc;
}

ClRcT snmpIdXlationInit(void)
{
    ClRcT rc = CL_OK;
    ClSnmpOidMapTableItemT *oidMapTable = NULL;
    ClUint32T i = 0;
    ClMedTgtIdT corIdentifier;

    memset(&corIdentifier, 0, sizeof(ClMedTgtIdT));
    
    rc = snmpBuildOidMapTable(&oidMapTable);
    if (rc != CL_OK)
    {
        goto failure;
    }

    /* Looks like following two for loops differ only in the
       mediation library function they call. Combine them.
    */
    for (i = 0; oidMapTable[i].agntId.id; ++i)
    {
        memset(&corIdentifier, 0, sizeof(ClMedTgtIdT));
        corIdentifier.type = CL_MED_XLN_TYPE_COR;

        /* TODO: move to snmpBuildOidMapTable */
        oidMapTable[i].agntId.len = strlen((ClCharT*)oidMapTable[i].agntId.id) + 1;

        corIdentifier.info.corId.moId = &oidMapTable[i].moId;
        corIdentifier.info.corId.attrId = oidMapTable[i].corAttrId;

        rc = clMedIdentifierTranslationInsert(gSubAgentInfo.medHdl,
                                              oidMapTable[i].agntId, 
                                              &corIdentifier,
                                              1);
        if (rc != CL_OK)
        {
            clLogError("SNMP", "SUB", "identifier translation insertion to the "
                       "mediation library failed, error %#x", rc);
            goto failure;
        }
    }

    for (i = 0; oidMapTable[i].agntId.id; ++i)
    {
        memset(&corIdentifier, 0, sizeof(ClMedTgtIdT));
        corIdentifier.type = CL_MED_XLN_TYPE_COR;
        
        oidMapTable[i].agntId.len = strlen((ClCharT*)oidMapTable[i].agntId.id) + 1;

        corIdentifier.info.corId.moId = &oidMapTable[i].moId;
        corIdentifier.info.corId.attrId = oidMapTable[i].corAttrId;

        rc = clMedInstanceTranslationInsert(gSubAgentInfo.medHdl,
                                            oidMapTable[i].agntId, 
                                            &corIdentifier,
                                            1);
        if (rc != CL_OK)
        {
            clLogError("SNMP", "SUB", "instance translation insertion to the "
                       "mediation library failed, error %#x", rc);
            goto failure;
        }
    }

    return CL_OK;
failure:
    return rc;
}

ClRcT snmpErrorCodeXlationInit(void)
{
    ClRcT rc = CL_OK;
    ClUint32T i = 0;
    ClUint32T nerrors = sizeof(oidErrMapTable)/sizeof(oidErrMapTable[0]);

    for (i = 0; i < nerrors; ++i)
    {
        rc = clMedErrorIdentifierTranslationInsert(gSubAgentInfo.medHdl, 
                                                   oidErrMapTable[i].sysErrorType,
                                                   oidErrMapTable[i].sysErrorId, 
                                                   oidErrMapTable[i].agntErrorId);
        if (rc != CL_OK)
        {
            clLogError("SNMP", "SUB", "error code translation addition to the "
                       "mediation library failed, error %#x", rc);
            goto failure;
        }
    }
    
    return CL_OK;
failure:
    return rc;
}

ClRcT snmpOidAdd(ClSnmpOidInfoT *pTable)
{
    return clCntNodeAdd(gSubAgentInfo.oidCntHdl,
                        (ClCntKeyHandleT)pTable->oid,
                        (ClCntDataHandleT)pTable,
                        NULL);
}

ClRcT snmpBuildTrapOidMapTable(ClSnmpTrapOidMapTableItemT **trapOidMapTable)
{
    ClRcT rc = CL_OK;
    ClSnmpMibInfoT *mibInfo = &gSubAgentInfo.mibInfo;
    ClUint32T ntraps = 0;
    ClUint32T i = 0;
    ClSnmpTrapOidMapTableItemT *trapOidMap = NULL;
    ClListHeadT *iter = NULL;
    ClListHeadT *items = NULL;
    
    ntraps = mibInfo->nTraps+1;
    i = 0;

    rc = CL_SNMP_RC(CL_ERR_NO_MEMORY);
    trapOidMap = clHeapAllocate(ntraps * sizeof(ClSnmpTrapOidMapTableItemT));
    if (!trapOidMap)
    {
        goto failure;
    }

    items = &mibInfo->traps;
    CL_LIST_FOR_EACH(iter, items)
    {
        ClSnmpTrapOidMapTableItemT *item = trapOidMap+i;
        ClSnmpTrapInfoT *t = CL_LIST_ENTRY(iter, ClSnmpTrapInfoT, list);

        snmpFillTrapOidMapTableItem(&item, t->oid);

        ++i;
    }

    CL_ASSERT(i == mibInfo->nTraps);

    *trapOidMapTable = trapOidMap;

    return CL_OK;
failure:
    return rc;
}

ClRcT snmpAlarmIdToOidGet(ClCorMOIdPtrT pMoId,
                          ClUint32T probableCause,
                          ClAlarmSpecificProblemT specificProblem, 
                          ClCharT *pTrapOid,
                          ClCharT *pDes)
{
    ClRcT rc = CL_OK;
    ClUint32T i = 0;
    ClInt32T equal = 0;
    ClSnmpTrapOidMapTableItemT *trapOidMapTable = NULL;
    
    rc = snmpBuildTrapOidMapTable(&trapOidMapTable);
    if (rc != CL_OK)
    {
        goto done;
    }
    
    rc = CL_SNMP_RC(CL_ERR_NOT_EXIST);
    return rc;

    for (i = 0; trapOidMapTable[i].agntId.id; ++i)
    {
        ClSnmpTrapOidMapTableItemT *item = trapOidMapTable+i;
        
        if ((item->probableCause == probableCause) &&
            (item->specificProblem == specificProblem))
        {
            equal = clCorMoIdCompare(&item->moId, pMoId);
            if ((equal == 0) || (equal == 1))
            {
                strncpy(pTrapOid,
                        (ClCharT*)item->agntId.id,
                        strlen((ClCharT*)item->agntId.id));

                if (!item->pTrapDes)
                {
                    strcpy(pDes, (ClCharT*)item->pTrapDes);
                }
                goto done;
            }
        }
    }
    return CL_OK;
done:
    return rc;
}

ClRcT snmpIndexToOidGet(ClAlarmUtilTlvInfoPtrT pIndexTlvInfo,
                        ClUint32T *pObjOid)
{
    ClRcT rc = CL_OK;
    ClUint32T numIndexTlv = 0;

    if (!pIndexTlvInfo)
    {
        rc = CL_SNMP_RC(CL_ERR_NULL_POINTER);
        goto failure;
    }

    if ((pIndexTlvInfo->numTlvs > 0) && (!pObjOid))
    {
        rc = CL_SNMP_RC(CL_ERR_NULL_POINTER);
        goto failure;
    }

    for (numIndexTlv= 0; numIndexTlv < pIndexTlvInfo->numTlvs; ++numIndexTlv)
    {
        switch (pIndexTlvInfo->pTlv[numIndexTlv].type)
        {
            case CL_COR_INT8:
            case CL_COR_UINT8:
            {
                if (pIndexTlvInfo->pTlv[numIndexTlv].length == 1)
                {
                    *pObjOid++ = (ClUint32T )(*(ClUint8T *)pIndexTlvInfo->pTlv[numIndexTlv].value);
                }
                else
                {
                    ClUint32T count = 0;

                    *pObjOid++ = pIndexTlvInfo->pTlv[numIndexTlv].length;
                    for (count = 0; count < pIndexTlvInfo->pTlv[numIndexTlv].length; ++count)
                    {
                        *pObjOid++ = (ClUint32T)(*(ClUint8T *)
                                                 ((ClUint8T *)(pIndexTlvInfo->pTlv[numIndexTlv].value) +
                                                  count) );
                    }
                }
                break;
            }
            case CL_COR_INT32:
            case CL_COR_UINT32:
            {
                if (pIndexTlvInfo->pTlv[numIndexTlv].length == 4)
                {
                    *pObjOid++ = *(ClUint32T *)pIndexTlvInfo->pTlv[numIndexTlv].value;
                }
                else
                {
                    *pObjOid++ =  pIndexTlvInfo->pTlv[numIndexTlv].length / 4;
                    memcpy(pObjOid,
                           pIndexTlvInfo->pTlv[numIndexTlv].value,
                           pIndexTlvInfo->pTlv[numIndexTlv].length);
                }
                break;
            }
            default:
                break;
        }
    }

    return CL_OK;
failure:
    return rc;
}

ClRcT snmpTrapPayloadGet(ClAlarmUtilTlvPtrT pTlv,
                         ClUint32T oidLen,
                         ClUint32T *pObjOid,
                         ClTrapPayLoadListT *pTrapPayloadList)
{
    ClRcT rc = CL_SNMP_RC(CL_ERR_NO_MEMORY);
    
    pTrapPayloadList->pPayLoad[pTrapPayloadList->numTrapPayloadEntry].oidLen = oidLen;
    pTrapPayloadList->pPayLoad[pTrapPayloadList->numTrapPayloadEntry].objOid = clHeapAllocate(oidLen);
    if (!(pTrapPayloadList->pPayLoad[pTrapPayloadList->numTrapPayloadEntry].objOid))
    {
        goto failure;
    }

    memcpy(pTrapPayloadList->pPayLoad[pTrapPayloadList->numTrapPayloadEntry].objOid, pObjOid, oidLen);
    pTrapPayloadList->pPayLoad[pTrapPayloadList->numTrapPayloadEntry].valLen = pTlv->length;
    pTrapPayloadList->pPayLoad[pTrapPayloadList->numTrapPayloadEntry++].val = pTlv->value;

    return CL_OK;
failure:
    return rc;
}

ClRcT snmpAppNotifyIndexGet(ClUint32T objectType,
                            ClCorMOIdPtrT pMoId,
                            ClAlarmUtilTlvInfoPtrT pTlvList)
{
    return CL_OK;
}

void snmpAppNotify(ClTrapPayLoadListT *pTrapPayloadList)
{
    return;
}

ClRcT snmpAppNotifyHandler(ClCharT *pTrapOid,
                           ClCharT *pData,
                           ClUint32T dataLen,
                           ClCharT *pTrapDes)
{
    ClRcT rc = CL_OK;
    ClUint32T oidLen = 0;
    ClUint32T tlvCount = 0;
    ClUint32T allocatedOidLen = 0;
    ClUint32T *pObjOid = NULL;
    ClAlarmHandleInfoT* pAlarmHandleInfo = NULL;
    ClAlarmUtilTlvInfoT indexTlvInfo = {0};
    ClAlarmUtilPayLoadListT *pAlarmPayLoadList = NULL;
    ClTrapPayLoadListT trapPayloadList = {0};

    if (!pTrapOid)
    {
        rc = CL_SNMP_RC(CL_ERR_NULL_POINTER);
        goto failure;
    }

    if (!dataLen)
    {
        rc = CL_SNMP_RC(CL_ERR_INVALID_PARAMETER);
        goto failure;
    }

    pAlarmHandleInfo = (ClAlarmHandleInfoT*) pData;
    if (pAlarmHandleInfo->alarmInfo.len > 0)
    {
        ClUint32T i = 0;
        ClUint32T j = 0;
        ClUint32T nIndexEntries = 0;

        rc = clAlarmUtilPayLoadExtract((ClUint8T *)pAlarmHandleInfo->alarmInfo.buff,
                                       pAlarmHandleInfo->alarmInfo.len,
                                       &pAlarmPayLoadList);
        if (rc != CL_OK)
        {
            clLogError("SNMP", "SUB",
                       "Failed to extract alarm payload, error %#x",
                       rc);
            goto failure;
        }

        if (!pAlarmPayLoadList)
        {
            rc = CL_SNMP_RC(CL_ERR_NULL_POINTER);
            goto failure;
        }

        /* Calculate total number of object entries in the trap
           notification.
         */
        for (i = 0; i < pAlarmPayLoadList->numPayLoadEnteries; i++)
        {
            nIndexEntries += pAlarmPayLoadList->pPayload[i].numTlvs;
        }

        trapPayloadList.pPayLoad = clHeapAllocate(nIndexEntries *
                                                  sizeof(ClTrapPayLoadT));
        if (!trapPayloadList.pPayLoad)
        {
            rc = CL_SNMP_RC(CL_ERR_NO_MEMORY);
            goto failure;
        }

        /* Go through all the alarm payload entries in
           alarmPayLoadList.
        */
        for (i = 0; i < pAlarmPayLoadList->numPayLoadEnteries; ++i)
        {
            /* For each alarm payload entry go through all the
               tlvs.
            */
            for (j = 0; j < pAlarmPayLoadList->pPayload[i].numTlvs; ++j)
            {
                ClUint32T numTlvInIndex = 0;
                oidLen = 0;

                /*
                 * This is kept inside the loop to consider the case
                 * where same moId can map to multiple table.  In this
                 * case for same moID, we will have different indices.
                 */
                
                /* Convert the moId information to index and then fill
                   the indexTlvInfo structure.
                */
                rc = snmpAppNotifyIndexGet(tlvCount,
                                           pAlarmPayLoadList->pPayload[i].pMoId,
                                           &indexTlvInfo);
                if (rc != CL_OK)
                {
                    clLogError("SNMP", "SUB",
                               "MOID to index get failed, error %#x",
                               rc);
                    goto failure;
                }

                /* Calculate space required for index oid.
                 */
                for (numTlvInIndex = 0; numTlvInIndex < indexTlvInfo.numTlvs; ++numTlvInIndex)
                {
                    if ((CL_COR_INT8 == indexTlvInfo.pTlv[numTlvInIndex].type) ||
                        (CL_COR_UINT8 == indexTlvInfo.pTlv[numTlvInIndex].type))
                    {
                        oidLen += (indexTlvInfo.pTlv[numTlvInIndex].length * 4);
                        if (indexTlvInfo.pTlv[numTlvInIndex].length > 1)
                        {
                            oidLen += 4;
                        }
                    }
                    else if ((CL_COR_INT32 == indexTlvInfo.pTlv[numTlvInIndex].type) ||
                             (CL_COR_UINT32 == indexTlvInfo.pTlv[numTlvInIndex].type))
                    {
                        oidLen += indexTlvInfo.pTlv[numTlvInIndex].length;
                        if (indexTlvInfo.pTlv[numTlvInIndex].length > 4)
                        {
                            oidLen += 4;
                        }
                    }
                }
                if (oidLen > allocatedOidLen)
                {
                    pObjOid = clHeapRealloc(pObjOid, oidLen);
                    if (!pObjOid)
                    {
                        rc = CL_SNMP_RC(CL_ERR_NO_MEMORY);
                        goto failure;
                    }
                    allocatedOidLen = oidLen;
                }
                
                rc = snmpIndexToOidGet(&indexTlvInfo, pObjOid);
                if (rc != CL_OK)
                {
                    clLogError("SNMP", "SUB",
                               "Failed to convert tlv to oid, error %#x",
                               rc);
                    goto failure;
                }
                
                /* Form a structure of type trapPayLoad from this.
                 */
                rc = snmpTrapPayloadGet((pAlarmPayLoadList->pPayload[i].pTlv + j),
                                        oidLen,
                                        pObjOid,
                                        &trapPayloadList);
                if (rc != CL_OK)
                {
                    clLogError("SNMP", "SUB",
                               "Failed to get trap payload from trap oid, error %#x",
                               rc);
                    goto failure;
                }
                
                /* This has internal pointers so need to freed
                   carefully.
                */
                {
                    ClUint32T tlvCount = 0;
                    for (tlvCount = 0; tlvCount < indexTlvInfo.numTlvs; tlvCount++)
                    {
                        clHeapFree(indexTlvInfo.pTlv[tlvCount].value);
                    }
                    clHeapFree(indexTlvInfo.pTlv);
                }
                tlvCount++;
            }
        }
        
        snmpAppNotify(&trapPayloadList);

        if (pObjOid)
        {
            clHeapFree(pObjOid);
            pObjOid = NULL;
        }

        for(i = 0; i < trapPayloadList.numTrapPayloadEntry; ++i)
        {
            if (trapPayloadList.pPayLoad[i].objOid)
            {
                clHeapFree(trapPayloadList.pPayLoad[i].objOid);
                trapPayloadList.pPayLoad[i].objOid = NULL;
            }
        }

        if(trapPayloadList.pPayLoad)
        {
            clHeapFree(trapPayloadList.pPayLoad);
            trapPayloadList.pPayLoad = NULL;
        }
        clAlarmUtilPayloadListFree(pAlarmPayLoadList);
    }
    else
    {
        snmpAppNotify(NULL);
    }

    return CL_OK;
failure:
    return rc;
}

ClRcT snmpNotifyHandler(ClCorMOIdPtrT pMoId,
                        ClUint32T probableCause,
                        ClAlarmSpecificProblemT specificProblem,
                        ClCharT *pData,
                        ClUint32T dataLen)
{
    ClRcT rc = CL_OK;
    ClCharT pTrapOid[CL_MAX_NAME_LENGTH];
    ClCharT pTrapDes[CL_MAX_NAME_LENGTH];

    if (!pMoId)
    {
        rc = CL_SNMP_RC(CL_ERR_NULL_POINTER);
        goto failure;
    }

    memset(pTrapOid, 0, sizeof(pTrapOid));
    memset(pTrapDes, 0, sizeof(pTrapDes));

    rc = snmpAlarmIdToOidGet(pMoId,
                             probableCause,
                             specificProblem,
                             pTrapOid,
                             pTrapDes);
    if (rc != CL_OK)
    {
        clLogWarning("SNMP", "SUB", "Traps not configured");
        goto failure;
    }

    rc = snmpAppNotifyHandler(pTrapOid, pData, dataLen, pTrapDes);
    if (rc != CL_OK)
    {
        clLogError("SNMP", "SUB", "SNMP app notify handler "
                   "failed, error %#x",
                   rc);
        goto failure;
    }

    return CL_OK;
failure:
    return rc;
}

ClRcT snmpInstXlator(const ClMedAgentIdT *pAgntId,
                     ClCorMOIdPtrT pMoId,
                     ClCorAttrPathPtrT containedPath,
                     void **pRetInstData,
                     ClUint32T *instLen,
                     ClPtrT pCorAttrValueDescList,
                     ClMedInstXlationOpEnumT instXlnOp,
                     ClPtrT cookie)
{
    ClRcT rc = CL_OK;
    ClCntNodeHandleT nodeHandle = 0;
    ClSnmpOidInfoT *pOidInfo = NULL;

    if (!pMoId)
    {
        rc = CL_SNMP_RC(CL_ERR_NULL_POINTER);
        goto failure;
    }

    rc = clCntNodeFind(gSubAgentInfo.oidCntHdl,
                       (ClCntKeyHandleT)pAgntId->id,
                       &nodeHandle);
    if (rc != CL_OK)
    {
        goto failure;
    }

    rc = clCntNodeUserDataGet(gSubAgentInfo.oidCntHdl,
                              nodeHandle,
                              (ClCntDataHandleT *)&pOidInfo);
    if (rc != CL_OK)
    {
        goto failure;
    }
    
    rc = pOidInfo->fpInstXlator(pAgntId,
                                pMoId,
                                containedPath,
                                pRetInstData,
                                instLen,
                                NULL,
                                instXlnOp,
                                pOidInfo->cookie);

    if (rc != CL_OK)
    {
        clLogError("SNMP", "SUB", "Running instance translator "
                   "failed, error %#x",
                   rc);
        goto failure;
    }
    
    if (instXlnOp == CL_MED_CREATION_BY_NORTHBOUND)
    {
        if (!pOidInfo->fpTblIndexCorAttrGet)
        {
            clLogError("SNMP", "SUB", "No callback to get COR attributes for table indices");
            rc = CL_SNMP_RC(CL_ERR_NULL_POINTER);
            goto failure;
        }
        
        rc = pOidInfo->fpTblIndexCorAttrGet(*pRetInstData,
                                            pCorAttrValueDescList);
        if (rc != CL_OK)
        {
            clLogError("SNMP", "SUB", "Running index COR attribute get "
                       "failed, error %#x",
                       rc);
            goto failure;
        }
    }

    return CL_OK;
failure:
    return rc;
}

ClInt32T snmpInstCompare(ClCntKeyHandleT key1,
                         ClCntKeyHandleT key2)
{
    ClRcT rc = CL_OK;
    ClSnmpReqInfoT *reqInfo1 = NULL;
    ClSnmpReqInfoT *reqInfo2 = NULL;
    ClUint32T cmp = 0;
    ClSnmpOidInfoT *pOidInfo = NULL;
    ClCntNodeHandleT nodeHandle;

    if (key1)
    {
        reqInfo1 = (ClSnmpReqInfoT*)key1;
    }
    
    if (key2)
    {
        reqInfo2 = (ClSnmpReqInfoT*)key2;
    }

    if (!reqInfo1 && reqInfo2)
    {
        cmp = -1;
        goto done;
    }
    else if (reqInfo1 && !reqInfo2)
    {
        cmp = 1;
        goto done;
    }
    else if (!reqInfo1 && !reqInfo2)
    {
        cmp = 0;
        goto done;
    }

    rc = clCntNodeFind(gSubAgentInfo.oidCntHdl,
                       (ClCntKeyHandleT)reqInfo1->oid,
                       &nodeHandle);
    if (rc != CL_OK)
    {
        clLogError("SNMP", "SUB", "Not able to find oid %s, "
                   "error %#x",
                   (ClCharT *)reqInfo1->oid,
                   rc);
        cmp = rc; /* ?? */
        goto done;
    }
    
    rc = clCntNodeUserDataGet(gSubAgentInfo.oidCntHdl,
                              nodeHandle,
                              (ClCntDataHandleT *)&pOidInfo);
    if (rc != CL_OK)
    {
        cmp = rc; /* ?? */
        goto done;
    }

    return pOidInfo->fpInstCompare(key1, key2);
done:
    return cmp;
}

ClRcT snmpMedInit(void)
{
    ClRcT rc = CL_OK;
    ClSnmpOidInfoT *appOidInfo = NULL;
    ClUint32T i = 0;
    
    rc = snmpBuildAppOIDInfo(&appOidInfo);
    if (rc != CL_OK)
    {
        goto failure;
    }

    rc = clCntLlistCreate(snmpOidCompare,
                          snmpOidNodeDelete,
                          snmpOidNodeDelete,
                          CL_CNT_UNIQUE_KEY,
                          &gSubAgentInfo.oidCntHdl);
    if (rc != CL_OK)
    {
        goto failure;
    }

    for (i = 0; appOidInfo[i].oid; ++i)
    {
        snmpOidAdd(&appOidInfo[i]);
    }

    rc = clMedInitialize(&gSubAgentInfo.medHdl,
                         snmpNotifyHandler,
                         snmpInstXlator,
                         snmpInstCompare);
    if (rc != CL_OK)
    {
        goto failure;
    }

    rc = snmpOpCodeXlationInit();
    if (rc != CL_OK)
    {
        goto failure;
    }

    rc = snmpIdXlationInit();
    if (rc != CL_OK)
    {
        goto failure;
    }

    rc = snmpErrorCodeXlationInit();
    if (rc != CL_OK)
    {
        goto failure;
    }

    return CL_OK;
failure:
    return rc;
}

/* convert this to clovis convention later */
oid *get_oid_array(const char *oid_in, int *len)
{
    char oid_str[BUFSIZ];
    char *o;
    char *saveptr;
    int n;
    oid *oid_array;
    int i;

    strncpy(oid_str, oid_in, BUFSIZ-1);

    n = 0;
    o = strtok_r(oid_str, ".", &saveptr);
    while (o)
    {  
        ++n;
        o = strtok_r(NULL, ".", &saveptr);
    }

    oid_array = clHeapAllocate(n * sizeof(oid));
    assert(oid_array != NULL);

    strncpy(oid_str, oid_in, BUFSIZ-1);

    for (i = 0, o = strtok_r(oid_str, ".", &saveptr);
         o;
         ++i, o = strtok_r(NULL, ".", &saveptr))
    {  
        oid_array[i] = atoi(o);
    }

    if (len)
        *len = n;

    return oid_array;
}

int snmpGenericScalarHandler(netsnmp_mib_handler *handler,
                             netsnmp_handler_registration *reginfo,
                             netsnmp_agent_request_info *reqinfo,
                             netsnmp_request_info *requests)
{
    int rc = SNMP_ERR_NOERROR;
    ClSnmpReqInfoT requestInfo;
    ClInt32T noOfOp = 0;
    netsnmp_variable_list *reqVar = NULL;
    ClSnmpScalarInfoT *s = reginfo->my_reg_void;
    oid *scalarOid = NULL;
    ClInt32T scalarOidLen = 0;

    clLogDebug("SNMP", "SUB", "Generic scalar handler");

    if (!requests || !reqinfo)
    {
        return CL_SNMP_RC(CL_ERR_NULL_POINTER);
    }

    memset(&requestInfo, 0, sizeof(ClSnmpReqInfoT));
    reqVar = requests->requestvb;

    clLogDebug("SNMP", "SUB", "Received request for scalar [%s]", s->name);

    scalarOid = get_oid_array(s->oid, &scalarOidLen);
    requestInfo.oidLen = scalarOidLen;
    clSnmpOidCpy(&requestInfo, reqVar->name);

    requestInfo.tableType = s->tableType; /* CL_$name.uc_SCALARS */

    switch (reqinfo->mode)
    {
        case MODE_GET:
        {
            ClRcT ret = CL_OK;
            ClInt32T errorCode = 0;
            _ClSnmpGetReqInfoT reqAddInfo;
            
            clLogDebug("SNMP", "SUB", "Received GET request");

            requestInfo.index.scalarType = 0;
            requestInfo.opCode = CL_SNMP_GET;
            reqAddInfo.pRequest = requests;
            reqAddInfo.columnType = s->type;
            
            switch (s->type)
            {
                case ASN_INTEGER:
                {
                    requestInfo.dataLen = sizeof(ClInt32T);
                    break;
                }
                case ASN_COUNTER:
                case ASN_TIMETICKS:
                case ASN_UNSIGNED: /* also ASN_GAUGE */
                {
                    requestInfo.dataLen = sizeof(ClUint32T);
                    break;
                }
                case ASN_OCTET_STR:
                {
                    requestInfo.dataLen = (CL_MAX_NAME_LENGTH-1) * sizeof(ClCharT);
                    break;
                }
                case ASN_OBJECT_ID:
                {
                    requestInfo.dataLen = CL_SNMP_MAX_OID_LEN * sizeof(ClUint32T);
                    break;
                }
                case ASN_COUNTER64:
                {
                    requestInfo.dataLen = sizeof(ClUint64T);
                    break;
                }
                case ASN_IPADDRESS:
                {
                    requestInfo.dataLen = 4 * sizeof(ClUint8T);
                    break;
                }
                case ASN_OPAQUE:
                {
                    requestInfo.dataLen = sizeof(ClUint8T);
                    break;
                }
                default:
                {
                    clLogDebug("SNMP", "SUB",
                               "Invalid data type [%d] for scalar [%s]",
                               s->type,
                               s->name);
                    break;
                }
            }

            ret = clRequestAdd(&requestInfo,
                               NULL,
                               NULL,
                               &errorCode,
                               &reqAddInfo);
            if ((ret != CL_OK) || (errorCode < 0))
            {
                netsnmp_request_set_error(requests, errorCode);
                rc = ret;
                goto failure;
            }
            break;
        }
        case MODE_SET_RESERVE1:
        {
            int ret = 0;
            ClUint32T scalarType = 0;

            clLogDebug("SNMP", "SUB", "Received RESERVE1 request");

            if (s->settable)
            {
                switch (scalarType)
                {
                    /* TODO: verification for scalars
                       for which enums, ranges are
                       defined */
                    default:
                    {
                        break;
                    }
                }

                ret = netsnmp_check_vb_type(reqVar, scalarType);
                if (ret != SNMP_ERR_NOERROR)
                {
                    netsnmp_request_set_error(requests, ret);
                    rc = ret;
                    goto failure;
                }
            }
            break;
        }
        case MODE_SET_FREE:
        {
            clLogDebug("SNMP", "SUB", "Received SET FREE request");
            clSnmpUndo();
            break;
        }
        case MODE_SET_RESERVE2:
        {
            ClRcT ret = CL_OK;
            ClInt32T errorCode = 0;
            ClSnmpGenValT val;
            ClPtrT pVal = NULL;

            clLogDebug("SNMP", "SUB", "Received RESERVE2 request");

            requestInfo.opCode = CL_SNMP_SET;

            clLogDebug("SNMP", "SUB", "Request for scalar [%s]", s->name);

            switch (s->type)
            {
                /* TODO:
                   Here again based on the column
                   data type fill in the pVal and
                   requestInfo.dataLen.
                */
                case ASN_INTEGER:
                {
                    val.i = 0;
                    pVal = &val.i;
                    requestInfo.dataLen = sizeof(ClInt32T);
                    break;
                }
                case ASN_COUNTER:
                case ASN_TIMETICKS:
                case ASN_UNSIGNED: /* also ASN_GAUGE */
                {
                    val.ui = 0;
                    pVal = &val.ui;
                    requestInfo.dataLen = sizeof(ClUint32T);
                    break;
                }
                case ASN_OCTET_STR:
                {
                    memset(val.octet, 0, sizeof(val.octet));
                    pVal = val.octet;
                    requestInfo.dataLen = sizeof(val.octet);
                    clLogDebug("SNMP", "SUB", "size of data for octet string is %d", requestInfo.dataLen);
                    break;
                }
                case ASN_OBJECT_ID:
                {
                    memset(val.oid, 0, sizeof(val.oid));
                    pVal = val.oid;
                    requestInfo.dataLen = sizeof(val.oid);
                    clLogDebug("SNMP", "SUB", "size of data for object id is %d", requestInfo.dataLen);
                    break;
                }
                case ASN_COUNTER64:
                {
                    /* TODO */
                    break;
                }
                case ASN_IPADDRESS:
                {
                    memset(val.ip, 0, sizeof(val.ip));
                    pVal = val.ip;
                    requestInfo.dataLen = sizeof(val.ip);
                    break;
                }
                case ASN_OPAQUE:
                {
                    val.opaque = 0;
                    pVal = &val.opaque;
                    requestInfo.dataLen = val.opaque;
                    break;
                }
                default:
                {
                    clLogDebug("SNMP", "SUB",
                               "Invalid data type [%d] for scalar [%s]",
                               s->type,
                               s->name);
                    break;
                }
            }

            memcpy(pVal,
                   reqVar->val.integer,
                   requestInfo.dataLen);

            clLogDebug("SNMP", "SUB", "Adding the request");

            ret = clRequestAdd(&requestInfo,
                               pVal,
                               &noOfOp,
                               &errorCode,
                               NULL);
            if (ret != CL_OK)
            {
                clLogError("SNMP", "SUB",
                           "Failed to add request request, "
                           "error [%#x], error code [%#x]",
                           ret,
                           errorCode);
                netsnmp_request_set_error(requests, errorCode);
                rc = ret;
                goto failure;
            }

            clLogDebug("SNMP", "SUB",
                       "Successfully added request");
            break;
        }
        case MODE_SET_ACTION:
        {
            ClRcT ret = CL_OK;
            ClMedErrorListT medErrList;

            memset(&medErrList, 0, sizeof(ClMedErrorListT));

            clLogDebug("SNMP", "SUB",
                       "Received ACTION request for oid [%s]",
                       requestInfo.oid);

            ret = clSnmpCommit(&medErrList);
            if (ret != CL_OK)
            {
                clLogDebug("SNMP", "SUB",
                           "SNMP commit failed, error count [%d]",
                           medErrList.count);
                if (ret == CL_ERR_NO_MEMORY)
                {
                    /* Set SNMP equivalent error from oidErrMapTable in clSnmpInit.c */
                    ret = CL_SNMP_ERR_NOCREATION;

                    clLogError("SUB", "TBL",
                               "ACTION phase failed, "
                               "setting error code [%#x] for oid [%s]", 
                               ret,
                               requestInfo.oid);
                    netsnmp_request_set_error(requests, ret);
                    rc = ret;
                    goto failure;
                }
                else
                {
                    ClUint32T i;

                    for(i = 0; i < medErrList.count; i++)
                    {
                        /* if the oid matches the request,
                           set error and return. */
                        if (!memcmp(requestInfo.oid, 
                                    medErrList.pErrorList[i].oidInfo.id,
                                    medErrList.pErrorList[i].oidInfo.len)) 
                        {
                            clLogError("SNMP", "SUB",
                                       "ACTION phase failed, "
                                       "setting error code [%#x] "
                                       "for oid [%s]", 
                                       medErrList.pErrorList[i].errId,
                                       requestInfo.oid);
                            netsnmp_request_set_error(requests,
                                                      medErrList.
                                                      pErrorList[i].errId);
                            rc = ret;
                            goto failure;
                        }
                    }
                }
            }
            break;
        }
        case MODE_SET_COMMIT:
        {
            clLogDebug("SNMP", "SUB", "Received COMMIT request");
            clSnmpUndo();
            break;
        }
        case MODE_SET_UNDO:
        {
            clLogDebug("SNMP", "SUB", "Received UNDO request");
            clSnmpUndo();
            break;
        }
        default:
        {
            clLogError("SNMP", "SUB",
                       "Unknown mode [%d] in generic scalar handler",
                       reqinfo->mode);
            rc = SNMP_ERR_GENERR;
            goto failure;
        }
    }

    return SNMP_ERR_NOERROR;
failure:
    return rc;
}

ClRcT snmpScalarsInit(void)
{
    ClListHeadT *iter = NULL;
    ClListHeadT *items = NULL;
    ClSnmpMibInfoT *mibInfo = &gSubAgentInfo.mibInfo;

    items = &mibInfo->scalars;
    CL_LIST_FOR_EACH(iter, items)
    {
        netsnmp_mib_handler *cb_handler = NULL;
        netsnmp_handler_registration *reg = NULL;

        ClSnmpScalarInfoT *s = CL_LIST_ENTRY(iter, ClSnmpScalarInfoT, list);
        int scalarOidLen = 0;
        oid *scalarOid = get_oid_array(s->oid, &scalarOidLen);
        int mode = s->settable ? HANDLER_CAN_RWRITE : HANDLER_CAN_RONLY;

        reg = netsnmp_create_handler_registration(s->name,
                                                  snmpGenericScalarHandler,
                                                  scalarOid,
                                                  scalarOidLen,
                                                  mode);

        int rc = netsnmp_register_scalar(reg);
        if (rc != MIB_REGISTERED_OK)
        {
            clLogError("SNMP", "SUB", "Failed to register MIB");
        }

        cb_handler = netsnmp_get_mode_end_call_handler(
                        netsnmp_mode_end_call_add_mode_callback(NULL,
                                                                NETSNMP_MODE_END_ALL_MODES,
                                                                netsnmp_create_handler("my_end_callback_handler",
                                                                                       clSnmpModeEndCallback)));
        reg->my_reg_void = s;
        netsnmp_inject_handler(reg, cb_handler);
    }
    
    return CL_OK;
}

int snmpGenericTableHandler(netsnmp_mib_handler *handler,
                            netsnmp_handler_registration *reginfo,
                            netsnmp_agent_request_info *reqinfo,
                            netsnmp_request_info *requests)
{
    int rc = SNMP_ERR_NOERROR;
    netsnmp_request_info *request = NULL;
    ClSnmpReqInfoT requestInfo;
    ClInt32T noOfOp = 0;
    ClSnmpTableInfoT *t = reginfo->my_reg_void;
    oid *tableOid = NULL;
    ClInt32T tableOidLen = 0;

    clLogDebug("SNMP", "SUB", "Generic table handler");

    if (!requests || !reqinfo)
    {
        rc = SNMP_ERR_NOERROR;
        goto failure;
    }

    memset(&requestInfo, 0, sizeof(ClSnmpReqInfoT));

    clLogDebug("SNMP", "SUB", "Received request for table [%s]", t->name);
    
    requestInfo.tableType = t->tableType; /* CL_CLOCKTABLE; */

    for (request = requests; request; request = request->next) 
    {
        ++noOfOp;
    }

    for (request = requests; request; request = request->next) 
    {
        netsnmp_variable_list *reqVar = request->requestvb;
        netsnmp_table_request_info *table_info = NULL;
        netsnmp_variable_list *index = NULL;

        if (request->processed)
        {
            clLogDebug("SNMP", "SUB",
                       "The job is already processed, skipping this job.");
            continue;
        }

        if ((!netsnmp_extract_iterator_context(request)) &&
            (reqinfo->mode == MODE_GET))
        {
            netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
            clLogDebug("SNMP", "SUB",
                       "No iterator context for the GET job, skipping it.");
            continue;
        }

        table_info = netsnmp_extract_table_info(request);
        if (!table_info)
        {
            clLogDebug("SNMP", "SUB",
                       "Not able to find the table_info for this job, skipping it.");
            continue;
        }

        /* COPY COLUMN OID FROM reqVar->name TO requestInfo.oid */
        /* oidLen is tableoid length + depth till attribute oid. So, in this case depth till column will be 2. 
           1-entry oid of table
           2-actual column. */
        
        tableOid = get_oid_array(t->oid, &tableOidLen);
        requestInfo.oidLen = tableOidLen + 2; /* ?? */
        clSnmpOidCpy(&requestInfo, reqVar->name);

        /* COPY THE INDEX VALUES */

        if (!(table_info->indexes))
        {
            rc = SNMP_ERR_GENERR;
            goto failure;
        }

        index = table_info->indexes;
        if (index->val_len > sizeof(long))
        {
            /* Index value length exceeds specified limit. */
            rc = SNMP_ERR_GENERR;
            goto failure;
        }
        else
        {
            memcpy(&(requestInfo.index.tableInfo.data),
                   index->val.integer,
                   sizeof(ClInt32T));
        }
        index = index->next_variable;

        switch (reqinfo->mode) 
        {
            case MODE_GET:
            {
                ClRcT ret = CL_OK;
                ClInt32T errorCode = 0;
                _ClSnmpGetReqInfoT reqAddInfo;
                ClSnmpScalarInfoT *s = NULL;

                memset(&reqAddInfo, 0, sizeof(_ClSnmpGetReqInfoT));
                
                clLogDebug("SNMP", "SUB", "Received GET request");
                
                requestInfo.opCode = CL_SNMP_GET;
                reqAddInfo.pRequest = request;

                s = snmpColumnScalarGet(t, table_info->colnum);
                if (s)
                {
                    clLogDebug("SNMP", "SUB", "Request for column [%s]", s->name);
                }

                reqAddInfo.columnType = s->type;

                switch (s->type)
                {
                    case ASN_INTEGER:
                    {
                        requestInfo.dataLen = sizeof(ClInt32T);
                        break;
                    }
                    case ASN_COUNTER:
                    case ASN_TIMETICKS:
                    case ASN_UNSIGNED: /* also ASN_GAUGE */
                    {
                        requestInfo.dataLen = sizeof(ClUint32T);
                        break;
                    }
                    case ASN_OCTET_STR:
                    {
                        requestInfo.dataLen = (CL_MAX_NAME_LENGTH-1) * sizeof(ClCharT);
                        break;
                    }
                    case ASN_OBJECT_ID:
                    {
                        requestInfo.dataLen = CL_SNMP_MAX_OID_LEN * sizeof(ClUint32T);
                        break;
                    }
                    case ASN_COUNTER64:
                    {
                        requestInfo.dataLen = sizeof(ClUint64T);
                        clLogDebug("SNMP", "SUB", "Filled in size as [%d]", requestInfo.dataLen);
                        break;
                    }
                    case ASN_IPADDRESS:
                    {
                        requestInfo.dataLen = 4 * sizeof(ClUint8T);
                        clLogDebug("SNMP", "SUB", "Filled in size as [%d]", requestInfo.dataLen);
                        break;
                    }
                    case ASN_OPAQUE:
                    {
                        requestInfo.dataLen = sizeof(ClUint8T);
                        break;
                    }
                    default:
                    {
                        clLogDebug("SNMP", "SUB",
                                   "Invalid data type [%d] for scalar [%s]",
                                   s->type,
                                   s->name);
                        break;
                    }
                }

                ret = clRequestAdd(&requestInfo,
                                   NULL,
                                   NULL,
                                   &errorCode,
                                   &reqAddInfo);
                if ((ret != CL_OK) || (errorCode < 0))
                {
                    netsnmp_request_set_error(request, errorCode);
                    rc = ret;
                    goto failure;
                }
                break;
            }
            case MODE_SET_RESERVE1:
            {
                ClSnmpScalarInfoT *s = NULL;
                ClUint32T columnType = 0;
                int ret = 0;

                clLogDebug("SNMP", "SUB", "Received RESERVE1 request");

                s = snmpColumnScalarGet(t, table_info->colnum);
                if (s)
                {
                    clLogDebug("SNMP", "SUB", "Request for column [%s]", s->name);
                }

                columnType = s->type;

                if (s->settable)
                {
                    switch (columnType)
                    {
                        /* TODO: verification for columns
                           for which enums, ranges are
                           defined */
                        default:
                        {
                            break;
                        }
                    }

                    ret = netsnmp_check_vb_type(reqVar, columnType);
                    if (ret != SNMP_ERR_NOERROR) 
                    {
                        netsnmp_request_set_error(request, ret);
                        rc = ret;
                        goto failure;
                    }
                }
                break;
            }
            case MODE_SET_FREE:
            {
                clLogDebug("SNMP", "SUB", "Received SET FREE request");
                clSnmpUndo();
                break;
            }
            case MODE_SET_RESERVE2:
            {
                ClSnmpScalarInfoT *s = NULL;
                ClRcT ret = CL_OK;
                ClInt32T errorCode = 0;
                ClSnmpGenValT val;
                ClPtrT pVal = NULL;

                clLogDebug("SNMP", "SUB", "Received RESERVE2 request");

                requestInfo.opCode = CL_SNMP_SET;

                s = snmpColumnScalarGet(t, table_info->colnum);
                if (s)
                {
                    clLogDebug("SNMP", "SUB", "Request for column [%s]", s->name);
                }

                switch (s->type)
                {
                    /* TODO:
                       Here again based on the column
                       data type fill in the pVal and
                       requestInfo.dataLen.
                    */
                    case ASN_INTEGER:
                    {
                        val.i = 0;
                        pVal = &val.i;
                        requestInfo.dataLen = sizeof(ClInt32T);
                        break;
                    }
                    case ASN_COUNTER:
                    case ASN_TIMETICKS:
                    case ASN_UNSIGNED: /* also ASN_GAUGE */
                    {
                        val.ui = 0;
                        pVal = &val.ui;
                        requestInfo.dataLen = sizeof(ClUint32T);
                        break;
                    }
                    case ASN_OCTET_STR:
                    {
                        memset(val.octet, 0, sizeof(val.octet));
                        pVal = val.octet;
                        requestInfo.dataLen = sizeof(val.octet);
                        clLogDebug("SNMP", "SUB", "size of data for octet string is %d", requestInfo.dataLen);
                        break;
                    }
                    case ASN_OBJECT_ID:
                    {
                        memset(val.oid, 0, sizeof(val.oid));
                        pVal = val.oid;
                        requestInfo.dataLen = sizeof(val.oid);
                        clLogDebug("SNMP", "SUB", "size of data for object id is %d", requestInfo.dataLen);
                        break;
                    }
                    case ASN_COUNTER64:
                    {
                        /* TODO */
                        break;
                    }
                    case ASN_IPADDRESS:
                    {
                        memset(val.ip, 0, sizeof(val.ip));
                        pVal = val.ip;
                        requestInfo.dataLen = sizeof(val.ip);
                        break;
                    }
                    case ASN_OPAQUE:
                    {
                        val.opaque = 0;
                        pVal = &val.opaque;
                        requestInfo.dataLen = val.opaque;
                        break;
                    }
                    default:
                    {
                        clLogDebug("SNMP", "SUB",
                                   "Invalid data type [%d] for scalar [%s]",
                                   s->type,
                                   s->name);
                        break;
                    }
                }

                memcpy(pVal,
                       reqVar->val.integer,
                       requestInfo.dataLen);

                clLogDebug("SNMP", "SUB", "Adding the request");

                ret = clRequestAdd(&requestInfo,
                                   pVal,
                                   &noOfOp,
                                   &errorCode,
                                   NULL);
                if (ret != CL_OK)
                {
                    clLogError("SNMP", "SUB",
                               "Failed to add request request, "
                               "error [%#x], error code [%#x]",
                               ret,
                               errorCode);
                    netsnmp_request_set_error(request, errorCode);
                    rc = ret;
                    goto failure;
                }

                clLogDebug("SNMP", "SUB",
                           "Successfully added request");
                break;
            }
            case MODE_SET_ACTION:
            {
                ClRcT ret = CL_OK;
                ClMedErrorListT medErrList;

                memset(&medErrList, 0, sizeof(ClMedErrorListT));

                clLogDebug("SNMP", "SUB",
                           "Received ACTION request for oid [%s]",
                           requestInfo.oid);

                ret = clSnmpCommit(&medErrList);
                if (ret != CL_OK)
                {
                    clLogDebug("SNMP", "SUB",
                               "SNMP commit failed, error count [%d]",
                               medErrList.count);
                    if (ret == CL_ERR_NO_MEMORY)
                    {
                        /* Set SNMP equivalent error from oidErrMapTable in clSnmpInit.c */
                        ret = CL_SNMP_ERR_NOCREATION;

                        clLogError("SUB", "TBL",
                                   "ACTION phase failed, "
                                   "setting error code [%#x] for oid [%s]", 
                                   ret,
                                   requestInfo.oid);
                        netsnmp_request_set_error(request, ret);
                        rc = ret;
                        goto failure;
                    }
                    else
                    {
                        ClUint32T i;

                        for(i = 0; i < medErrList.count; i++)
                        {
                            /* if the oid matches the request,
                               set error and return. */
                            if (!memcmp(requestInfo.oid, 
                                        medErrList.pErrorList[i].oidInfo.id,
                                        medErrList.pErrorList[i].oidInfo.len)) 
                            {
                                clLogError("SNMP", "SUB",
                                           "ACTION phase failed, "
                                           "setting error code [%#x] "
                                           "for oid [%s]", 
                                           medErrList.pErrorList[i].errId,
                                           requestInfo.oid);
                                netsnmp_request_set_error(request,
                                                          medErrList.
                                                          pErrorList[i].errId);
                                rc = ret;
                                goto failure;
                            }
                        }
                    }
                }
                break;
            }
            case MODE_SET_COMMIT:
            {
                clLogDebug("SNMP", "SUB", "Received COMMIT request");
                clSnmpUndo();
                break;
            }
            case MODE_SET_UNDO:
            {
                clLogDebug("SNMP", "SUB", "Received UNDO request");
                clSnmpUndo();
                break;
            }
            default:
            {
                clLogError("SNMP", "SUB",
                           "Unknown mode [%d] in generic table handler",
                           reqinfo->mode);
                rc = SNMP_ERR_GENERR;
                goto failure;
            }
        }
        memset(&requestInfo, 0, sizeof(ClSnmpReqInfoT));
    }

    return SNMP_ERR_NOERROR;
failure:
    return rc;
}

netsnmp_variable_list
*snmpGenericGetFirstDataPoint(void **my_loop_context,
                              void **my_data_context,
                              netsnmp_variable_list *put_index_data,
                              netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;
    ClSnmpTableInfoT *t = (ClSnmpTableInfoT *)mydata->myvoid;
        
    clLogDebug("SNMP", "SUB", "Generic table get first data point");

    clLogInfo("SNMP", "SUB", "Got request for oid %s, table type %d", t->oid, t->tableType);

    rc = clSnmpSendSyncRequestToServer(my_loop_context,
                                       my_data_context,
                                       put_index_data,
                                       t->tableType,
                                       CL_SNMP_GET_FIRST);
    if (rc != CL_OK)
    {
        clLogError("SNMP", "SUB",
                   "Sending sync request to server failed, error [%#x]",
                   rc);
        put_index_data = NULL;
    }

    return put_index_data; 
}

netsnmp_variable_list
*snmpGenericGetNextDataPoint(void **my_loop_context,
                             void **my_data_context,
                             netsnmp_variable_list *put_index_data,
                             netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;
    ClSnmpTableInfoT *t = (ClSnmpTableInfoT *)mydata->myvoid;

    clLogDebug("SNMP", "SUB", "Generic table get next data point");

    rc = clSnmpSendSyncRequestToServer(my_loop_context,
                                       my_data_context,
                                       put_index_data,
                                       t->tableType,
                                       CL_SNMP_GET_NEXT);
    if (rc != CL_OK)
    {
        clLogError("SNMP", "SUB",
                   "Sending sync request to server failed, error [%#x]",
                   rc);
        put_index_data = NULL;

        if (my_loop_context)
        {
            /* No more rows in table. */
            *(ClInt32T*)*my_loop_context = 1;
        }
    }
    
    return put_index_data; 
}

ClRcT snmpTablesInit(void)
{
    ClRcT rc = CL_SNMP_RC(CL_ERR_NO_MEMORY);
    ClListHeadT *iter = NULL;
    ClListHeadT *items = NULL;
    ClSnmpMibInfoT *mibInfo = &gSubAgentInfo.mibInfo;

    items = &mibInfo->tables;
    CL_LIST_FOR_EACH(iter, items)
    {
        netsnmp_mib_handler *cb_handler = NULL;
        netsnmp_mib_handler *cb = NULL;
        netsnmp_handler_registration *reg = NULL;
        netsnmp_iterator_info *iinfo = NULL;
        netsnmp_table_registration_info *table_info = NULL;


        ClSnmpTableInfoT *t = CL_LIST_ENTRY(iter, ClSnmpTableInfoT, list);
        int tableOidLen = 0;
        oid *tableOid = get_oid_array(t->oid, &tableOidLen);
        int mode = t->settable ? HANDLER_CAN_RWRITE : HANDLER_CAN_RONLY;

        ClListHeadT *iter2 = NULL;
        ClListHeadT *items2 = NULL;

        table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
        iinfo = SNMP_MALLOC_TYPEDEF(netsnmp_iterator_info);
        iinfo->flags |= NETSNMP_ITERATOR_FLAG_SORTED;

        if (!table_info || !iinfo)
        {
            SNMP_FREE(table_info);
            SNMP_FREE(iinfo);
            goto failure;
        }

        reg = netsnmp_create_handler_registration(t->name,
                                                  snmpGenericTableHandler,
                                                  tableOid,
                                                  tableOidLen,
                                                  mode);
        if (!reg)
        {
            clLogError("SNMP", "SUB", "Failed to create registration handler");
            SNMP_FREE(table_info);
            SNMP_FREE(iinfo);
            goto failure;
        }

        items2 = &t->indexes;
        CL_LIST_FOR_EACH(iter2, items2)
        {
            ClSnmpScalarInfoT *s = CL_LIST_ENTRY(iter2,
                                                 ClSnmpScalarInfoT,
                                                 list);
            
            netsnmp_table_helper_add_index(table_info,
                                           s->type);
        }

        table_info->min_column = t->minColumn;
        table_info->max_column = t->maxColumn;

        iinfo->get_first_data_point = snmpGenericGetFirstDataPoint;
        iinfo->get_next_data_point  = snmpGenericGetNextDataPoint;
        iinfo->table_reginfo = table_info;
        iinfo->myvoid = t;

        netsnmp_register_table_iterator(reg, iinfo);

        cb = netsnmp_create_handler("mode_end_callback_handler",
                                    clSnmpModeEndCallback);
        cb->myvoid = t;
        
        cb_handler = netsnmp_get_mode_end_call_handler(
                         netsnmp_mode_end_call_add_mode_callback(NULL,
                                                                 NETSNMP_MODE_END_ALL_MODES,
                                                                 cb));
        reg->my_reg_void = t;
        netsnmp_inject_handler(reg, cb_handler);

        /* TODO: add code for supports creation */
    }

    return CL_OK;
failure:
    return rc;
}

ClRcT snmpInitAppMib(void)
{
    ClRcT rc = CL_OK;

    rc = snmpScalarsInit();
    if (rc != CL_OK)
    {
        goto failure;
    }

    rc = snmpTablesInit();
    if (rc != CL_OK)
    {
        goto failure;
    }

    return CL_OK;
failure:
    return rc;
}

void *snmpSubAgentInit(void *arg)
{
    snmpDataInit(arg);

    snmpMedInit();

    snmp_enable_stderrlog();

    netsnmp_ds_set_boolean(NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_AGENT_ROLE, 1);
    netsnmp_ds_set_boolean(NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_AGENT_NO_ROOT_ACCESS, 1);
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_DISABLE_PERSISTENT_LOAD, 1);

    init_agent(gSubAgentInfo.mibInfo.name);

    snmpInitAppMib();

    init_snmp(gSubAgentInfo.mibInfo.name);

    while (1)
    {
        agent_check_and_process(1);
    }

    snmp_shutdown(gSubAgentInfo.mibInfo.name);

    return NULL;
}

ClRcT clSnmpSubagentClientLibInit(const ClCharT *file)
{
    clSnmpMutexInit();
    
    return clOsalTaskCreateDetached(gSubAgentInfo.mibInfo.name,
                                    CL_OSAL_SCHED_OTHER,
                                    0,
                                    0,
                                    snmpSubAgentInit,
                                    (void *)file);
}
