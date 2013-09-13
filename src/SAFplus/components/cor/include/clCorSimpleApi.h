#ifndef CL_COR_SIMPLE_API_H
#define CL_COR_SIMPLE_API_H

#include <clCorApi.h>
#include <clCorServiceId.h>
#include <clMemGroup.h>

typedef struct ClCorObjectReference
{
   ClCorObjectHandleT objHandle;
   ClCorMOIdT moId;
   /*
    * maybe something extra that I am not aware of: like txn session
ID, etc.
    */
} ClCorObjectReferenceT;

#define clFieldOffset(_type,_field) ((ClUint32T)(ClWordT)&(((_type*) 0)->_field))
#define clField(_type,_field) ((ClUint32T)(ClWordT)&(((_type*) 0)->_field)), sizeof((((_type*) 0)->_field))

typedef ClCorObjectReferenceT* ClCorHandleT;

typedef enum ClCorObjectFlags
{
   CL_COR_OBJECT_CREATE,
} ClCorObjectFlagsT;



typedef struct ClCor2CEntry
{
    ClCorAttrIdT attrId;
    ClUint32T    offset;
    ClUint32T    length;
    
} ClCor2CEntryT;

typedef struct ClCor2C
{
    ClUint32T     length;
    ClCor2CEntryT* itemDef;    
} ClCor2CT;

/**
 * COR Tree common elements (i.e. base class)
 *
 */  
typedef struct ClCorTree
{
    /** All nodes in this tree are allocated from this memory group */
    ClMemGroupT  allocated;
    /** Modification mutex */
    ClOsalMutexT mutex;
    ClWordT      nodeSize;    
} ClCorTreeT;

typedef struct ClCorTreeNode
{
    struct ClCorTreeNode* sib;
    struct ClCorTreeNode* child;
    struct ClCorTreeNode* parent;
} ClCorTreeNodeT;

/** Object Tree node.  This tree contains actual objects and their values */
typedef struct ClCorObjectTreeNode
{
    ClCorTreeNodeT      cmn;
    
    SaNameT             name;    
    ClPtrT*             value;
    ClWordT             size;
    ClCorTypeT          type;
    ClCorAttrTypeT      ordinality;    
} ClCorObjectTreeNodeT;

/** Class tree node */
typedef struct ClCorClassTreeNode
{
    ClCorTreeNodeT      cmn;
    SaNameT             name;
} ClCorClassTreeNodeT;

/** Object Tree.  This tree contains actual objects and their values */
typedef struct ClCorObjectTree
{
    /** Elements common to all COR trees */
    ClCorTreeT            cmn;
    /** the top of the tree */
    ClCorObjectTreeNodeT* root;    
} ClCorObjectTreeT;

/** Class Tree.  This tree contains the COR class structure.  It does not contain any object instances.  */
typedef struct ClCorClassTree
{
    /** Elements common to all COR trees */
    ClCorTreeT            cmn;
    /** the top of the tree */
    ClCorClassTreeNodeT* root;    
} ClCorClassTreeT;


/**
 *  \brief Translate a MOID string into an encoded MOID object.
 *
 *  \par Header File:
 *  clCorSimpleApi.h
 *
 *  \par Library File:
 *  libClCorClient
 *
 *  \param name (in) The string MOID.  For example "\Chassis:0\myobj:10"
 *  \param type (in) The service type you want.  If in doubt you want to use: CL_COR_SVC_ID_PROVISIONING_MANAGEMENT
 *
 *  \retval A reference to the MOID object which consist of both ClCorObjectHandleT and ClCorMOIdT.
 *
 *  \par Description:
 *  This API is used to translate a MOID string into an encoded MOID object.  The encoded MOID is used in a lot of other COR APIs.
 *  When done, you must delete this object using clCorReleaseObjRef
 *
 *  \sa clCorReleaseObjRef
 */
ClCorObjectReferenceT *clCorObjRefGet(const ClCharT *name,ClCorServiceIdT type);

/**
 *  \brief Release a reference gotten from clCorGetObjRef
 *
 *  \par Header File:
 *  clCorSimpleApi.h
 *
 *  \par Library File:
 *  libClCorClient
 *
 *  \param name (in) The string MOID.  For example "\Chassis:0\myobj:10"
 *  \param type (in) The service type you want.  If in doubt you want to use: CL_COR_SVC_ID_PROVISIONING_MANAGEMENT
 *
 *  \retval Nothing
 *
 *  \par Description:
 *  This API is used to return an object reference.
 *
 *  \sa clCorGetObjRef
 */
void clCorObjRefRelease(ClCorObjectReferenceT *ref);

/**
 *  \brief Open a COR object for access
 *
 *  \par Header File:
 *  clCorSimpleApi.h
 *
 *  \par Library File:
 *  libClCorClient
 *
 *  \param name (in) The string MOID.  For example "\Chassis:0\myobj:10"
 *  \param flags (in) If you want to create the object (if it does not exist), pass CL_COR_OBJECT_CREATE, otherwise pass 0.
 *  \param fd (out) A handle to the opened object.
 *
 *  \retval Nothing
 *
 *  \par Description:
 *  This API is used to open or create a COR object.  When finished, you must return the handle, using the API clCorHandleDelete.
 *
 *  \sa clCorHandleRelease
 */
