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
 * ModuleName  : cor
 * File        : clCorClassApis.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains client API for COR class manipulations
 *****************************************************************************/

#include <string.h>
#include <clCommon.h>
#include <clDebugApi.h>
#include <clCorMetaData.h>
#include <clCorApi.h>
#include <clCorUtilityApi.h>
#include <clCorErrors.h>
#include <clCpmApi.h>
#include <clCorRMDWrap.h>
#include <clCorClient.h>
#include <clCorDmData.h>

#include <xdrCorClsIntf_t.h>
#include <xdrCorAttrIntf_t.h>
#include <xdrCorNiOpInf_t.h>
#include <xdrClCorAttrDefT.h>
#include <xdrClCorAttrDefIDLT.h>


#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

ClRcT clCorNIClassAdd(char *name, ClCorClassTypeT key);
ClRcT clCorNIClassDelete(ClCorClassTypeT classId);
ClRcT clCorNIClassIdGet(char *name, ClCorClassTypeT *key);
ClRcT clCorNIClassNameGet(ClCorClassTypeT key, char *name );
ClRcT clCorNIAttrAdd(ClCorClassTypeT  classId, ClCorAttrIdT  attrId, char *name);
ClRcT clCorNIAttrDel(ClCorClassTypeT classId, ClCorAttrIdT  attrId);
ClRcT clCorNIAttrNameGet(ClCorClassTypeT classId, ClCorAttrIdT  attrId, char *name );

static ClRcT
_corClassAttrCreate(ClCorClassTypeT  classId, 
                    ClCorAttrIdT     attrId,
                    ClCorTypeT       attrType,
                    ClCorClassTypeT  subClass,
                    ClInt32T       min,
                    ClInt32T       max);

static ClRcT 
_corClassAttributeListGet(ClCorClassTypeT classId,
                    ClCorAttrFlagT attrFlags,
                    ClUint32T* pAttrCount,
                    ClCorAttrDefT** pAttrDefList);
/**
 *  Create a new Class Type.
 *
 *  API to create a new class type. Creates a new class type with the
 *  given class id and super class id.  Super class id specified is
 *  the parent class from which the new class type inhertis (all the
 *  super class attribute definitions are inherited by this new type).
 *
 *  On success, the API returns a class handle that shall be used to
 *  add attributes to the newly created class type.
 *
 *  @param classId      Class Type to be created.
 *  @param superClassId Super (inherited from) Class Type. (zero if none)
 *
 *  @returns 
 *    CL_OK on success <br/>
 *    CL_COR_ERR_CLASS_PRESENT  classId already defined <br/>
 *    CL_COR_ERR_CLASS_NOT_PRESENT superClassId specfied is not present <br/>
 *    CL_COR_ERR_NO_MEM Out of memory <br/>
 */
ClRcT 
clCorClassCreate(ClCorClassTypeT classId, 
               ClCorClassTypeT superClassId)
{
    ClRcT              rc = CL_OK;
    corClsIntf_t        param;
    ClUint32T          size =  sizeof(param);

	memset(&param, 0, sizeof(param));
    CL_FUNC_ENTER();

    /* Basic Validations */
    if( 0 >= classId || 0 > superClassId)
    {
        CL_FUNC_EXIT();
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Invalid argguments. Class Id 0 is reserved : Class:%04x SuperClass:%04x", classId, superClassId));
        return CL_COR_SET_RC(CL_COR_ERR_INVALID_CLASS);
    }

    /* param.version = CL_COR_VERSION_NO; */
	CL_COR_VERSION_SET(param.version);
    param.op      = COR_CLASS_OP_CREATE;
    param.classId = classId;
    param.superClassId = superClassId;
    
    /* Call the RMD now */
    COR_CALL_RMD(COR_EO_CLASS_OP,
                 VDECL_VER(clXdrMarshallcorClsIntf_t, 4, 0, 0),
                 &param,
                 size,
                 VDECL_VER(clXdrUnmarshallcorClsIntf_t, 4, 0, 0),
                 &param,
                 &size,
                 rc);

    if (rc != CL_OK)
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Create failed Class:%04x SuperClass:%04x [rc 0x%x]", 
                          classId, superClassId, rc));

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "ClassCreate Class:%04x SuperClass:%04x", classId, superClassId));
    CL_FUNC_EXIT();
    return rc; 
}

/**
 *  Delete class type.
 *
 *  API to delete a given class type.  If the class have instances, or
 *  if its inherited by another class, then the class type can't be
 *  deleted.
 *                                                                        
 *  @param classId       Identifier of the class
 *
 *  @returns 
 *    CL_OK on success <br/>
 *    CL_COR_ERR_CLASS_INSTANCES_PRESENT  instances are present for the type<br/>
 *
 */
ClRcT 
clCorClassDelete(ClCorClassTypeT classId)
{
    ClRcT              rc = CL_OK;
    corClsIntf_t        param;
    ClUint32T          size =  sizeof(param);

    CL_FUNC_ENTER();

	memset(&param, 0, sizeof(param));
    if (0 >= classId)
    {
          CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "ClassDelete: Invalid Class Type"));
          return CL_COR_SET_RC(CL_COR_ERR_INVALID_CLASS);
    }

    /* param.version = CL_COR_VERSION_NO; */
	CL_COR_VERSION_SET(param.version);
    param.op      = COR_CLASS_OP_DELETE;
    param.classId = classId;
    
    /* Call the RMD now */
    COR_CALL_RMD(COR_EO_CLASS_OP,
                 VDECL_VER(clXdrMarshallcorClsIntf_t, 4, 0, 0),
                 &param,
                 size,
                 VDECL_VER(clXdrUnmarshallcorClsIntf_t, 4, 0, 0),
                 &param,
                 &size,
                 rc);
    if (rc != CL_OK)
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "ClassDelete failed  Class:%04x [rc 0x%x]", classId, rc));

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "ClassDelete Class:%04x", classId));
    CL_FUNC_EXIT();
    return (rc);
}

/**
 *  Create a new attribute and add to class type.
 *
 *  API to create a new attribute with given information and add the
 *  attribute definition to the class type. The function will succeed
 *  only if there are no instances created on the given class type.
 *                                                                        
 *  @param Class ID     Identifier of the class 
 *  @param attrId       Attribute Identifier
 *  @param attrType     Attribute Type
 *
 *  @returns 
 *    CL_OK on success <br/>
 *    CL_COR_ERR_CLASS_ATTR_PRESENT Attribute already defined <br/>
 *    CL_COR_ERR_CLASS_INSTANCES_PRESENT Class Type have instances <br/>
 *    CL_COR_ERR_NO_MEM Out of memory <br/>
 *
 */
