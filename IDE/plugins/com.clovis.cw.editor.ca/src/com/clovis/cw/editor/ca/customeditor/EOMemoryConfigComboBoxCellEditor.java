/**
 * 
 */
package com.clovis.cw.editor.ca.customeditor;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import org.eclipse.core.resources.IContainer;
import org.eclipse.core.resources.IResource;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.ComboBoxCellEditor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.ui.dialog.PushButtonDialog;
import com.clovis.common.utils.ui.table.FormView;
import com.clovis.cw.editor.ca.dialog.EOConfigurationDialog;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * Combo Box Cell Editor for Memory in EO Configuration Dialog.
 * 
 * @author Suraj Rajyaguru
 *
 */
public class EOMemoryConfigComboBoxCellEditor extends ComboBoxCellEditor {

	private Environment _env = null;

	private EStructuralFeature _feature = null;

	/**
	 * Constructor.
	 *
	 * @param parent
	 *            Composite
	 * @param feature
	 *            EStructuralFeature
	 * @param env
	 *            Environment
	 */
	public EOMemoryConfigComboBoxCellEditor(Composite parent,
			EStructuralFeature feature, Environment env) {
		super(parent, new String[0], SWT.BORDER | SWT.READ_ONLY);
		_env = env;
		_feature = feature;
	}

	/**
	 * Create Editor Instance.
	 * @param parent  Composite
	 * @param feature EStructuralFeature
	 * @param env     Environment
	 * @return cell Editor
	 */
	public static CellEditor createEditor(Composite parent,
			EStructuralFeature feature, Environment env) {
		return new EOMemoryConfigComboBoxCellEditor(parent, feature, env);
	}

	/**
	 * @return the Value
	 */
	protected Object doGetValue() {
		if (((Integer) super.doGetValue()).intValue() == -1) {
			return new String("");
		} else {
			return getItems()[((Integer) super.doGetValue()).intValue()];
		}
	}

	/**
	 * @param value -
	 *            Value to be set
	 */
	protected void doSetValue(Object value) {
		setItems(getComboValues());
		int index = -1;
		String[] items = this.getItems();
		for (int i = 0; i < items.length; i++) {
			if (items[i].equals(value)) {
				index = i;
			}
		}
		super.doSetValue(new Integer(index));
	}

	/**
	 * Returns the combo values
	 * 
	 * @return the combo values
	 */
	protected String[] getComboValues() {
		List comboList = new ArrayList();

		PushButtonDialog pbd = (PushButtonDialog) _env.getValue("container");
		EOConfigurationDialog dialog = (EOConfigurationDialog) pbd.getShell()
				.getParent().getShell().getData();

		IResource project = dialog.getProject();
		ProjectDataModel pdm = ProjectDataModel
				.getProjectDataModel((IContainer) project);

		Model eoDefinitions = pdm.getEODefinitions();
		EObject eoDefinitionsObj = eoDefinitions.getEObject();

		if (_feature.getName().equals("heapConfig")) {
			EObject memoryConfigurationObj = (EObject) EcoreUtils.getValue(
					eoDefinitionsObj, "memoryConfiguration");
			EObject heapConfigPoolObj = (EObject) EcoreUtils.getValue(
					memoryConfigurationObj, "heapConfigPool");
			EList heapConfigList = (EList) EcoreUtils.getValue(
					heapConfigPoolObj, "heapConfig");

			Iterator heapConfigIterator = heapConfigList.iterator();
			while (heapConfigIterator.hasNext()) {
				EObject obj = (EObject) heapConfigIterator.next();
				String name = (String) EcoreUtils.getValue(obj, "name");
				comboList.add(name);
			}

		} else if (_feature.getName().equals("bufferConfig")) {
			EObject memoryConfigurationObj = (EObject) EcoreUtils.getValue(
					eoDefinitionsObj, "memoryConfiguration");
			EObject bufferConfigPoolObj = (EObject) EcoreUtils.getValue(
					memoryConfigurationObj, "bufferConfigPool");
			EList bufferConfigList = (EList) EcoreUtils.getValue(
					bufferConfigPoolObj, "bufferConfig");

			Iterator bufferConfigIterator = bufferConfigList.iterator();
			while (bufferConfigIterator.hasNext()) {
				EObject obj = (EObject) bufferConfigIterator.next();
				String name = (String) EcoreUtils.getValue(obj, "name");
				comboList.add(name);
			}

		} else if (_feature.getName().equals("memoryConfig")) {
			EObject memoryConfigurationObj = (EObject) EcoreUtils.getValue(
					eoDefinitionsObj, "memoryConfiguration");
			EObject memoryConfigPoolObj = (EObject) EcoreUtils.getValue(
					memoryConfigurationObj, "memoryConfigPool");
			EList memoryConfigList = (EList) EcoreUtils.getValue(
					memoryConfigPoolObj, "memoryConfig");

			Iterator memoryConfigIterator = memoryConfigList.iterator();
			while (memoryConfigIterator.hasNext()) {
				EObject obj = (EObject) memoryConfigIterator.next();
				String name = (String) EcoreUtils.getValue(obj, "name");
				comboList.add(name);
			}

		} else if (_feature.getName().equals("iocConfig")) {
			EObject iocConfigurationObj = (EObject) EcoreUtils.getValue(
					eoDefinitionsObj, "iocConfiguration");
			EObject iocConfigPoolObj = (EObject) EcoreUtils.getValue(
					iocConfigurationObj, "iocConfigPool");
			EList iocConfigList = (EList) EcoreUtils.getValue(iocConfigPoolObj,
					"iocConfig");

			comboList.add("");
			Iterator iocConfigIterator = iocConfigList.iterator();
			while (iocConfigIterator.hasNext()) {
				EObject obj = (EObject) iocConfigIterator.next();
				String name = (String) EcoreUtils.getValue(obj, "name");
				comboList.add(name);
			}
		}

		String comboValues[] = (String[]) comboList.toArray(new String[] {});
		return comboValues;
	}

	/**
	 * Dont Deactivate editor in FormView.
	 */
	public void deactivate() {
		if (_env instanceof FormView) {
			fireCancelEditor();
		} else {
			super.deactivate();
		}
	}
}
