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

import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;

import org.eclipse.jface.preference.PreferencePage;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;

import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.ui.DialogValidator;
import com.clovis.common.utils.ui.factory.WidgetProviderFactory;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;

/**
 * @author pushparaj
 *
 * Class to hold the UI related details of Data Structure
 */
public class DataStructurePropertiesPage extends PreferencePage
    implements ClassEditorConstants
{
    private       EObject               _classObj;
    private       WidgetProviderFactory _wfactory;
    private       DialogValidator        _validator = null;
    /**
     * Constructor.
     * @param name     Name of the Page.
     * @param classObj Class Object to be shown
     */
    public DataStructurePropertiesPage(String name, EObject classObj)
    {
        super(name);
        setTitle(name);
        noDefaultAndApplyButton();
        _classObj = classObj;
        ClovisNotifyingListImpl list = new ClovisNotifyingListImpl();
        list.add(_classObj);
        Model model = new Model(null, list, null);
        _validator = new DialogValidator(this, model);
        _wfactory = new WidgetProviderFactory(_classObj, this, _validator);
    }

    /**
     * @param baseComposite base control
     * @return Control base control
     */
    public Control createContents(Composite baseComposite)
    {
        Composite container = new Composite(baseComposite, SWT.NONE);
        GridLayout containerLayout = new GridLayout();
        containerLayout.numColumns = 4;
        container.setLayout(containerLayout);
        GridData containerData = new GridData();
        container.setLayoutData(containerData);

        Label namelbl = new Label(container, SWT.NONE);
        GridData lgd = new GridData(GridData.BEGINNING);
        namelbl.setLayoutData(lgd);

        EClass eClass = _classObj.eClass();
        EStructuralFeature feature = eClass.getEStructuralFeature(CLASS_NAME);
        namelbl.setText(EcoreUtils.getLabel(feature) + ":");
        Text nameText = (Text) _wfactory.getControl(
                container, SWT.SINGLE | SWT.LEFT | SWT.BORDER, feature);
        GridData gd = new GridData(GridData.BEGINNING);
        gd.horizontalAlignment = GridData.FILL;
        gd.horizontalSpan = 3;
        gd.grabExcessHorizontalSpace = true;
        nameText.setLayoutData(gd);

        Label docLabel = new Label(container, SWT.NONE);
        docLabel.setText(CLASS_DOCUMENTATION + ":");
        GridData docgd = new GridData(GridData.BEGINNING);
        docgd.horizontalSpan = 4;
        docLabel.setLayoutData(docgd);

        int style = SWT.MULTI | SWT.V_SCROLL | SWT.H_SCROLL | SWT.BORDER;
        feature = eClass.getEStructuralFeature(CLASS_DOCUMENTATION);
        Control doc = _wfactory.getControl(container, style, feature);
        GridData gdDoc = new GridData(GridData.FILL_BOTH);
        gdDoc.horizontalSpan = 4;
        doc.setLayoutData(gdDoc);
        return container;
    }
    /**
     * remove the listeners attached on disposure.
     */
    public void dispose()
    {
        super.dispose();
        _validator.removeListeners();
        _validator = null;
    }
    /**
    *
    * @return dialog validator
    */
   public DialogValidator getValidator()
   {
       return _validator;
   }

}