ClRcT 
clCorClassAttributeCreate(ClCorClassTypeT    classId,
                  ClCorAttrIdT       attrId,
                  ClCorTypeT         attrType)
{
    ClRcT              rc = CL_OK;
    CL_FUNC_ENTER();
    rc  = _corClassAttrCreate(classId, attrId, attrType, 0, 0, 0);
    CL_FUNC_EXIT();
    return (rc);
}

/**
 *  Create assocation between classes.
 *
 *  API to add association between classes.  Adds information like
 *  class type, its associated class and the relationship cardinality
 *  (maximum number of instances that can be associated with an
 *  instance of the given class type).
 * 
 *  @param classHandle       Handle to class type
 *  @param attrId            Attribute Identifier
 *  @param associatedClass   Associated Class Identifer
 *  @param max               Max instances associated 
 *
 *  @returns 
 *    CL_OK on success <br/>
 *
 */
ClRcT 
clCorClassAssociationCreate(ClCorClassTypeT classId,
                    ClCorAttrIdT    attrId,
                    ClCorClassTypeT assocClass,
                    ClInt32T      max)
{
    ClRcT    rc = CL_OK;

    CL_FUNC_ENTER();

    /* Bug Id      : 5402 
     * Description : The COR Server is showing abrupt behavior when max-association 
         parameter of the clCorClassAssociationCreate api is given 0 or 1.
     * Fix         : Check if the max association is < 1, and return from the client itself.
     */
    if (max < 1)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid Max Associations specified for the attribute [0x%x] in class [0x%x]\n", attrId, classId));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
    }

    rc  = _corClassAttrCreate(classId, attrId, CL_COR_ASSOCIATION_ATTR, assocClass,0, max);

    CL_FUNC_EXIT();
    return rc;
}

/**
 *  Create containment between classes.
 *
 *  API to add containment between classes.  Adds information like
 *  class type, its contained class type and the containment
 *  cardinality. Minimum captures the default objects that are created
 *  when a new instance of class type is created. Maximum captures the
 *  maximum number of instances that can be contained within an
 *  instance of the given class.
 *                                                                        
 *  @param classHandle       Handle to class type
 *  @param attrId            Attribute Identifier
 *  @param containedClass    Contained Class Identifer
 *  @param min               Min instances (By default these are created)
 *  @param max               Max instances 
 *
 *  @returns 
 *    CL_OK on success <br/>
 *
 */
ClRcT 
clCorClassContainmentAttributeCreate(ClCorClassTypeT classId,
                         ClCorAttrIdT    attrId,
                         ClCorClassTypeT containedClass,
                         ClInt32T      min,
                         ClInt32T      max)
{
    ClRcT    rc = CL_OK;

	if(classId == containedClass)
		{
		CL_COR_RETURN_ERROR(CL_DEBUG_ERROR,"\nContained class can not be same as container class\n", CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
		}
	
    rc  = _corClassAttrCreate(classId, attrId, CL_COR_CONTAINMENT_ATTR, containedClass, min, max);
    return rc;
}

/**
 *  Create a new array attribute and add to class type.
 *
 *  API to create a new array type attribute with given information
 *  and add the attribute definition to the class type. The function
 *  will succeed only if there are no instances created on the given
 *  class type.
 *                                                                        
 *  @param classHandle  Handle to class type
 *  @param attrId       Attribute Identifier
 *  @param attrType     Attribute Type
 *  @param arraySize    Array Size (should be > 0)
 *
 *  @returns 
 *    CL_OK on success <br/>
 *    CL_COR_ERR_CLASS_ATTR_PRESENT Attribute already defined <br/>
 *    CL_COR_ERR_CLASS_INSTANCES_PRESENT Class Type have instances <br/>
 *    CL_COR_ERR_NO_MEM Out of memory <br/>
 *    CL_COR_ERR_CLASS_ATTR_INVALID_VAL Invalid array size <br/>
 */
ClRcT 
clCorClassAttributeArrayCreate(ClCorClassTypeT classId,
                        ClCorAttrIdT    attrId,
                        ClCorTypeT      attrType,
                        ClInt32T      arraySize)
{
    ClRcT              rc = CL_OK;
    corAttrIntf_t       param;
    ClUint32T          size =  sizeof(param);

    CL_FUNC_ENTER();

	memset(&param, 0, sizeof(param));
    /* Validity checks */
    if (0 >= classId)
    {
          CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorClassAttributeArrayCreate Invalid Attribute Type"));
          return CL_COR_SET_RC(CL_COR_ERR_INVALID_CLASS);
    }
    if ((0 >= attrType) || ((ClInt32T)CL_COR_MAX_TYPE <= attrType) || (0 > attrId))
    {
          CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "ClassAttrCreate Invalid Attribute Type"));
          return CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_TYPE);
    }

    /* Extra checks for special attributes  */
    if (0 > arraySize) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "ClassAttrCreate Invalid upper limit"));
        return CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_VAL);
    }
    
    /* Call the RMD now */
    /* param.version    = CL_COR_VERSION_NO; */
	CL_COR_VERSION_SET(param.version);
    param.op         = COR_ATTR_OP_CREATE;
    param.classId    = classId;
    param.attrId     = attrId;
    param.attrType   = CL_COR_ARRAY_ATTR;
    param.subAttrType= attrType;
    param.min        = arraySize;
    param.max        = -1;

    COR_CALL_RMD(COR_EO_CLASS_ATTR_OP,
                 VDECL_VER(clXdrMarshallcorAttrIntf_t, 4, 0, 0),
                 &param,
                 size,
                 VDECL_VER(clXdrUnmarshallcorAttrIntf_t, 4, 0, 0),
                 &param,
                 &size,
                 rc);
    if (rc != CL_OK)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Attr Create failed : Class:%04x AttrId:%x", classId, attrId));

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Attr Created: Class:%04x AttrId:%x, Typ:%d ", classId, attrId, attrType));
    CL_FUNC_EXIT();
    return rc;
}

/*
 *   Utility routine for attribute create
 */
