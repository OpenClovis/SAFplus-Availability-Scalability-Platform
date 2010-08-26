/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/snmp/LoadAction.java $
 * $Author: srajyaguru $
 * $Date: 2007/04/30 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.editor.ca.snmp;


import java.io.File;
import java.io.IOException;
import java.lang.Boolean;
import java.lang.Throwable;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.UtilsPlugin;
import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.common.utils.menu.IActionClassAdapter;
import com.ireasoning.protocol.snmp.MibUtil;
import com.ireasoning.util.MibParseException;
import com.ireasoning.util.MibTreeNode;

/**
 * @author ravik
 *
 * Action class to load the mib in to the tree.
 */
public class LoadAction extends IActionClassAdapter
{
    private String []  _fileNames;
     private static final Log LOG = Log.getLog(UtilsPlugin.getDefault());
     /**
      * @param args 0- Shell
      * param args 1- TableViewer
      *  param args 2- TreeViewer
      * @return whether Action is successfull.
      */
    public boolean run(Object[] args)
    {

        Shell shell = (Shell) args[0];
        
        TreeViewer treeViewer = (TreeViewer) args[2];

        int style = SWT.MULTI;
        FileDialog fileselDialog = new FileDialog(shell, style);
        fileselDialog.setFilterPath(UtilsPlugin.getDialogSettingsValue("LOADMIB"));
        fileselDialog.open();
        _fileNames = fileselDialog.getFileNames();
        ClovisNotifyingListImpl mibnames = MibFilesReader.getInstance().
        getFileNames();
        for (int i = 0; i < _fileNames.length; i++) {
        	UtilsPlugin.saveDialogSettings("LOADMIB", fileselDialog.getFilterPath());
            _fileNames[i] = fileselDialog.getFilterPath() + File.separator
            + _fileNames[i];
            int isMibloaded  = ismibfileLoaded(mibnames, _fileNames[i]);
            if (isMibloaded == -1) {
            	continue;
            } else if (isMibloaded == 1) {
                mibnames.add(_fileNames[i]);
            }
            
        }
        ClovisNotifyingListImpl input = MibFilesReader.getInstance().
        getTreeInput();
        if (treeViewer != null) {
        	treeViewer.setInput(input);
        }

    return true;
    }

    /**
     * @param mibfilenames Vector
     * @param fname String
     * @return int return 0 if mibfile is already loaded.
     *  else return 1, return -1 on error
     */
    private int ismibfileLoaded(ClovisNotifyingListImpl mibfilenames,
            String fname)
    {
        MibTreeNode node = null;
        try {
        	MibUtil.unloadAllMibs();
		node = MibUtil.parseMib(fname,false);
            if (node == null) {
            	MessageDialog.openError(
                        null, "Mib File Loading Error", "Could not load MIB file. Error occured while parsing MIB file");
                return -1;
            } 
        } catch (MibParseException e) {
        	MessageDialog.openError(
                    null, "A Mib File Parsing Error", e.toString());
                LOG.error("Exception has occured" + (new Throwable().getStackTrace()).toString(), e);
            return -1;
        } catch (IOException e) {
             LOG.error("Could not load the MibFile. IO Exception has occured", e);
             return -1;
        }

        for (int i = 0; i < mibfilenames.size(); i++) {
            EObject eobj = ClovisMibUtils.getEObject((String)
                    mibfilenames.get(i));
            if (eobj != null && node != null) {
            String modulename = (String) EcoreUtils.getValue(eobj, "MibName");

                if (node.getModuleName().equals(modulename)) {
                    return 0;
                }
            }

        }

        return 1;
    }
}
