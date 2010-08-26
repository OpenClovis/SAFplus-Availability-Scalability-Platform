package com.clovis.cw.editor.ca.manageability.ui;

import java.util.ArrayList;
import java.util.List;

import org.eclipse.jface.viewers.CheckStateChangedEvent;
import org.eclipse.jface.viewers.CheckboxTreeViewer;
import org.eclipse.jface.viewers.ICheckStateListener;
import org.eclipse.swt.custom.BusyIndicator;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.TreeItem;

import com.clovis.cw.editor.ca.manageability.common.ResourceTreeNode;
import com.ireasoning.util.MibTreeNode;

/**
 * Selection Handler for Checkbox TreeViewer
 * @author Pushparaj
 *
 */
public class TreeSelectionHandler implements ICheckStateListener {

	public TreeSelectionHandler() {
		
	}
	public void checkStateChanged(final CheckStateChangedEvent event) {
		final CheckboxTreeViewer viewer = (CheckboxTreeViewer) event.getSource();
		ResourceTreeNode node = (ResourceTreeNode) event.getElement();
		if (node.getNode() == null) {
			if (event.getChecked()) {
				viewer.setGrayed(node, true);
				viewer.setChecked(node, true);
			} else {
				viewer.setGrayed(node, false);
			}
		} else if (node.getNode().isTableNode()
				|| (node.getNode().isGroupNode() && isGroupWithScalar(node
						.getNode()))) {
			viewer.setChecked(node, event.getChecked());
			if(node.getNode().isGroupNode() && event.getChecked()) {
				expandGroupNode(viewer, node);
			}
		} else {
			if (event.getChecked()) {
				viewer.setGrayed(node, true);
				viewer.setChecked(node, true);
			} else {
				viewer.setGrayed(node, false);
				viewer.setChecked(node, false);
			}
		}

		BusyIndicator.showWhile(Display.getCurrent(), new Runnable() {
			public void run() {
				viewer.setSubtreeChecked(event.getElement(), event.getChecked());
				setSubtreeChecked(viewer, (ResourceTreeNode) event.getElement(), event
						.getChecked());
			}
		});
	}

	public void setSubtreeChecked(CheckboxTreeViewer viewer,
			ResourceTreeNode parent, boolean checked) {
		List<ResourceTreeNode> nodes = parent.getChildNodes();
		for (int i = 0; i < nodes.size(); i++) {
			ResourceTreeNode node = nodes.get(i);
			if (node.getNode() == null) {
				if (checked) {
					viewer.setGrayed(node, true);
					viewer.setChecked(node, true);
				} else {
					viewer.setGrayed(node, false);
					viewer.setChecked(node, false);
				}
			} else if (node.getNode().isTableNode()
					|| (node.getNode().isGroupNode() && isGroupWithScalar(node
							.getNode()))) {
				viewer.setChecked(node, checked);
			} else {
				if (checked) {
					viewer.setGrayed(node, true);
					viewer.setChecked(node, true);
				} else {
					viewer.setGrayed(node, false);
					viewer.setChecked(node, false);
				}
			}
			setSubtreeChecked(viewer, node, checked);
		}
	}
	 
	/**
	 * Verify if the Group node contains any scalar node
	 * @param node
	 * @return true if node contains scalar nodes else false.
	 */
	private boolean isGroupWithScalar(MibTreeNode node){
		List children = node.getChildNodes();
		for (int k = 0; k < children.size(); k++) {
			MibTreeNode child = (MibTreeNode) children.get(k);
			if (child.isScalarNode()) {
				return true;
			}
		}
		return false;
	}
	/**
	 * Expand all tables/scalars for Group node 
	 */
	private void expandGroupNode(CheckboxTreeViewer viewer, ResourceTreeNode node){
		viewer.setExpandedState(node, true);
		List children = node.getChildNodes();
		for (int k = 0; k < children.size(); k++) {
			ResourceTreeNode child = (ResourceTreeNode) children.get(k);
			if (child.getNode().isScalarNode() || child.getNode().isTableNode()) {
				viewer.setExpandedState(child, true);
			}
			expandGroupNode(viewer, child);
		}
	}
}