ClRcT
_corClassAttrCreate(ClCorClassTypeT  classId, 
                    ClCorAttrIdT     attrId,
                    ClCorTypeT       attrType,
                    ClCorClassTypeT  subClass,
                    ClInt32T       min,
                    ClInt32T       max)
{
    ClRcT              rc = CL_OK;
    corAttrIntf_t       param;
    ClUint32T          size =  sizeof(param);

	memset(&param, 0, sizeof(param));
    CL_FUNC_ENTER();

    /* Validity checks */
    if (0 >= classId)
    {
          CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "ClassAttrCreate Invalid Class Type"));
          return CL_COR_SET_RC(CL_COR_ERR_INVALID_CLASS);
    }

    if ((0 >= attrType) || (0 > attrId))
    {
          CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "ClassAttrCreate Invalid Attribute Type"));
          return CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_TYPE);
    }

    if ((attrType != (ClInt32T)CL_COR_CONTAINMENT_ATTR) && 
        (attrType != (ClInt32T)CL_COR_ASSOCIATION_ATTR))
    {
        if ((ClInt32T)CL_COR_MAX_TYPE <= attrType)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "ClassAttrCreate Invalid Attribute Type"));
            return CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_TYPE);
        }
    }
    /* Extra checks for special attributes  */
    if ((attrType == (ClInt32T)CL_COR_CONTAINMENT_ATTR) || (attrType == (ClInt32T)CL_COR_ASSOCIATION_ATTR))
    {
        if (0 > subClass)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "ClassAttrCreate Invalid subclass Id"));
            return CL_COR_SET_RC(CL_COR_ERR_INVALID_CLASS); 
        }
    }
 /*   if (attrType == CL_COR_ARRAY_ATTR)
    {
        if (0 > min) 
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "ClassAttrCreate Invalid upper limit"));
            return CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_VAL);
        }
    }
*/    
        
    
    /* Call the RMD now */
    /* param.version    = CL_COR_VERSION_NO; */
	CL_COR_VERSION_SET(param.version);
    param.op         = COR_ATTR_OP_CREATE;
    param.classId    = classId;
    param.attrId     = attrId;
    param.attrType   = attrType;
    param.subClassId = subClass;
    param.min        = min;
    param.max        = max;

    COR_CALL_RMD(COR_EO_CLASS_ATTR_OP,
                 VDECL_VER(clXdrMarshallcorAttrIntf_t, 4, 0, 0),
                 &param,
                 size,
                 VDECL_VER(clXdrUnmarshallcorAttrIntf_t, 4, 0, 0),
                 &param,
                 &size,
                 rc);
    if (rc != CL_OK)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Attr Create failed : Class:%04x AttrId:%x", classId, attrId));

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Attr Created: Class:%04x AttrId:%x, Typ:%d ", classId, attrId, attrType));
    CL_FUNC_EXIT();
    return (rc);
}

/**
 *  Set attribute info.
 *
 *  API to set attribute information. Captures attribute capabilities
 *  like range (minimum and maximum value the instances can take) in
 *  the class type.
 *
 *  NOTE: This API have some sideeffects in case of array /
 *  containment or association types. Should be used only for
 *  primitive data types.
 *                                                                        
 *  @param classId      class identifier 
 *  @param attrId       Attribute Identifier
 *  @param init         Initial value (default)
 *  @param min          Attr min value 
 *  @param max          Attr max value 
 *
 *  @returns 
 *    CL_OK on success <br/>
 *    CL_COR_ERR_CLASS_ATTR_NOT_PRESENT  Attribute not present <br/>
 *    CL_COR_ERR_CLASS_ATTR_INVALID_VAL Invalid values passed to set <br/>
 *
 */
ClRcT 
clCorClassAttributeValueSet(ClCorClassTypeT  classId, 
                     ClCorAttrIdT     attrId,
                     ClInt64T       init,
                     ClInt64T       min, 
                     ClInt64T       max)
{
    ClRcT              rc = CL_OK;
    corAttrIntf_t       param;
    ClUint32T          size =  sizeof(param);

    CL_FUNC_ENTER();

	memset(&param, 0, sizeof(param));
    /* Validity checks */
    if (0 >= classId)
    {
          CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "ClassAttrValueSet Invalid Class Type"));
          return CL_COR_SET_RC(CL_COR_ERR_INVALID_CLASS);
    }

    if (0 > attrId)
    {
          CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "ClassAttrValueSet Invalid Attribute Type"));
          return CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_TYPE);
    }
    /*if (min > max)
    {
          CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "ClassAttrValueSet Invalid initial values"));
          CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Class 0x %x, Attr 0x %x  Min 0x %x  Max 0x %x", classId, attrId, min, max));
          return CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_VAL);
    }*/
    /* Call the RMD now */
    /* param.version    = CL_COR_VERSION_NO; */
	CL_COR_VERSION_SET(param.version);
    param.op         = COR_ATTR_OP_VALS_SET;
    param.classId    = classId;
    param.attrId     = attrId;
    param.init       = init;
    param.min        = min;
    param.max        = max;

    COR_CALL_RMD_WITHOUT_ATMOST_ONCE(COR_EO_CLASS_ATTR_OP,
                                     VDECL_VER(clXdrMarshallcorAttrIntf_t, 4, 0, 0),
                                     &param,
                                     size, 
                                     VDECL_VER(clXdrUnmarshallcorAttrIntf_t, 4, 0, 0),
                                     &param,
                                     &size,
                                     rc);
    if (rc != CL_OK)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Attr Values set failed : Class:%04x AttrId:%x", classId, attrId));

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Attr Values set : Class:%04x AttrId:%x", classId, attrId));
    CL_FUNC_EXIT();
    return (rc);
}

/**
 *  Set user defined flags
 *
 *  
 *  Set the user defined flags for a given attribute in a class
 *                                                                        
 *  @param classHandle  Handle to class type
 *  @param attrId       Attribute Identifier
 *  @param flags         the value of flags to be set
 *
 *  @returns 
 *    CL_OK on success <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_NOT_PRESENT)  Attribute not present <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM)  Invalid values passed to set <br/>
 *
 */

ClRcT
clCorClassAttributeUserFlagsSet(ClCorClassTypeT classId, ClCorAttrIdT attrId, ClUint32T flags)
{
    ClRcT              rc = CL_OK;
    corAttrIntf_t       param;
    ClUint32T          size =  sizeof(param);

	memset(&param, 0, sizeof(param));
    CL_FUNC_ENTER();

    /* Validity checks */
    if (0 >= classId)
    {
          CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorClassAttributeUserFlagsSet: Invalid Class Type"));
          return CL_COR_SET_RC(CL_COR_ERR_INVALID_CLASS);
    }

    if (0 > attrId)
    {
          CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "ClassAttrFlagsSet Invalid Attribute Type"));
          return CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_TYPE);
    }
    /* Call the RMD now */
    /* param.version    = CL_COR_VERSION_NO; */
	CL_COR_VERSION_SET(param.version);
    param.op         = COR_ATTR_OP_FLAGS_SET;
    param.classId    = classId;
    param.attrId     = attrId;
    param.flags      = flags;

    COR_CALL_RMD_WITHOUT_ATMOST_ONCE(COR_EO_CLASS_ATTR_OP,
                                     VDECL_VER(clXdrMarshallcorAttrIntf_t, 4, 0, 0),
                                     &param,
                                     size, 
                                     VDECL_VER(clXdrUnmarshallcorAttrIntf_t, 4, 0, 0),
                                     &param,
                                     &size,
                                     rc);
    if (rc != CL_OK)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Attr Flags set : Class:%04x AttrId:%x , flags: 0x%x", classId, attrId, flags));

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Attr Flags set failed : Class:%04x AttrId:%x", classId, attrId));
    CL_FUNC_EXIT();
    return (rc);
}

