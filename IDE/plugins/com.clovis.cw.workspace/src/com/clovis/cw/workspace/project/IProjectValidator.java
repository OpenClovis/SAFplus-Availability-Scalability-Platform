/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/project/IProjectValidator.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.project;

import java.util.List;

import com.clovis.cw.project.data.ProjectDataModel;

/**
 * @author Manish
 *
 * Model Validator Interface.
 */
public interface IProjectValidator {
	
	/*
	 *  Validates ProjectDataModel and returns a List of Problem objects.
	 */ 
	public List validate (ProjectDataModel pdm);

}
