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
 * ModuleName  : dbal
 * File        : clDbalCfg.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * DBAL Component configuration module                       
 ********************************************************************************/


/********************************************************************************/
/******************************** DBAL  APIs ************************************/
/********************************************************************************/
/*                                                                              */
/*  clDbalLibInitialize           	                                            */
/*  clDbalLibFinalize           	                                            */
/*                                                                              */
/********************************************************************************/

/**
 *  \file
 *  \brief  Header file of DBAL Component Configuration APIs
 *  \ingroup dbal_apis
 */

/**
 *  \addtogroup dbal_apis
 *  \{
 */

#ifndef _DBALCOMPCFG_H_
#define _DBALCOMPCFG_H_

#ifdef __cplusplus
extern "C" {
#endif

/* GLOBALs */

/* Berkeley DB environment path */    
#define CL_DBAL_BERKELEY_ENV_PATH  ""

/* SQLite DB path */    
#define CL_DBAL_SQLITE_DB_PATH  ""

typedef struct ClDbalBerkeleyConfT {

/**
 * Path where the database files are to be stored.
 */
    ClUint8T engineEnvironmentPath[1024];
}ClDbalBerkeleyConfT;

typedef struct ClDbalSQLiteConfT {

    /* Path where the database files are to be stored. */
    ClUint8T enginePath[1024];
}ClDbalSQLiteConfT;

typedef struct ClDbalConfigT {
    ClUint32T engineType;
    union {
        ClDbalBerkeleyConfT berkeleyConfig;
        ClDbalSQLiteConfT sqliteConfig;
    }Database ;
}ClDbalConfigT;


/**
 ************************************
 *  \brief DBAL configuration module entry-point.
 *
 *  \par Header File:
 *   clDbalCfg.h
 *
 *  \par Library Files:
 *
 *  \par Parameters:
 *   None
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.  
 *
 *  \par Description:
 *  This API is Database Abstraction Layer configuration module entry-point.
 *  This is used to set the configuration parameters of the DBAL. 
 *  It initializes the Database Abstraction Layer. Once this API is executed 
 *  successfully the DBAL is ready for use. This API must be executed before 
 *  executing any other API.
 *
 *  \sa clDbalLibFinalize()
 *
 */

ClRcT clDbalLibInitialize(void);
/**
 ************************************
 *  \brief DBAL configuration module exit-point.
 *
 *  \par Header File:
 *   clDbalCfg.h
 *
 *  \par Library Files:
 *
 *  \par Parameters:
 *   None
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER If the lib is not intialized before calling
 *  this function.
 *
 *  \par Description:
 *  This API is Database Abstraction Layer configuration module exit-point.
 *  This function closes the association between the invoking process and 
 *  Database Abstraction Layer. The process must have invoked
 *  clDbalLibInitialize() before it invokes this function. If the 
 *  clDbalLibFinalize() function returns successfully, it releases all 
 *  resources acquired when clDbalLibInitialize() was called.
 *
 *  \sa clDbalLibInitialize() 
 *
 */
ClRcT clDbalLibFinalize(void);



#ifdef __cplusplus
}
#endif

#endif


/** \} */