ClRcT clCorOpen(const ClCharT *name, ClCorObjectFlagsT flags, ClCorHandleT *fd);

/**
 *  \brief Give a handle back (and close the associated object)
 *
 *  \par Header File:
 *  clCorSimpleApi.h
 *
 *  \par Library File:
 *  libClCorClient
 *
 *  \param hdl (in) The handle
 *
 *  \retval CL_OK or and error if the handle could not be released.
 *
 *
 *  \sa clCorObjectOpen
 */
ClRcT clCorHandleRelease(ClCorHandleT hdl);

/**
 *  \brief Delete a COR object
 *
 *  \par Header File:
 *  clCorSimpleApi.h
 *
 *  \par Library File:
 *  libClCorClient
 *
 *  \param name (in) The string MOID.  For example "\Chassis:0\myobj:10"
 *  \param type (in) What
 *  \param fd (out) A handle to the opened object.
 *
 *  \retval Nothing
 *
 *  \par Description:
 *  This API is used to delte a COR object.
 *
 *  \sa clCorObjectOpen
 */
ClRcT clCorDelete(const ClCharT *name);

/**
 *  \brief Create a COR object
 *
 *  \par Header File:
 *  clCorSimpleApi.h
 *
 *  \par Library File:
 *  libClCorClient
 *
 *  \param name (in) The string MOID.  For example "\Chassis:0\myobj:10"
 *  \param myObj (in) a pointer to a structure containing initial values for the object's fields
 *  \param objDef (in) a pointer to a structure that describes the contents of myObj
 *  \param fd (out) A handle to the opened object.
 *
 *  \retval ClRcT
 *
 *  \par Description:
 *  This API is used to create a COR object.  Object values are filled in from the passed buffer, using the supplied object definition.
 *
 *  \sa clCorObjectOpen
 */
ClRcT clCorCreate(const ClCharT *name,void* myObj, ClCor2CT* objDef, ClCorHandleT *fd);

/**
 *  \brief Set this object implementer as active
 *
 *  \par Header File:
 *  clCorSimpleApi.h
 *
 *  \par Library File:
 *  libClCorClient
 *
 *  \param name (in) The string MOID.  For example "\Chassis:0\myobj:10"
 *  \param val (in) CL_TRUE to make this OI primary, CL_FALSE to remove this OI as primary.
 *
 *  \retval ClRcT
 *
 *  \par Description:
 *  After this API is called, COR will send read/write requests from apps on the object represented the MOID to this OI.
 *
 *  \sa clCorSetPrimaryByMoid
 */
ClRcT clCorSetPrimary(const ClCharT *name, ClBoolT val);

/**
 *  \brief Set this object implementer as active
 *
 *  \par Header File:
 *  clCorSimpleApi.h
 *
 *  \par Library File:
 *  libClCorClient
 *
 *  \param moId (in) The MOID object.
 *  \param val (in) CL_TRUE to make this OI primary, CL_FALSE to remove this OI as primary.
 *
 *  \retval ClRcT
 *
 *  \par Description:
 *  After this API is called, COR will send read/write requests from apps on the object represented the MOID to this OI.
 *  This variant of the API is useful in certain callbacks where the moId is provided as an encoded value, not as a string.
 *
 *  \sa clCorSetPrimaryByMoid
 */
ClRcT clCorSetPrimaryByMoid(void* moId, ClBoolT state);

/**
 *  \brief Read the COR Object and put the fields into a structure.
 *
 *  \par Header File:
 *  clCorSimpleApi.h
 *
 *  \par Library File:
 *  libClCorClient
 *
 *  \param moId (in) The MOID object.
 *  \param myObj (in) a pointer to a structure containing initial values for the object's fields
 *  \param objDef (in) a pointer to a structure that describes the contents of myObj
 *
 *  \retval ClRcT
 *
 *  \par Description:
 *  This variant of the API is useful in certain callbacks where the moId is provided as an encoded value, not as a string.
 *
 *  \sa clCorReadByMoid
 */
ClRcT clCorRead(const ClCharT *name,void* myObj, ClCor2CT* objDef);

/**
 *  \brief Read the COR Object and put the fields into a structure.
 *
 *  \par Header File:
 *  clCorSimpleApi.h
 *
 *  \par Library File:
 *  libClCorClient
 *
 *  \param moId (in) The MOID object.
 *  \param myObj (in) a pointer to a structure containing initial values for the object's fields
 *  \param objDef (in) a pointer to a structure that describes the contents of myObj
 *
 *  \retval ClRcT
 *
 *  \par Description:
 *  This variant of the API is useful in certain callbacks where the moId is provided as an encoded value, not as a string.
 *
 *  \sa clCorRead
 */
ClRcT clCorReadByMoid(void* MoId, void* myObj, ClCor2CT* objDef);

