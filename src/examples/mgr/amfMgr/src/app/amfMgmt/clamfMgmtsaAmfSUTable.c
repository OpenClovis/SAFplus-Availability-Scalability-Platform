/******************************************************************************
 *
 * clamfMgmtsaAmfSUTable.c
 *
 ******************************************************************************
 * This code is auto-generated by OpenClovis IDE Version 3.1
 *
 *****************************************************************************/

#include <clamfMgmtOAMPConfig.h>
#include <clDebugApi.h>


/*
 * ---BEGIN_APPLICATION_CODE---
 */
#include "clAmfMgmt.h"
/*
 * Additional #includes and globals go here.
 */

/*
 * ---END_APPLICATION_CODE---
 */


#define clprintf(severity, ...)   clAppLog(CL_LOG_HANDLE_APP, severity, 10,                                   CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,                                  __VA_ARGS__)

/*
 * ---BEGIN_APPLICATION_CODE---
 */

/*
 * Additional user defined functions go here.
 */


/*
 * ---END_APPLICATION_CODE---
 */

/**** Prov functions */


/**
 * The contructor function is called by the provisioning library when a Managed Object 
 * is created. Inside this function, the callback functions for validate, update, 
 * rollback and read are assigned. 
 *
 * There are two variants of these callback functions, one at the object level and 
 * another at the attribute level. The object level functions are used to 
 * group validate/update/rollback/read multiple requests, and the other one is used 
 * to handle one request at a time. If both the callbacks are filled (i.e. non-null),
 * by default the object level callback will be invoked by prov library. To handle 
 * a single request at a time, the object level callbacks has to be set to NULL.
 * 
 * Inside this constructor function, user can add his/her own logic which need to be 
 * done on per object basis. If this logic involves allocation of some resource, 
 * it can be freed in the destructor function, which is called when the Managed 
 * Object is deleted.
 */ 
ClRcT clamfMgmtSAAMFSUTABLEProvConstructor( void *pThis, void *pUsrData, ClUint32T usrDataLen )
{
    ClRcT rc = CL_OK;

    /* Override "semantic check" virtual method in provClass*/
    ((CL_OM_PROV_CLASS*)pThis)->clProvObjectStart = clamfMgmtSAAMFSUTABLEProvObjectStart;      
    ((CL_OM_PROV_CLASS*)pThis)->clProvValidate = (fp)clamfMgmtSAAMFSUTABLEProvValidate;    
    ((CL_OM_PROV_CLASS*)pThis)->clProvUpdate = (fp)clamfMgmtSAAMFSUTABLEProvUpdate;    
    ((CL_OM_PROV_CLASS*)pThis)->clProvRollback = (fp)clamfMgmtSAAMFSUTABLEProvRollback;
    ((CL_OM_PROV_CLASS*)pThis)->clProvRead = (fp)clamfMgmtSAAMFSUTABLEProvRead;
	((CL_OM_PROV_CLASS*)pThis)->clProvObjectValidate = clamfMgmtSAAMFSUTABLEProvObjectValidate; 	 
    ((CL_OM_PROV_CLASS*)pThis)->clProvObjectUpdate = clamfMgmtSAAMFSUTABLEProvObjectUpdate; 	 
    ((CL_OM_PROV_CLASS*)pThis)->clProvObjectRollback = clamfMgmtSAAMFSUTABLEProvObjectRollback;
    ((CL_OM_PROV_CLASS*)pThis)->clProvObjectRead = clamfMgmtSAAMFSUTABLEProvObjectRead;
    ((CL_OM_PROV_CLASS*)pThis)->clProvObjectEnd = clamfMgmtSAAMFSUTABLEProvObjectEnd;      
    
    /*
     * ---BEGIN_APPLICATION_CODE---
    */

    clprintf(CL_LOG_SEV_INFO, "Inside the function %s", __FUNCTION__);
    ((CL_OM_PROV_CLASS*)pThis)->clProvValidate = (fp)NULL;
    ((CL_OM_PROV_CLASS*)pThis)->clProvUpdate = (fp)NULL;
    ((CL_OM_PROV_CLASS*)pThis)->clProvRollback = (fp)NULL;
    ((CL_OM_PROV_CLASS*)pThis)->clProvRead = (fp)NULL;
    /*
     * ---END_APPLICATION_CODE---
     */
     
    return rc;
}

