/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/dialog/AttributesPage.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.dialog;

import java.io.InputStreamReader;

import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Table;

import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.menu.MenuBuilder;
import com.clovis.common.utils.ui.DialogValidator;
import com.clovis.common.utils.ui.table.TableUI;
/**
 * @author shubhada
 *
 * Class to show the details related to attributes and operations
 */
public class AttributesPage extends PreferencePage
{
	private EClass     _eClass    = null;
    private EObject    _topLevelObject    = null;
    private NotifyingList _eObjects  = null;
    private AttributesPageValidator _pageValidator = null;
    /**
     * Constructor.
     * @param name     Name of Page.
     * @param eClass   Class
     * @param eObjects Objects
     */
    public AttributesPage(String name,
            EClass eClass, NotifyingList eObjects)
    {
        super(name);
        noDefaultAndApplyButton();
        _eClass   = eClass;
        _eObjects = eObjects;
        Model model = new Model(null, _eObjects, null);
        _pageValidator = new AttributesPageValidator(this, model);
    }
    
    /**
     * Constructor.
     * @param name     Name of Page.
     * @param eClass   Class
     * @param eObjects Objects
     */
    public AttributesPage(String name,
            EClass eClass, EObject topLevelObj, NotifyingList eObjects)
    {
        super(name);
        noDefaultAndApplyButton();
        _eClass   = eClass;
        _topLevelObject = topLevelObj;
        _eObjects = eObjects;
        Model model = new Model(null, _eObjects, null);
        _pageValidator = new AttributesPageValidator(this, model);
    }

    /**
     * Creates the content of the page.
     * @param  baseComposite Parent Composite
     * @return Dialog contents
     */
    public Control createContents(Composite baseComposite)
    {
        Composite container = new Composite(baseComposite, SWT.NONE);
        GridLayout containerLayout = new GridLayout();
        containerLayout.numColumns = 2;
        container.setLayout(containerLayout);
        GridData containerData = new GridData();
        container.setLayoutData(containerData);

        int style = SWT.MULTI | SWT.BORDER | SWT.H_SCROLL
            | SWT.V_SCROLL | SWT.FULL_SELECTION | SWT.HIDE_SELECTION;
        Table table = new Table(container, style);
        ClassLoader loader = getClass().getClassLoader();
        TableUI tableViewer = new TableUI(table, _eClass, loader);
        tableViewer.setValue("container", this);
        tableViewer.setValue("dialogvalidator", _pageValidator);
        tableViewer.setValue("topLevelObject", _topLevelObject);
        
        GridData gridData1 = new GridData();
        gridData1.horizontalAlignment = GridData.FILL;
        gridData1.grabExcessHorizontalSpace = true;
        gridData1.grabExcessVerticalSpace = true;
        gridData1.verticalAlignment = GridData.FILL;
        
        gridData1.heightHint =
        	container.getDisplay().getClientArea().height / 10;
        
        table.setLayoutData(gridData1);
        table.setLinesVisible(true);
        table.setHeaderVisible(true);
        tableViewer.setInput(_eObjects);
        table.setSelection(0);

        
        //Add Buttons (toolbar) for table.
        try {
            InputStreamReader reader = new InputStreamReader(
                getClass().getResourceAsStream("classdialogtoolbar.xml"));
            new MenuBuilder(reader, tableViewer).getToolbar(container, 0);
        } catch (Exception e) { e.printStackTrace(); }
        return container;
    }
    /**
     * remove the listeners attached on disposure.
     */
    public void dispose()
    {
        _pageValidator.removeListeners();
        _pageValidator = null;
        super.dispose();
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
