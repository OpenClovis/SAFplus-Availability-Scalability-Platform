#include <clNtfXdr.h>

/* Marshalling function for SaNtfClassIdT structure types */
ClRcT   marshallNtfClassIdT(SaNtfClassIdT *classId, ClBufferHandleT bufferHandle)
{

    CHECK_RETURN(clXdrMarshallClUint32T(&classId->vendorId, bufferHandle, 0), CL_OK);
    CHECK_RETURN(clXdrMarshallClUint16T(&classId->majorId, bufferHandle, 0), CL_OK);
    CHECK_RETURN(clXdrMarshallClUint16T(&classId->minorId, bufferHandle, 0), CL_OK);

    return CL_OK;
}

/* UnMarshalling function for SaNtfClassIdT structure types */
ClRcT   unmarshallNtfClassIdT(ClBufferHandleT bufferHandle, SaNtfClassIdT *classId)
{

    CHECK_RETURN(clXdrUnmarshallClUint32T(bufferHandle, &classId->vendorId), CL_OK);
    CHECK_RETURN(clXdrUnmarshallClUint16T(bufferHandle, &classId->majorId), CL_OK);
    CHECK_RETURN(clXdrUnmarshallClUint16T(bufferHandle, &classId->minorId), CL_OK);

    return CL_OK;
}

/* Marshalling function for SaNtfAdditionalInfoT structure types */
ClRcT   marshallNtfAdditionalInfoT(SaNtfAdditionalInfoT *addInfo, ClBufferHandleT bufferHandle)
{
    CHECK_RETURN(clXdrMarshallClUint16T(&addInfo->infoId, bufferHandle, 0), CL_OK);
    CHECK_RETURN(clXdrMarshallClUint32T(&addInfo->infoType, bufferHandle, 0), CL_OK);
    CHECK_RETURN(marshallNtfValueTypeT(addInfo->infoType, &addInfo->infoValue, bufferHandle), CL_OK);
    return CL_OK;
}

/* UnMarshalling function for SaNtfAdditionalInfoT structure types */
ClRcT   unmarshallNtfAdditionalInfoT(ClBufferHandleT bufferHandle, SaNtfAdditionalInfoT *addInfo)
{
    CHECK_RETURN(clXdrUnmarshallClUint16T(bufferHandle, &addInfo->infoId), CL_OK);
    CHECK_RETURN(clXdrUnmarshallClUint32T(bufferHandle, &addInfo->infoType), CL_OK);
    CHECK_RETURN(unmarshallNtfValueTypeT(bufferHandle, addInfo->infoType, &addInfo->infoValue), CL_OK);
    return CL_OK;
}