/**
 * This function is called by the provisioning library when the managed resource 
 * is deleted. This can have logic of deleting the resources which might have 
 * been allocated in the constructor function.
 */ 
ClRcT clamfMgmtSAAMFSUTABLEProvDestructor ( void *pThis , void  *pUsrData, ClUint32T usrDataLen )
{
    ClRcT rc = CL_OK;
    
    /*
     * ---BEGIN_APPLICATION_CODE---
     */

    clprintf(CL_LOG_SEV_INFO, "Inside the function %s", __FUNCTION__);

    /*
     * ---END_APPLICATION_CODE---
     */

    return rc;  
}

/**
 * This function is called by the provisioning library before forwarding any of the
 * transaction requests to this application on this managed resource. Please refer the file
 * clamfMgmtSAAMFSUTABLEOAMPConfig.h for detailed documentation for this function.
 */
void clamfMgmtSAAMFSUTABLEProvObjectStart(ClCorMOIdPtrT pMoId, ClHandleT txnHandle)
{
    /*
     * ---BEGIN_APPLICATION_CODE---
    */

    clprintf(CL_LOG_SEV_INFO, "Inside the function %s", __FUNCTION__);

    /*
     * ---END_APPLICATION_CODE---
    */
}

/**
 * This function is called by the provisioning library whenever the object modification 
 * operation is done on the resource being managed. This callback is called to validate
 * the job. For a single job request, this is called once before calling update or 
 * rollback based on the success or failure of this function. For a request containing
 * multiple jobs, this function is called for all the jobs for validating them.
 * Only when validation for all the jobs are success, the update callback is called for
 * all the jobs. In case of failure, the rollback it called for all the jobs till which
 * the failure was detected.
 *
 * The pThis is a pointer to the provisioning class. 
 * The txnHandle is the unique handle for all the jobs which are part of same session. So for
 * a session containing multiple jobs, then it is unique of all the jobs and it can be used
 * to identify them.
 *
 * The pProvTxnData is a pointer to the ClProvTxnDataT structure which contains 
 * the information about the job being performed on the managed resource. 
 * The pProvTxnData->provCmd stores the operation type which can be 
 * CL_COR_OP_CREATE_AND_SET, CL_COR_OP_CREATE, CL_COR_OP_SET or CL_COR_OP_DELETE
 * based on what is the operation happening on the resource.
 * In the case of set operation, the various fields of pProvTxnData provide the
 * the details about the operation which is happening on the resource. It contain 
 * MOID of the managed resource and information about the attribute on which set
 * operation is done. For a create, create-set and delete operation, the pProvTxnData 
 * contains the MOID of the resource on which the given operation is done.
 *
 * ** Note : This function is being deprecated, if clProvObjectValidate() callback 
 * is filled in the constructor, then that callback function will be called 
 * instead of this to group validate all the requests.
 */ 
ClRcT clamfMgmtSAAMFSUTABLEProvValidate(CL_OM_PROV_CLASS* pThis, ClHandleT txnHandle, ClProvTxnDataT* pProvTxnData)
{
    ClRcT rc = CL_OK;
    
    /*
     * ---BEGIN_APPLICATION_CODE---
     */

    clprintf(CL_LOG_SEV_INFO, "Inside the function %s", __FUNCTION__);

    /*
     * ---END_APPLICATION_CODE---
     */
        
    return rc;
}

/**
 * This function is called by the provisioning library whenever the object modification 
 * operation is done on the resource being managed. This is called to update the jobs
 * after being validated in validate phase.
 *
 * The pThis is a pointer to the provisioning class. 
 * The txnHandle is the unique handle for all the jobs which are part of same session. So for
 * a session containing multiple jobs, then it is unique of all the jobs and it can be used
 * to identify them.
 *
 * The pProvTxnData is a pointer to the ClProvTxnDataT structure which contains 
 * the information about the job being performed on the managed resource. 
 * The pProvTxnData->provCmd stores the operation type which can be 
 * CL_COR_OP_CREATE_AND_SET, CL_COR_OP_CREATE, CL_COR_OP_SET or CL_COR_OP_DELETE
 * based on what is the operation happening on the resource.
 * In the case of set operation, the various fields of pProvTxnData provide the
 * the details about the operation which is happening on the resource. It contain 
 * MOID of the managed resource and information about the attribute on which set
 * operation is done. For a create, create-set and delete operation, the pProvTxnData 
 * contains the MOID of the resource on which the given operation is done.
 *
 * ** Note : This function is being deprecated, if clProvObjectUpdate() callback 
 * is filled in the constructor, then that callback function will be called 
 * instead of this to group update all the requests.
 */ 