/**
 *  Set class name.
 *
 *  API to set class name for a given class type.
 * 
 *  @param classHandle       Handle to class type
 *  @param name              Class Name (null terminated)
 *
 *  @returns 
 *    CL_OK on success <br/>
 *
 */
ClRcT 
clCorClassNameSet(ClCorClassTypeT   classId,
                char* name)
{
    ClRcT ret = CL_OK;

    CL_FUNC_ENTER();

    if(NULL == name )
    {
        ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    else
    {
         ret =  clCorNIClassAdd(name, classId);
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, 
              ( "ClassNameSet Class:%04x  name:%s => RET [%04x]",
               classId,
               name,
               ret));

    CL_FUNC_EXIT();
    return (ret);

}


/**
 *  Get class name.
 *
 *  API to Get class name for a given class type. Class name is copied
 *  in parameter 'name'.  Size of name is passed and when the API
 *  returns back the updated size is returned back as out param.
 * 
 *  @param classHandle       Handle to class type
 *  @param name              [Out] Class Name
 *  @param size              [In/Out] Size of name
 *
 *  @returns 
 *    CL_OK on success <br/>
 *
 */
ClRcT 
clCorClassNameGet(ClCorClassTypeT   classId,
                char             *name,
                ClUint32T       *size)
{
    ClRcT ret = CL_OK;

    CL_FUNC_ENTER();

    if( (NULL == name) || (NULL == size))
    {
        ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    else
    {
        ret = clCorNIClassNameGet(classId, name);
        *size = strlen(name);
    }

    CL_FUNC_EXIT();
    return (ret);
}

/**
 *  Get class type.
 *
 *  API to Get class type for a given class name. Class  type is copied
 *  in parameter 'classId'.
 * 
 *  @param name              [In]  Class Name
 *  @param classId           [Out] class Type
 *
 *  @returns 
 *    CL_OK on success <br/>
 *
 */
ClRcT 
clCorClassTypeFromNameGet(char             *name,
                  ClCorClassTypeT  *classId)
{
    ClRcT ret = CL_OK;

    CL_FUNC_ENTER();

    if( (NULL == name) || (NULL == classId))
    {
        ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    else
    {
        ret = clCorNIClassIdGet(name, classId);
    }

    CL_FUNC_EXIT();
    return (ret);
}



/**
 *  Set attribute name.
 *
 *  API to Set attribute name for a given class type. 
 * 
 *  @param classId           Class Identifier 
 *  @param attrID            Attribute Identifier 
 *  @param name              Name 
 *
 *  @returns 
 *    CL_OK on success <br/>
 *
 */
ClRcT 
clCorClassAttributeNameSet(ClCorClassTypeT  classId, ClCorAttrIdT attrId, char *attrName)
{
    if (attrName == NULL) 
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    return clCorNIAttrAdd(classId, attrId, attrName);
}

/**
 *  Get attribute name.
 *
 *  API to Set attribute name for a given class type. 
 * 
 *  @param classId           Class Identifier 
 *  @param attrID            Attribute Identifier 
 *  @param name              Name 
 *  @param size              [In/Out] Size of name
 *
 *  @returns 
 *    CL_OK on success <br/>
 *
 */
ClRcT 
clCorClassAttributeNameGet(ClCorClassTypeT  classId, ClCorAttrIdT attrId, char *attrName, ClUint32T* size)
{
    if ((attrName == NULL) || (size == NULL))
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    return clCorNIAttrNameGet(classId, attrId, attrName);
}

/**
 *  Delete the attribute of the given class.
 *
 *  Delete the given attribute from the class provided
 *  there are no instances created on the given class type.
 *                                                                        
 *  @param classHandle  Handle to class type
 *  @param attrId       Attribute Identifier
 *
 *  @returns 
 *    CL_OK on success <br/>
 *    CL_COR_ERR_CLASS_ATTR_PRESENT Attribute already defined <br/>
 *    CL_COR_ERR_CLASS_INSTANCES_PRESENT Class Type have instances <br/>
 *
 */
ClRcT 
clCorClassAttributeDelete(ClCorClassTypeT classId, ClCorAttrIdT attrId)
{
    ClRcT              rc = CL_OK;
    corAttrIntf_t       param;
    ClUint32T          size =  sizeof(param);

    CL_FUNC_ENTER();

    /* Validity checks */
    if (0 > attrId)
    {
          CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "ClassAttrDelete Invalid Attribute id"));
          return CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_TYPE);
    }

    if (0 >= classId)
    {
          CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "ClassAttrDelete Invalid Class id"));
          return CL_COR_SET_RC(CL_COR_ERR_INVALID_CLASS);
    }
    /* Call the RMD now */
    memset(&param, '\0',sizeof(param));
    /* param.version    = CL_COR_VERSION_NO; */
	CL_COR_VERSION_SET(param.version);
    param.op         = COR_ATTR_OP_DELETE;
    param.classId    = classId;
    param.attrId     = attrId;

    COR_CALL_RMD(COR_EO_CLASS_ATTR_OP,
                 VDECL_VER(clXdrMarshallcorAttrIntf_t, 4, 0, 0),
                 &param,
                 size,
                 VDECL_VER(clXdrUnmarshallcorAttrIntf_t, 4, 0, 0),
                 &param,
                 &size,
                 rc);
    if (rc != CL_OK)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Attr Delete failed : Class:%04x AttrId:%x", classId, attrId));

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Attr Deleted: Class:%04x AttrId:%x", classId, attrId));
    CL_FUNC_EXIT();
    return (rc);
}

ClRcT clCorClassAttrListGet(ClCharT* className,
                                ClCorAttrFlagT attrFlags,
                                ClUint32T* pAttrCount,
                                ClCorAttrDefT** pAttrDefList)
{
    ClRcT rc = CL_OK;
    ClCorClassTypeT classId = 0;

    rc = clCorNIClassIdGet(className, &classId);
    if (rc != CL_OK)
    {
        clLogError("COR", "ATTRLIST", "Failed to get the class Id from class name [%s]. rc [0x%x]", className, rc);
        return rc;
    }

    rc = _corClassAttributeListGet(classId, attrFlags, pAttrCount, pAttrDefList);
    if (rc != CL_OK)
    {
        clLogError("COR", "ATTRLIST", "Failed to get the attribute list for class Id [%d]. rc [0x%x]", classId, rc);
        return rc;
    }

    return CL_OK;
}

