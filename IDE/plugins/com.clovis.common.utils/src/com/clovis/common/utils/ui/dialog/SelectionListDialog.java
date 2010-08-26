/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/dialog/SelectionListDialog.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui.dialog;

import java.util.List;

import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.UtilsPlugin;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.log.Log;
import com.clovis.common.utils.ui.list.SelectionListView;

/**
 * @author shubhada
 *
 *Selection List Dialog which shows Objects to be selected.
 */
public class SelectionListDialog extends TitleAreaDialog
{
    private List  _selList   = null;
    private List  _origList  = null;
    private Model _viewModel = null;
    private String _title = null;
    private static final Log LOG = Log.getLog(UtilsPlugin.getDefault());
    /**
     * @param parentShell - Shell
     * @param title String
     * @param selList - SelectionList
     * @param originalList - Original List
     */
    public SelectionListDialog(Shell parentShell, String title, List selList,
            List originalList)
    {
        super(parentShell);
        _title = title;
        _selList = selList;
        _origList = originalList;

    }
    /**
     * Save the Model.
     */
    protected void okPressed()
    {
        if (_viewModel != null) {
            try {
                _viewModel.save(false);
            } catch (Exception e) {
                LOG.error("Save Error.", e);
                e.printStackTrace();
            }
        }
        super.okPressed();
    }
    /**
     * Closing Dialog.
     * @return super.close()
     */
    public boolean close()
    {
        if (_viewModel != null) {
            _viewModel.dispose();
            _viewModel = null;
        }
        return super.close();
    }
    /**
     * create the contents of the Dialog.
     * @param  parent Parent Composite
     * @return Dialog area.
     */
    protected Control createDialogArea(Composite parent)
    {
        Composite container = new Composite(parent, SWT.NONE);
        container.setLayoutData(new GridData(GridData.FILL_BOTH));
        GridLayout containerLayout = new GridLayout();
        container.setLayout(containerLayout);
        Model model = new Model(null, (NotifyingList) _selList, null);
        _viewModel  = model.getViewModel();
        ClassLoader loader = getClass().getClassLoader();
        SelectionListView listView = new SelectionListView(container,
                SWT.NONE, _viewModel.getEList(), _origList, loader, true);
        listView.setLayoutData(new GridData(GridData.FILL_BOTH));
        if (_origList.isEmpty()) {
            setMessage("List from which values to be chosen is empty",
                    IMessageProvider.INFORMATION);
        }
        setTitle(_title);
        getShell().setText("Select From List");
        return container;
    }

}
