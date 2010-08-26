/**
 * 
 */
package com.clovis.cw.editor.ca.pm;

import java.util.ArrayList;
import java.util.Vector;

import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TreeSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Tree;

import com.clovis.cw.editor.ca.manageability.common.LoadMibComposite;
import com.clovis.cw.editor.ca.manageability.common.LoadedMibUtils;
import com.clovis.cw.editor.ca.manageability.common.ResourceTreeNode;
import com.clovis.cw.editor.ca.manageability.common.ResourceTypeBrowserUI;
import com.clovis.cw.editor.ca.manageability.ui.TreeSelectionHandler;

/**
 * Composite for resource browsing and mib loading/unloading.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class ResourceBrowserComposite extends Composite {

	private ResourceTypeBrowserUI _availableResViewer;
	private PMEditor _editor;
	
	/**
	 * Constructor.
	 * 
	 * @param parent
	 * @param editor
	 */
	public ResourceBrowserComposite(Composite parent, PMEditor editor) {
		super(parent, SWT.NONE);
		_editor = editor;
		createControls();
	}

	/**
	 * Creates the child controls.
	 */
	private void createControls() {
		GridLayout layout = new GridLayout();
		layout.marginWidth = layout.marginHeight = 0;
		setLayout(layout);
		setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

		Group resourceBrowserGroup = new Group(this, SWT.NONE);
		resourceBrowserGroup.setBackground(PMEditor.COLOR_PMEDITOR_BACKGROUND);
		resourceBrowserGroup.setText("Resource Browser");

		resourceBrowserGroup.setLayout(new GridLayout(2, false));
		resourceBrowserGroup.setLayoutData(new GridData(SWT.FILL, SWT.FILL,
				true, true));

		LoadMibComposite mibComposite = new LoadMibComposite(
				resourceBrowserGroup, SWT.NONE, _editor.getProjectDataModel().getLoadedMibs(), _editor
						.getProjectDataModel());
		mibComposite.setBackground(PMEditor.COLOR_PMEDITOR_BACKGROUND);
		GridLayout mibCompositeLayout = new GridLayout(2, false);
		mibCompositeLayout.marginWidth = mibCompositeLayout.marginHeight = 0;
		mibComposite.setLayout(mibCompositeLayout);
		mibComposite.setLayoutData(new GridData(SWT.FILL, 0, true, false));

		int style = SWT.SINGLE | SWT.CHECK | SWT.H_SCROLL | SWT.V_SCROLL
				| SWT.FULL_SELECTION | SWT.HIDE_SELECTION;
		_availableResViewer = new ResourceTypeBrowserUI(resourceBrowserGroup,
				style, getClass().getClassLoader());

		final Tree availableResTree = _availableResViewer.getTree();
		GridData resTreeData = new GridData(SWT.FILL, SWT.FILL, true, true);
		resTreeData.horizontalSpan = 2;
		availableResTree.setLayoutData(resTreeData);

		_availableResViewer
				.setContentProvider(new PMResourceTreeContentProvider());
		_availableResViewer.setInput(new Vector<ResourceTreeNode>());
		_availableResViewer.addCheckStateListener(new TreeSelectionHandler());
		_availableResViewer
				.addSelectionChangedListener(new ISelectionChangedListener() {
					/*
					 * (non-Javadoc)
					 * 
					 * @see org.eclipse.jface.viewers.ISelectionChangedListener#selectionChanged(org.eclipse.jface.viewers.SelectionChangedEvent)
					 */
					public void selectionChanged(SelectionChangedEvent event) {
						_editor
								.getThresholdCrossingComposite()
								.showCurrentAssociation(
										(ResourceTreeNode) ((TreeSelection) event
												.getSelection())
												.getFirstElement());
					}
				});
		_availableResViewer.expandAll();

		mibComposite.setResourceTypeBrowser(_availableResViewer);
		LoadedMibUtils.loadExistingMibs(_editor.getProjectDataModel(),
				_editor.getProjectDataModel().getLoadedMibs(), _availableResViewer);
	}

	/**
	 * Returns checked items.
	 * 
	 * @return
	 */
	public Object[] getCheckedItems() {
		return _availableResViewer.getCheckedElements();
	}
}
