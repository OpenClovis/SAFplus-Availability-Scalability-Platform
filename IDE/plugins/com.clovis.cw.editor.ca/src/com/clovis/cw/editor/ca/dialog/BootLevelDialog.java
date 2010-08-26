/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/dialog/BootLevelDialog.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.dialog;

import java.util.List;
import java.util.ListIterator;
import java.util.Vector;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.HelpEvent;
import org.eclipse.swt.events.HelpListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Cursor;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Link;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.swt.widgets.ToolItem;
import org.eclipse.ui.PlatformUI;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.editor.ca.CaPlugin;

/**
 * @author shubhada
 *
 * Shows the details of Boot Levels of each Boot Configuration
 */
public class BootLevelDialog extends PreferenceDialog
{
    private PreferenceManager _preferenceManager;
    private EObject           _bootConfigObj = null;
    private Model             _viewModel     = null;
    private Model             _model     = null;
    private int               _maxbootlevel  = 0;
    private String 			  _contextHelpId = null;
    private static final Log LOG = Log.getLog(CaPlugin.getDefault());
    /**
     * @param parentShell - Shell
     * @param manager - Preference Manager
     * @param eobj - EObject
     */
    public BootLevelDialog(Shell parentShell, PreferenceManager manager,
            EObject eobj)
    {
        super(parentShell, manager);
        _preferenceManager = manager;
        _model        = new Model(null, eobj);
        _viewModel         = _model.getViewModel();
        _bootConfigObj     = _viewModel.getEObject();
        _contextHelpId     = EcoreUtils.getAnnotationVal(_bootConfigObj.eClass(), null, "Help");
        addPreferenceNodes();
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
     * Adds the nodes to tree on the left side of the dialog
     *
     */
    private void addPreferenceNodes()
    {
        _maxbootlevel = ((Integer) EcoreUtils.
                getValue(_bootConfigObj, "maxbootlevel")).intValue();
        EReference bootLevelsRef = (EReference) _bootConfigObj.eClass().
                getEStructuralFeature("bootlevels");
        EObject bootlevelObj = (EObject) _bootConfigObj.eGet(bootLevelsRef);
        EReference bootLevelRef = (EReference) bootLevelsRef.
                getEReferenceType().getEStructuralFeature("bootlevel");
        EList bootLevelList = (EList) bootlevelObj.eGet(bootLevelRef);
        if (bootLevelList.isEmpty()) {
            for (int i = 0; i < _maxbootlevel; i++) {
                EObject bootObj = EcoreUtils.createEObject(bootLevelRef.
                        getEReferenceType(), true);
                bootLevelList.add(bootObj);
            }
        }

        EObject cpmObj = _model.getEObject().eContainer().eContainer();
        EObject cpmConfObj = (EObject) EcoreUtils.
                getValue(cpmObj, "cpmconfig");
        List suinstList = (List) EcoreUtils.
                getValue(cpmConfObj, "suinst");
        List originalList = new Vector();
        for (int i = 0; i < suinstList.size(); i++) {
            EObject eobj = (EObject) suinstList.get(i);
            String suName = (String) EcoreUtils.getValue(eobj, "name");
            originalList.add(suName);
        }
        List origAspSusList = new Vector();
        origAspSusList.add("eventSU");
        origAspSusList.add("nameSU");
        origAspSusList.add("ckptSU");
        origAspSusList.add("corSU");
        origAspSusList.add("oampSU");
        origAspSusList.add("cmSU");
        origAspSusList.add("snmpSU");
        origAspSusList.add("debugSU");
       if (_maxbootlevel > bootLevelList.size()) {
    	   int diff = _maxbootlevel - bootLevelList.size();
           for (int i = 0; i < diff; i++) {
               EObject bootObj = EcoreUtils.createEObject(bootLevelRef.
                       getEReferenceType(), true);
               bootLevelList.add(bootObj);
           }
       } else if (_maxbootlevel < bootLevelList.size()) {
    	   ListIterator iterator = bootLevelList.listIterator();
    	   while (iterator.hasNext()) {
    		   EObject bootObj = (EObject) iterator.next();
    		   if (bootLevelList.indexOf(bootObj) >= _maxbootlevel) {
    			   iterator.remove();
    		   }
    	   }
       }
       for (int i = 0; i < bootLevelList.size(); i++) {
           EObject bootObj = (EObject) bootLevelList.get(i);
           EcoreUtils.setValue(bootObj, "level", String.valueOf(i + 1));
           EObject susObj = (EObject) EcoreUtils.getValue(bootObj, "sus");
           EObject aspsusObj = (EObject) EcoreUtils.
               getValue(bootObj, "aspsus");
           List suList = (List) EcoreUtils.getValue(susObj, "su");
           List aspsuList = (List) EcoreUtils.getValue(aspsusObj, "su");
           String pageName = "BootLevel " + (i + 1);
           PreferencePage page = new BootLevelsPage(pageName, suList,
                   originalList, aspsuList, origAspSusList);
           _preferenceManager.addToRoot(new PreferenceNode(pageName, page));
       }

    }
    protected Control createContents(Composite parent) {
		Control control = super.createContents(parent);
		if (_contextHelpId != null) {
			control.addHelpListener(new HelpListener() {
				public void helpRequested(HelpEvent e) {
					PlatformUI.getWorkbench().getHelpSystem().displayHelp(
							_contextHelpId);
				}
			});
		}
		return control;
	}
    /***** This code needs to be removed in future ******/
    protected Control createHelpControl(Composite parent) {
		Image helpImage = JFaceResources.getImage(DLG_IMG_HELP);
		if (helpImage != null) {
			return createHelpImageButton(parent, helpImage);
		}
		return createHelpLink(parent);
    }
	private ToolBar createHelpImageButton(Composite parent, Image image) {
        ToolBar toolBar = new ToolBar(parent, SWT.FLAT | SWT.NO_FOCUS);
        ((GridLayout) parent.getLayout()).numColumns++;
		toolBar.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_CENTER));
		final Cursor cursor = new Cursor(parent.getDisplay(), SWT.CURSOR_HAND);
		toolBar.setCursor(cursor);
		toolBar.addDisposeListener(new DisposeListener() {
			public void widgetDisposed(DisposeEvent e) {
				cursor.dispose();
			}
		});		
        ToolItem item = new ToolItem(toolBar, SWT.NONE);
		item.setImage(image);
		item.setToolTipText(JFaceResources.getString("helpToolTip")); //$NON-NLS-1$
		item.addSelectionListener(new SelectionAdapter() {
            public void widgetSelected(SelectionEvent e) {
            	PlatformUI.getWorkbench().getHelpSystem().displayHelp(_contextHelpId);
            }
        });
		return toolBar;
	}
	private Link createHelpLink(Composite parent) {
		Link link = new Link(parent, SWT.WRAP | SWT.NO_FOCUS);
        ((GridLayout) parent.getLayout()).numColumns++;
		link.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_CENTER));
		link.setText("<a>"+IDialogConstants.HELP_LABEL+"</a>"); //$NON-NLS-1$ //$NON-NLS-2$
		link.setToolTipText(IDialogConstants.HELP_LABEL);
		link.addSelectionListener(new SelectionAdapter() {
            public void widgetSelected(SelectionEvent e) {
            	PlatformUI.getWorkbench().getHelpSystem().displayHelp(_contextHelpId);
            }
        });
		return link;
	}
	/***** This code needs to be removed in future ******/
}
