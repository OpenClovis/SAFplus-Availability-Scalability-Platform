package com.clovis.common.utils.ui.table;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.notify.impl.AdapterImpl;
import org.eclipse.emf.ecore.EEnum;
import org.eclipse.emf.ecore.EEnumLiteral;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.ComboBoxCellEditor;

import com.clovis.common.utils.ecore.EcoreUtils;

/**
 * This adapter class is responsible for the updation of the Controls on the
 * formview if its corresponding feature value is changed in respected EObject.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class FormViewRefresher extends AdapterImpl {

	FormView _formView = null;

	/**
	 * Constructor
	 * 
	 * @param view
	 *            FormView
	 */
	public FormViewRefresher(FormView view) {
		_formView = view;
		EcoreUtils.addListener((EObject) view.getValue("model"), this, 1);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.emf.common.notify.Adapter#notifyChanged(org.eclipse.emf.common.notify.Notification)
	 */
	public void notifyChanged(Notification notification) {
		switch (notification.getEventType()) {
		case Notification.SET:
			EObject eObject = (EObject) notification.getNotifier();
			EStructuralFeature feature = (EStructuralFeature) notification
					.getFeature();
			updateControlStatus(eObject, feature);
			break;
		}
	}

	/**
	 * This will get the respective control for the feature in the form view and
	 * update its status.
	 * 
	 * @param eObject
	 *            EObject
	 * @param feature
	 *            EStructuralFeature
	 * @param value
	 *            String
	 */
	private void updateControlStatus(EObject eObject, EStructuralFeature feature) {
		if(EcoreUtils.isHidden(feature)) {
			return;
		}
		Object value = eObject.eGet(feature);
		CellEditor editor = (CellEditor) _formView.getFeatureCellEditorMap().get(
				feature);
		if(editor instanceof ComboBoxCellEditor) {
			EEnum uiEnum = EcoreUtils.getUIEnum(feature);
			if (uiEnum != null) {
				EEnumLiteral literal = (EEnumLiteral) eObject.eGet(feature);
				value = uiEnum.getEEnumLiteral(literal.getValue()).getName();
			}
		}
		if(value != null) {
			editor.setValue(value);
		}
	}
}