ClRcT clamfMgmtSAAMFSUTABLEProvUpdate(CL_OM_PROV_CLASS* pThis, ClHandleT txnHandle, ClProvTxnDataT* pProvTxnData)
{
    ClRcT rc = CL_OK;

    /*
     * ---BEGIN_APPLICATION_CODE---
     */

    clprintf(CL_LOG_SEV_INFO, "Inside the function %s", __FUNCTION__);

    /*
     * ---END_APPLICATION_CODE---
     */
     
    switch (pProvTxnData->provCmd)
    {
        case CL_COR_OP_CREATE :
        case CL_COR_OP_CREATE_AND_SET:
            
        /*
         * ---BEGIN_APPLICATION_CODE---
         */

        /*
         * ---END_APPLICATION_CODE---
         */

        break;
        case CL_COR_OP_SET:
        
        /*
         * ---BEGIN_APPLICATION_CODE---
         */
    
        /*
         * ---END_APPLICATION_CODE---
         */

        break;

        case  CL_COR_OP_DELETE:

        /*
         * ---BEGIN_APPLICATION_CODE---
         */

        /*
         * ---END_APPLICATION_CODE---
         */

        break;
        default:
            clprintf (CL_LOG_SEV_ERROR, "Prov command is not proper");

    }

    /*
     * ---BEGIN_APPLICATION_CODE---
     */

    /*
     * ---END_APPLICATION_CODE---
     */

    return rc;   
}


/**
 * This function is called by the provisioning library whenever the object 
 * modification operation is done on the resource being managed. For a single job 
 * request, this function is called if the validate function is failed. For a 
 * multiple job request, on encountering a validate failure, this function is 
 * called to rollback all the jobs before the one (including) on which the 
 * validation failure is reported.
 *
 * The pThis is a pointer to the provisioning class. 
 * The txnHandle is the unique handle for all the jobs which are part of same session. So for
 * a session containing multiple jobs, then it is unique of all the jobs and it can be used
 * to identify them.
 * 
 * The pProvTxnData is a pointer to the ClProvTxnDataT structure which contains 
 * the information about the job being performed on the managed resource. 
 * The pProvTxnData->provCmd stores the operation type which can be 
 * CL_COR_OP_CREATE_AND_SET, CL_COR_OP_CREATE, CL_COR_OP_SET or CL_COR_OP_DELETE
 * based on what is the operation happening on the resource.
 * In the case of set operation, the various fields of pProvTxnData provide the
 * the details about the operation which is happening on the resource. It contain 
 * MOID of the managed resource and information about the attribute on which set
 * operation is done. For a create, create-set and delete operation, the pProvTxnData 
 * contains the MOID of the resource on which the given operation is done.
 *
 * ** Note : This function is being deprecated, if clProvObjectRollback() callback 
 * is filled in the constructor, then that callback function will be called 
 * instead of this to group rollback all the requests.
 */
ClRcT clamfMgmtSAAMFSUTABLEProvRollback(CL_OM_PROV_CLASS* pThis, ClHandleT txnHandle, ClProvTxnDataT* pProvTxnData)
{
    ClRcT rc = CL_OK;

    /*
     * ---BEGIN_APPLICATION_CODE---
     */

    clprintf(CL_LOG_SEV_INFO, "Inside the function %s", __FUNCTION__);

    /*
     * ---END_APPLICATION_CODE---
     */

    return rc;
}

