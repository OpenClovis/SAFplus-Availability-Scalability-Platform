/**
 * 
 */
package com.clovis.cw.workspace.modelTemplate;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EEnum;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.viewers.ILabelProviderListener;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.swt.graphics.Image;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ui.tree.TreeNode;

/**
 * Label provider for the key-value type tree viewers.
 * 
 * @author Suraj Rajyaguru
 *
 */
public class KeyValueTreeLabelProvider implements ITableLabelProvider {

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object,
	 *      int)
	 */
	public Image getColumnImage(Object element, int columnIndex) {
		return null;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object,
	 *      int)
	 */
	public String getColumnText(Object element, int columnIndex) {
		TreeNode node = (TreeNode) element;
		EStructuralFeature feature = node.getFeature();
		Object val = node.getValue();

		if (val instanceof EObject && feature != null) {
			String label = feature.getName();
			String featureLabel = EcoreUtils.getAnnotationVal(feature, null,
					"label");
			if (featureLabel != null) {
				label = featureLabel;
			}

			if (columnIndex == 0) {
				EObject eobj = (EObject) val;
				if (feature.getEType() instanceof EEnum) {
					return label;
				}
				String name = EcoreUtils.getName(eobj);
				if (name != null) {
					return name;
				} else {
					return label;
				}
			} else if (columnIndex == 1) {
				if (feature.getEType() instanceof EEnum) {
					return val.toString();
				}
				return "";
			}

		} else if (val instanceof EObject && feature == null) {
			if (columnIndex == 0) {
				EObject eobj = (EObject) val;
				String name = EcoreUtils.getName(eobj);
				if (name != null) {
					return name;
				} else {
					return eobj.eClass().getName();
				}
			} else if (columnIndex == 1) {
				return "";
			}

		} else if (val instanceof EList) {
			if (columnIndex == 0) {
				String label = feature.getName();
				String featureLabel = EcoreUtils.getAnnotationVal(feature,
						null, "label");
				if (featureLabel != null) {
					label = featureLabel;
				}
				return label;
			} else if (columnIndex == 1) {
				return "";
			}
		}

		if (feature != null && val != null && !(val instanceof EObject)
				&& !(val instanceof EList)) {
			String label = feature.getName();
			String featureLabel = EcoreUtils.getAnnotationVal(feature, null,
					"label");
			if (featureLabel != null) {
				label = featureLabel;
			}
			if (columnIndex == 0) {
				return label;
			} else if (columnIndex == 1) {
				return val.toString();
			}
		}

		return null;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#addListener(org.eclipse.jface.viewers.ILabelProviderListener)
	 */
	public void addListener(ILabelProviderListener listener) {
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#dispose()
	 */
	public void dispose() {
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#isLabelProperty(java.lang.Object,
	 *      java.lang.String)
	 */
	public boolean isLabelProperty(Object element, String property) {
		return false;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#removeListener(org.eclipse.jface.viewers.ILabelProviderListener)
	 */
	public void removeListener(ILabelProviderListener listener) {
	}
}
