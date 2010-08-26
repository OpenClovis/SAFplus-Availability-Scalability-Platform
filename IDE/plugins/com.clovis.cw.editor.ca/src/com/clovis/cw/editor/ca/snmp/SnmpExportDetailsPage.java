/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/snmp/SnmpExportDetailsPage.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.editor.ca.snmp;

import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.wizard.WizardPage;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;

import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.ui.DialogValidator;
import com.clovis.common.utils.ui.table.FormView;
/**
 * @author shubhada
 * Page1 for snmp export wizard
 */
public class SnmpExportDetailsPage extends WizardPage
{
    private SnmpExportWizard _wizard = null;
    private DialogValidator _pageValidator = null;
    /**
     * @param pageName PageName
     * @param wizard Wizard
     */
    protected SnmpExportDetailsPage(String pageName, SnmpExportWizard wizard)
    {
        super(pageName);
        _wizard = wizard;
        this.setTitle("SNMP Export Details");
        setMessage("Enter the Details", IMessageProvider.INFORMATION);
        Model model = new Model(null, _wizard.getDetailsObject());
        _pageValidator = new DialogValidator(this, model);
    }
    /**
     * @param parent
     *            Composite
     */
    public void createControl(Composite parent)
    {
        Composite container = new Composite(parent, SWT.NONE);
        container.setLayoutData(new GridData(GridData.FILL_BOTH));
        GridLayout containerLayout = new GridLayout();
        container.setLayout(containerLayout);
        ClassLoader loader = getClass().getClassLoader();
        FormView formView = new FormView(container, SWT.NONE, _wizard.
                getDetailsObject(), loader, this);
        formView.setLayoutData(new GridData(GridData.FILL_BOTH));
        this.setControl(container);
    }
    /**
     * remove the listeners attached on disposure.
     */
    public void dispose()
    {
        super.dispose();
        _pageValidator.removeListeners();
        _pageValidator = null;
    }
    /**
    *
    * @return dialog validator
    */
   public DialogValidator getValidator()
   {
       return _pageValidator;
   }
}
