/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/dialog/CpmConfigPage.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.dialog;

import java.util.List;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ui.table.FormView;
import com.clovis.cw.editor.ca.constants.SafConstants;

/**
 * @author shanth The Page for a node in which user can specify the config
 *         related to node.
 */
public class CpmConfigPage extends GenericPreferencePage
{
    private EObject _eobj = null;
    /**
     * Constructor.
     *
     * @param name
     *            Name of the Page.
     * @param eobj - EObject
     */
    public CpmConfigPage(String name, EObject eobj)
    {
        super(name);
        noDefaultAndApplyButton();
        _eobj = eobj;
        // temporarily initializing the bootConfig Object because it is made hidden in UI.
        initBootConfigObj();
    }

    private void initBootConfigObj()
    {
        EReference bootConfigsRef = (EReference) _eobj.eClass().getEStructuralFeature(
                SafConstants.BOOT_CONFIGS_NAME);
        EObject bootConfigsObj = (EObject) EcoreUtils.getValue(_eobj,
                SafConstants.BOOT_CONFIGS_NAME);
        if (bootConfigsObj == null) {
            bootConfigsObj = EcoreUtils.createEObject(bootConfigsRef.getEReferenceType(), true);
        }
        List bootConfigList = (List) EcoreUtils.getValue(bootConfigsObj,
                SafConstants.BOOT_CONFIGLIST_NAME);
        EReference bootConfigRef = (EReference) bootConfigsObj.eClass().getEStructuralFeature(
                SafConstants.BOOT_CONFIGLIST_NAME);
        if (bootConfigList.isEmpty()) {
            EObject bootObj = EcoreUtils.createEObject(bootConfigRef.
                    getEReferenceType(), true);
            EcoreUtils.setValue(bootObj, "name", "default");
            EcoreUtils.setValue(bootObj, "maxBootLevel", "6");
            EcoreUtils.setValue(bootObj, "defaultBootLevel", "5");
            bootConfigList.add(bootObj);
        }
        
    }

    /**
     * @param parent -
     *            Parent Composite
     * @return new Control
     */
    public Control createContents(Composite parent)
    {
        Composite container = new Composite(parent, SWT.NONE);
        GridLayout containerLayout = new GridLayout();
        container.setLayout(containerLayout);
        GridData containerData = new GridData(GridData.FILL_BOTH);
        container.setLayoutData(containerData);
        ClassLoader loader = getClass().getClassLoader();
        FormView formView =
            new FormView(container, SWT.NONE, (EObject) _eobj, loader, this);
        formView.setValue("container", this);
        formView.setLayoutData(new GridData(GridData.FILL_BOTH));
        return container;
    }
}