ClRcT _corClassAttributeListGet(ClCorClassTypeT classId,
                                    ClCorAttrFlagT attrFlags,
                                    ClUint32T* pAttrCount,
                                    ClCorAttrDefT** pAttrDefList)
{
    ClRcT rc = CL_OK;
    corAttrIntf_t   attrIntf = {{0}};
    VDECL_VER(ClCorAttrDefIDLT, 4, 1, 0) attrDefIDL = {0};
    ClBufferHandleT inMsgHdl = 0;
    ClBufferHandleT outMsgHdl = 0;

    if (classId == 0)
    {
        clLogError("COR", "ATTRLIST", "Invalid class Id [0x%x] passed.", classId);
        return CL_COR_SET_RC(CL_COR_ERR_INVALID_CLASS);
    }

    if (!pAttrCount || !pAttrDefList)
    {
        clLogError("COR", "ATTRLIST", "NULL pointer passed.");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    CL_COR_VERSION_SET(attrIntf.version);
    attrIntf.op      = COR_ATTR_OP_ID_LIST_GET;
    attrIntf.classId = classId;
    attrIntf.flags      = attrFlags;
    
    rc = clBufferCreate(&inMsgHdl);
    if (rc != CL_OK)
    {
        clLogError("COR", "ATTRLIST", "Buffer creation failed. rc [0x%x]", rc);
        return rc; 
    }

    rc = clBufferCreate(&outMsgHdl);
    if (rc != CL_OK)
    {
        clLogError("COR", "ATTRLIST", "Buffer creation failed. rc [0x%x]", rc);
        return rc; 
    }
    
    rc = VDECL_VER(clXdrMarshallcorAttrIntf_t, 4, 0, 0)((void *) &attrIntf, inMsgHdl, 0);
    if (rc != CL_OK)
    {
        clLogError("COR", "ATTRLIST", "Failed to marshall corAttrIntf_t. rc [0x%x]", rc);
        goto exit;
    }

    COR_CALL_RMD_SYNC_WITH_MSG(COR_EO_CLASS_ATTR_OP, inMsgHdl, outMsgHdl, rc);

    if (rc != CL_OK)
    {
        clLogError("COR", "ATTRLIST", "RMD to COR server failed. rc [0x%x]", rc);
        goto exit;
    }

    rc = VDECL_VER(clXdrUnmarshallClCorAttrDefIDLT, 4, 1, 0) (outMsgHdl, &attrDefIDL);
    if (rc != CL_OK)
    {
        clLogError("COR", "ATTRLIST", "Failed to unmarshall ClCorAttDefIDLT. rc [0x%x]", rc);
        goto exit;
    }

    *pAttrCount = attrDefIDL.numEntries;
    *pAttrDefList = attrDefIDL.pAttrDef;

exit:    
    clBufferDelete(&inMsgHdl);
    clBufferDelete(&outMsgHdl);

    return rc;
}

/** 
 *     Walk thru all the attribute of a class
 * 
 *  @param clsHdl    Handle of the class which contains this attrId
 *  @param usrClBck  User callback 
 *  @param cookie    Optional user data
 *
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_ERR_NULL_PTR on null parameter.
 *
 * 
 */

ClRcT  clCorClassAttributeWalk(ClCorClassTypeT      classId,
                                ClCorClassAttrWalkFunc  usrCallback, 
                                ClPtrT          cookie)
{
    ClRcT              rc = CL_OK;
    corAttrIntf_t      param;
    ClCorAttrDefT      attrDef;
    ClUint32T          size =  sizeof(param);
    ClUint32T          sizeAttrDef =  sizeof(ClCorAttrDefT);

    CL_FUNC_ENTER();
    if (usrCallback == NULL)
    {
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    if (classId == 0) 
    {
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_INVALID_CLASS));
    }
    /*
       Call one RMD to get an attribute info. Call this as many times as the number of attributes.
       Too many RMDs. An alternative would be to get the packed class and call the callback using that.
       Disadvantage of pack & unpack approach
          - Class can be modified once the unpack is done. So the caller might not see the latest class
          - Most probably the call back routine is designed to do some more operations on the attribute
            which any way call another RMD. That cant be avoided.

       Disadvantages of Multiple RMDs
          - Performance hit. But thats how life is with MP.
            So the callers must consider the performance issues. 
       
    */

    /* Call the RMD now */
    memset(&param, '\0',sizeof(param));
    param.attrId     = CL_COR_UNKNOWN_ATTRIB;

    do 
    {
        /* param.version    = CL_COR_VERSION_NO; */
		CL_COR_VERSION_SET(param.version);
        param.op         = COR_ATTR_OP_ID_GET_NEXT;
        param.classId    = classId;

        COR_CALL_RMD(COR_EO_CLASS_ATTR_OP,
                     VDECL_VER(clXdrMarshallcorAttrIntf_t, 4, 0, 0),
                     &param,
                     size, 
                     VDECL_VER(clXdrUnmarshallClCorAttrDefT, 4, 0, 0),
                     &attrDef,
                     &sizeAttrDef,
                     rc);
        if ((CL_OK != rc) && (CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_TILL_REACHED) != rc))
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Attribute Walk:failed : Class:%04x ", classId));
            return rc;
        }

        if (CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_TILL_REACHED) == rc)
        {
            return CL_OK;
        }
        param.attrId = attrDef.attrId;     
  
        if (CL_OK != (rc = usrCallback(param.classId, &attrDef, cookie)))
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Attribute Walk: Usr callback failed  Class:%04x", classId));
            return rc;
        }
    } while (CL_OK == rc);

    CL_FUNC_EXIT();
    return rc;
}

/** 
 *    API to get the minimum, maximum and default values of a given attribute 
 * 
 *  @param clsHdl    Handle of the class which contains this attrId
 *  @param attrId    attribute ID 
 *  @param pInit     Pointer to an integer which will be filled with minimum value.
 *  @param pMin      Pointer to an integer which will be filled with minimum value.
 *  @param pMax      Pointer to an integer which will be filled with minimum value.
 *
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_ERR_NULL_PTR on null parameter.
 *     CL_COR_ERR_CLASS_ATTR_NOT_PRESENT if the attribute is not present
 * 
 */
