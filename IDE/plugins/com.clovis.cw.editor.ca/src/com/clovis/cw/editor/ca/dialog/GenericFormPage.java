package com.clovis.cw.editor.ca.dialog;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;

import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.ui.DialogValidator;
import com.clovis.common.utils.ui.table.FormView;

/**
 * Holds EObject in the FormView for this preference page.
 * @author Suraj Rajyaguru
 */
public class GenericFormPage extends GenericPreferencePage {
	  
	private EObject _eObject = null;
	private DialogValidator _dialogValidator;
	  
	/**
	 * Constructor
	 * @param label Title for the page.
	 * @param obj EObject for the FormView.
	 * @param nodeList 
	 */
	public GenericFormPage(String label, EObject obj) {
		  super(label);
		  setTitle(label);
		  noDefaultAndApplyButton();
		  _eObject = obj;
		  Model model = new Model(null, _eObject);
		  _dialogValidator = new DialogValidator(this, model);
	}
	  
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	protected Control createContents(Composite parent) {
		  Composite container = new Composite(parent, SWT.NONE);
		  container.setLayout(new GridLayout());
		  container.setLayoutData(new GridData(GridData.FILL_BOTH));
		  FormView view = new FormView(container, SWT.NONE, _eObject,
				  getClass().getClassLoader(), this);
		  view.setLayoutData(new GridData(GridData.FILL_BOTH));
		  view.setValue("dialogvalidator", _dialogValidator);
		  return container;
	  }
	  
	/**
	 * @return EObject for the page
	 */
	public EObject getEObject() {
		  return _eObject;
	  }
    /**
     * remove the listeners attached on disposure.
     */
    public void dispose()
    {
        _dialogValidator.removeListeners();
        _dialogValidator = null;
        super.dispose();
    }
    /**
     * @return dialog validator
     */
    public DialogValidator getValidator()
    {
        return _dialogValidator;
    }
}
