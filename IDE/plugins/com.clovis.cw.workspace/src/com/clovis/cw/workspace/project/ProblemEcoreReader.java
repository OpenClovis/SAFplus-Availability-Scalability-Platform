/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/project/ProblemEcoreReader.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.project;

import java.io.File;
import java.net.URL;

import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EPackage;

import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.workspace.WorkspacePlugin;
/**
 * 
 * @author shubhada
 *
 * 
 */
public class ProblemEcoreReader
{
    private EClass _problemClass = null;
    private static ProblemEcoreReader instance = null;
    private static final Log LOG = Log.getLog(WorkspacePlugin.getDefault());
    /**
     * constructor
     *
     */
    protected ProblemEcoreReader()
    {
        readEcoreFile();
        
    }
    /**
     *
     * @return the single instance of the class.
     */
    public static ProblemEcoreReader getInstance()
    {
        if (instance == null) {
            instance = new ProblemEcoreReader();
        }
        return instance;
    }
    /**
     * reads the problem.ecore to get the Problem EClass
     *
     */
    private void readEcoreFile()
    {
        URL url = DataPlugin.getDefault().find(
                new Path("model" + File.separator
                         + ICWProject.PROBLEM_ECORE_FILENAME));
        try {
            File ecoreFile = new Path(Platform.resolve(url).getPath())
                    .toFile();
            EPackage pack = EcoreModels.get(ecoreFile.getAbsolutePath());
            _problemClass = (EClass) pack.getEClassifier("Problem");
        } catch (Exception e) {
            LOG.warn("Cannot read the problems ecore file", e);
        }
        
    }
    /**
     * 
     * @return the Problem EClass
     */
    public EClass getProblemClass()
    {
        return _problemClass;
    }
}
