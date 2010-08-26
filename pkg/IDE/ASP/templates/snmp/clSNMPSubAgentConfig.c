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
 * ModuleName  : snmp
 * File        : clSNMPSubAgentConfig.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This is the template file for SNMP subAgent
 *
 *
 *****************************************************************************/
#include <clSnmpDataDefinition.h> 

/**
 *  This function handles the Instance translation for a given MOId. 
 *  Mediation library  calls this function by passing MOId and attributeId 
 *  to retrieve instance id.
 */
ClRcT sampleInstXlator (
        const struct ClMedAgentId   *pAgntId,
        ClCorMOIdPtrT               hmoId,
        ClCorAttrPathPtrT           containedPath,
        void                        **pRetInstData,
        ClUint32T                   *instLen,
        ClUint32T                   create)
{
    ClRcT       rc = CL_OK;

    /**
     *  Get Instance Id by using hmoId, containedPath
     */

    
    return rc;
}


/**
 *  Clovis container calls this function before adding an entry to container.
 *  @param key1
 *  @param key2
 *  @return 0 - duplicate, 1 - key1 is greater that key2, 2 - key1 is smaller 
 *              than key2
 */
ClInt32T  sampleInstCompare (
        ClCntKeyHandleT         key1,    
        ClCntKeyHandleT         key2)
{
    ClInt32T    compareResult = 0;

    /**
     * Compare key1 and key2.
     */

    
    return compareResult;
}

/**
 * The SNMP table Id to be defined in this enum
 */
enum CLSampleTable {
    PROV_TABLE = CLOVIS_SNMP_TABLE_MAX+1
};


/**
 * This table is declared in the clSNMPDataDefintions.h, this needs to be 
 * filled in by the user. 
 */
clOidTable_t clOidTable[]= {
    {
        "1.3.6.1.4.1.20000.1",  /* OID of the specific sub-agent*/
        PROV_TABLE,             /* Id for the SNMP table */
        sampleInstXlator,       /* Instance Translator function pointer */
        sampleInstCompare       /* Instance Comparison function pointer */
    },
    {
        (char *)NULL, 
        -1,
        NULL,
        NULL
    }
};

