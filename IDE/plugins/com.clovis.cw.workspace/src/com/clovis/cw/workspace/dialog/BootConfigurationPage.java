/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/dialog/BootConfigurationPage.java $
 * $Author: srajyaguru $
 * $Date: 2007/04/30 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.dialog;

import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.List;

import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.menu.IEnvironmentListener;
import com.clovis.common.utils.ui.DialogValidator;
import com.clovis.common.utils.ui.list.ListView;
import com.clovis.common.utils.ui.property.PropertyViewer;
/**
 * @author shanth
 *
 * Boot Config Page.
 */
public class BootConfigurationPage extends PreferencePage
{
    protected NotifyingList _contents = new ClovisNotifyingListImpl();

    private DialogValidator _dialogValidator;

    /**
     * Constructor.
     * @param name Name of the Page.
     * @param list List of content to show on top block.
     */
    public BootConfigurationPage(String name, java.util.List list)
    {
        super(name);
        noDefaultAndApplyButton();
        _contents.addAll(list);
    }
    /**
     * Creates Content for this Page
     * @param  parent Parent Composite
     * @return Control
     */
    public Control createContents(Composite parent)
    {
        Composite container = new Composite(parent, SWT.NONE);
        GridLayout containerLayout = new GridLayout();
        container.setLayout(containerLayout);
        GridData containerData = new GridData(GridData.FILL_BOTH);
        container.setLayoutData(containerData);
        int style = SWT.BORDER | SWT.H_SCROLL | SWT.V_SCROLL;

        List list = new List(container, style);
        final ListView viewer = new ListView(list);
        list.setLayoutData(new GridData(GridData.FILL_BOTH));
        viewer.setInput(_contents);

        final PropertyViewer propView =
            new PropertyViewer(container, SWT.NONE, null);
        final PreferencePage page = this;
        viewer.getNotifier().addListener(new IEnvironmentListener() {

        	public void valueChanged(Object obj) {

        		if (page.getMessage() != null && !page.getMessage().equals("")) {
					MessageDialog
							.openError(getShell(), "Invalid Configuration",
									"Kindly change the current invalid configuration to do other configuration.");
					return;
				}

				if (obj.equals("selection")) {
					EObject node = (EObject) ((IStructuredSelection) viewer
							.getSelection()).getFirstElement();

					if (node != null) {
						propView.setInput(node);
						_dialogValidator = new DialogValidator(page, new Model(
								null, node));
					}
				}
			}
        });
        propView.getControl().setLayoutData(new GridData(GridData.FILL_BOTH));
        return container;
    }
	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.DialogPage#dispose()
	 */
	@Override
	public void dispose() {
		if(_dialogValidator != null) {
	        _dialogValidator.removeListeners();
	        _dialogValidator = null;
		}
        super.dispose();
	}
}
