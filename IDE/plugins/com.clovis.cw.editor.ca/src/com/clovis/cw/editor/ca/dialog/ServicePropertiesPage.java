/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
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


/**
 * @author pushparaj
 *
 * Class for Alarm/Prov Service Properties Page
 */
public class ServicePropertiesPage extends PreferencePage
{
    //private       EObject               _classObj;
    //private       WidgetProviderFactory _wfactory;
    //private       boolean _isAlarmPage;
    //private DialogValidator _pageValidator = null;

    /**
     * Constructor.
     * @param name     Name of the Page.
     * @param classObj Class Object to be shown
     */
    public ServicePropertiesPage(String name)
    {
        super(name);
        setTitle(name);
        noDefaultAndApplyButton();
        //_classObj = classObj;
        //_isAlarmPage = isAlarmPage;
        //Model model = new Model(null, classObj);
        //_pageValidator = new DialogValidator(this, model);
        //_wfactory = new WidgetProviderFactory(_classObj, this, _pageValidator);
    }
    /**
     *
     * Creates the content of the page
     * @param baseComposite base control
     * @return Control base conrtol
     */
    public Control createContents(Composite baseComposite)
    {
        Composite container = new Composite(baseComposite, SWT.NONE);
        GridLayout containerLayout = new GridLayout();
        containerLayout.numColumns = 4;
        container.setLayout(containerLayout);
        GridData data = new GridData();
        container.setLayoutData(data);

        /*EClass eClass = _classObj.eClass();
        EStructuralFeature feature = eClass.getEStructuralFeature("isEnabled");
        Button enableButton = (Button) _wfactory.getButton(
                container, SWT.CHECK, feature);
        GridData gd = new GridData(GridData.BEGINNING);
        gd.horizontalSpan = 4;
        enableButton.setLayoutData(gd);
        if(!_isAlarmPage && _classObj.eContainer().eClass().getName().equals("MibResource")){
        	//enableButton.setSelection(true);
        	if(enableButton.getSelection())
        		enableButton.setEnabled(false);
        }*/
        /*if (_isAlarmPage) {
            Label pollinglbl = new Label(container, SWT.NONE);
            GridData gd1  = new GridData(GridData.BEGINNING);
            gd1.horizontalSpan = 2;
            pollinglbl.setLayoutData(gd1);

            eClass = _classObj.eClass();
            feature = eClass.getEStructuralFeature(ALARM_POLLING_INTERVAL);
            pollinglbl.setText(EcoreUtils.getLabel(feature) + ":");
            Text pollText = (Text) _wfactory.getControl(
                    container, SWT.SINGLE | SWT.LEFT | SWT.BORDER, feature);
            GridData gd2 = new GridData(GridData.FILL_HORIZONTAL);
            gd2.horizontalSpan = 2;
            pollText.setLayoutData(gd2);
        }*/
        
        return container;
    }
    /**
     * remove the listeners attached on disposure.
     */
    public void dispose()
    {
        super.dispose();
        //_pageValidator.removeListeners();
        //_pageValidator = null;
    }
    /**
    *
    * @return dialog validator
    */
   /*public DialogValidator getValidator()
   {
       return _pageValidator;
   }*/
}
