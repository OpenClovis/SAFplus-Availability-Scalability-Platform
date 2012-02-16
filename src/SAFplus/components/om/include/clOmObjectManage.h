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
 * ModuleName  : om
 * File        : clOmObjectManage.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This module contains the definitions for the Base Object. 
 *              Also, this file  contains the function prototypes of the interfaces
 *              that are exported.     
 *
 *****************************************************************************/
#ifndef _CL_OM_OBJECT_MANAGE_H_
#define _CL_OM_OBJECT_MANAGE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>

/* Class Inheritance macros */

/** @pkg cl.om */
/**
 * Object manager Library
 *
 *
 * <h1>Overview</h1>
 * 
 * The Object Manager(OM) library allows user to create classes and define
 *  inheritance relationship between them. Also it allows some of the features
 * supported by C++ programming language such as class inheritance 
 * and virtual function definitions. It also allows users to create instances
 * of these classes and maintain them. 
 *
 * The class defined using OM, contains data members as well as functions
 * just like C++ with few differences. All the member functions in an OM class
 * are virtual functions (something like pure virtual function). Non virtual functions are not part of the OM class
 * and is captured in a seperate structure. Also the class contains only function
 * pointers and these function pointers are assigned with appropriate functions
 * in a class constructor.
 *
 * An object created using OM library is associated with an MO or MSO object.
 * This association is also captured by the OM library. It maintains mapping
 * between OM ID and MOID.
 *
 *<b>Why would user use Object Manager when COR already supports data 
 * storing and inheritance ? </b>
 *
 * The OM can be used by the application to store its private data. The COR
 * stores data which may be persistent and which is managed by user. While
 * application (like provisioning, alarm, HA etc) may need some private data to
 * be stored and manipulated. Such data can be stored using simple structures
 * by the applications. But in that case it can not be extended easily. OM
 * allows user to extend the functionality provided by the application easily.
 *
 * Following section explains how user can use OM to capture private data and 
 * define virtual functions which can be implemented by the users.
 *
 *  <h2>Object Manager Usage scenarios</h2>       
 *
 * Let's take an example of provisioning service and explain how OM is used by
 * the provisioning.
 *
 * <h3>Class Inheritance and virtual Functions</h3>
 *
 * There are two macros CL_OM_BEGIN_CLASS and CL_OM_END which are used to define classes &
 * class inheritance. These macros define a new class with the name passed 
 * as second parameter. It includes parent structure into this class as base. 
 * There is no restriction on types of members that can be included as part of class definition. 
 * The virtual functions have to be defined as a member of the class using these macros.
 *
 * Following code explains how CL_OM_BEGIN_CLASS and CL_OM_END macros can be 
 * used to create class inheritance and virtual function definitions.
 *
 *CL_OM_BEGIN_CLASS(CimBaseDataClass, provClass)<br>
 *	ClInt8T 	_szProvString[80];<br>
 *	ClUint8T	_provFuncTblSize;<br>
 *         ClHalObjHandleT 	hProvHalObj;<br>
 *<br>
 *	//virtual method function pointers <br>
 *	ClRcT (**fpProvFuncTbl)(provClass* pThis, <br>
 *		ClUint8T provCmd, void *hMoId, void *pInProvData);<br>
 *	ifp	fpProvDataGet;<br>
 *	ifp	fpProvDataSemChk;<br>
 *	fpProvRefreshData_t fpProvRefresh;<br>
 *CL_OM_END<br>
 *
 * As can seen by this example, OM provides a nice way to extend CimBaseDataClass. the
 * above code shall create a new structure called provClass and include struct CimBaseDataClass
 * as its first member.
 *
 * As can be seen, all data except managed data is stored in the provClass. You certainly do not
 * want to put pointer to HAL object in COR. 
 *
 * All the function pointers defined in this class are virtual functions which can be overriden by
 * applications. Let's take an example of function pointer "fpProvDataSemChk" and explain
 * how it works. The intention of this member function is to check semantics of the attributes
 * which are provisioned by the user. Now different applications may have different rules to
 * check for the semantics which ASP do not know. What ASP knows is when this semantic checking has
 * to be done. And depending on application ASP wants to call an application specific semCheck
 * function. Let's look at some of the following code pieces to see how it is done.
 *
 * The code that calls "fpProvDataSemChk" in ASP:-
 *
 * ClRcT provClassProvDataSemChk(provClass* pThis, ClUint8T provCmd,<br>
 *		void *pInProvData, ClUint32T *pInLen)<br>
 *{<br>
 * 	------<br>
 *	------<br>
 *		if (pThis->fpProvDataSemChk != NULL)<br>
 *		{<br>
 *		// Call the instantiated virtual method <br>
 *		rc = pThis->fpProvDataSemChk(pThis, provCmd, pInProvData, pInLen);<br>
 *		}<br>
 *	else<br>
 *		{<br>
 *		// Do not report error if there is no non-virtual method defined <br>
 *		rc = CL_OK;<br>
 *		CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("%s: 'semantic check' method is not "<br>
 *			"implemented, rc 0x%x!!!\r\n", aFuncName, rc));<br>
 *		}<br>
 *	------<br>
 *}<br>
 *
 * In this example, depending on passed provClass pointer (pThis), appropriate SemChk
 * function is called.
 *
 * Different applications can write their own SemCheck function and assign it to this function
 * pointer. Following piece of code shows how OSPF object uses the prov class.
 *
 *CL_OM_BEGIN_CLASS(provClass, provOSPFExportClass)<br>
 *    ClUint32T            tSst;            // Service state<br>
 *    ospfConfAttr_t            ospfConfAttrs;<br>
 *CL_OM_END<br>
 *
 * The provOSPFExportClass is derived from provClass and two more members are added to it.
 *
 *CL_OM_BEGIN_CLASS(provOSPFExportClass, provOSPFClass)<br>
 *    provOSPFData_t      tProvOSPFData;<br>
 *CL_OM_END<br>
 *
 * The provOSPFClass is derived from provOSPFExportClass. The constructor of this
 * class assigns OSPF SemCheck function to fpProvDataSemChk function pointer.
 *
 *ClRcT provOSPFClass_constructor(provOSPFClass* pThis, <br>
 *        void *pUsrData, ClUint32T usrDataLen)<br>
 *{<br>
 * 	------<br>
 *	------<br>
 *((provClass *)pThis)->fpProvDataSemChk = (ifp)provOSPFClass_fSemCheck;<br>
 *	------<br>
 *}<br>
 *
 * Now when an object of type provOSPFClass is passed to the function provClassProvDataSemChk,
 * the function provOSPFClass_fSemCheck will be invoked.
 *
 * Please Note that the virtual functions defined in the base class are like pure virtual functions in C++.
 * Meaning that they have to be implemented in all the derived classes. Else they will be pointing to 
 * NULL.
 *
 *<h3>Defining non virtual methods for a class</h3>
 * 
 * The non-virtual functions within a class can be defined using another set of macros, 
 * CL_OM_BEGIN_METHOD and CL_OM_INIT_METHOD_TABLE. The CL_OM_BEGIN_METHOD
 * macro creates a structure of function pointers and CL_OM_INIT_METHOD_TABLE
 * assigns functions to the function pointers. These functions are also like static or
 * class functions in C++. Meaning you need not have to have an object to be created to
 * call these functions. While for the virtual functions in above example, an object has to
 * be created since the function pointers are initialized during constructor call.
 *
 * Let's continue with the example of provisioning service and illustrate how these two
 * macros are being used
 *
 * CL_OM_BEGIN_METHOD(provClass)<br>
 *	// External APIs<br>
 *	ClRcT (*fpProvDataSet)(provClass* pThis, <br>
 *		ClUint8T provCmd, void *hMoId, void *pInProvData);<br>
 *	ifp	fpProvDataGet;<br>
 *	ifp	fpProvDataSemChk;<br>
 *
 *	ClRcT (*fpProvStringGet)(provClass* pThis, ClInt8T *szProvString);<br>
 *	ClRcT (*fpProvDataShow)(provClass* pThis);<br>
 *	ClRcT (*fpFuncInstall)(provClass* pThis, fpProvFunc fpFunc,<br>
 *		ClUint8T funcIdx); <br>
 *	ClRcT (*fpFuncTblInstall)(provClass* pThis, fpProvFunc *pProvFuncTbl,<br>
 *		ClUint8T numOfFuncs); <br>
 *	fpCpyAttribToOmObj	fpAttribCpy;<br>
 *CL_OM_END<br>
 *
 * The above piece of code creates a structure named provClassMethods. This is a
 * structure of function pointers and the corresponding functions are defined by
 * using another macro CL_OM_INIT_METHOD_TABLE.
 *
 * CL_OM_INIT_METHOD_TABLE(provClass)<br>
 *    provClassProvDataSet,<br>
 *    (ifp)provClassProvDataGet,<br>
 *    (ifp)provClassProvDataSemChk,<br>
 *    provClassProvStringGet,<br>
 *    provClassProvDataShow,<br>
 *    provClassFuncInstall,<br>
 *    provClassFuncTblInstall,<br>
 *    provClassAttribCpy,<br>
 *CL_OM_END<br>
 *
 *The above piece of code will create a structure provClassMethodsMapping of type
 * provClassMethods.
 *
 *If a function corresponding to the function pointer is to be called, then another macro
 * clOmCALL is to be used. Following is sample code illustrating how a non-virtual function
 * is invoked using clOmCALL macro. Let's take an example of function "fpProvDataSet".
 *
 *	rc = clOmCALL(provClass, <br>
 *		fpProvDataSet((provClass *)pThis, DELETEREQ, NULL, pUsrData));<br>
 *
 * In order for this call to be successful, the mapping of functions to function pointers
 * has to be done. In our example of provOSPFClass above, following is how the mapping 
 * is done.
 *
 * The assignment of functions in the structure created by CL_OM_INIT_METHOD_TABLE to the
 * function pointers is to be done in another table called xxxOmClassTbl. there is one table 
 * defined in ASP for ASP OM classes - gCmnOmClassTbl. For the classes defined by applications
 * the application has to define its own OmClassTable.
 *
 * Following piece of code illustrates how the table gCmnOmClassTbl looks like.
 *
 *
 *ClOmClassControlBlockT gCmnOmClassTbl[] = {<br>
 *<br>
 *	// Entry for the base object <br>
 *	-----<br>
 *	-----<br>
 *	// Entry for the provClass<br>
 *	{<br>
 *	  PROV_CLASS_NAME,           // provClass name <br>
 *	  sizeof(provClass),        // size <br>
 *	  CL_OM_CIM_BASE_DATA_CLASS,    // extends from <br>
 *	   provClassConstructor,    // constructor <br>
 *	  provClassDestructor,     // destructor <br>
 *	  PROV_CLASS_METHOD_TABLE,   // pointer to methods struct <br>
 *	  PROV_CLASS_VERSION,		// version <br>
 *	  0,						// Instance table ptr <br>
 *	  MAX_OBJ,	// Maximum number of classes <br>
 *	  0,						// cur instance count <br>
 *	  PROV_CLASS_MAX_SLOTS,		// max slots <br>
 *	  CL_OM_PROV_CLASS			// my class type<br>
 *	},<br>
 *	----<br>
 *	----<br>
 *}<br>
 *
 * Here PROV_CLASS_METHOD_TABLE is defined as provClassMethodsMapping which is a
 * structure of type provClassMethods (the structure created using CL_OM_BEGIN_METHOD) and
 * is initialized to appropriate functions defined in CL_OM_INIT_METHOD_TABLE. 
 *
 *
 * @pkgdoc  cl.om
 * @pkgdoctid Object Manager Library.
 *
 */
