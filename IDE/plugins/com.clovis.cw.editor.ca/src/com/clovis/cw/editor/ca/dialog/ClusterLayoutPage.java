/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/dialog/ClusterLayoutPage.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.dialog;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;

import com.clovis.common.utils.ui.table.FormView;

/**
 * @author shanth The Page for a node in which user can specify the config
 *         related to node.
 */
public class ClusterLayoutPage extends PreferencePage {
	private EObject _eobj = null;

	/**
	 * Constructor.
	 * 
	 * @param name
	 *            Name of the Page.
	 * @param eobj -
	 *            EObject
	 */
	public ClusterLayoutPage(String name, EObject eobj) {
		super(name);
		noDefaultAndApplyButton();
		_eobj = eobj;
	}

	/**
	 * @param parent -
	 *            Parent Composite
	 * @return new Control
	 */
	public Control createContents(Composite parent) {
		Composite container = new Composite(parent, SWT.NONE);
		GridLayout containerLayout = new GridLayout();
		container.setLayout(containerLayout);
		GridData containerData = new GridData(GridData.FILL_BOTH);
		container.setLayoutData(containerData);
		ClassLoader loader = getClass().getClassLoader();
		FormView formView = new FormView(container, SWT.NONE, (EObject) _eobj,
				loader, this);
		formView.setValue("container", this);
		formView.setLayoutData(new GridData(GridData.FILL_BOTH));
		return container;
	}
}
