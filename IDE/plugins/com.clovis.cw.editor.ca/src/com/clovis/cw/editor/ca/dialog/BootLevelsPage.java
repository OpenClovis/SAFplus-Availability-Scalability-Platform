/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/dialog/BootLevelsPage.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.dialog;

import java.util.List;

import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;

import com.clovis.common.utils.ui.list.SelectionListView;

/**
 * @author shubhada
 *
 * BootLevels Page
 */
public class BootLevelsPage extends PreferencePage
{
    private List _selList = null;
    private List _origList = null;
    private List _aspSelList = null;
    private List _aspOrigList = null;
    /**
     *
     * @param name     Name of the page
     * @param selList  Selected List
     * @param origList Original List
     * @param aspSelList  Selected List of Asp Su
     * @param aspOrigList Original List Asp Su
     */
    public BootLevelsPage(String name, List selList, List origList,
            List aspSelList, List aspOrigList)
    {
       super(name);
       setMessage("Select Service Units");
       _selList  = selList;
       _origList = origList;
       _aspSelList = aspSelList;
       _aspOrigList = aspOrigList;
       noDefaultAndApplyButton();
    }
    /**
     * @param parent - Composite
     * @return Control
     */
    protected Control createContents(Composite parent)
    {
        Composite container = new Composite(parent, SWT.NONE);
        container.setLayoutData(new GridData(GridData.FILL_BOTH));
        GridLayout containerLayout = new GridLayout();
        container.setLayout(containerLayout);
        Group origGroup = new Group(container, SWT.SHADOW_OUT);
        origGroup.setText("USER SU");
        origGroup.setLayout(new GridLayout());
        GridData origgd = new GridData(GridData.FILL_BOTH);
        origGroup.setLayoutData(origgd);
        ClassLoader loader = getClass().getClassLoader();
        int style = SWT.MULTI | SWT.BORDER | SWT.H_SCROLL
        | SWT.V_SCROLL | SWT.FULL_SELECTION;
        SelectionListView listView = new SelectionListView(origGroup,
                style, _selList, _origList, loader, true);
        listView.setLayoutData(new GridData(GridData.FILL_BOTH));
        Group attrGroup = new Group(container, SWT.SHADOW_OUT);
        attrGroup.setText("ASP SU");
        attrGroup.setLayout(new GridLayout());
        GridData gd = new GridData(GridData.FILL_BOTH);
        attrGroup.setLayoutData(gd);
        SelectionListView asplistView = new SelectionListView(attrGroup,
                style, _aspSelList, _aspOrigList, loader, true);
        asplistView.setLayoutData(new GridData(GridData.FILL_BOTH));
        return container;
    }
}
