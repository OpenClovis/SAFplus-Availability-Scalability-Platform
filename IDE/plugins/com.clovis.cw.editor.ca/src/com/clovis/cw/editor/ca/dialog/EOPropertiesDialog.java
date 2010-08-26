/**
 * 
 */
package com.clovis.cw.editor.ca.dialog;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;

import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.constants.AnnotationConstants;
import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.ui.ObjectValidator;
import com.clovis.common.utils.ui.dialog.PushButtonDialog;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;

/**
 * Dialog for the EO Properties.
 * 
 * @author Suraj Rajyaguru
 *
 */
public class EOPropertiesDialog extends PushButtonDialog {

	/**
	 * Constructor.
	 * 
	 * @param shell
	 * @param eClass
	 * @param value
	 * @param parentEnv
	 */
	public EOPropertiesDialog(Shell shell, EClass eClass, Object value,
			Environment parentEnv) {
		super(shell, eClass, value, parentEnv);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.common.utils.ui.dialog.PushButtonDialog#okPressed()
	 */
	@Override
	protected void okPressed() {
		EObject safCompVM = (EObject) _parentEnv.getValue("model");
		String rdn = EcoreUtils.getValue(safCompVM,
				ModelConstants.RDN_FEATURE_NAME).toString();

		EObject safComp = (EObject) ((Model) _parentEnv.getValue("containerModel")).getEObject();
		EStructuralFeature containerFeature = safComp.eContainingFeature();
		EObject containerObj = safComp.eContainer();
		List containerList = (List) containerObj.eGet(containerFeature);

		List compList = new ArrayList();
		Iterator<EObject> itr = containerList.iterator();
		while (itr.hasNext()) {
			EObject obj = itr.next();
			if (!EcoreUtils.getValue(obj, ModelConstants.RDN_FEATURE_NAME)
					.toString().equals(rdn)) {
				compList.add(EcoreUtils.getValue(obj,
						ComponentEditorConstants.EO_PROPERTIES_NAME));
			}
		}

		EClass eClass = ((EObject) getViewModel().getEObject()).eClass();
		Vector featureNames = new Vector();
		if (eClass != null) {
			String featureName = EcoreUtils.getAnnotationVal(eClass, null,
					AnnotationConstants.VALIDATION_FEATURES);
			if (featureName != null) {
				int index = featureName.indexOf(',');
				while (index != -1) {
					String feature = featureName.substring(0, index);
					featureNames.add(feature);
					featureName = featureName.substring(index + 1);
					index = featureName.indexOf(',');
				}
				featureNames.add(featureName);
			}
		}

		String msg = (new ObjectValidator(featureNames) {
			public String checkDuplicate(EObject eobj, List eList) {
				return super.isDuplicate(eobj, eList, null);
			}
		}).checkDuplicate(getViewModel().getEObject(), compList);
		if (msg == null) {
			int portNo = Integer.parseInt(String.valueOf(EcoreUtils.getValue(getViewModel().getEObject(),
			"iocPortNumber")));
			if(portNo!= 0) {
				if(isDuplicatePortNumber(portNo, compList)) {
					msg = "Duplicate port number (" + portNo + "), Try using different port number.";
				}
			}
		}
		if (msg != null) {
			setMessage(msg, IMessageProvider.ERROR);
		} else {
			super.okPressed();
		}
	}
	/**
	 * Verifys the duplication of port number
	 * @param value
	 * @param compList
	 * @return
	 */
	private boolean isDuplicatePortNumber(int value, List compList) {
		for(int i = 0; i < compList.size(); i++) {
			EObject compObj = (EObject)compList.get(i);
			int portNo = Integer.parseInt(String.valueOf(EcoreUtils.getValue(compObj,
			"iocPortNumber")));
			if(portNo == value)
				return true;
		}
		return false;
	}
}
