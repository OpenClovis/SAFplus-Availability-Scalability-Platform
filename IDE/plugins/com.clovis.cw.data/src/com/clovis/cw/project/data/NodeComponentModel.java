/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.data/src/com/clovis/cw/project/data/NodeComponentModel.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.project.data;

import java.io.File;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;
import java.lang.System;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.resource.Resource;

import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;

/**
 * @author pushparaj Model class for Component's Node
 */
public class NodeComponentModel
{
    private IProject         _project;

    private String           _name;

    private ArrayList        _systemComps       = new ArrayList();

    private static final Log LOG = Log.getLog(DataPlugin.getDefault());
    /**
     * Constructor
     * @param project
     *            Project
     * @param name
     *            Node Name
     */
    public NodeComponentModel(IProject project, String name)
    {
        _project = project;
        _name    = name;
        createSystemComponentResources();
    }
    /**
     * Sets node name
     * @param name
     *            Name
     */
    public void setName(String name)
    {
        _name = name;
    }
    /**
     * Returns name
     * @return Node Name.
     */
    public String getName()
    {
        return _name;
    }
    /**
     * Returns System component's resource list
     * @return ArrayList
     */
    public ArrayList getSystemComponentResources()
    {
        if (_systemComps  == null) {
            createSystemComponentResources();
        }
        return _systemComps;
    }
    /**
     * Creats resources list for boot time component's ecore
     */
    private void createSystemComponentResources()
    {
        //This needs to be cleaned
        URL url = DataPlugin
                .getDefault()
                .find(new Path("model" + File.separator + ICWProject
                                .CW_ASP_COMPONENTS_CONFIGS_FOLDER_NAME
                                + File.separator + ICWProject
                                .CW_SYSTEM_COMPONENTS_FOLDER_NAME));
        try {
            File systemFolder = new Path(Platform.resolve(url).getPath()).toFile();
            File ecoreFiles[] = systemFolder.listFiles(); /* Skip directories */

            for (int i = 0; i < ecoreFiles.length; i++) {
                // System.out.println("file is " + ecoreFiles[i]);
            	if(!ecoreFiles[i].toString().endsWith(".ecore")) continue;

                Path path = new Path("model" + File
                        .separator + ICWProject
                        .CW_ASP_COMPONENTS_CONFIGS_FOLDER_NAME
                        + File.separator + ICWProject
                        .CW_SYSTEM_COMPONENTS_FOLDER_NAME + File
                        .separator + ecoreFiles[i].getName());
                // System.out.println("path is " + path);
                url = DataPlugin.getDefault().find(path);
                // System.out.println("URL is " + url);

                File ecoreFile = new Path(Platform.resolve(url).getPath()).toFile();
                if(!ecoreFile.isFile())
                	continue;
                EPackage pack = EcoreModels.get(ecoreFile.getAbsolutePath());
                EClass eClass = (EClass) pack.getEClassifier("BootConfig");
                String hidden = EcoreUtils.getAnnotationVal(eClass, null, "isHidden");
                if (hidden != null) {
                    boolean isHidden = Boolean.parseBoolean(hidden);
                    if (isHidden) {
                        continue;
                    }
                }
                String dataFilePath = _project.getLocation().toOSString()
                                      + File.separator
                                      + ICWProject
                                      .CW_PROJECT_CONFIG_DIR_NAME
                                      + File.separator + pack.getName()
                                      + ".xml";
                URI uri = URI.createFileURI(dataFilePath);
                File xmiFile = new File(dataFilePath);

                Resource systemComps = xmiFile.exists() ? EcoreModels
                        .getUpdatedResource(uri) : EcoreModels.create(uri);
                List contents = systemComps.getContents();
                if (contents.isEmpty()) {
                    EObject obj = EcoreUtils.createEObject((EClass) pack
                            .getEClassifier("BootConfig"), true);
                    systemComps.getContents().add(obj);
                }
                EObject obj = (EObject) contents.get(0);
                List features = obj.eClass().getEAllStructuralFeatures();
                for (int j = 0; j < features.size(); j++) {
                    EReference ref = (EReference) features.get(j);
                    EObject refObj = (EObject) obj.eGet(ref);
                    if (refObj == null) {
                        obj.eSet(ref, EcoreUtils.createEObject(ref
                                .getEReferenceType(), true));
                    }
                }
                _systemComps.add(systemComps.getContents().get(0));
            }
        } catch (Exception e) {
            LOG.error("Error loading system configuration for " + _name, e);
        }
    }
}