/**
 * This function is called to perform a get operation on the transient attribute
 * which is owned by the primary object implementer (OI). As the COR doesn't have
 * the latest value of the transient attribute, it is obtained from the OI. This
 * function is called in the OI's context which it can use to fill the latest
 * value of the runtime or transient attribute.
 *
 * The pThis is pointer to the provisioning class.
 * The txnHandle is used to identify the jobs which are part of same bundle request.
 * For single request this field is of not much significance, but for a multiple job
 * request, this feild is used to identify all the jobs which are part of same
 * bundle request sent by COR.
 *
 * The pProvTxnData contains the information about the attribute jobs. It contains 
 * the MOId of the managed resource, the attribute identifier, its type (array or 
 * simple), its basic type (data type), index (in case it is array attriubte), 
 * size and the pointer (allocated by the library) to the memory on which the 
 * data can be copied.
 *
 * For a request containing only single job, this function is called only once. But
 * for a multiple job request, this is called for all the attributes one at a time.
 *
 * ** Note : This function is being deprecated, if clProvObjectRead() callback is filled 
 * in the constructor, then that callback function will be called instead of this 
 * to group read all the requests.
 */ 
ClRcT clamfMgmtSAAMFSUTABLEProvRead(CL_OM_PROV_CLASS* pThis, ClHandleT txnHandle, ClProvTxnDataT* pProvTxnData)
{
    ClRcT rc = CL_OK;

    /*
     * ---BEGIN_APPLICATION_CODE---
     */

    ClAmsEntityT entity = {0};
    ClAmsSUConfigT *pSUConfig = NULL;
    ClAmsSUStatusT *pSUStatus = NULL;
    clprintf(CL_LOG_SEV_INFO, "Inside the function %s", __FUNCTION__);
    if(!gClAmsMgmtHandle)
        return CL_AMS_RC(CL_ERR_NOT_INITIALIZED);
    if(!pProvTxnData || !pProvTxnData->pProvData || !pProvTxnData->pMoId)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    entity.type = CL_AMS_ENTITY_TYPE_SU;
    rc = clAmsMgmtOIGet(pProvTxnData->pMoId, &entity);
    if(rc != CL_OK)
    {
        SaNameT moidName = {0};
        if(clCorMoIdToMoIdNameGet(pProvTxnData->pMoId, &moidName) == CL_OK)
            clLogError("OI", "READ", "AMF entity get for moid [%.*s] returned [%#x]",
                    moidName.length, moidName.value, rc);
        return rc;
    }
    rc = clAmsMgmtEntityGetConfig(gClAmsMgmtHandle, &entity, (ClAmsEntityConfigT**)&pSUConfig);
    if(rc != CL_OK)
    {
        clLogError("OI", "READ", "SU [%s] config returned [%#x]", entity.name.value, rc);
        return rc;
    }
    rc = clAmsMgmtEntityGetStatus(gClAmsMgmtHandle, &entity, (ClAmsEntityStatusT**)&pSUStatus);
    if(rc != CL_OK)
    {
        clLogError("OI", "READ", "SU [%s] status returned [%#x]", entity.name.value, rc);
        clHeapFree(pSUConfig);
        return rc;
    }
    switch(pProvTxnData->attrId)
    {
        case SAAMFSUTABLE_SAAMFSUNUMCURRACTIVESIS:
        case SAAMFSUTABLE_SAAMFSUNUMCURRSTANDBYSIS:
            {
                if(pProvTxnData->size != (ClUint32T)sizeof(pSUStatus->numActiveSIs))
                {
                    clLogError("OI", "READ", "Read size [%d] doesnt match expected size [%d]",
                            pProvTxnData->size, (ClUint32T)sizeof(pSUStatus->numActiveSIs));
                    rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
                    goto out_free;
                }
                if(pProvTxnData->attrId == SAAMFSUTABLE_SAAMFSUNUMCURRACTIVESIS)
                    memcpy(pProvTxnData->pProvData, &pSUStatus->numActiveSIs, pProvTxnData->size);
                else
                    memcpy(pProvTxnData->pProvData, &pSUStatus->numStandbySIs, pProvTxnData->size);
            }
            break;

        case SAAMFSUTABLE_SAAMFSUADMINSTATE:
            {
                if(pProvTxnData->size != (ClUint32T)sizeof(pSUConfig->adminState))
                {
                    clLogError("OI", "READ", "Read size [%d] doesnt match expected size [%d]",
                            pProvTxnData->size, (ClUint32T)sizeof(pSUConfig->adminState));
                    rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
                    goto out_free;
                }
                memcpy(pProvTxnData->pProvData, &pSUConfig->adminState, pProvTxnData->size);
            }
            break;

        case SAAMFSUTABLE_SAAMFSUREADINESSSTATE:
            {
                if(pProvTxnData->size != (ClUint32T)sizeof(pSUStatus->readinessState))
                {
                    clLogError("OI", "READ", "Read size [%d] doesnt match expected size [%d]",
                            pProvTxnData->size, (ClUint32T)sizeof(pSUStatus->readinessState));
                    rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
                    goto out_free;
                }
                memcpy(pProvTxnData->pProvData, &pSUStatus->readinessState, pProvTxnData->size);
                break;
            }

        case SAAMFSUTABLE_SAAMFSUOPERSTATE:
            {
                if(pProvTxnData->size != (ClUint32T)sizeof(pSUStatus->operState))
                {
                    clLogError("OI", "READ", "Read size [%d] doesnt match expected size [%d]",
                            pProvTxnData->size, (ClUint32T)sizeof(pSUStatus->operState));
                    rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
                    goto out_free;
                }
                memcpy(pProvTxnData->pProvData, &pSUStatus->operState, pProvTxnData->size);
                break;
            }

        case SAAMFSUTABLE_SAAMFSUPRESENCESTATE:
            {
                if(pProvTxnData->size != (ClUint32T)sizeof(pSUStatus->presenceState))
                {
                    clLogError("OI", "READ", "Read size [%d] doesnt match expected size [%d]",
                            pProvTxnData->size, (ClUint32T)sizeof(pSUStatus->presenceState));
                    rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
                    goto out_free;
                }
                memcpy(pProvTxnData->pProvData, &pSUStatus->presenceState, pProvTxnData->size);
                break;
            }

        case SAAMFSUTABLE_SAAMFSUPREINSTANTIABLE:
            {
                ClUint32T value = 0;
                if(pProvTxnData->size != (ClUint32T)sizeof(value))
                {
                    clLogError("OI", "READ", "Read size [%d] doesnt match expected size [%d]",
                            pProvTxnData->size, (ClUint32T)sizeof(value));
                    rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
                    goto out_free;
                }
                if(pSUConfig->isPreinstantiable)
                    value = 2;
                else
                    value = 1;
                memcpy(pProvTxnData->pProvData, &value, pProvTxnData->size);
                break;
            }
        default:
            rc = CL_AMS_RC(CL_ERR_NOT_SUPPORTED);
            goto out_free;
    }

out_free:
    if(pSUConfig)
        clHeapFree(pSUConfig);
    if(pSUStatus)
        clHeapFree(pSUStatus);



    /*
     * ---END_APPLICATION_CODE---
     */

    return rc;
}

