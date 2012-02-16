#include <clNtfApi.h>

#ifndef _CL_NTF_XDR_H
#define _CL_NTF_XDR_H
    
# ifdef __cplusplus
extern "C"
{   
# endif

#define CHECK_RETURN(x,y)   do { \
    ClRcT   rc = CL_OK;          \
    rc = (x);                    \
    if (rc != (y))               \
        return rc;               \
}while(0)

/* Marshalling functions */
extern ClRcT   marshallNtfClassIdT(SaNtfClassIdT *classId, ClBufferHandleT bufferHandle);

extern ClRcT   marshallNtfAdditionalInfoT(SaNtfAdditionalInfoT *addInfo, ClBufferHandleT bufferHandle);

extern ClRcT   marshallNtfValueTypeT(SaNtfValueTypeT infoType, SaNtfValueT *infoValue, ClBufferHandleT bufferHandle);

extern ClRcT   marshallNotificationHeaderBuffer(ClBufferHandleT bufferHandle, ClNtfNotificationHeaderBufferT *ntfPtr);

/*Unmarshall functions */
extern ClRcT   unmarshallNtfClassIdT(ClBufferHandleT bufferHandle, SaNtfClassIdT *classId);

extern ClRcT   unmarshallNtfAdditionalInfoT(ClBufferHandleT bufferHandle, SaNtfAdditionalInfoT *addInfo);

extern ClRcT   unmarshallNtfValueTypeT(ClBufferHandleT bufferHandle, SaNtfValueTypeT infoType, SaNtfValueT *infoValue);

extern ClRcT   unmarshallNotificationHeaderBuffer(ClBufferHandleT bufferHandle, ClNtfNotificationHeaderBufferT *ntfPtr);
# ifdef __cplusplus
}
# endif

#endif 

