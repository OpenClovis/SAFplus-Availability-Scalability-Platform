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
 * ModuleName  : cor                                                           
 * File        : clCorMetaData.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 *  The file contains all the MetaData data structures.
 *
 *
 *****************************************************************************/

/**
 *  \file
 *  \brief Header file of all MetaData data structures
 *  \ingroup cor_apis
 */

/**
 *  \addtogroup cor_apis
 *  \{
 */



#ifndef _CL_COR_META_DATA_H_

#define _CL_COR_META_DATA_H_

#ifdef __cplusplus
    extern "C" {
#endif

#include <clCommon.h>
#include <clCntApi.h>
#include <clIocApi.h>
#include <clOsalApi.h>


/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/

/**
 *  \e CookieId of COR client for transaction forwarding.
 */
#define CL_COR_WTH_COOKIE_ID         CL_EO_COR_SERVER_COOKIE_ID+1  


#define CL_COR_VERSION_NO  0x0100
#define CL_COR_DEFAULT_MAX_SESSIONS      5

#define CL_COR_DEFAULT_MAX_RETRIES       3

/**
 * This value is 3 seconds.
 */
#define CL_COR_DEFAULT_TIMEOUT           3000 

/**
 *  Maximum size of Name for Class, Attributes.
 */
#define CL_COR_MAX_NAME_SZ              CL_MAX_NAME_LENGTH

/**
 * Persistency related Macros.
 */
#define CL_COR_NO_SAVE                 0
#define CL_COR_SAVE_PER_TXN            1   
#define CL_COR_PERIODIC_SAVE           2
#define CL_COR_DELTA_SAVE              3

/**
 * Invalid attribute
 */
#define CL_COR_UNKNOWN_ATTRIB  -1

/**
 *  String length for cli commands
 */


#define CL_COR_CLI_STR_LEN     1024


/**
 * Currently not supported. Will be included future releases.
 */

#define CL_COR_VERSION_NO         0x0100
#define CL_COR_HANDLE_MAX_DEPTH       20
#define CL_COR_INVALID_MO_ID          -1
#define CL_COR_INVALID_MO_INSTANCE    -1
#define CL_COR_INVALID_SVC_ID         -1
#define CL_COR_SVC_ID_DEFAULT 0

/**
 * Wild Card Class Id.
 */
#define CL_COR_CLASS_WILD_CARD     ((ClCorClassTypeT)0xFFFFFFFE)

/**
 * Wild Card Instance Id.
 */
#define CL_COR_INSTANCE_WILD_CARD  0xFFFFFFFE 
#define CL_COR_SVC_WILD_CARD       ((ClCorMOServiceIdT)0xFFFE)


/**
 * Constants related to \e ClCorAttrPath.
 */
#define CL_COR_CONT_ATTR_MAX_DEPTH     10

/**
 * Invalid Attribute Id.
 */
#define CL_COR_INVALID_ATTR_ID         -1

/**
 * Invalid Attribute Index.
 */
#define CL_COR_INVALID_ATTR_IDX        -1
#define CL_COR_ATTR_WILD_CARD          -2
#define CL_COR_INDEX_WILD_CARD         -2 

/**
 * Simple transaction with only one job.
 */
#define CL_COR_SIMPLE_TXN  NULL

#define CL_COR_OH_MAX_TYPES    16
#define CL_COR_OH_MAX_LEVELS   64

/** 
 *   End Marker for a OH Mask
 */
#define CL_COR_OH_MASK_END_MARKER   0xFF

/**
 * Macros for defining the attribute flags. It should be ORed value of some of 
 * of these flags.
 */

/**
 * Attribute type is config (Stored in database).
 */ 
#define CL_COR_ATTR_CONFIG          0x01000000
/**
 * Attribute type is runtime (NOT stored in database).
 */ 
#define CL_COR_ATTR_RUNTIME         0x02000000
/**
 * Not IMPLEMENTED
 */ 
#define CL_COR_ATTR_OPERATIONAL     0x04000000
/**
 * Attribute type is writeable.
 */ 
#define CL_COR_ATTR_WRITABLE        0x00010000
/**
 * Attribute type is intialized (upon creation, assign a default value -- MUST use clCorObjectCreateAndSet to create)
 */ 
#define CL_COR_ATTR_INITIALIZED     0x00020000

/**
 * Attribute type is cached (stored in RAM in COR)
 */ 
#define CL_COR_ATTR_CACHED          0x00000100
/**
 * Attribute type is persistent.
 */ 
#define CL_COR_ATTR_PERSISTENT      0x00000200

/******************************************************************************
 *  Data Types 
 *****************************************************************************/
        
/**
 * The type of an identifier for the COR class. 
 */
typedef ClInt32T   ClCorClassTypeT;

/**
 * The type of an identifier for a COR attribute. 
 */
typedef ClInt32T   ClCorAttrIdT;

/**
 * The type of an identifier for a COR instance. 
 */
typedef ClInt32T   ClCorInstanceIdT;

/**
 * Typedef for storing the attribute flags. 
 */
typedef ClUint32T  ClCorAttrFlagT;

/**
 * Status of each failed job.
 */
typedef ClUint32T ClCorJobStatusT;

/**
 * Type definition of the bundle handle.
 */
typedef     ClHandleT   ClCorBundleHandleT;
typedef     ClHandleT*  ClCorBundleHandlePtrT;


/**
 * The ClCorType enumeration contains the basic COR data types.
 */
typedef enum ClCorType {
    
  CL_COR_INVALID_DATA_TYPE = -1,

/**
 * Void data type.
 */
  CL_COR_VOID,                                        

/**
 * Character data type.
 */
  CL_COR_INT8,                                        

/**
 * Unsigned character.
 */
  CL_COR_UINT8,                                       

/**
 * Short data type.
 */
  CL_COR_INT16,

/**
 * Unsigned short.
 */
  CL_COR_UINT16,                                      

/**
 * Integer data type.
 */
  CL_COR_INT32,                                       

/**
 * Unsigned integer data type.
 */
  CL_COR_UINT32,                                      

/**
 * Long long data type.
 */
  CL_COR_INT64,                                       

/**
 * Unsigned long long data type.
 */
  CL_COR_UINT64,                                      

/**
 * Float data type.
 * \note
 * This data type will be supported in future releases, if used will default to \c CL_COR_UINT32.
 */
  CL_COR_FLOAT,                                       

/**
 * Double data type.
 * \note
 * This data type will be supported in future releases, if used will default to \c CL_COR_UINT32.
 */
  CL_COR_DOUBLE,                                      

/**
 * Counter data type.
 * \note
 * This data type will be supported in future releases, if used will default to \c CL_COR_UINT32.
 */
  CL_COR_COUNTER32,                                   

/**
 * Counter a 64-bits data type.
 * \note
 * This data type will be supported in future releases, if used will default to \c CL_COR_UINT32.
 */
  CL_COR_COUNTER64,                                   

/**
 * Sequence number data type.
 * \note
 * This data type will be supported in future releases, if used will default to \c CL_COR_UINT32.
 */
  CL_COR_SEQUENCE32                                  

} ClCorTypeT;

/**
 * The values of this enumeration type refer to the COR attribute types.
 */
typedef enum ClCorAttrType { 

    CL_COR_MAX_TYPE = CL_COR_SEQUENCE32, 

/**
 * Simple data type.
 */ 
    CL_COR_SIMPLE_ATTR,                                

/**
 * Array data type.
 */ 
    CL_COR_ARRAY_ATTR,                                 

/**
 * Containment type.
 */ 
    CL_COR_CONTAINMENT_ATTR,                           

/**
 * Association type.
 */ 
    CL_COR_ASSOCIATION_ATTR,                           

/**
 * Virtual type.
 * \note This type is currently not supported.
 */ 
    CL_COR_VIRTUAL_ATTR                               

} ClCorAttrTypeT ; 

/**
 * The enumeration ClCorAttrCmpFlagT contains the comparison flags used to compare the
 * attribute values with a specified value. It is used to filter the attributes of the MO while
 * performing the walk operation.
 */
typedef enum{ 
            /**
             * The comparison flag is invalid. This should be used if filter
             * need not do any attribute based comparison.
             */ 
              CL_COR_ATTR_CMP_FLAG_INVALID = 0,
/**
* Only the MO with attribute value equal to the value specified 
* is(are) returned.
*/
              CL_COR_ATTR_CMP_FLAG_VALUE_EQUAL_TO,
/**
* All the MO whose attribute value of the attribute is greater than
* than the value specified are returned.
*/ 
              CL_COR_ATTR_CMP_FLAG_VALUE_LESS_THAN,
/**
* All the MO whose attribute value of the attribute is greater and equal
* to the value specified are returned.
*/ 
              CL_COR_ATTR_CMP_FLAG_VALUE_LESS_OR_EQUALS,
/**
* All the MOs whose attribute value of the attribute is less than
* the value specified are returned.
*/ 
              CL_COR_ATTR_CMP_FLAG_VALUE_GREATER_THAN,
/**
* All the MOs whose attribute value of the attribute is less than and equal
* to the value specified are returned.
*/ 
              CL_COR_ATTR_CMP_FLAG_VALUE_GREATER_OR_EQUALS,
/**
* Maximum value of the attribute flag. This is just an end indicator of the
* attribute flag enum.
*/ 
              CL_COR_ATTR_CMP_FLAG_MAX
            }ClCorAttrCmpFlagT;

/**
 * The enumeration ClCorAttrWalkOpT contains the various options for walk operation on the
 * attributes. If CL_COR_ATTR_WALK_ONLY_MATCHED_ATTR is specified, the walk is performed
 * only on the matching attributes in MO. If CL_COR_ATTR_WALK_ALL_ATTR is specified, the walk
 * is performed on all the attributes in MO.
 */
typedef enum{ 
/**
       * This should be given when the attribute walk option
            * is not required.
                 */
             CL_COR_ATTR_INVALID_OPTION = 0,
  /**
         * The flag which indicate COR to return all the attributes
              * of the MO matching the filter.
                   */
             CL_COR_ATTR_WALK_ALL_ATTR,
/**
        * The flag which specifies COR to return only the attributes
              * matching the search criteria.
                    */
             CL_COR_ATTR_WALK_ONLY_MATCHED_ATTR,
/**
        * This is just an enum delimiter.
              */
             CL_COR_ATTR_WALK_MAX
            }ClCorAttrWalkOpT;

/**
 * Structure capturing initialization, minimum and maximum value of simple attribute.
 */
struct ClCorAttrValues{
/**
 * Default value of the attribute.
 */
       ClInt64T init;

/**
 * Minimum value of the attribute.
 */
       ClInt64T min;

/**
 * Maximum value of the attribute.
 */
       ClInt64T max;
};

/** 
 * Type definition for ClCorAttrValues.
 */
typedef struct ClCorAttrValues ClCorAttrValuesT;

/**
 *  Structure for complex attribute types like Association, Array and Containment.
 */
struct ClCorAttrAdditionalInfo{

/**
 *  The data type for array attributes, for instance, integer array with ten elements. Used only for Array attributes.
 */
       ClCorTypeT   arrDataType; 

/**
 * Maximum number of elements in Array, Contained or Association attributes.
 */ 
       ClUint32T        maxElement;   

/**  
 * Associated or Contained class type. Invalid for Array attributes.
 */ 
       ClCorClassTypeT  classId;       
       };
typedef struct ClCorAttrAdditionalInfo  ClCorAttrAdditionalInfoT;
/**
 *  Attribute Definition structure.
 */
struct ClCorAttrDef
       {
         ClCorAttrIdT  attrId;
         ClCorAttrTypeT attrType;
         /* union{ - NOTE: Struct made in place of Union becoz of IDL related issues.  */
         struct 
         {

/** 
 * Applicable only to 8, 16 and 32-bit integers.
 */
                ClCorAttrValuesT simpleAttrVals;

/**
 *  Additional attribute information (for Array, Containment, Associtation attribute type.)
 */                                                          
                ClCorAttrAdditionalInfoT attrInfo; 
                 
        }u;                                      
         /*     }u; */
       };
typedef struct ClCorAttrDef ClCorAttrDefT;

/**
 * Attribute Walk function prototype.
 */
typedef ClRcT (* ClCorClassAttrWalkFunc)(ClCorClassTypeT clsId, ClCorAttrDefT *attrDef, ClPtrT cookie);

/**
 * This type used to know the type of object the object-handle contains. This is used in the
 * API clCorObjectHandleToTypeGet() which will return the type of object (MO or MSO) the object-handle 
 * points to.
 */

/**
 * The values of this enumeration type is used to obtain the type of the
 * COR object. This is used by clCorObjectHandleToTypeGet() API which takes the 
 * object handle as the IN parameter and returns the type of the object as the OUT parameter.
 */
typedef enum {
    /**
         * The object type is simple. This is not supported
            * in the API.
               */ 
  CL_COR_OBJ_TYPE_SIMPLE,
   /**
        * To get the MO object given the object handle.
           */  
  CL_COR_OBJ_TYPE_MO,
   /**
        * To get the MSO object given the object handle.
           */  
  CL_COR_OBJ_TYPE_MSO
} ClCorObjTypesT;

/**
 * Lock related definitions.
 */

typedef enum {

    CL_COR_LOCK_OBJECT,
    CL_COR_LOCK_SUBTREE

} ClCorObjLockFlagsT;

/**
 * This enumeration type contains the walk related definitions. It is used to
 * perform a walk operation on a MO tree. The following values are only currently supported.
 * \arg CL_COR_MO_WALK - The walk is performed through the object tree and returns the
 * MOs below the root \e MoId that satisfies the filter criteria.
 * \arg CL_COR_MSO_WALK - Returns the object handle of all the MSO objects below the root
 * \e MoId that satisfies the filter criteria.
 */
typedef enum {
 /**
        * Walks the MO Tree. Not supported in this release.
             */ 
        CL_COR_MOTREE_WALK,
           /**
                  * The walk is performed through the object tree and returns the
                       * MOs below the root MoId that satisfies the filter criteria.
                            */    
    CL_COR_MO_WALK,
    /**
           * Returns the object handle of all the MSO objects below the root
                * MOId that satisfies the filter criteria.
                     */ 
    CL_COR_MSO_WALK,
     /**
            * This is to walk the mo-subtree below a given root. This is not supported
                 * in this release.
                      */ 
        CL_COR_MO_SUBTREE_WALK,
         /**
                * This is to walk the mso-subtree below a given root. This is not supported
                     * in this release.
                          */
        CL_COR_MSO_SUBTREE_WALK,
          /**
                 * Walk the MO objects in the object tree upwards. This is not supported.
                      */ 
    CL_COR_MO_WALK_UP,
     /**
            * Walk the MSO objects in the object tree upwards. This is not supported.
                 */ 
    CL_COR_MSO_WALK_UP

} ClCorObjWalkFlagsT;

/**
 * The values of the ClCorMoIdClassGetFlagsT enumeration type will be used in the
 * clCorMoIdToClassGet() API. This API is used to request the class ID of the MO or MSO.
 */
typedef enum {
     /**
            * If the MO class of the MOID is needed. 
                 */ 
  CL_COR_MO_CLASS_GET,
   /**
          * If the MSO class of the MOID is needed.
               */ 
  CL_COR_MSO_CLASS_GET
} ClCorMoIdClassGetFlagsT;

/**
 *  The structure ClCorObjectHandle is the handle to MO. This handle is a compressed version
 *  of the clCoirMOIdT and identifies the hierarchy in the object tree. objhandle hierarchy is
 *  compressed and indicates the indexes. objTree handle represents the tree. The COR server
 *  returns this handle to the client after the object is created.
 */
/**
 * The type for the Object Handle.
 */
typedef ClPtrT ClCorObjectHandleT;

#define CL_COR_OBJ_HANDLE_NULL              NULL 

#define CL_COR_OBJ_HANDLE_INIT(objH) \
    do \
    { \
        if (objH != NULL) \
        { \
            ClUint16T size = 0; \
            ClRcT rc = CL_OK; \
            ClUint8T* tempHandle = (ClUint8T *) objH; \
            rc = clCorObjectHandleSizeGet(objH, &size); \
            if (rc != CL_OK) \
                return rc; \
            tempHandle += sizeof(ClUint16T); \
            memset((void *) tempHandle, 0, size - sizeof(ClUint16T)); \
        } \
    } while(0)

#define CL_COR_OBJ_HANDLE_ISNULL(objH) (objH == NULL)

/**
 * The type of the handle of a COR transaction session. It is used by APIs that manipulate the
 * COR object such as:
 * \arg Creating objects
 * \arg Setting attributes
 * \arg Deleting objects
 */
typedef ClPtrT          ClCorTxnSessionIdT;

/**
 * The type of the COR transaction ID used to identify a transaction session. 
 */
typedef ClPtrT          ClCorTxnIdT;

/**
 * The type of COR transaction Job Id, used to identify a job uniquely within a transaction. 
 */
typedef ClUint32T       ClCorTxnJobIdT;

/**
 * The values of the ClCorOpsT enumeration type contain the Operation IDs. A combination of
 * Operation IDs can be used.
 */
typedef enum {
    CL_COR_OP_RESERVED = 0,
      /**
             * Operation type is create. This is set when the
                  * object is created.
                       */ 
    CL_COR_OP_CREATE = 0x1,
       /**
              * Operation type is set. This is set when the object
                   * attribute is updated.
                        */ 
    CL_COR_OP_SET =    0x2,
     /**
            * Operation type is delete. This is set when the object
                 * is delete.
                      */ 
    CL_COR_OP_DELETE = 0x4,
     /**
            * Operation type is get. This is set when the get is done on the
                 * the object.
                      */ 
    CL_COR_OP_GET =    0x8,
    /**
           * Operation type is create and set. This is set when create and set
                * operation is done on the managed object.
                     */ 
    CL_COR_OP_CREATE_AND_SET = 0x10,
    /**
           * Combination of all of operation types.
                */ 
    CL_COR_OP_ALL = 
        (CL_COR_OP_CREATE |
        CL_COR_OP_SET     |
        CL_COR_OP_DELETE  | 
        CL_COR_OP_GET     |
        CL_COR_OP_CREATE_AND_SET)
} ClCorOpsT;

/**
 * To set the status of a job given in the transaction.
 */

typedef enum ClCorTxnJobStatus
{
    CL_COR_TXN_JOB_PASS,
    CL_COR_TXN_JOB_FAIL
}ClCorTxnJobStatusT;

/**
 * MO and MSO related flags
 * Following flags are used to specify:
 * \arg Where to cache the MOs and the MSOs.
 * \arg whether the MOs MSOs should be made persistent.
 * \arg Whether a change in the MOs and MSO should generate a notification. 
 *
 * These flags are used in clCorObjectFlagsSet() and clCorObjectFlagsGet() APIs 
 * and can be applied to multiple MOs and MSOs by constructing an appropriate
 * wildcard filled \e moId.
 *
 * CACHE related flags, These CANNOT be OR'ed together. These determine where 
 * the physical copy of the object exists. 
 * \note 
 * All objects are accessible from any location where a COR instance is running. CACHE flags
 * inform the COR where the physical copy of the object is maintained.
 */

typedef enum {

/**
 * Object copy exists only on the blade.
 */
    CL_COR_OBJ_CACHE_LOCAL = 0x1,           

/**
 * Object copy exists only on the MASTER.
 */
    CL_COR_OBJ_CACHE_ONLY_ON_MASTER = 0x2, 

/**
 * Object copy exists on MASTER (e.g., SD)
 * and on the blade to which it belongs.
 */
    CL_COR_OBJ_CACHE_ON_MASTER = 0x4,       

/**
 * Object copy exists on every active COR.
 */
    CL_COR_OBJ_CACHE_GLOBAL = 0x8,          
    CL_COR_OBJ_CACHE_MAX = CL_COR_OBJ_CACHE_GLOBAL,
    CL_COR_OBJ_CACHE_MASK = 0xFF,

/**
 * This flag can be made optional with one cached flag.
 */
    CL_COR_OBJ_PERSIST = 0x100,

/**
 * This flag can be made optional with one cached flag.
 */
    CL_COR_OBJ_DO_NOT_PUBLISH = 0x200,

/**
 * This flag can be made optional with one cached flag.
 */
    CL_COR_OBJ_ALLOW_SUB_TREE_DELETE = 0x400,

/**
 * This flag can be made optional with one cached flags.
 */
    CL_COR_OBJ_FLAGS_ALL = (CL_COR_OBJ_CACHE_MASK |
                     CL_COR_OBJ_PERSIST    |
             CL_COR_OBJ_DO_NOT_PUBLISH |CL_COR_OBJ_ALLOW_SUB_TREE_DELETE)
} ClCorObjFlagsT;

#define CL_COR_OBJ_FLAGS_DEFAULT \
       (CL_COR_OBJ_CACHE_GLOBAL | CL_COR_OBJ_PERSIST | CL_COR_OBJ_ALLOW_SUB_TREE_DELETE)

/**
 * Type of the bundle operation.
 */
typedef enum ClCorBundleOperationType {
/**
 * Bundle type is transactional- for set/create/delete
 */
    CL_COR_BUNDLE_TRANSACTIONAL = 1,
/**
 * Bundle type is non-transactional
 */
    CL_COR_BUNDLE_NON_TRANSACTIONAL
}ClCorBundleOperationTypeT;


/**
 * This type definition contains the IOC physical address and the port address of the component
 * that registers itself as an OI for a MO.
 */
typedef ClIocPhysicalAddressT ClCorAddrT; 

/**
 * A pointer to the IOC physical address structure. This is populated with the IOC physical address
 * of the component that acts as an Object Implementor (OI) for a Managed Object (MO). It is
 * passed as a parameter to the OI registration API.
 */
typedef ClCorAddrT *          ClCorAddrPtrT;

/*
 * This structure ClCorCommInfo contains the communication information for COR. Used for COR 
 * communication configuration.
 */ 
struct ClCorCommInfo {

/**
 * Address to communicate.
 */
  ClCorAddrT         addr;                         

/**
 * Time-out value is in milliseconds.
 */
  ClUint32T        timeout;                      

/**
 * Maximum number of retries.
 */
  ClUint16T        maxRetries;                   

/**
 * Maximum number of sessions.
 */
  ClUint16T        maxSessions;                  

};

typedef struct ClCorCommInfo ClCorCommInfoT;
/**
 * The type of the pointer for COR communication configuration.
 */
typedef ClCorCommInfoT*     ClCorCommInfoPtrT;

/**
 *  This enumeration is also part of the ClCorMOIdT structure. The values of the enumeration
 *  indicates the type of the MOID required while working with the MOId. 
 *  \arg If the flag is CL_COR_MO_PATH_ABSOLUTE, the MOId is absolute and the first class ID 
 *  in the MOId is taken as the root. 
 *  \arg If the flag is CL_COR_MO_PATH_RELATIVE, the first class ID in the MOId is
 *  considered as the current location. The current implementation supports the
 *  CL_COR_MO_PATH_ABSOLUTE flag only.
 */
typedef enum {

/**
 * Signifies that the path is absolute, equivalent to Unix '/'.
 */
  CL_COR_MO_PATH_ABSOLUTE = 0,     

/**
 * Signifies that the path is relative.
 */
  CL_COR_MO_PATH_RELATIVE,         

/**
 * Signifies that the path is relative to blade position
 * in the COR hierarchy. equivalent to unix '~'.
 */
  CL_COR_MO_PATH_RELATIVE_TO_BASE,
 
  CL_COR_MO_PATH_QUALIFIER_MAX = CL_COR_MO_PATH_RELATIVE_TO_BASE

} ClCorMoPathQualifierT;

/**
 * Managed Object handle.
 * 
 * Identifies the managed object by its type and instance identifier.
 */
struct ClCorMOHandle {
  /**
   * Class Id.
   */
  ClCorClassTypeT    type;

  /**
   * Instance Id.
   */
  ClCorInstanceIdT   instance;
};

/**
 * Type definition for ClCorMOHandle.
 */
typedef struct ClCorMOHandle    ClCorMOHandleT;

/**
 * Pointer type definition for ClCorMOHandleT.
 */
typedef ClCorMOHandleT* ClCorMOHandlePtrT;

/* TODO: Finally need to use directly ClCorServiceIdT, but the issue with
 * enums is they occupy int space (4 bytes). So need to test and
 * figure out
 */
/**
 * This type is part of the ClCorMOIdT type to access a MO or MSO. This
 * stores the service ID of the MO/MSO whose value is equal to any of the values in the
 * enumeration type ClCorServiceIdT. For PROV MSO, the value of the service ID is
 * CL_COR_SVC_ID_PROVISIONING_MANAGEMENT.
 */
typedef ClInt16T         ClCorMOServiceIdT;  
  
/**
 * The structure ClCorMOId contains MoId of the object, which is the address of the COR object.
 * It provides a unique identification for the MO object. 
 * \par The attributes of this structure are:
 * \arg node: MO handle address. This is the combination of class ID and instance ID that
 * uniquely identifies a node and provides the path to access the object node. The class ID
 * and instance ID can use wild card entries by assigning the macros,
 * CL_COR_CLASS_WILD_CARD and CL_COR_INSTANCE_WILD_CARD.
 * \arg svcId: Service ID. It is 16 bits in length. This takes the values of the enumeration
 * \e ClCorServiceIdT. The service ID CL_COR_INVALID_SRVC_ID, is used to access the
 * MO. The service ID CL_COR_SVC_ID_ALARM_MANAGEMENT is used to access the alarm
 * MSO. The service ID CL_COR_SVC_ID_PROVISIONING_MANAGEMENT is used to access
 * the provisioning MSO.
 * \arg depth: Depth of the MoId. It is the number of elements in ClCorMOHandleT - 1.
 * \arg qualifier: Handle qualifier. This must contain the value, CL_COR_MO_PATH_ABSOLUTE.
 */
struct ClCorMOId {

/**
 * MO Handle address. This is the combination of class ID and instance ID that
 * uniquely identifies a node and provides the path to access the object node. The class ID
 * and instance ID can use wild card entries by assigning the macros CL_COR_CLASS_WILD_CARD and CL_COR_INSTANCE_WILD_CARD.
 */
  ClCorMOHandleT        node[CL_COR_HANDLE_MAX_DEPTH];   

/**
 * Service ID. It is 16 bits in length. This takes the values of the enumeration
 * \e ClCorServiceIdT. The service ID CL_COR_INVALID_SRVC_ID, is used to access the
 * MO. The service ID CL_COR_SVC_ID_ALARM_MANAGEMENT is used to access the alarm
 * MSO. The service ID CL_COR_SVC_ID_PROVISIONING_MANAGEMENT is used to access
 * the provisioning MSO.
 */
  ClCorMOServiceIdT     svcId;                        

/**
 * Depth of \e MoId. It is the number of elements in ClCorMOHandleT - 1.
 */
  ClUint16T        depth;                        

/**
 * Handle qualifier.This must contain the value, CL_COR_MO_PATH_ABSOLUTE.
 */
  ClCorMoPathQualifierT qualifier;                   

/**
 * Version information.
 */
  ClVersionT    version;
};

/**
 * Type definition for ClCorMOId.
 */
typedef struct ClCorMOId     ClCorMOIdT;

/**
 * A pointer type to ClCorMOIdT.
 */
typedef ClCorMOIdT*         ClCorMOIdPtrT;

/**
 * Attribute ID and index pair.
 */
struct ClCorAttrIdIdxPair
{
   /**
    * Attribute Id.
    */
   ClCorAttrIdT attrId;

   /**
    * Attribute index.
    */
   ClUint32T  index;

};

/**
 * Type definition for ClCorAttrIdIdxPair.
 */
typedef struct ClCorAttrIdIdxPair ClCorAttrIdIdxPairT;

/**
 * Pointer type definition for ClCorAttrIdIdxPairT.
 */
typedef ClCorAttrIdIdxPairT *ClCorAttrIdIdxPairPtr;

/**
 * The structure ClCorAttrPath contains the path-list of the attribute. This structure is
 * deprecated.
 * \arg depth: Depth of the path.
 * \arg node: Attribute ID and index pair.
 * \arg tmp: Used for padding.
 */
struct ClCorAttrPath
{
    /**
     * AttrId and Index pair array.
     */
    ClCorAttrIdIdxPairT node[CL_COR_CONT_ATTR_MAX_DEPTH];

    /**
     * Depth of the path.
     */
    ClUint16T        depth;

    /**
     * This 16 bits is used for padding.
     */
    ClUint16T        tmp;   
};

/**
 * Type definition for ClCorAttrPath.
 */
typedef struct ClCorAttrPath ClCorAttrPathT;

/**
 * The pointer to ClCorAttrPathT.
 */
typedef ClCorAttrPathT* ClCorAttrPathPtrT;

/**
 * The structure ClCorObjAttrWalkFilter is used to specify filter properties while performing
 * attribute walk operation.
 */
struct ClCorObjAttrWalkFilter
{
    /**
     * This is a depreciated feature and must be CL_TRUE for attribute walk
     */
          ClUint8T              baseAttrWalk;

/**
 * This is a depricated feature and must be CL_FALSE for attribute walk. 
 */
          ClUint8T              contAttrWalk;

/**
 * This is a depricated feature and must be NULL for attribute walk.
 */
          ClCorAttrPathT       *pAttrPath;  

/**
 * This must contain either a valid attribute ID or CL_COR_INVALID_ATTR_ID. If the
 * value is set to CL_COR_INVALID_ATTR_ID, no attribute value comparison is performed.
 */
          ClCorAttrIdT          attrId;    

/**
 * It is used to specify the index for ARRAY attributes. For a SIMPLE attribute, the
 * index is set to CL_COR_INVALID_ATTR_IDX.
 *
 * \note This value will not be processed if attrId is CL_COR_INVALID_ATTR_ID.
 */
          ClInt32T              index;     

/**
 * The comparison flag is used to compare an attribute ID against a specified
 * value. Following are the comparison flags
 * 
 * \arg CL_COR_ATTR_CMP_FLAG_VALUE_EQUAL_TO: The attributes whose value is equal
 * to the specified value is matched.
 * \arg CL_COR_ATTR_CMP_FLAG_VALUE_LESS_THAN: The attributes whose value is
 * greater than the specified value is matched.
 * \arg CL_COR_ATTR_CMP_FLAG_VALUE_LESS_OR_EQUALS: The attributes whose value
 * is greater than or equal to the specified value is matched.
 * \arg CL_COR_ATTR_CMP_FLAG_VALUE_GREATER_THAN: The attributes whose value is
 * less than the specified value is matched.
 * \arg CL_COR_ATTR_CMP_FLAG_VALUE_GREATER_OR_EQUALS: The attributes whose
 * value is less than or equal to the specified value is matched.
 * \note This value will not be processed if attrId is CL_COR_INVALID_ATTR_ID.
 */
          ClCorAttrCmpFlagT     cmpFlag;   

 /**
  * The attrWalkOption can be set to CL_COR_ATTR_WALK_ALL_ATTR or CL_COR_ATTR_WALK_ONLY_MATCHED_ATTR.
  * \note This value will not be processed if attrId is CL_COR_INVALID_ATTR_ID.
  */
          ClCorAttrWalkOpT      attrWalkOption;     

/**
 * Size of the value. 
 * \note This value will not be processed if attrId is CL_COR_INVALID_ATTR_ID.
 */
          ClUint32T             size;      

/**
 * Pointer to the value. 
 * \note This value will not be processed if attrId is CL_COR_INVALID_ATTR_ID.
 */
          void                 *value;     
       };

/**
 * Typedef for ClCorObjAttrWalkFilter.
 */
typedef struct ClCorObjAttrWalkFilter ClCorObjAttrWalkFilterT;

/**
 ************************************************
 *  \brief The type of the callback API that is invoked for every attribute within a COR object, during the
 *  walk operation.
 *
 *  \param pAttrPath (out) Path of contained object whose attribute is being walked. This must be NULL
 *  for base object attributes.
 *  \param attrId (out) Attribute ID.
 *  \param attrType (out) Attribute type. It can take one of the following values:
 *  \arg CL_COR_SIMPLE_ATTR
 *  \arg CL_COR_ARRAY_ATTR
 *  \arg CL_COR_ASSOCIATION_ATTR
 *  \param attrDataType (out) Data type of the attribute. This is valid for SIMPLE and ARRAY attributes
 *  only. For ASSOCIATION attributes, attrDataType is CL_COR_INVALID_DATA_TYPE.
 *  \param value (out) Pointer to the value of the attribute.
 *  \param size (out) Size of the value.
 *  \param attrData (out) Pointer to the attribute data, ClCorAttrFlagT which contains 
 *  the flags set for the attribute.
 *  \param cookie (out) The user data (or cookie) that is passed as a parameter to the
 *  clCorObjectAttributeWalk() API.
 *
 *  \par Description:
 *  This function type is used to define the callback functions which will be passed as the 
 *  parameter to clCorObjectAttributeWalk() API. This function will be called for each of the 
 *  attribute walked in that MO.
 *
 *  \sa clCorObjectAttributeWalk()
 */

typedef ClRcT (*ClCorObjAttrWalkFuncT) (

/*
 * Attribute path of contained object,whose attribute is being walked.
 * pAttrPath is NULL, if attribute are of the base object (containing object).
 */
ClCorAttrPathPtrT pAttrPath, 

/*
 * Attribute ID.
 */
ClCorAttrIdT attrId, 

/*
 * Attribute type. It can be any of the following values:
 * \arg \c CL_COR_SIMPLE_ATTR
 * \arg \c CL_COR_ARRAY_ATTR 
 * \arg \c CL_COR_ASSOCIATION_ATTR
 */
ClCorAttrTypeT attrType,
              
/*
 * Data type of an attribute. E.g.: \c CL_COR_UINT32.
 * \note
 * For attrType \c CL_COR_ASSOCIATION_ATTR, attrDataType is \c CL_COR_INVALID_DATA_TYPE.
 */
ClCorTypeT attrDataType, 

/*
 * Pointer to actual value of \e attrId.
 */
void *value, 

/*
 * Size of value.
 */
ClUint32T size, 

/*
 * Attribute additional Data.
 */
ClCorAttrFlagT attrData,

/*
 * Cookie passed by you.
 */
void *cookie);

/*
 * COR MOTree walk information.
 * 
 * Provides the class Id, flags and maximum number of instances. In case of MSOs, the 
 * maximum number of instances should be ignored.
 */
struct ClCorMOClassTreeWalkInfo
{

/**
 * Class identifier.
 */
  ClCorClassTypeT    classId;             

/**
 * Flags to be defined.
 */
  ClUint16T        flags;               

/**
 * Maximum number of instances possible. Should be ignored in case of MSOs.
 */
  ClUint32T        maxInstances;        
};

typedef struct ClCorMOClassTreeWalkInfo ClCorMOClassTreeWalkInfoT;
typedef ClCorMOClassTreeWalkInfoT* ClCorMOClassTreeWalkInfoPtrT;

/**
 * The structure ClCorMOClassPath contains the COR object handle.
 * It is used to Identifies the COR managed object instance with its hierarchy
 * information.
 * 
 */
struct ClCorMOClassPath {

/**
 * COR address.
 */
  ClCorClassTypeT    node[CL_COR_HANDLE_MAX_DEPTH];   

/**
 * Depth of COR address.
 */
  ClUint32T        depth;                        

/**
 * Handle qualifier.
 */
  ClCorMoPathQualifierT qualifier;                   

};

typedef struct ClCorMOClassPath  ClCorMOClassPathT;
typedef ClCorMOClassPathT*      ClCorMOClassPathPtrT;

/**
 * COR Txn Failed Job Information.
 *
 *
 * Txn Information that is needed by the client.
 * The typical information needed are moId, attrPath, attrId and 
 * the operation type.
 */

struct ClCorTxnInfo
{
/**
 * Operation type that is SET, CREATE or DELETE.
 */
    ClCorOpsT opType;
/**
 * MoId of the failed Job.
 */
    ClCorMOIdT moId;
/** 
 * Containment Path of the failed Job.
 */
    ClCorAttrPathT attrPath;
/**
 * Attribute Id of the failed job.
 */
    ClCorAttrIdT attrId;
/**
 *  Transaction failed job id.
 */
    ClUint32T jobStatus;
};

/** 
 * Type definition for ClCorTxnInfo.
 */
typedef struct ClCorTxnInfo ClCorTxnInfoT;

/**
 * Pointer type definition for ClCorTxnInfoT.
 */
typedef ClCorTxnInfoT* ClCorTxnInfoPtrT;

/*
 * Operations used while storing and retrieving the failed jobs in/from the container. 
 */
 
typedef enum  ClCorTxnEntryId
{
    CL_COR_TXN_INFO_ADD,
    CL_COR_TXN_INFO_FIRST_GET,
    CL_COR_TXN_INFO_NEXT_GET,
    CL_COR_TXN_INFO_CLEAN
}ClCorTxnEntryIdT;

/*
 * The data that is used to form the key while storing the
 * failed jobs into the container
 */
struct ClCorTxnInfoStore
{
    ClCorTxnEntryIdT op;
    
    ClCorTxnSessionIdT txnSessionId;

    ClCorTxnInfoT txnInfo;
};

typedef struct ClCorTxnInfoStore ClCorTxnInfoStoreT;
typedef ClCorTxnInfoStoreT* ClCorTxnInfoStorePtrT;

/**
 * Type to provide value for the initialized attributes
 */
struct ClCorAttributeValue
{
    /** 
     * Attribute Path
     */
    ClCorAttrPathPtrT pAttrPath;

    /**
     * Attribute Identifier
     */
    ClCorAttrIdT attrId;

    /**
     * Index of the attribute
     */
    ClInt32T index;

    /**
     * Pointer to the buffer which contain the data
     */
    ClPtrT bufferPtr;

    /**
     * Size of the buffer
     */
    ClInt32T bufferSize;
};

/**
 * Type definition for ClCorAttributeValue.
 */
typedef struct ClCorAttributeValue ClCorAttributeValueT;

/**
 * Pointer type for ClCorAttributeValueT
 */
typedef ClCorAttributeValueT* ClCorAttributeValuePtrT;

/**
 * Type to provide the list of attribute values for initialized attributes.
 */
struct ClCorAttributeValueList
{
    /**
     * Number of attribute values
     */
    ClUint32T numOfValues;

    /**
     * List of attribute values
     */
    ClCorAttributeValuePtrT pAttributeValue;
};

/**
 * Type definition for ClCorAttributeValueList
 */
typedef struct ClCorAttributeValueList ClCorAttributeValueListT;

/**
 * Pointer type for ClCorAttributeValueListT
 */
typedef ClCorAttributeValueListT * ClCorAttributeValueListPtrT;

/**
 * Structure to attribute job which will be added to the bundle.
 */
struct ClCorAttrValueDescriptor
{
/**
 * Pointer to the attribute path. It will be NULL for non-alarm attributes.
 */
    ClCorAttrPathPtrT pAttrPath;
/**
 * Attribute Identifier.
 */
    ClCorAttrIdT      attrId;
/**
 * Index of the array attribute.
 */
    ClInt32T          index;
/**
 *  Pointer to the buffer which contain the data.
 */
    ClPtrT      bufferPtr;
/**
 * Size of the buffer pointed by bufferPtr.
 */
    ClInt32T    bufferSize;
/**
 * Status of the job.
 */
    ClCorJobStatusT * pJobStatus;
};

/**
 * Type definition for ClCorAttrValueDescriptor.
 */
typedef struct ClCorAttrValueDescriptor ClCorAttrValueDescriptorT;

/**
 * Pointer type for ClCorAttrValueDescriptorT. 
 */
typedef ClCorAttrValueDescriptorT *     ClCorAttrValueDescriptorPtrT;

/**
 * Type for the attribute value list. It contains the number of descriptors
 * and the pointer to the actual descriptors.
 */
struct ClCorAttrValueDescriptorList
{
/**
 * Number of Attribute Descriptor.
 */
    ClUint32T numOfDescriptor;

/**
 * Pointer to the list of attribute descriptors.
 */
    ClCorAttrValueDescriptorPtrT pAttrDescriptor;
};

/**
 * Type definition for ClCorAttrValueDescriptorList.
 */
typedef struct ClCorAttrValueDescriptorList ClCorAttrValueDescriptorListT;

/** 
 * Pointer type to ClCorAttrValueDescriptorListT.
 */
typedef ClCorAttrValueDescriptorListT * ClCorAttrValueDescriptorListPtrT;

/**
 * Structure to store read job data.
 * This will be used while doing bulk get.
 */

struct ClCorJobDescriptor {
/**
 * Pointer to the object handle. For create operation it will store the object handle of the object 
 * created. For set/get/delete operation it will store the handle to the object on which operation
 * need to be done.
 */
    ClCorObjectHandleT          *objHandle;
/** 
 * Pointer to the MOID.
 */
    ClCorMOIdPtrT               pMoId;
/**
 *  Operation type which will take one of the following: 
 *  CL_COR_OP_CREATE, CL_COR_OP_SET, CL_COR_OP_GET, CL_COR_OP_DELETE.
 */
    ClCorOpsT                   opType;
/**
 * Number of job attribute descriptors.
 */
    ClUint32T                   numOfAttrDesc;
/**
 *  Array of attribute descriptors.
 */
    ClCorAttrValueDescriptorPtrT  pAttrDesc;
};

typedef  struct ClCorJobDescriptor ClCorJobDescriptorT;
typedef   ClCorJobDescriptorT  *ClCorJobDescriptorPtrT ;


/**
 * Structure for storing the bundle configuration.
 */
struct ClCorBundleConfig
{
/**
 * Type of bundle.
 */
    ClCorBundleOperationTypeT bundleType;
};

typedef struct ClCorBundleConfig ClCorBundleConfigT;
typedef ClCorBundleConfigT *    ClCorBundleConfigPtrT;


/**
 ************************************************
 * \brief Type of callback function used for Object Walk.
 *
 * \param data (out) Handle of the MO object being walked.
 * \param cookie (out) User passed data.
 *
 * \retval ClRcT User passed return value.
 *
 * \par Description:
 * This typedef is the prototype for the COR Object Walk callback function that must be provided
 * as a parameter to the clCorObjectWalk() API. The callback API is defined by the application
 * and is called for each object.
 * 
 * \sa
 * None
 */
typedef ClRcT (*ClCorObjectWalkFunT)(void * data, void *cookie);

/**
 *  IDL Marshall Function Prototype
 */
typedef ClRcT (*clCorXdrMarshallFP)(void *pGenVar, ClBufferHandleT msg, ClUint32T isDelete);

/**
 *  IDL UnMarshall Function Prototype
 */
typedef ClRcT (*clCorXdrUnmarshallFP)(ClBufferHandleT msg, void *pGenVar);

/**
 ************************************************
 *  \brief The callback function for the asynchronous bunlde.
 *
 *  \par Header File:
 *  clCorApi.h
 *
 *  \param bundleHandle (in) The bundle handle obtained after doing \e clCorBundleInitialize().
 *  \param userArg (in) Pointer to the user data or cookie passed while initializing the bundle.
 *
 *  \retval CL_OK The callback function completed successfully.
 *
 *  \par Description:
 *   This callback function would be called after getting the response of the bundle which was
 *   started asynchronously. The user buffer for all the get operations will be populated
 *   before this function is called. So inside this function, the user can access the user buffer
 *   only if the status of job for that get operation is \e CL_OK.
 *
 *
 *  \par Library File:
 *  ClCorClient
 *
 *  \sa clCorBundleInitialize(), clCorBundleFinalize().
 *
 */

typedef ClRcT (*ClCorBundleCallbackPtrT) (ClCorBundleHandleT bundleHandle, ClPtrT userArg);


#ifdef __cplusplus
}
#endif
#endif  /* _CL_COR_META_DATA_H_ */


/** \} */
