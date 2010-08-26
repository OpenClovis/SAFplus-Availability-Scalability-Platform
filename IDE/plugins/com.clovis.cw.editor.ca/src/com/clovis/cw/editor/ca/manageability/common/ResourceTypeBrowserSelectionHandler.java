package com.clovis.cw.editor.ca.manageability.common;

import org.eclipse.jface.viewers.CheckStateChangedEvent;
import org.eclipse.jface.viewers.CheckboxTreeViewer;
import org.eclipse.jface.viewers.ICheckStateListener;

/**
 * Selection Handler for Checkbox TreeViewer
 * @author Pushparaj
 *
 */
public class ResourceTypeBrowserSelectionHandler implements ICheckStateListener {

	ResourceTypeBrowserSelectionHandler() {
		
	}
	
	public void checkStateChanged(CheckStateChangedEvent event) {
		CheckboxTreeViewer viewer = (CheckboxTreeViewer) event.getSource();
		ResourceTreeNode node = (ResourceTreeNode) event.getElement();
		viewer.setSubtreeChecked(event.getElement(), event.getChecked());
	}
}
