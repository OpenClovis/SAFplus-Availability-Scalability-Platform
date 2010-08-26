/**
 * 
 */
package com.clovis.cw.editor.ca.pm;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.VerifyEvent;
import org.eclipse.swt.events.VerifyListener;

/**
 * Verify listener for Numeric Values.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class NumericVerifyListener implements VerifyListener {

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.swt.events.VerifyListener#verifyText(org.eclipse.swt.events.VerifyEvent)
	 */
	public void verifyText(VerifyEvent e) {

		switch (e.keyCode) {
		case SWT.BS:
		case SWT.DEL:
			return;
		}

		if (!e.text.matches("[0-9]")) {
			e.doit = false;
		}
	}
}