ClRcT  clCorClassAttributeValuesGet(ClCorClassTypeT     classId, 
                                     ClCorAttrIdT        attrId,
                                     ClInt32T          *pInit,
                                     ClInt32T          *pMin,
                                     ClInt32T          *pMax)
{
    ClRcT              rc = CL_OK;
    corAttrIntf_t       param;
    ClUint32T          size =  sizeof(param);

    CL_FUNC_ENTER();

    /* Validity checks */
    if ((classId == 0) || (pInit == NULL) ||
        (pMin == NULL) || (pMax == NULL))
    {
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    if (classId <= 0)
    {
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_INVALID_CLASS));
    }
    /* Call the RMD now */
    memset(&param, '\0',sizeof(param));
    /* param.version    = CL_COR_VERSION_NO; */
	CL_COR_VERSION_SET(param.version);
    param.op         = COR_ATTR_OP_VALS_GET;
    param.classId    = classId;
    param.attrId     = attrId;

    COR_CALL_RMD_WITHOUT_ATMOST_ONCE(COR_EO_CLASS_ATTR_OP,
                                     VDECL_VER(clXdrMarshallcorAttrIntf_t, 4, 0, 0),
                                     &param,
                                     size, 
                                     VDECL_VER(clXdrUnmarshallcorAttrIntf_t, 4, 0, 0),
                                     &param,
                                     &size,
                                     rc);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Class Attr Value Get failed : Class:%04x AttrId:%x", classId, attrId));
    }
    else
    {
        *pInit = param.init;
        *pMin  = param.min;
        *pMax  = param.max;
    }
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Class Attr Value Get : Class:%04x AttrId:0%x, Init:0%x, Min:0%x, Max: 0%x ",
		     classId, attrId, *pInit, *pMin, *pMax));
    CL_FUNC_EXIT();
    return rc;
}

/**
 *  Get user defined flags
 *
 *  
 *  Get the user defined flags for a given attribute in a class
 *                                                                        
 *  @param classHandle  Handle to class type
 *  @param attrId       Attribute Identifier
 *  @param flags         [Out]the value of flags to be get
 *
 *  @returns 
 *    CL_OK on success <br/>
 *    CL_COR_ERR_CLASS_ATTR_NOT_PRESENT  Attribute not present <br/>
 *    CL_COR_ERR_CLASS_ATTR_INVALID_VAL Invalid values passed to set <br/>
 *
 */

ClRcT clCorClassAttributeUserFlagsGet(ClCorClassTypeT classId, ClCorAttrIdT attrId, ClUint32T* pFlags)
{
    ClRcT              rc = CL_OK;
    corAttrIntf_t       param;
    ClUint32T          size =  sizeof(param);

    CL_FUNC_ENTER();

    /* Validity checks */
    if (0 > attrId)
    {
          CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorClassAttributeUserFlagsGet Invalid Attribute id"));
          return CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_TYPE);
    }

    if (NULL == pFlags)
    {
          CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( " clCorClassAttributeUserFlagsGet : Null Pointer"));
          return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    if (classId <= 0)
    {
        CL_FUNC_EXIT();
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorClassAttributeUserFlagsGet Invalid class id"));
        return (CL_COR_SET_RC(CL_COR_ERR_INVALID_CLASS));
    }
    /* Call the RMD now */
    memset(&param, '\0',sizeof(param));
    /* param.version    = CL_COR_VERSION_NO; */
	CL_COR_VERSION_SET(param.version);
    param.op         = COR_ATTR_OP_FLAGS_GET;
    param.classId    = classId;
    param.attrId     = attrId;

    COR_CALL_RMD_WITHOUT_ATMOST_ONCE(COR_EO_CLASS_ATTR_OP,
                                     VDECL_VER(clXdrMarshallcorAttrIntf_t, 4, 0, 0),
                                     &param,
                                     size, 
                                     VDECL_VER(clXdrUnmarshallcorAttrIntf_t, 4, 0, 0),
                                     &param,
                                     &size,
                                     rc);
    if (rc != CL_OK)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Class Attr User Flags Get failed : Class:%04x AttrId:%x", classId, attrId));
    else *pFlags = param.flags;

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Class Attr User Flags Get: Class:%04x AttrId:%x", classId, attrId));
    CL_FUNC_EXIT();
    return (rc);
}

/**
 *  Get attribute type
 *
 *  
 *  Get the type of attribute
 *                                                                        
 *  @param classId      class type
 *  @param attrId       Attribute Identifier
 *  @param AttrType     [Out]the value of the attribute type
 *
 *  @returns 
 *    CL_OK on success <br/>
 *    CL_COR_ERR_CLASS_ATTR_NOT_PRESENT  Attribute not present <br/>
 *
 */

ClRcT clCorClassAttributeTypeGet(ClCorClassTypeT classId, ClCorAttrIdT attrId, ClCorAttrTypeT  *pAttrType)
{
    ClRcT              rc = CL_OK;
    corAttrIntf_t       param;
    ClUint32T          size =  sizeof(param);

    CL_FUNC_ENTER();

    /* Validity checks */
    if (0 > attrId)
    {
          CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "ClassAttrDelete Invalid Attribute id"));
          return CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_TYPE);
    }

    if (NULL == pAttrType)
    {
          CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "ClassAttrDelete Invalid Attribute id"));
          return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    if (0 >= classId)
    {
          CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "ClassAttrDelete Invalid Attribute id"));
          return CL_COR_SET_RC(CL_COR_ERR_INVALID_CLASS);
    }
    /* Call the RMD now */
    memset(&param, '\0',sizeof(param));
    /* param.version    = CL_COR_VERSION_NO; */
	CL_COR_VERSION_SET(param.version);
    param.op         = COR_ATTR_OP_TYPE_GET;
    param.classId    = classId;
    param.attrId     = attrId;

    COR_CALL_RMD_WITHOUT_ATMOST_ONCE(COR_EO_CLASS_ATTR_OP,
                                     VDECL_VER(clXdrMarshallcorAttrIntf_t, 4, 0, 0),
                                     &param,
                                     size, 
                                     VDECL_VER(clXdrUnmarshallcorAttrIntf_t, 4, 0, 0),
                                     &param,
                                     &size,
                                     rc);
    if (rc != CL_OK)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorClassAttributeTypeGet: Class:%04x AttrId:%x", classId, attrId));
    else *pAttrType = param.attrType;

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "clCorClassAttributeTypeGet: Class:%04x AttrId:%x", classId, attrId));
    CL_FUNC_EXIT();
    return (rc);
}

/*
 *     Add a name to Class name table
 */
