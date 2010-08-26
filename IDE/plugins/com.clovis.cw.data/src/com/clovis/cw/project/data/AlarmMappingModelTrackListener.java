/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.data/src/com/clovis/cw/project/data/AlarmMappingModelTrackListener.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.project.data;

import java.util.List;

import org.eclipse.emf.ecore.EObject;

/**
 * 
 * @author pushparaj
 * Listener to handle resource-alarm mapping model
 * changes
 */
public class AlarmMappingModelTrackListener extends MappingModelTrackListener {
	
	public AlarmMappingModelTrackListener(ProjectDataModel dataModel) {
		super(dataModel);
	}
	/**
	 * Creates Resources List
	 * @param dataModel
	 */
	protected void createResourcesList() {
		List caList = _dataModel.getCAModel().getEList();
		EObject rootObject = (EObject) caList.get(0);
		List resources = (List) rootObject.eGet(rootObject.eClass().getEStructuralFeature("softwareResource"));
		_resourcesList.addAll(resources);
		resources = (List) rootObject.eGet(rootObject.eClass().getEStructuralFeature("mibResource"));
		_resourcesList.addAll(resources);
		resources = (List) rootObject.eGet(rootObject.eClass().getEStructuralFeature("hardwareResource"));
		_resourcesList.addAll(resources);
		resources = (List) rootObject.eGet(rootObject.eClass().getEStructuralFeature("nodeHardwareResource"));
		_resourcesList.addAll(resources);
	}
	
	
}