/* Marshalling function for SaNtfValueT structure types */
ClRcT   marshallNtfValueTypeT(SaNtfValueTypeT infoType, SaNtfValueT *infoValue, ClBufferHandleT bufferHandle)
{
    switch ((ClInt32T)infoType)
    {
        case 0:
            /* This indicates that the variable fo type SaNtfValueT was just created
             * by intializing to 0 and is not being used at all. So we shouldn't marshall
             * anything for it. */
            break;
        case SA_NTF_VALUE_UINT8:
            CHECK_RETURN(clXdrMarshallClUint8T(&infoValue->uint8Val, bufferHandle, 0), CL_OK);
            break;
        case SA_NTF_VALUE_INT8:
            CHECK_RETURN(clXdrMarshallClInt8T(&infoValue->int8Val, bufferHandle, 0), CL_OK);
            break;
        case SA_NTF_VALUE_UINT16:
            CHECK_RETURN(clXdrMarshallClUint16T(&infoValue->uint16Val, bufferHandle, 0), CL_OK);
            break;
        case SA_NTF_VALUE_INT16:
            CHECK_RETURN(clXdrMarshallClInt16T(&infoValue->int16Val, bufferHandle, 0), CL_OK);
            break;
        case SA_NTF_VALUE_UINT32:
            CHECK_RETURN(clXdrMarshallClUint32T(&infoValue->uint32Val, bufferHandle, 0), CL_OK);
            break;
        case SA_NTF_VALUE_INT32:
            CHECK_RETURN(clXdrMarshallClInt32T(&infoValue->int32Val, bufferHandle, 0), CL_OK);
            break;
        case SA_NTF_VALUE_FLOAT:
            /* NOTICE - marshalling float as ClInt32T */
            CHECK_RETURN(clXdrMarshallClInt32T(&infoValue->floatVal, bufferHandle, 0), CL_OK);
            break;
        case SA_NTF_VALUE_UINT64:
            CHECK_RETURN(clXdrMarshallClUint64T(&infoValue->uint64Val, bufferHandle, 0), CL_OK);
            break;
        case SA_NTF_VALUE_INT64:
            CHECK_RETURN(clXdrMarshallClInt64T(&infoValue->int64Val, bufferHandle, 0), CL_OK);
            break;
        case SA_NTF_VALUE_DOUBLE:
            /* NOTICE - marshalling double as ClInt64T */
            CHECK_RETURN(clXdrMarshallClInt64T(&infoValue->doubleVal, bufferHandle, 0), CL_OK);
            break;
        case SA_NTF_VALUE_LDAP_NAME:
        case SA_NTF_VALUE_STRING:
        case SA_NTF_VALUE_IPADDRESS:
        case SA_NTF_VALUE_BINARY:
            CHECK_RETURN(clXdrMarshallClInt16T(&infoValue->ptrVal.dataOffset, bufferHandle, 0), CL_OK);
            CHECK_RETURN(clXdrMarshallClInt16T(&infoValue->ptrVal.dataSize, bufferHandle, 0), CL_OK);
            break;
        case SA_NTF_VALUE_ARRAY:
            CHECK_RETURN(clXdrMarshallClInt16T(&infoValue->arrayVal.arrayOffset, bufferHandle, 0), CL_OK);
            CHECK_RETURN(clXdrMarshallClInt16T(&infoValue->arrayVal.numElements, bufferHandle, 0), CL_OK);
            CHECK_RETURN(clXdrMarshallClInt16T(&infoValue->arrayVal.elementSize, bufferHandle, 0), CL_OK);
            break;
        default:
            clLogError("NTF","XDR","Invalid infoType [%d] found during marshalling", infoType);
            break;
    }
    return CL_OK;
}

