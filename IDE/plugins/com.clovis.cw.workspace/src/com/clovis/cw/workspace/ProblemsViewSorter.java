/**
 * 
 */
package com.clovis.cw.workspace;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerSorter;

import com.clovis.common.utils.ecore.EcoreUtils;

/**
 * Sorter for the Problems View.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class ProblemsViewSorter extends ViewerSorter {

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer,
	 *      java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2) {
		String level1 = EcoreUtils.getValue((EObject) e1, "level").toString();
		String level2 = EcoreUtils.getValue((EObject) e2, "level").toString();

		if (level1.equalsIgnoreCase("ERROR")
				|| (level1.equalsIgnoreCase("WARNING") && !level2
						.equalsIgnoreCase("ERROR"))) {

			return -1;
		} else {
			return 1;
		}
	}
}
