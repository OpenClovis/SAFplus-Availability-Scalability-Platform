/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/snmp/LoadedMibsTab.java $
 * $Author: bkpavan $
 * $Date: 2007/05/09 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.editor.ca.snmp;


import java.io.InputStreamReader;

import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CTabFolder;
import org.eclipse.swt.custom.CTabItem;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Table;

import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
import com.clovis.common.utils.log.Log;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.menu.MenuBuilder;
import com.clovis.common.utils.ui.table.TableUI;
import com.clovis.cw.editor.ca.CaPlugin;

/**
 * @author shubhada
 *
 * Tab to show the loaded mibs and other details to the user.
 */
public class LoadedMibsTab extends CTabItem
{
    private Table _table = null;
    private TableUI _tableViewer = null;
    private Environment _env   = null;
    private static final Log LOG = Log.getLog(CaPlugin.getDefault());
    /**
     * Constructor
     * @param parent CTabFolder
     * @param style int
     * @param env Environment
     */
    public LoadedMibsTab(CTabFolder parent, int style, Environment env)
    {
        super(parent, style);
        _env = env;
        setControl(createContents(parent));
        MibFilesReader.getInstance().setShell(getControl().getShell());
        setText("Loaded MIBs");
    }
    /**
    *
    * @param arg0 parent CTabFolder
    * @return Composite
    */
    protected Composite createContents(CTabFolder arg0)
    {
        Composite container = new Composite(arg0, SWT.NONE);
        GridLayout containerLayout = new GridLayout();
        containerLayout.numColumns = 2;
        container.setLayout(containerLayout);
        GridData containerData = new GridData();
        container.setLayoutData(containerData);
        int style = SWT.BORDER | SWT.H_SCROLL
        | SWT.V_SCROLL | SWT.FULL_SELECTION | SWT.HIDE_SELECTION | SWT.MULTI
        | SWT.READ_ONLY;
        _table = new Table (container, style);
        ClassLoader loader = getClass().getClassLoader();
        _tableViewer = new TableUI(_table,
                ClovisMibUtils.getUiECLass(), loader, true);

        GridData gridData1 = new GridData();
        gridData1.horizontalAlignment = GridData.FILL;
        gridData1.grabExcessHorizontalSpace = true;
        gridData1.grabExcessVerticalSpace = true;
        gridData1.verticalAlignment = GridData.FILL;
        gridData1.heightHint = container.getDisplay().getClientArea().height / 10;
        _table.setLayoutData(gridData1);
        
        _table.setLinesVisible(true);
        _table.setHeaderVisible(true);
        ClovisNotifyingListImpl elist = MibFilesReader.getInstance().
        getTableInput();
        _tableViewer.setInput(elist);

        //Add Buttons (toolbar) for table.
        try {
            InputStreamReader reader = new InputStreamReader(
                getClass().getResourceAsStream("treetoolbar.xml"));
            new MenuBuilder(reader, _env).getToolbar(container, 0);
        } catch (Exception e) {
            LOG.error("Toolbar XML could not be loaded, using no toolbar.", e);
        }
        return container;
    }
    /**
    *
    * @return TableViewer
    */
    public TableUI getTableViewer()
    {
        return _tableViewer;
    }
}