/* UnMarshalling function for SaNtfValueT structure types */
ClRcT   unmarshallNtfValueTypeT(ClBufferHandleT bufferHandle, SaNtfValueTypeT infoType, SaNtfValueT *infoValue)
{
    switch ((ClInt32T)infoType)
    {
        case 0:
            /* This indicates that the variable fo type SaNtfValueT was just created
             * by intializing to 0 and is not being used at all. So we shouldn't unmarshall
             * anything for it. */
            break;
        case SA_NTF_VALUE_UINT8:
            CHECK_RETURN(clXdrUnmarshallClUint8T(bufferHandle, &infoValue->uint8Val), CL_OK);
            break;
        case SA_NTF_VALUE_INT8:
            CHECK_RETURN(clXdrUnmarshallClInt8T(bufferHandle, &infoValue->int8Val), CL_OK);
            break;
        case SA_NTF_VALUE_UINT16:
            CHECK_RETURN(clXdrUnmarshallClUint16T(bufferHandle, &infoValue->uint16Val), CL_OK);
            break;
        case SA_NTF_VALUE_INT16:
            CHECK_RETURN(clXdrUnmarshallClInt16T(bufferHandle, &infoValue->int16Val), CL_OK);
            break;
        case SA_NTF_VALUE_UINT32:
            CHECK_RETURN(clXdrUnmarshallClUint32T(bufferHandle, &infoValue->uint32Val), CL_OK);
            break;
        case SA_NTF_VALUE_INT32:
            CHECK_RETURN(clXdrUnmarshallClInt32T(bufferHandle, &infoValue->int32Val), CL_OK);
            break;
        case SA_NTF_VALUE_FLOAT:
            /* unmarshalling float as ClInt32T */
            CHECK_RETURN(clXdrUnmarshallClInt32T(bufferHandle, &infoValue->floatVal), CL_OK);
            break;
        case SA_NTF_VALUE_UINT64:
            CHECK_RETURN(clXdrUnmarshallClUint64T(bufferHandle, &infoValue->uint64Val), CL_OK);
            break;
        case SA_NTF_VALUE_INT64:
            CHECK_RETURN(clXdrUnmarshallClInt64T(bufferHandle, &infoValue->int64Val), CL_OK);
            break;
        case SA_NTF_VALUE_DOUBLE:
            /* unmarshalling double as ClInt64T */
            CHECK_RETURN(clXdrUnmarshallClInt64T(bufferHandle, &infoValue->doubleVal), CL_OK);
            break;
        case SA_NTF_VALUE_LDAP_NAME:
        case SA_NTF_VALUE_STRING:
        case SA_NTF_VALUE_IPADDRESS:
        case SA_NTF_VALUE_BINARY:
            CHECK_RETURN(clXdrUnmarshallClInt16T(bufferHandle, &infoValue->ptrVal.dataOffset), CL_OK);
            CHECK_RETURN(clXdrUnmarshallClInt16T(bufferHandle, &infoValue->ptrVal.dataSize), CL_OK);
            break;
        case SA_NTF_VALUE_ARRAY:
            CHECK_RETURN(clXdrUnmarshallClInt16T(bufferHandle, &infoValue->arrayVal.arrayOffset), CL_OK);
            CHECK_RETURN(clXdrUnmarshallClInt16T(bufferHandle, &infoValue->arrayVal.numElements), CL_OK);
            CHECK_RETURN(clXdrUnmarshallClInt16T(bufferHandle, &infoValue->arrayVal.elementSize), CL_OK);
            break;
        default:
            clLogError("NTF","XDR","Invalid infoType [%d] found during unmarshalling", infoType);
            break;
    }
    return CL_OK;
}


/* Marshall function for all the fields of notification Header buffer */
ClRcT   marshallNotificationHeaderBuffer(ClBufferHandleT bufferHandle, 
                                         ClNtfNotificationHeaderBufferT *ntfPtr)
{
    ClUint32T   index = 0;

    CHECK_RETURN(clXdrMarshallClUint64T(&ntfPtr->notificationHandle, bufferHandle, 0), CL_OK);
    CHECK_RETURN(clXdrMarshallClUint32T(&ntfPtr->eventType, bufferHandle, 0), CL_OK);
    CHECK_RETURN(clXdrMarshallClNameT(&ntfPtr->notificationObject, bufferHandle, 0), CL_OK);
    CHECK_RETURN(clXdrMarshallClNameT(&ntfPtr->notifyingObject, bufferHandle, 0), CL_OK);
    CHECK_RETURN(marshallNtfClassIdT(&ntfPtr->notificationClassId, bufferHandle), CL_OK);
    CHECK_RETURN(clXdrMarshallClInt64T(&ntfPtr->eventTime, bufferHandle, 0), CL_OK);
    CHECK_RETURN(clXdrMarshallClInt16T(&ntfPtr->numCorrelatedNotifications, bufferHandle, 0), CL_OK);
    CHECK_RETURN(clXdrMarshallClInt16T(&ntfPtr->lengthAdditionalText, bufferHandle, 0), CL_OK);
    CHECK_RETURN(clXdrMarshallClInt16T(&ntfPtr->numAdditionalInfo, bufferHandle, 0), CL_OK);
    CHECK_RETURN(clXdrMarshallClInt64T(&ntfPtr->notificationId, bufferHandle, 0), CL_OK);
    for (index = 0; index < ntfPtr->numCorrelatedNotifications; index++)
        CHECK_RETURN(clXdrMarshallClInt64T(&ntfPtr->correlatedNotifications[index], bufferHandle, 0), CL_OK);
    CHECK_RETURN(clXdrMarshallArrayClCharT(ntfPtr->additionalText, ntfPtr->lengthAdditionalText, bufferHandle, 0), CL_OK);
    for (index = 0; index < ntfPtr->numAdditionalInfo; index++)
        CHECK_RETURN(marshallNtfAdditionalInfoT(&ntfPtr->additionalInfo[index], bufferHandle), CL_OK);

    return CL_OK;
}