/**
 * This function is called by the provisioning library after all the transaction requests
 * for this managed resource are forwarded to this application. Please refer the file
 * clamfMgmtSAAMFSUTABLEOAMPConfig.h for detailed documentation for this function.
 */
void clamfMgmtSAAMFSUTABLEProvObjectEnd(ClCorMOIdPtrT pMoId, ClHandleT txnHandle)
{
    /*
     * ---BEGIN_APPLICATION_CODE---
     */

    clprintf(CL_LOG_SEV_INFO, "Inside the function %s", __FUNCTION__);

    /*
     * ---END_APPLICATION_CODE---
     */
}

/**
 * Validate all the jobs in an object sent from northbound as a single request.
 * Please refer protptype of this function in the file clamfMgmtSAAMFSUTABLEOAMPConfig.h
 * for detailed documentation for this function.
 */
ClRcT clamfMgmtSAAMFSUTABLEProvObjectValidate(CL_OM_PROV_CLASS* pThis, ClHandleT txnHandle, ClProvTxnDataT* pProvTxnDataList, ClUint32T txnDataEntries)
{
    ClRcT rc = CL_OK;
	
	/*
	 * ---BEGIN_APPLICATION_CODE$objectValidate$---
	 */
 	
    clprintf(CL_LOG_SEV_INFO, "Inside the function %s", __FUNCTION__);

	/*
	 * ---END_APPLICATION_CODE---
	 */
 	 
    return rc;
}

