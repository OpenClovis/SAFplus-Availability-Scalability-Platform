/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/snmp/SnmpExportWizard.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.editor.ca.snmp;

import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.util.List;

import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.wizard.Wizard;
import org.eclipse.jface.wizard.WizardPage;
import org.eclipse.core.resources.IContainer;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.resource.Resource;

import com.clovis.common.utils.Python;
import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.editor.ca.constants.SnmpConstants;
/**
 * @author shubhada Wizard for SNMP export
 *
 */
public class SnmpExportWizard extends Wizard
{
    private IContainer _projResource;
    private EClass     _detailsClass = null;
    private Resource   _resource = null;
    private static final Log LOG = Log.getLog(CaPlugin.getDefault());

    /**
     * Constructor
     * @param proj IContainer
     */
    public SnmpExportWizard(IContainer proj)
    {
        _projResource = proj;
        readEcoreFile();
        readResource();
        addPage(new SnmpExportDetailsPage("1", this));
        addPage(new MoSelectionPage("3", proj));
    }
    /**
     * Need previous and Next Button in the wizard
     * @return true
     */
    public boolean needsPreviousAndNextButtons()
    {
        return true;
    }
    /**
     * click on finish implementation
     * @return whether perform finish is successfull
     */
    public boolean performFinish()
    {
        try {
            EcoreModels.save(_resource);
        } catch (IOException e) {
            LOG.error("Export Data cannot be saved", e);
        }
        EObject detailsObj = (EObject) _resource.getContents().get(0);
        boolean oidOverride = ((Boolean) EcoreUtils.
            getValue(detailsObj, "generateNewOIDS")).booleanValue();
        ResourceIdMapUtil idMapUtil = new ResourceIdMapUtil(_projResource);
        List resList = ((MoSelectionPage) getPage("3")).getSelectedMOlist();
        idMapUtil.updateMOList(resList, oidOverride);
        String args[] = new String[11 + resList.size()];
        for (int i = 0; i < resList.size(); i++) {
            EObject eobj = (EObject) resList.get(i);
            String key = (String) EcoreUtils.getValue(eobj,
            		ModelConstants.RDN_FEATURE_NAME);
            args[11 + i] = key;
        }
         URL url = DataPlugin.getDefault().getBundle().getEntry("/");
         try {
            url = Platform.resolve(url);
        } catch (IOException e) {
            LOG.error("Cannot get the URL of the file", e);
            return false;
        }
        String dirName = (String) EcoreUtils.getValue(
                detailsObj, "mibFileLocation");
        File pythonFile = new File(dirName);
        if (!pythonFile.isDirectory()) {
            MessageDialog.openError(getShell(), "SNMP Export validation",
                    "Specified mib file location is not a valid path, "
                    + "please specify a valid path");
            return false;
        }
        String scriptDir = _projResource.getLocation().toOSString()
            + File.separator + ICWProject.PROJECT_SCRIPT_FOLDER;
        args[0] = scriptDir + File.separator + "mibexport.py";
        args[4] = (String) EcoreUtils.getValue(detailsObj, "prefix");
        args[5] = (String) EcoreUtils.getValue(detailsObj, "orgName");
        args[6] = (String) EcoreUtils.getValue(detailsObj, "contactInfo");
        args[7] = ((Long) EcoreUtils.
                    getValue(detailsObj, "enterpriseOID")).toString();
        args[8] = (String) EcoreUtils.getValue(detailsObj, "moduleName");
        String fileName = (String) EcoreUtils.getValue(detailsObj, "fileName");
        args[9] = dirName + File.separator + fileName;
        args[10] = String.valueOf(oidOverride);
        String modelDir = _projResource.getLocation().toOSString()
            + File.separator + ICWProject.CW_PROJECT_MODEL_DIR_NAME
            + File.separator;
        args[2] = modelDir + SnmpConstants.MO_XMI_FILE_NAME;
        args[3] = modelDir + ICWProject.RESOURCE_XML_DATA_FILENAME;
        args[1] = url.getFile() + ICWProject.PLUGIN_XML_FOLDER + File.separator
        + SnmpConstants.DATA_TYPES_XMI_FILE_NAME;
        try {
            Python.main(args);
        } catch (Exception e1) {
            LOG.error("Unhandled Error during exporting Mib", e1);
        }
        return true;
    }
    /**
     * reads Ecore Files
     *
     */
    private void readEcoreFile()
    {
        try {
            URL cpmURL = DataPlugin.getDefault().find(new Path("model"
                    + File.separator + SnmpConstants.EXPORT_ECORE_FILE_NAME));
            File ecoreFile = new Path(Platform.resolve(cpmURL).getPath())
                    .toFile();
            EPackage detailsPackage = EcoreModels.get(
                ecoreFile.getAbsolutePath());
            _detailsClass = (EClass) detailsPackage.getEClassifier("details");
        } catch (IOException ex) {
            LOG.error("Export Details Ecore File cannot be read", ex);
        }
    }
    /**
     * read the xmi file
     *
     */
    private void readResource()
    {
        try {
//            Now get the xmi file from the project which may or
            //may not have data
            String dataFilePath = _projResource.getLocation().toOSString()
                + File.separator + ICWProject.CW_PROJECT_MODEL_DIR_NAME
                + File.separator + SnmpConstants.EXPORT_XMI_FILE_NAME;
            URI uri = URI.createFileURI(dataFilePath);
            File xmiFile = new File(dataFilePath);

            _resource = xmiFile.exists()
                ? EcoreModels.getResource(uri) : EcoreModels.create(uri);
            List detailsList = _resource.getContents();
            if (detailsList.isEmpty()) {
                EObject detailsObj =
                    EcoreUtils.createEObject(_detailsClass, true);
                detailsList.add(detailsObj);
            }
        } catch (Exception exception) {
            LOG.error("Error while reading export data file", exception);
        }
    }
    /**
     *
     * @return Details EObject
     */
    public EObject getDetailsObject()
    {
        return (EObject) _resource.getContents().get(0);
    }

}
