/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/dialog/ClassPropertiesPage.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
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
import com.clovis.common.utils.ui.factory.WidgetProviderFactory;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;

/**
 * @author shubhada
 *
 * Class to hold the UI related details of Data Class
 */
public class ClassPropertiesPage extends PreferencePage
    implements ClassEditorConstants
{
    private       EObject               _classObj;
    private       WidgetProviderFactory _wfactory;
    private AttributesValidator _pageValidator = null;

    /**
     * Constructor.
     * @param name     Name of the Page.
     * @param classObj Class Object to be shown
     */
    public ClassPropertiesPage(String name, EObject classObj)
    {
        super(name);
        setTitle(name);
        noDefaultAndApplyButton();
        _classObj = classObj;
        ClovisNotifyingListImpl list = new ClovisNotifyingListImpl();
        list.add(_classObj);
        Model model = new Model(null, list, null);
        _pageValidator = new AttributesValidator(this, model);
        _wfactory = new WidgetProviderFactory(_classObj, this, _pageValidator);
    }
    /**
     * Creates the content of the page.
     * @param baseComposite Parent Composite
     * @return Content of this Page.
     */
    public Control createContents(Composite baseComposite)
    {
    	boolean isMibResource = _classObj.eClass().getName().equals(MIB_RESOURCE_NAME);
        Composite container = new Composite(baseComposite, SWT.NONE);
        GridLayout containerLayout = new GridLayout();
        containerLayout.numColumns = 4;
        container.setLayout(containerLayout);
        GridData containerData = new GridData();
        container.setLayoutData(containerData);

        Label namelbl = new Label(container, SWT.NONE);
        GridData lgd  = new GridData(GridData.BEGINNING);
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
        if(isMibResource)
        	nameText.setEnabled(false);
        Label instanceslbl = new Label(container, SWT.NONE);
        lgd  = new GridData(GridData.BEGINNING);
        instanceslbl.setLayoutData(lgd);
        feature = eClass.getEStructuralFeature(CLASS_MAX_INSTANCES);
        instanceslbl.setText(EcoreUtils.getLabel(feature) + ":");
        Text instancesText = (Text) _wfactory.getControl(
                container, SWT.SINGLE | SWT.LEFT | SWT.BORDER, feature);
        gd = new GridData(GridData.BEGINNING);
        gd.horizontalAlignment = GridData.FILL;
        gd.horizontalSpan = 3;
        gd.grabExcessHorizontalSpace = true;
        instancesText.setLayoutData(gd);
        if(isMibResource) {
        	boolean isScalar = ((Boolean)EcoreUtils.getValue(_classObj, "isScalar")).booleanValue();
        	if(isScalar)
        		instancesText.setEnabled(false);
        }
        
        /*feature = eClass.getEStructuralFeature(CLASS_IS_PERSISTENT);
        Button persistentButton = (Button)
            _wfactory.getButton(container, SWT.CHECK, feature);
        GridData gd1 = new GridData(GridData.BEGINNING);
        gd1.widthHint = 150;
        gd1.horizontalSpan = 2;
        gd1.horizontalAlignment = GridData.FILL;
        gd1.grabExcessHorizontalSpace = true;
        persistentButton.setLayoutData(gd1);

        feature = eClass.getEStructuralFeature(CLASS_IS_ABSTRACT);
        Button abstractButton = (Button)
            _wfactory.getButton(container, SWT.CHECK, feature);
        GridData gd2 = new GridData(GridData.BEGINNING);
        gd2.horizontalAlignment = GridData.FILL;
        gd2.grabExcessHorizontalSpace = true;
        abstractButton.setLayoutData(gd2);


        feature = eClass.getEStructuralFeature(CLASS_IS_DISTRIBUTED);
        Button distributedButton = (Button)
            _wfactory.getButton(container, SWT.CHECK, feature);
        GridData gd3 = new GridData(GridData.BEGINNING);
        gd3.horizontalAlignment = GridData.FILL;
        gd3.grabExcessHorizontalSpace = true;
        distributedButton.setLayoutData(gd3);*/

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
        _pageValidator.removeListeners();
        _pageValidator = null;
    }
    /**
    *
    * @return dialog validator
    */
   public AttributesValidator getValidator()
   {
       return _pageValidator;
   }
 }
