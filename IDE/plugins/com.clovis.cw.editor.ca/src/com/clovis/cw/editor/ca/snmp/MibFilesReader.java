/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/snmp/MibFilesReader.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.editor.ca.snmp;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;

import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.notify.Notifier;
import org.eclipse.emf.common.notify.impl.AdapterImpl;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.editor.ca.CaPlugin;

/**
 * @author shubhada
 *Singleton class which will read the existing mib files and keeps them in a
 *Notifying List which will send notifications on addition or deletion
 */
public class MibFilesReader
{
    private static MibFilesReader instance = null;
    private static final Log LOG = Log.getLog(CaPlugin.getDefault());

    private ClovisNotifyingListImpl _mibfilenames = new
                                     ClovisNotifyingListImpl();
    private ClovisNotifyingListImpl _treeObjects  = new
                                     ClovisNotifyingListImpl();
    private ClovisNotifyingListImpl _tableObjects = new
                                     ClovisNotifyingListImpl();
    private Shell _shell = null;
    
    private FileListListener _listener= new FileListListener();
    /**
     * constructor
     *
     */
    protected MibFilesReader()
    {

        ((Notifier) _mibfilenames.getNotifier()).eAdapters().add(_listener);
        readMibfiles();
    }
    /**
     *
     * @return the single instance of the class.
     */
    public static MibFilesReader getInstance()
    {
        if (instance == null) {
            instance = new MibFilesReader();
        }
        return instance;
    }
    /**
     * removes the attchached listeners
     *
     */
    public void removeListeners()
    {
        ((Notifier) _mibfilenames.getNotifier()).eAdapters().remove(_listener);
    }
    /**
     * Reads Previously loaded mibs from filesystem
     *
     */
    private  void readMibfiles()
    {
        try {
        	File mibInfoFile = new File(Platform.getInstanceLocation()
                    .getURL().getPath().concat("mibfilenames.txt"));
        	if (mibInfoFile.exists()) {
	            FileReader file = new FileReader(Platform.getInstanceLocation()
	                    .getURL().getPath().concat("mibfilenames.txt"));
	            
	            if (file != null) {
	                BufferedReader reader = new BufferedReader(file);
	
	                String filename = reader.readLine();
	                while (filename != null) {
	                    _mibfilenames.add(filename);
	                    filename = reader.readLine();
	                }
	                file.close();
	                reader.close();
	            }
        	}
        } catch (IOException e) {
            LOG.error("MibFile cannot be opened for reading", e);
        }
    }
    /**
     *
     * @return the list of mibfilenames loaded.
     */
    public ClovisNotifyingListImpl getFileNames()
    {
        return _mibfilenames;
    }
    /**
     *
     * @return the input to the tree.
     */
    public ClovisNotifyingListImpl getTreeInput()
    {
        return _treeObjects;
    }
    /**
     *
     * @return the input to the table.
     */
    public ClovisNotifyingListImpl getTableInput()
    {
        return _tableObjects;
    }
    /**
     *
     * @param shell - Shell
     * sets the shell
     */
    public void setShell(Shell shell)
    {
        _shell = shell;
    }
    /**
     *
     * @author shubhada
     *Listener which listenes to addition and removal of filenames in the list.
     */
    class FileListListener extends AdapterImpl
    {
        /**
         * @param msg -Notification
         */
        public void notifyChanged(Notification msg)
        {
            EObject node = null;
            switch (msg.getEventType()) {
            case Notification.ADD:
                ClovisMibUtils.setShell(_shell);
                node = ClovisMibUtils.populateNode(_mibfilenames);
                if (node != null) {
                    _treeObjects.clear();
                    _treeObjects.add(node);
                    String newfile =  (String) msg.getNewValue();
                    EObject newObject =
                        ClovisMibUtils.populateUINode(newfile);
                    if (newObject != null) {
                    _tableObjects.add(newObject);
                    }
                } else {
                    String newfile =  (String) msg.getNewValue();
                    _mibfilenames.remove(newfile);
                }

                break;
            case Notification.REMOVE:
                ClovisMibUtils.setShell(_shell);
                node = ClovisMibUtils.populateNode(_mibfilenames);
                _treeObjects.clear();
                _treeObjects.add(node);
                String  oldfile = (String) msg.getOldValue();
                EObject eobj = ClovisMibUtils.getEObject(oldfile);
                _tableObjects.remove(eobj);
                ClovisMibUtils.updateMap(oldfile);
                break;
            }
        }
    }
}