ClRcT clCorNIClassAdd(char *name, ClCorClassTypeT key)
{
    ClRcT   rc = CL_OK;
    if (NULL == name) 
      return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    ClIocNodeAddressT   sdAddr = 0;
    corNiOpInf_t     entry;
    ClUint32T          size =  sizeof(corNiOpInf_t);
    memset(&entry, '\0', size);
    CL_COR_VERSION_SET(entry.version);
    entry.classId = key;
    entry.op = COR_NI_OP_CLASS_NAME_SET;
    strcpy(entry.name, name);
    CL_CPM_MASTER_ADDRESS_GET(&sdAddr );
    COR_CALL_RMD_WITH_DEST(sdAddr,
                               COR_EO_NI_OP,
                               VDECL_VER(clXdrMarshallcorNiOpInf_t, 4, 0, 0),
                               (ClUint8T *)&entry,
                               size,
                               VDECL_VER(clXdrUnmarshallcorNiOpInf_t, 4, 0, 0),
                               NULL,
                               NULL,
                               rc);
#if 0
        rc  = !!!OBSOLETErmdCallPayloadReturn!!!(sdAddr, CL_IOC_COR_PORT, COR_EO_NI_CLASS_ADD, 
                                   (char *)&entry, size, NULL, NULL,  COR_RMD_DFLT_TIME_OUT , 0, 0);
#endif
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nRMD Failed  [rc x%x]\n ", rc));
        }
    return rc;
}

/*
 *     Add an attribute name to name table
 */
ClRcT clCorNIAttrAdd(ClCorClassTypeT  classId, ClCorAttrIdT  attrId, char *name)
{
    ClRcT   rc = CL_OK;
    if (NULL == name) 
      return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    ClIocNodeAddressT   sdAddr = 0;
    corNiOpInf_t        entry;
    ClUint32T          size =  sizeof(corNiOpInf_t);
    memset(&entry, '\0', size);
    CL_COR_VERSION_SET(entry.version);
    entry.classId = classId;
    entry.attrId  = attrId;
    entry.op = COR_NI_OP_ATTR_NAME_SET;
    strcpy(entry.name, name);
    CL_CPM_MASTER_ADDRESS_GET(&sdAddr );
    COR_CALL_RMD_WITH_DEST(sdAddr,
                               COR_EO_NI_OP,
                               VDECL_VER(clXdrMarshallcorNiOpInf_t, 4, 0, 0),
                               (ClUint8T *)&entry,
                               size, 
                               VDECL_VER(clXdrUnmarshallcorNiOpInf_t, 4, 0, 0),
                               NULL,
                               NULL,
                               rc);
#if 0
        rc  = !!!OBSOLETErmdCallPayloadReturn!!!(sdAddr, CL_IOC_COR_PORT, COR_EO_NI_ATTR_ADD, 
                                   (char *)&entry, size, NULL, NULL,  COR_RMD_DFLT_TIME_OUT , 0, 0);
#endif
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nRMD Failed  [rc x%x]\n ", rc));
        }
    return rc;
}

/*
 *     Delete a name from Class name table
 */
ClRcT clCorNIClassDelete(ClCorClassTypeT classId)
{
    ClRcT   rc = CL_OK;

        ClIocNodeAddressT   sdAddr = 0;
        corNiOpInf_t     entry;
        ClUint32T          size =  sizeof(corNiOpInf_t);

        memset(&entry, '\0', size);
		CL_COR_VERSION_SET(entry.version);
        entry.classId = classId;
        entry.op      = COR_NI_OP_CLASS_NAME_DELETE;
        CL_CPM_MASTER_ADDRESS_GET(&sdAddr );
        
        COR_CALL_RMD_WITH_DEST(sdAddr, COR_EO_NI_OP,
                               VDECL_VER(clXdrMarshallcorNiOpInf_t, 4, 0, 0),
                               (ClUint8T *)&entry,
                               size, 
                               VDECL_VER(clXdrUnmarshallcorNiOpInf_t, 4, 0, 0),
                               NULL,
                               NULL,
                               rc);

        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nRMD Failed  [rc x%x]\n ", rc));
        }
    return rc;
}

/*
 *     Delete an attribute  name from name table
 */
ClRcT clCorNIAttrDel(ClCorClassTypeT classId, ClCorAttrIdT  attrId)
{
    ClRcT   rc = CL_OK;

        ClIocNodeAddressT  sdAddr = 0;
        corNiOpInf_t       entry;
        ClUint32T          size =  sizeof(corNiOpInf_t);


        memset(&entry, '\0', size);
		CL_COR_VERSION_SET(entry.version);
        entry.classId = classId;
        entry.attrId  = attrId;
        entry.op      = COR_NI_OP_ATTR_NAME_DELETE;
        CL_CPM_MASTER_ADDRESS_GET(&sdAddr );

        COR_CALL_RMD_WITH_DEST(sdAddr,
                               COR_EO_NI_OP,
                               VDECL_VER(clXdrMarshallcorNiOpInf_t, 4, 0, 0),
                               (ClUint8T *)&entry,
                               size, 
                               VDECL_VER(clXdrUnmarshallcorNiOpInf_t, 4, 0, 0),
                               NULL,
                               NULL,
                               rc);

#if 0
        rc  = !!!OBSOLETErmdCallPayloadReturn!!!(sdAddr, CL_IOC_COR_PORT, COR_EO_NI_ATTR_DEL, 
                                   (char *)&attr, size, NULL, NULL,  COR_RMD_DFLT_TIME_OUT , 0, 0);
#endif
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nRMD Failed  [rc x%x]\n ", rc));
        }
    return rc;
}

/*
 *     Get class Id from Class name table
 */
ClRcT clCorNIClassIdGet(char *name, ClCorClassTypeT *pClassId)
{
    ClRcT   rc = CL_OK;

    if ((NULL == name)||(NULL ==pClassId))
      return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);

    ClIocNodeAddressT   sdAddr = 0;
    corNiOpInf_t        entry;
    ClUint32T          size =  sizeof(ClCorClassTypeT);

    memset(&entry, '\0', sizeof(corNiOpInf_t));
    strcpy(entry.name, name);
    CL_COR_VERSION_SET(entry.version);
    entry.op = COR_NI_OP_CLASS_ID_GET;
    CL_CPM_MASTER_ADDRESS_GET(&sdAddr );
    COR_CALL_RMD_WITH_DEST(sdAddr,
                           COR_EO_NI_OP,
                           VDECL_VER(clXdrMarshallcorNiOpInf_t, 4, 0, 0),
                           (ClUint8T *)&entry,
                           sizeof(corNiOpInf_t),
                           VDECL_VER(clXdrUnmarshallcorNiOpInf_t, 4, 0, 0),
                           (ClUint8T *)&entry,
                           &size,
                           rc);
