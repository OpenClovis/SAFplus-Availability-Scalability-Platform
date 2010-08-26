/**
 * 
 */
package com.clovis.cw.workspace.modelTemplate;

import java.util.ArrayList;
import java.util.List;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EEnum;
import org.eclipse.emf.ecore.ENamedElement;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.viewers.ITreeContentProvider;
import org.eclipse.jface.viewers.Viewer;

import com.clovis.common.utils.constants.AnnotationConstants;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ui.tree.TreeNode;

/**
 * Content provider for the key-value type tree viewers.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class KeyValueTreeContentProvider implements ITreeContentProvider {

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.ITreeContentProvider#getChildren(java.lang.Object)
	 */
	public Object[] getChildren(Object parentElement) {
		TreeNode node = (TreeNode) parentElement;
		Object parentObj = node.getValue();

		ArrayList<TreeNode> childrenList = new ArrayList<TreeNode>();
		if (parentObj instanceof EObject) {

			EObject parentEObj = (EObject) parentObj;
			List<EStructuralFeature> featureList = getFeatureList(parentEObj
					.eClass(), true);

			for (int i = 0; i < featureList.size(); i++) {
				EStructuralFeature feature = featureList.get(i);

				Object value = parentEObj.eGet(feature);
				if (value == null)
					continue;

				if (value instanceof List) {
					List refList = (List) value;

					for (int j = 0; j < refList.size(); j++) {
						childrenList.add(new TreeNode(feature, node, refList
								.get(j)));
					}
				} else {
					childrenList.add(new TreeNode(feature, node, value));

				}
			}

		} else if (parentObj instanceof List) {
			List parentObjList = (List) parentObj;

			for (int i = 0; i < parentObjList.size(); i++) {
				Object object = parentObjList.get(i);

				if (object != null) {
					childrenList.add(new TreeNode(null, node.getParent(),
							object));
				}
			}
		}

		return childrenList.toArray();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.ITreeContentProvider#getParent(java.lang.Object)
	 */
	public Object getParent(Object element) {
		TreeNode node = (TreeNode) element;
		return node.getParent();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.ITreeContentProvider#hasChildren(java.lang.Object)
	 */
	public boolean hasChildren(Object element) {
		Object val = ((TreeNode) element).getValue();
		EStructuralFeature feature = ((TreeNode) element).getFeature();
		if (feature != null) {
			return ((val instanceof EObject || val instanceof EList) && !(feature
					.getEType() instanceof EEnum));
		} else {
			return (val instanceof EObject || val instanceof EList);
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.IStructuredContentProvider#getElements(java.lang.Object)
	 */
	public Object[] getElements(Object inputElement) {
		List elementList = (List) inputElement;
		ArrayList<TreeNode> elements = new ArrayList<TreeNode>();

		for (int i = 0; i < elementList.size(); i++) {
			elements.add(new TreeNode(null, null, elementList.get(i)));
		}
		return elements.toArray();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.IContentProvider#dispose()
	 */
	public void dispose() {
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.IContentProvider#inputChanged(org.eclipse.jface.viewers.Viewer,
	 *      java.lang.Object, java.lang.Object)
	 */
	public void inputChanged(Viewer viewer, Object oldInput, Object newInput) {
	}

	/**
	 * Get List of features for this EClass.
	 * 
	 * @param eClass
	 *            EClass
	 * @param viewOnly
	 *            true for view only features
	 * @return List of features.
	 */
	@SuppressWarnings("unchecked")
	private List<EStructuralFeature> getFeatureList(EClass eClass,
			boolean viewOnly) {
		List<EStructuralFeature> featureList = new ArrayList<EStructuralFeature>();
		List<EStructuralFeature> features = eClass.getEAllStructuralFeatures();

		for (int i = 0; i < features.size(); i++) {
			EStructuralFeature feature = features.get(i);

			if (!viewOnly || !(isHidden(feature))) {
				featureList.add(feature);
			}
		}

		return featureList;
	}

	/**
	 * Return hide/unhide nature of this named element. Uses isHidden key in
	 * CwAnnotation to find this.
	 * 
	 * @param element
	 *            Element
	 * @return true if hidden.
	 */
	private boolean isHidden(ENamedElement element) {
		String isHiddenForKeyValueTreeView = EcoreUtils.getAnnotationVal(
				element, null, "isHiddenForKeyValueTreeView");
		if (isHiddenForKeyValueTreeView != null) {
			return Boolean.parseBoolean(isHiddenForKeyValueTreeView);
		}

		String isHidden = EcoreUtils.getAnnotationVal(element, null,
				AnnotationConstants.IS_HIDDEN);

		return isHidden != null ? Boolean.parseBoolean(isHidden) : false;
	}
}