/**
 *  \brief Read the COR Object and put the fields into a structure.
 *
 *  \par Header File:
 *  clCorSimpleApi.h
 *
 *  \par Library File:
 *  libClCorClient
 *
 *  \param moId (in) The MOID object.
 *  \param myObj (in) a pointer to a structure containing initial values for the object's fields
 *  \param objDef (in) a pointer to a structure that describes the contents of myObj
 *
 *  \retval ClRcT
 *
 *  \par Description:
 *  This variant of the API is useful in certain callbacks where the moId is provided as an encoded value, not as a string.
 *
 *  \sa clCorRead
 */
ClCorAttributeValueListT* clCorGetAttValueList(void* myObj,ClCor2CT* objDef);
void clCorReleaseAttValueList(ClCorAttributeValueListT* obj);


ClCorAttrValueDescriptorListT* clCorGetAttValueDescList(void* myObj,ClCor2CT* objDef,ClCorJobStatusT* jobStatus);
void clCorReleaseAttValueDescList(ClCorAttrValueDescriptorListT* obj);

/**
 *  \brief Initialize a tree node object. Not really used outside of the library
 *  \sa clCorObjectTreeNodeInit(), clCorClassTreeNodeInit()
 */
void clCorTreeNodeInit(ClCorTreeNodeT* obj);


/**
 *  \brief Initialize a tree object.  Use the class/object versions of this function.
 *  \sa clCorObjectTreeInit(), clCorClassTreeInit()
 */
void clCorTreeInit(ClCorTreeT* obj,ClWordT nSize);

/**
 *  \brief Delete a tree object.  Use the class/object versions of this function.
 *  \sa clCorObjectTreeDelete(),clCorClassTreeDelete()
 */
void clCorTreeDelete(ClCorTreeT* obj);

/**
 *  \brief Initialize an object tree.
 *  \sa clCorObjectTreeDelete()
 */
void clCorObjectTreeInit(ClCorObjectTreeT* obj);

/**
 *  \brief Delete an object tree.
 *  \sa clCorObjectTreeInit()
 */
void clCorObjectTreeDelete(ClCorObjectTreeT* obj);

/**
 *  \brief Initialize a class tree.
 *  \sa clCorClassTreeDelete()
 */
void clCorClassTreeInit(ClCorClassTreeT* obj);

/**
 *  \brief Delete a class tree.
 *  \sa clCorClassTreeDelete()
 */
void clCorClassTreeDelete(ClCorClassTreeT* obj);


void clCorObjectTreeNodeInit(ClCorObjectTreeNodeT* obj);
void clCorClassTreeNodeInit(ClCorClassTreeNodeT* obj);

/**
 *  \brief Create a new Object tree node, for a specific object tree.
 *
 *  \par Description:
 *  Nodes in the object tree are allocated within a memory group, so you must use this API when allocating nodes.
 * 
 */
ClCorObjectTreeNodeT* clCorObjectTreeNewNode(ClCorObjectTreeT* tree);

/**
 *  \brief Create a new Object tree node, given an object tree
 *
 *  \par Description:
 *  Nodes in the object tree are allocated within a memory group, so you must use this API when allocating nodes.
 *
 */
ClCorClassTreeNodeT* clCorClassTreeNewNode(ClCorClassTreeT* tree);

/**
 *  \brief Dump a class tree (for debugging)
 */
void clCorClassTreePrint(ClCorClassTreeT* tree);

/**
 *  \brief Dump an object tree (for debugging)
 */
void clCorObjectTreePrint(ClCorObjectTreeT* tree);

/**
 *  \brief Fill the class tree from data in COR
 *
 *  \par Header File:
 *  clCorSimpleApi.h
 *
 *  \par Library File:
 *  libClCorClient
 *
 *  \retval ClRcT
 *
 *  \par Description:
 *  This API fills a class tree with all the data from COR.  A class tree contains the COR classes but no object instances.
 *
 *  \sa clCorObjectTreeFill
 */
ClRcT clCorClassTreeFill(ClCorClassTreeT* tree);

/**
 *  \brief Fill the object tree from data in COR
 *
 *  \par Header File:
 *  clCorSimpleApi.h
 *
 *  \par Library File:
 *  libClCorClient
 *
 *  \retval ClRcT
 *
 *  \par Description:
 *  This API fills an object tree with all the data from COR.  A class tree contains the COR classes but no object instances.
 *
 *  \sa clCorClassTreeFill
 */
ClRcT clCorObjectTreeFill(ClCorObjectTreeT* tree);

/* internal functions (not really independently useful) */
ClCorObjectTreeNodeT* clCorObjectTreeNodeFindSib(ClCorObjectTreeNodeT* sib,char* str,int len);
ClCorClassTreeNodeT* clCorClassTreeNodeFindSib(ClCorClassTreeNodeT* sib,char* str,int len);

void clCorObjectTreeNodePrint(ClCorObjectTreeNodeT* node,ClWordT indent);
void clCorClassTreeNodePrint(ClCorClassTreeNodeT* node,ClWordT indent);

#endif
