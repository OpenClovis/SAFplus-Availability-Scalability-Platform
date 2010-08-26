/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/snmp/MoSelectionPage.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.editor.ca.snmp;

import org.eclipse.core.resources.IContainer;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.wizard.WizardPage;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;

import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
import com.clovis.common.utils.ui.list.SelectionListView;
import com.clovis.cw.editor.ca.ResourceDataUtils;
import com.clovis.cw.project.data.ProjectDataModel;
/**
 * @author shubhada
 *
 * Page which displays list of MO's to be exported to MIB file.
 */
public class MoSelectionPage extends WizardPage
{
    private IContainer   _project;
    private NotifyingList _selList = null;
    /**
     * @param pageName PageName
     * @param proj     Project Container
     */
    protected MoSelectionPage(String pageName, IContainer proj)
    {
        super(pageName);
        _project = proj;
        this.setTitle("Select MO's");
        setMessage("Select MO's to be exported", IMessageProvider.INFORMATION);
    }
    /**
     * @param parent
     *            Composite
     */
    public void createControl(Composite parent)
    {
        Composite baseComposite = new Composite(parent, SWT.NONE);

        GridLayout gridLayout = new GridLayout();
        baseComposite.setLayout(gridLayout);
        ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
        java.util.List resourceList =  pdm.getCAModel().getEList();
        ClovisNotifyingListImpl rList = (ClovisNotifyingListImpl)
            ResourceDataUtils.getResourcesList(resourceList);
        ClassLoader loader = getClass().getClassLoader();
        _selList = new ClovisNotifyingListImpl();
        SelectionListView selView = new SelectionListView(baseComposite,
                SWT.NONE, _selList, rList, loader, false);
        selView.setLayoutData(new GridData(GridData.FILL_BOTH));
        this.setControl(baseComposite);
    }
    /**
     * Gets List of selected MOs.
     * @return list of slected MOs.
     */
    public NotifyingList getSelectedMOlist()
    {
        return  _selList;
    }
}
