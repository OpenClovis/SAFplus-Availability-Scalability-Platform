/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/codegen/CodeGenLogger.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.codegen;

import com.clovis.cw.workspace.builders.ClovisActionLogger;


/**
 * @author Pushparaj
 * Logger for the Code Generation.
 */
public class CodeGenLogger extends ClovisActionLogger
{
    
    protected String checkForCustomError()
    {
    	String docText = _MC.getDocument().get().toLowerCase();

    	if (docText.indexOf("you don\'t have the snmp perl module installed") != -1)
    	{
    		return "The SNMP perl module is not installed correctly.";
    	}
    	
    	if (docText.indexOf("cannot find module") != -1)
    	{
    		return "Error generating mib code because of a problem with the snmp sub agent mib path.";
    	}
    	
    	if (docText.indexOf("didn't give mib2c a valid oid to start with") != -1)
    	{
    		return "There was a problem compiling the mib. Check the snmpautocodegen output for more information.";
    	}
    	
    	if(docText.indexOf("mib2c: not found") != -1)
    	{
    		return "Unable to find mib2c";
    	}
    	
    	if ((docText.indexOf("error") != -1) || (docText.indexOf("then come back and try mib2c") != -1))
    	{
    		return "An unexpected error occurred.";
    	}
    	return null;
    }
    
    /**
     * Constructor.
     * Sets console title as well as success and fail messages.
     */
    public CodeGenLogger()
    {
        super("Code Generation");
    }
}