/**
 * Update all the jobs of an object sent from north bound as a single request. 
 * Please refer protptype of this function in the file clamfMgmtSAAMFSUTABLEOAMPConfig.h
 * for detailed documentation for this function.
 */
ClRcT clamfMgmtSAAMFSUTABLEProvObjectUpdate(CL_OM_PROV_CLASS* pThis, ClHandleT txnHandle, ClProvTxnDataT* pProvTxnDataList, ClUint32T txnDataEntries)
{
    ClRcT rc = CL_OK;
    ClProvTxnDataT* pProvTxnData = NULL;
    ClUint32T i = 0;
	
	/*
	 * ---BEGIN_APPLICATION_CODE$objectUpdate$---
	 */
 	
    clprintf(CL_LOG_SEV_INFO, "Inside the function %s", __FUNCTION__);

    /*
     * ---END_APPLICATION_CODE---
     */

    for (i=0; i < txnDataEntries; i++)
    {
        pProvTxnData = (pProvTxnDataList + i);

        switch (pProvTxnData->provCmd)
        {
            case CL_COR_OP_CREATE :
            case CL_COR_OP_CREATE_AND_SET:
                
            /*
             * ---BEGIN_APPLICATION_CODE$objectUpdateCreateAndSet$---
             */

            /*
             * ---END_APPLICATION_CODE---
             */

            break;
            case CL_COR_OP_SET:
            
            /*
             * ---BEGIN_APPLICATION_CODE$objectUpdateSet$---
             */
        
            /*
             * ---END_APPLICATION_CODE---
             */

            break;

            case  CL_COR_OP_DELETE:

            /*
             * ---BEGIN_APPLICATION_CODE$objectUpdateDelete$---
             */

            /*
             * ---END_APPLICATION_CODE---
             */

            break;
            default:
                clprintf (CL_LOG_SEV_ERROR, "Prov command is not proper");

        }
    }

    /*
     * ---BEGIN_APPLICATION_CODE$objectUpdate1$---
     */

	/*
	 * ---END_APPLICATION_CODE---
	 */
 	 
    return rc;
}

/**
 * Rollback all the jobs of an object whose validation failed.
 * Please refer protptype of this function in the file clamfMgmtSAAMFSUTABLEOAMPConfig.h
 * for detailed documentation for this function.
 */
ClRcT clamfMgmtSAAMFSUTABLEProvObjectRollback(CL_OM_PROV_CLASS* pThis, ClHandleT txnHandle, ClProvTxnDataT* pProvTxnDataList, ClUint32T txnDataEntries)
{
    ClRcT rc = CL_OK;
	
	/*
	 * ---BEGIN_APPLICATION_CODE$objectRollback$---
	 */
 	
    clprintf(CL_LOG_SEV_INFO, "Inside the function %s", __FUNCTION__);

	/*
	 * ---END_APPLICATION_CODE---
	 */
 	 
    return rc;
}

/**
 * Read all the transient attributes values in an object in a single request.
 * Please refer protptype of this function in the file clamfMgmtSAAMFSUTABLEOAMPConfig.h
 * for detailed documentation for this function.
 */
ClRcT clamfMgmtSAAMFSUTABLEProvObjectRead(CL_OM_PROV_CLASS* pThis, ClHandleT txnHandle, ClProvTxnDataT* pProvTxnDataList, ClUint32T txnDataEntries)
{
    ClRcT rc = CL_OK;
	
	/*
	 * ---BEGIN_APPLICATION_CODE$objectRead$---
	 */
    ClUint32T i;
    clprintf(CL_LOG_SEV_INFO, "Inside the function %s", __FUNCTION__);
    for(i = 0; i < txnDataEntries; ++i)
    {
        rc = clamfMgmtSAAMFSUTABLEProvRead(pThis, txnHandle, pProvTxnDataList+i);
        if(rc != CL_OK)
            return rc;
    }
	/*
	 * ---END_APPLICATION_CODE---
	 */
 	 
    return rc;
}


