/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/project/migration/MigrationInfoReader.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.project.migration;

import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.util.HashMap;
import java.util.List;
import java.util.StringTokenizer;
import java.util.Vector;

import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;


/**
 * 
 * @author shubhada
 *
 * Reader class to read the migration related data
 */
public class MigrationInfoReader
{
    private HashMap _versionMigrationListMap = new HashMap();
    private HashMap _versionHandlerListMap = new HashMap();
    private List _versionOrderList = new Vector();
    private static final Log LOG = Log.getLog(DataPlugin.getDefault());
    /**
     * constructor
     * @param oldVersion - Old version of projects to be migrated
     * @param newVersion - New version to which projects have to be migrated
     *
     */
    public MigrationInfoReader(String oldVersion, String newVersion)
    {
        readMigrationInfo(oldVersion, newVersion);
    }
    /**
     * Reads the Migration related info from DataPlugin
     * @param oldVersion - Existing Vesion for migration
     * @param newVersion - New version to be migrated to 
     *
     */
    private void readMigrationInfo(String oldVersion, String newVersion)
    {
        URL url = DataPlugin.getDefault().getBundle().getEntry("/");
        try {
            url = Platform.resolve(url);
            String migrationFileName = ICWProject.
                MIGRATION_XMI_DATA_FILENAME + "_"
                + oldVersion + "_" + newVersion + ".xmi";
            String fileName = url.getFile() + ICWProject.PLUGIN_MIGRATION_FOLDER + File.separator
                + "old" + File.separator + migrationFileName;
            URI uri = URI.createFileURI(fileName);
            File file = new File(fileName);
            if (file.exists()) {
	            List migrationInfoList = (NotifyingList) EcoreModels.read(uri);
	            if (!migrationInfoList.isEmpty()) {
	                List migrationList = (List) EcoreUtils.getValue(
	                        (EObject) migrationInfoList.get(0), "changeInfo");
	                List handlerList = (List) EcoreUtils.getValue(
	                        (EObject) migrationInfoList.get(0), "migrationHandler");
	                _versionMigrationListMap.put(
	                		migrationFileName, migrationList);
	                _versionHandlerListMap.put(migrationFileName, handlerList);
	                _versionOrderList.add(migrationFileName);
	            }
            } else {
            	URL migrationURL = DataPlugin.getDefault().find(
                        new Path(ICWProject.PLUGIN_MIGRATION_FOLDER + File.separator
                                + "old"));
                Path migrationFilesPath = new Path(Platform.resolve(migrationURL).getPath());
                File migrationDir  = new File(migrationFilesPath.toOSString());
                File[] files   = migrationDir.listFiles();
                File startFile = null, endFile = null;
                String nextStartVersion = null;
                HashMap<String, File> startVersionFileMap = new HashMap<String, File>();
                for (int i = 0; i < files.length; i++) {
                	File migrationFile = files[i];
                	String name = migrationFile.getName();
                	StringTokenizer tokenizer = new StringTokenizer(name, "_");
                	if (tokenizer.countTokens() == 3) {
	                	tokenizer.nextToken();
	                	String startVersion = tokenizer.nextToken();
	                    String endVersion = tokenizer.nextToken();
	                    endVersion = endVersion.replace(".xmi", "");
	                    startVersionFileMap.put(startVersion, migrationFile);
	                    if (startVersion.equals(oldVersion)) {
	                    	startFile = migrationFile;
	                    	nextStartVersion = endVersion;
	                    }
	                    if (endVersion.equals(newVersion)) {
	                    	endFile = migrationFile;
	                    }
                	}
                }
            
                if (startFile != null && endFile != null) {
                	uri = URI.createFileURI(startFile.getAbsolutePath());
            List migrationInfoList = (NotifyingList) EcoreModels.read(uri);
            if (!migrationInfoList.isEmpty()) {
    	                List migrationList = (List) EcoreUtils.getValue(
    	                        (EObject) migrationInfoList.get(0), "changeInfo");
    	                List handlerList = (List) EcoreUtils.getValue(
    	                        (EObject) migrationInfoList.get(0), "migrationHandler");
    	                String name = startFile.getName();
    	                _versionMigrationListMap.put(name, migrationList);
    	                _versionHandlerListMap.put(name, handlerList);
    	                _versionOrderList.add(name);
    	            }
                }
                File nextFile = startVersionFileMap.get(nextStartVersion);
                while (nextFile != null) {
                	String name = nextFile.getName();
                	StringTokenizer tokenizer = new StringTokenizer(name, "_");
                	tokenizer.nextToken();
                	tokenizer.nextToken();
                    String endVersion = tokenizer.nextToken();
                    endVersion = endVersion.replace(".xmi", "");
                	nextStartVersion = endVersion;
                	uri = URI.createFileURI(nextFile.getAbsolutePath());
                	List migrationInfoList = (NotifyingList) EcoreModels.read(uri);
    	            if (!migrationInfoList.isEmpty()) {
    	                List migrationList = (List) EcoreUtils.getValue(
                        (EObject) migrationInfoList.get(0), "changeInfo");
    	                List handlerList = (List) EcoreUtils.getValue(
    	                        (EObject) migrationInfoList.get(0), "migrationHandler");
    	                _versionMigrationListMap.put(name, migrationList);
    	                _versionHandlerListMap.put(name, handlerList);
    	                _versionOrderList.add(name);
            }
    	            if (endVersion.equals(newVersion)) {
    	            	break;
                    }
    	            nextFile = startVersionFileMap.get(nextStartVersion);
                	
                }
            }
        } catch (IOException e) {
            LOG.error("Error reading Migration Information", e);
        }
        
    }
    /**
     * 
     * @return the 
     * Version and Migration Info
     */
    public HashMap getMigrationInfo()
    {
        return _versionMigrationListMap;
    }
    /**
     * 
     * @return the 
     * Version and Migration Info
     */
    public HashMap getHandlerMap()
    {
        return _versionHandlerListMap;
    }
    /**
     * 
     * @return the Version Order List
     */
    public List getVersionOrderList()
    {
    	return _versionOrderList;
    }
}