/* unmarshall function for all the fields of notification Header buffer */
ClRcT   unmarshallNotificationHeaderBuffer(ClBufferHandleT bufferHandle, 
                                           ClNtfNotificationHeaderBufferT *ntfPtr)
{
    ClUint32T   index = 0;

    CHECK_RETURN(clXdrUnmarshallClUint64T(bufferHandle, &ntfPtr->notificationHandle), CL_OK);
    CHECK_RETURN(clXdrUnmarshallClUint32T(bufferHandle, &ntfPtr->eventType), CL_OK);
    CHECK_RETURN(clXdrUnmarshallClNameT(bufferHandle, &ntfPtr->notificationObject), CL_OK);
    CHECK_RETURN(clXdrUnmarshallClNameT(bufferHandle, &ntfPtr->notifyingObject), CL_OK);
    CHECK_RETURN(unmarshallNtfClassIdT(bufferHandle, &ntfPtr->notificationClassId), CL_OK);
    CHECK_RETURN(clXdrUnmarshallClInt64T(bufferHandle, &ntfPtr->eventTime), CL_OK);
    CHECK_RETURN(clXdrUnmarshallClInt16T(bufferHandle, &ntfPtr->numCorrelatedNotifications), CL_OK);
    CHECK_RETURN(clXdrUnmarshallClInt16T(bufferHandle, &ntfPtr->lengthAdditionalText), CL_OK);
    CHECK_RETURN(clXdrUnmarshallClInt16T(bufferHandle, &ntfPtr->numAdditionalInfo), CL_OK);
    CHECK_RETURN(clXdrUnmarshallClInt64T(bufferHandle, &ntfPtr->notificationId), CL_OK);
    /* Allocate memory for correlatedNotifications array */
    ntfPtr->correlatedNotifications = clHeapCalloc(1,(sizeof(SaNtfIdentifierT) * ntfPtr->numCorrelatedNotifications));
    if (ntfPtr->correlatedNotifications == NULL)
        return CL_ERR_NO_MEMORY;

    /* UnMarshall the eliments of correlated notifications array */
    for (index = 0; index < ntfPtr->numCorrelatedNotifications; index++)
        CHECK_RETURN(clXdrUnmarshallClInt64T(bufferHandle, &ntfPtr->correlatedNotifications[index]), CL_OK);

    /* Allocate memory for additionalText char array */
    ntfPtr->additionalText = clHeapCalloc(1, ntfPtr->lengthAdditionalText);
    if (ntfPtr->additionalText == NULL) 
        return CL_ERR_NO_MEMORY;
    /* Unmarshall the additionalText data */
    CHECK_RETURN(clXdrUnmarshallArrayClCharT(bufferHandle, ntfPtr->additionalText, ntfPtr->lengthAdditionalText), CL_OK);

    /* Allocate memory for additionInfo array */
    ntfPtr->additionalInfo = clHeapCalloc(1, (sizeof(SaNtfAdditionalInfoT) * ntfPtr->numAdditionalInfo));
    if (ntfPtr->additionalInfo == NULL)
        return CL_ERR_NO_MEMORY;
    /* Unmarshall additionalInfo array data */
    for (index = 0; index < ntfPtr->numAdditionalInfo; index++)
        CHECK_RETURN(unmarshallNtfAdditionalInfoT(bufferHandle, &ntfPtr->additionalInfo[index]), CL_OK);

    return CL_OK;
}