#define CL_OM_BEGIN_CLASS(PARENT, CHILD)     \
        typedef struct CHILD CHILD;     \
        struct CHILD {                  \
               struct PARENT base; 

#define CL_OM_END             };

/* Macro to mimic the "new" and delete feature in C++/Java */
#define clOmNEW(X, HANDLEPTR) (struct X *)clOmObjectCreate(X##_TYPE, 1, HANDLEPTR, \
							NULL, 0)
#define clOmDELETE(HANDLE)    clOmObjectDelete(HANDLE, 1, NULL, 0)

/* Macro to invoke the virtual method for a given class */
#define clOmCALL(OBJECT, METHOD)    \
    ((OBJECT##Methods *)((clOmClassEntryGet(OBJECT##_TYPE))->pfpMethodsTab))->METHOD

#define clOmGET_FUNCTAB_PTR(OBJECT)    \
    ((OBJECT##Methods *)((clOmClassEntryGet(OBJECT##_TYPE))->pfpMethodsTab))

/* Method table definition macros */
#define CL_OM_BEGIN_METHOD(OBJECT) typedef struct OBJECT##Methods OBJECT##Methods; \
                              struct OBJECT##Methods {

/* Method table initialization macros */
#define CL_OM_INIT_METHOD_TABLE(OBJECT) struct OBJECT##Methods OBJECT##MethodsMapping = {

#ifdef __cplusplus
}
#endif

#endif /* _CL_OM_OBJECT_MANAGE_H_ */