#if 0
    rc  = !!!OBSOLETErmdCallPayloadReturn!!!(sdAddr, CL_IOC_COR_PORT, COR_EO_NI_CLASS_ID_GET, 
                               (char *)&entry, sizeof(corNiOpInf_t), (char *)&entry.classId, &size,  COR_RMD_DFLT_TIME_OUT , 0, 0);
#endif
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\nRMD Failed  [rc x%x]\n ", rc));
    }
    else
    {
        *pClassId = entry.classId;
    }

    return rc;
}

/*
 *     Get class name from Class name table
 */
ClRcT clCorNIClassNameGet(ClCorClassTypeT key, char *name )
{
    ClRcT   rc = CL_OK;
    if (NULL == name) 
      return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
 
    ClIocNodeAddressT   sdAddr = 0;
    corNiOpInf_t        entry;
    ClUint32T           size =  sizeof(entry.name);

    memset(&entry, '\0', sizeof(corNiOpInf_t));
    CL_COR_VERSION_SET(entry.version);
    entry.classId = key;
    entry.op = COR_NI_OP_CLASS_NAME_GET;
    CL_CPM_MASTER_ADDRESS_GET(&sdAddr );
    COR_CALL_RMD_WITH_DEST(sdAddr,
                           COR_EO_NI_OP,
                           VDECL_VER(clXdrMarshallcorNiOpInf_t, 4, 0, 0),
                           (ClUint8T *)&entry,
                           sizeof(corNiOpInf_t),
                           VDECL_VER(clXdrUnmarshallcorNiOpInf_t, 4, 0, 0),
                           (ClUint8T *)&entry,
                           &size,
                           rc);
#if 0
    rc  = !!!OBSOLETErmdCallPayloadReturn!!!(sdAddr, CL_IOC_COR_PORT, COR_EO_NI_CLASS_NAME_GET, 
                     (char *)&entry, sizeof(corNiOpInf_t), (char *)entry.name, &size,  COR_RMD_DFLT_TIME_OUT , 0, 0);
#endif
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\nRMD Failed  [rc x%x]\n ", rc));
    }
    else
    {
        strcpy(name, entry.name);
    }
		
    return rc;
}


/*
 *     Get attribute Id from name table
 */
ClRcT clCorNIAttrIdGet(ClCorClassTypeT classId, char *name,  ClCorAttrIdT  *attrId)
{
    ClRcT   rc = CL_OK;
    if (NULL == name) 
      return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    ClIocNodeAddressT   sdAddr = 0;
    corNiOpInf_t        entry;
    ClUint32T          size =  sizeof(ClCorAttrIdT);
    memset(&entry, '\0', sizeof(corNiOpInf_t));
    CL_COR_VERSION_SET(entry.version);
    entry.classId = classId;
    entry.op = COR_NI_OP_ATTR_ID_GET;
    strcpy(entry.name, name);
    CL_CPM_MASTER_ADDRESS_GET(&sdAddr );

    COR_CALL_RMD_WITH_DEST(sdAddr,
                               COR_EO_NI_OP,
                               VDECL_VER(clXdrMarshallcorNiOpInf_t, 4, 0, 0),
                               (ClUint8T *)&entry,
                               sizeof(corNiOpInf_t),
                               VDECL_VER(clXdrUnmarshallcorNiOpInf_t, 4, 0, 0),
                               (ClUint8T *)&entry,
                               &size,
                               rc);
#if 0
        rc  = !!!OBSOLETErmdCallPayloadReturn!!!(sdAddr, CL_IOC_COR_PORT, COR_EO_NI_ATTR_NAME_GET, 
                                   (char *)&entry, sizeof(corNiOpInf_t), (char *)&entry.attrId, &size,  COR_RMD_DFLT_TIME_OUT , 0, 0);
#endif
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\nRMD Failed  [rc x%x]\n ", rc));
        }
        *attrId = entry.attrId;
    return rc;
}


/*
 *     Get attribute name from name table
 */
ClRcT clCorNIAttrNameGet(ClCorClassTypeT classId, ClCorAttrIdT  attrId, char *name )
{
    ClRcT   rc = CL_OK;
    if (NULL == name) 
      return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    ClIocNodeAddressT   sdAddr = 0;
    corNiOpInf_t        entry;
    ClUint32T          size =  sizeof(entry.name);

    memset(&entry, '\0', sizeof(corNiOpInf_t));
    CL_COR_VERSION_SET(entry.version);
    entry.classId = classId;
    entry.attrId = attrId;
    entry.op = COR_NI_OP_ATTR_NAME_GET;
    CL_CPM_MASTER_ADDRESS_GET(&sdAddr );

    COR_CALL_RMD_WITH_DEST(sdAddr, COR_EO_NI_OP,
                           VDECL_VER(clXdrMarshallcorNiOpInf_t, 4, 0, 0),
                           (ClUint8T *)&entry,
                           sizeof(corNiOpInf_t),
                           VDECL_VER(clXdrUnmarshallcorNiOpInf_t, 4, 0, 0),
                           (ClUint8T *)&entry,
                           &size,
                           rc);
#if 0
    rc  = !!!OBSOLETErmdCallPayloadReturn!!!(sdAddr, CL_IOC_COR_PORT, COR_EO_NI_ATTR_NAME_GET, 
                               (char *)&entry, sizeof(corNiOpInf_t), (char *)entry.name, &size,  COR_RMD_DFLT_TIME_OUT , 0, 0);
#endif
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\nRMD Failed  [rc x%x]\n ", rc));
    }
    else
    {
        strcpy(name, entry.name);
    }

    return rc;
}

/**
 * MIB Table related APIs.
 * To be moved into separate .c file.
 */
ClRcT clCorMibTableAttrListGet(ClCharT* mibTable,
                                ClCorAttrFlagT attrFlags,
                                ClUint32T* pAttrCount,
                                ClCorAttrDefT** pAttrDefList)
{
    ClRcT rc = CL_OK;
    ClCharT tableName[256] = {0};
    ClCorClassTypeT classId = 0;

    sprintf(tableName, "CLASS_%s_PROV_MSO", mibTable); 

    rc = clCorNIClassIdGet(tableName, &classId);
    if (rc != CL_OK)
    {
        clLogError("COR", "ATTRLIST", "Failed to get the class Id from table name [%s]. rc [0x%x]", tableName, rc);
        return rc;
    }

    rc = _corClassAttributeListGet(classId, attrFlags, pAttrCount, pAttrDefList);
    if (rc != CL_OK)
    {
        clLogError("COR", "ATTRLIST", "Failed to get the attribute list for class Id [%d]. rc [0x%x]", classId, rc);
        return rc;
    }

    return CL_OK;
}
