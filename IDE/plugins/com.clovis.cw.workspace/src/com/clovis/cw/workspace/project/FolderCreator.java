/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.project;


import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.URISyntaxException;
import java.net.URL;
import java.util.Iterator;
import java.util.List;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IFolder;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.ResourceAttributes;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.FileLocator;
import org.eclipse.core.runtime.IPath;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.actions.CopyFilesAndFoldersOperation;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

import com.clovis.common.utils.constants.ProjectConstants;
import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.WorkspacePlugin;
import com.clovis.cw.workspace.migration.MigrationUtils;
/**
 * @author root
 *
 * To change the template for this generated type comment go to
 * Window&gt;Preferences&gt;Java&gt;Code Generation&gt;Code and Comments
 */
public class FolderCreator implements ICWProject
{
    private final IProject _project;
    // List of folders to be created in new Project.
    private static final String[] FOLDERS = new String[] {
        CW_PROJECT_MODEL_DIR_NAME,
        CW_PROJECT_CONFIG_DIR_NAME,
        CW_PROJECT_SCRIPT_DIR_NAME,
        CW_PROJECT_IDL_DIR_NAME,
        CW_PROJECT_TEMPLATE_DIR_NAME,
    };
    /**
     * Constructor.
     * @param project IProject.
     */
    public FolderCreator(IProject project)
    {
        _project = project;
    }
    /**
     * Create All folders and files
     * @throws CoreException Error
     * @throws IOException   Error
     * @throws URISyntaxException 
     */
    public void createAll()
        throws CoreException, IOException, URISyntaxException
    {
        createDefaultProjectFolders();
        copyScript();
        copyTemplates();
        copyCodeGenFiles();
        createDefaultModelFiles();
        createDefaultConfigFiles(false);
    }
    /**
     * Create Default folders.
     *
     * @param project
     * @throws CoreException
     */
    public void createDefaultProjectFolders()
        throws CoreException, IOException
    {
        for (int i = 0; i < FOLDERS.length; i++) {
            _project.getFolder(new Path(FOLDERS[i])).create(true, true, null);
        }
        /*_project.getFolder(
				new Path(CW_PROJECT_TEMPLATE_DIR_NAME)
						.append(CW_PROJECT_DEFAULT_TEMPLATE_GROUP_DIR_NAME))
				.create(true, true, null);*/
        _project.getFolder(new Path(FOLDERS[0])).getFolder(new Path(ICWProject.
                RESOURCE_TEMPLATE_FOLDER)).create(true, true, null);
        _project.getFolder(new Path(FOLDERS[0])).getFolder(new Path(ICWProject.
                COMPONENT_TEMPLATE_FOLDER)).create(true, true, null);
    }
    /**
     * Copy script and template files in to project
     * @throws CoreException
     * @throws IOException
     * @throws URISyntaxException 
     */
    public void copyCodeGenFiles() throws CoreException, IOException, URISyntaxException {
    	URL codegenURL = FileLocator.find(WorkspacePlugin.getDefault().getBundle(),
				new Path(PROJECT_CODEGEN_FOLDER), null);
    	Path codegenPath = new Path(FileLocator.resolve(codegenURL).getPath());
    	final String uris[] = new String[1];
    	uris[0] = codegenPath.toOSString();
    	Display.getDefault().syncExec(new Runnable() {
			public void run() {
				final CopyFilesAndFoldersOperation op = new CopyFilesAndFoldersOperation(new Shell());
				op.copyFiles(uris, _project);
				
			}});
    }
    /**
     * Copy script and build files into project.
     * @param project        Project
     * @throws CoreException In create
     * @throws IOException   File IO
     */
    public void copyScript()
        throws CoreException, IOException
    {
        //Copy Scripts to Project.
        /*URL scriptsURL = WorkspacePlugin.getDefault().find(
                new Path(PROJECT_SCRIPT_FOLDER));
        Path scriptsPath = new Path(Platform.resolve(scriptsURL).getPath());
        File scriptsDir  = new File(scriptsPath.toOSString());
        File[] scripts   = scriptsDir.listFiles();
        IPath dstPrefix  = new Path(PROJECT_SCRIPT_FOLDER);
        for (int i = 0; i < scripts.length; i++){
            if (scripts[i].isDirectory())
                continue;
            IFile dst = _project.getFile(
                    dstPrefix.append(scripts[i].getName()));
            if (dst.exists()) {
                dst.delete(true, true, null);
            }
            dst.create(new FileInputStream(scripts[i]), true, null);
        }
*/
    	URL buildURL = WorkspacePlugin.getDefault().find(
                new Path("builder"));
    	IPath builderPath = new Path(Platform.resolve(buildURL).getPath());
    	File builderDir = builderPath.toFile();
    	File files[] = builderDir.listFiles();
    	for (int i = 0; i < files.length; i++) {
    		if (files[i].isFile()) {
				IFile dstFile = _project.getFile(new Path(files[i].getName()));
				if (dstFile.exists()) {
					dstFile.delete(true, true, null);
				}
				dstFile.create(new FileInputStream(files[i].getAbsolutePath()),
						true, null);
				if (files[i].getName().endsWith(".sh")) {
					ResourceAttributes attributes = dstFile
							.getResourceAttributes();
					if (attributes != null) {
						attributes.setExecutable(true);
						dstFile.setResourceAttributes(attributes);
					}
				}
			}
    	}
        
    }
    /**
	 * Copies all the template files to the project
	 * 
	 * @throws CoreException
	 * @throws IOException
	 */
    public void copyTemplates()
    throws CoreException, IOException
{
    	//Copy Templates to Project.
    	/*URL templatesURL = WorkspacePlugin.getDefault().find(
                new Path(PROJECT_TEMPLATE_FOLDER).append(PROJECT_DEFAULT_TEMPLATE_GROUP_FOLDER));
        Path templatesPath = new Path(Platform.resolve(templatesURL).getPath());
        File templatesFile  = new File(templatesPath.append(".templates").toOSString());
		IPath dstPrefix  = new Path(PROJECT_TEMPLATE_FOLDER).append(PROJECT_DEFAULT_TEMPLATE_GROUP_FOLDER);
		IFile dstFile = _project.getFile(dstPrefix.append(".templates"));
		if (!dstFile.exists()) {
			dstFile.create(new FileInputStream(templatesFile), true, null);
		}
		IFolder saffolder = _project.getFolder(
				new Path(CW_PROJECT_TEMPLATE_DIR_NAME)
						.append(CW_PROJECT_DEFAULT_TEMPLATE_GROUP_DIR_NAME).append(PROJECT_DEFAULT_SAF_TEMPLATE_GROUP_FOLDER));
		if (!saffolder.exists()) {
			saffolder.create(true, true, null);
		}
        IFolder nsaffolder = _project.getFolder(
				new Path(CW_PROJECT_TEMPLATE_DIR_NAME)
						.append(CW_PROJECT_DEFAULT_TEMPLATE_GROUP_DIR_NAME).append(PROJECT_DEFAULT_NONSAF_TEMPLATE_GROUP_FOLDER));
		if (!nsaffolder.exists()) {
			nsaffolder.create(true, true, null);
		}		
    	copyTemplatesFolder(PROJECT_DEFAULT_SAF_TEMPLATE_GROUP_FOLDER);
    	copyTemplatesFolder(PROJECT_DEFAULT_NONSAF_TEMPLATE_GROUP_FOLDER);*/
        /*URL templatesURL = WorkspacePlugin.getDefault().find(
                new Path(PROJECT_TEMPLATE_FOLDER).append(PROJECT_DEFAULT_TEMPLATE_GROUP_FOLDER));
        Path templatesPath = new Path(Platform.resolve(templatesURL).getPath());
        File templatesDir  = new File(templatesPath.toOSString());
        File[] templates   = templatesDir.listFiles();
        IPath dstPrefix  = new Path(PROJECT_TEMPLATE_FOLDER).append(PROJECT_DEFAULT_TEMPLATE_GROUP_FOLDER);
        for (int i = 0; i < templates.length; i++) {
        	if (templates[i].isDirectory()){
        		continue;
        	}
        	IFile dst = _project.getFile(
                    dstPrefix.append(templates[i].getName()));
            if (dst.exists()) {
                dst.delete(true, true, null);
            }
            dst.create(new FileInputStream(templates[i]), true, null);
        }*/
        // copy resource templates to the project
        URL resourceURL = WorkspacePlugin.getDefault().find(
                new Path(RESOURCE_TEMPLATE_FOLDER));
        if (resourceURL != null) {
            Path resTemplatePath = new Path(Platform.resolve(resourceURL).getPath());
            File resTemplateDir  = new File(resTemplatePath.toOSString());
            File[] resTemplates   = resTemplateDir.listFiles();
            IPath dstPrefix  = new Path(CW_PROJECT_MODEL_DIR_NAME).
                append(RESOURCE_TEMPLATE_FOLDER);
            IFolder folder = _project.getFolder(new Path(FOLDERS[0])).getFolder(new Path(ICWProject.
                    RESOURCE_TEMPLATE_FOLDER));
            if (!folder.exists()) {
                folder.create(true, true, null);
            }
            for (int i = 0; i < resTemplates.length; i++) {
                IFile dst = _project.getFile(
                        dstPrefix.append(resTemplates[i].getName()));
                if (dst.exists()) {
                    dst.delete(true, true, null);
                }
                dst.create(new FileInputStream(resTemplates[i]), true, null);
            }
        }
//      copy component templates to the project
        URL componentURL = WorkspacePlugin.getDefault().find(
                new Path(COMPONENT_TEMPLATE_FOLDER));
        if (componentURL != null) {
            Path compTemplatePath = new Path(Platform.resolve(componentURL).getPath());
            File compTemplateDir  = new File(compTemplatePath.toOSString());
            File[] compTemplates   = compTemplateDir.listFiles();
            IPath dstPrefix  = new Path(CW_PROJECT_MODEL_DIR_NAME).append(COMPONENT_TEMPLATE_FOLDER);
            IFolder folder = _project.getFolder(new Path(FOLDERS[0])).getFolder(new Path(ICWProject.
                    COMPONENT_TEMPLATE_FOLDER));
            if (!folder.exists()) {
                folder.create(true, true, null);
            }
            for (int i = 0; i < compTemplates.length; i++) {
                IFile dst = _project.getFile(
                        dstPrefix.append(compTemplates[i].getName()));
                if (dst.exists()) {
                    dst.delete(true, true, null);
                }
                dst.create(new FileInputStream(compTemplates[i]), true, null);
            }
        }

}
    
    /**
     * Create Default Model Files.
     *
     * @param arg0 Project
     * @throws IOException
     */
    public void createDefaultModelFiles()
        throws CoreException, IOException
    {
        File resourceFile = _project.getLocation().append(
            CW_PROJECT_MODEL_DIR_NAME).
                append(RESOURCE_XML_DATA_FILENAME).toFile();
        if (!resourceFile.exists()) {
            URL xmiURL = DataPlugin.getDefault().find(
                new Path(PLUGIN_XML_FOLDER + File.separator
                     + RESOURCE_DEFAULT_XMI_FILENAME));

            Path xmiPath = new Path(Platform.resolve(xmiURL).getPath());
            String dataFilePath =
                CW_PROJECT_MODEL_DIR_NAME
                + File.separator
                + RESOURCE_XML_DATA_FILENAME;
            IFile dst = _project.getFile(new Path(dataFilePath));
            dst.create(new FileInputStream(xmiPath.toOSString()), true, null);
        }
        File componentFile = _project.getLocation().append(
                CW_PROJECT_MODEL_DIR_NAME).
                    append(COMPONENT_XMI_DATA_FILENAME).toFile();
        if (!componentFile.exists()) {
            URL xmiURL = DataPlugin.getDefault().find(
                new Path(PLUGIN_XML_FOLDER + File.separator
                    + COMPONENT_DEFAULT_XMI_FILENAME));

            Path xmiPath = new Path(Platform.resolve(xmiURL).getPath());
            String dataFilePath =
                CW_PROJECT_MODEL_DIR_NAME
                + File.separator + COMPONENT_XMI_DATA_FILENAME;
            IFile dst = _project.getFile(new Path(dataFilePath));
            dst.create(new FileInputStream(xmiPath.toOSString()), true, null);
        }
    }
    /**
     * Create Default Config Files.
     * @param updateFlag 
     *
     * @throws CoreException
     * @throws IOException
     */
    public void createDefaultConfigFiles(boolean updateFlag)
        throws CoreException, IOException
    {
    	boolean eoConfig = false, eoDef = false;

    	File compileConfigFile = _project.getLocation().append(
                CW_PROJECT_CONFIG_DIR_NAME).append(
                        COMPILE_CONFIGS_XMI_FILENAME).toFile();
        if (!compileConfigFile.exists()) {
            URL xmiURL = DataPlugin.getDefault().find(
                new Path(PLUGIN_XML_FOLDER + File.separator
                    + COMPILE_CONFIGS_XMI_FILENAME));

            Path xmiPath = new Path(Platform.resolve(xmiURL).getPath());
            String dataFilePath =
                CW_PROJECT_CONFIG_DIR_NAME
                + File.separator + COMPILE_CONFIGS_XMI_FILENAME;
            IFile dst = _project.getFile(new Path(dataFilePath));
            dst.create(new FileInputStream(xmiPath.toOSString()), true, null);
            addVersioningToFile(dst.getLocation().toOSString());
        }
        
        File logConfigFile = _project.getLocation().append(
                CW_PROJECT_CONFIG_DIR_NAME).append(
                        LOG_CONFIGS_XML_FILENAME).toFile();
        if (!logConfigFile.exists()) {
            URL xmiURL = DataPlugin.getDefault().find(
                new Path(PLUGIN_XML_FOLDER + File.separator
                    + LOG_DEFAULT_CONFIGS_XML_FILENAME));
            
            Path xmiPath = new Path(Platform.resolve(xmiURL).getPath());
            String dataFilePath =
                CW_PROJECT_CONFIG_DIR_NAME
                + File.separator + LOG_CONFIGS_XML_FILENAME;
            IFile dst = _project.getFile(new Path(dataFilePath));
            dst.create(new FileInputStream(xmiPath.toOSString()), true, null);
            addVersioningToFile(dst.getLocation().toOSString());
        }
        
        File clIocConfigFile = _project.getLocation().append(
                CW_PROJECT_CONFIG_DIR_NAME).append(
                        IOCBOOT_XML_DATA_FILENAME).toFile();
        if (!clIocConfigFile.exists()) {
            URL xmiURL = DataPlugin.getDefault().find(
                new Path(PLUGIN_XML_FOLDER + File.separator
                    + IOCBOOT_DEFAULT_XML_DATA_FILENAME));
            
            Path xmiPath = new Path(Platform.resolve(xmiURL).getPath());
            String dataFilePath =
                CW_PROJECT_CONFIG_DIR_NAME
                + File.separator + IOCBOOT_XML_DATA_FILENAME;
            IFile dst = _project.getFile(new Path(dataFilePath));
            dst.create(new FileInputStream(xmiPath.toOSString()), true, null);
            addVersioningToFile(dst.getLocation().toOSString());
        }
        
        File clEoConfigFile = _project.getLocation().append(
                CW_PROJECT_CONFIG_DIR_NAME).append(
                		EOCONFIG_XML_DATA_FILENAME).toFile();
        if (!clEoConfigFile.exists()) {
            URL xmiURL = DataPlugin.getDefault().find(
                new Path(PLUGIN_XML_FOLDER + File.separator
                    + EOCONFIG_DEFAULT_XML_DATA_FILENAME));
            
            Path xmiPath = new Path(Platform.resolve(xmiURL).getPath());
            String dataFilePath =
                CW_PROJECT_CONFIG_DIR_NAME
                + File.separator + EOCONFIG_XML_DATA_FILENAME;
            IFile dst = _project.getFile(new Path(dataFilePath));
            dst.create(new FileInputStream(xmiPath.toOSString()), true, null);
            addVersioningToFile(dst.getLocation().toOSString());

            if(updateFlag) {
            	eoConfig = true;
            }
        }
        
        File clEoDefinitionsFile = _project.getLocation().append(
                CW_PROJECT_CONFIG_DIR_NAME).append(
                		MEMORYCONFIG_XML_DATA_FILENAME).toFile();
        if (!clEoDefinitionsFile.exists()) {
            URL xmiURL = DataPlugin.getDefault().find(
                new Path(PLUGIN_XML_FOLDER + File.separator
                    + MEMORYCONFIG_DEFAULT_XML_DATA_FILENAME));
            
            Path xmiPath = new Path(Platform.resolve(xmiURL).getPath());
            String dataFilePath =
                CW_PROJECT_CONFIG_DIR_NAME
                + File.separator + MEMORYCONFIG_XML_DATA_FILENAME;
            IFile dst = _project.getFile(new Path(dataFilePath));
            dst.create(new FileInputStream(xmiPath.toOSString()), true, null);
            addVersioningToFile(dst.getLocation().toOSString());

            if(updateFlag) {
            	eoDef = true;
    		}
        }
        File slotConfigFile = _project.getLocation().append(
                CW_PROJECT_CONFIG_DIR_NAME).append(
                SLOT_INFORMATION_XML_FILENAME).toFile();
        if (!slotConfigFile.exists()) {
            URL xmlURL = DataPlugin.getDefault().find(
                new Path(PLUGIN_XML_FOLDER + File.separator
                    + SLOT_INFO_DEFAULT_CONFIGS_XML_FILENAME));
            
            Path xmlPath = new Path(Platform.resolve(xmlURL).getPath());
            String dataFilePath =
                CW_PROJECT_CONFIG_DIR_NAME
                + File.separator + SLOT_INFORMATION_XML_FILENAME;
            IFile dst = _project.getFile(new Path(dataFilePath));
            dst.create(new FileInputStream(xmlPath.toOSString()), true, null);
            addVersioningToFile(dst.getLocation().toOSString());
        }
        File gmsConfigFile = _project.getLocation().append(
                CW_PROJECT_CONFIG_DIR_NAME).append(
                GMS_CONFIGURATION_XML_FILENAME).toFile();
        if (!gmsConfigFile.exists()) {
            URL xmlURL = DataPlugin.getDefault().find(
                new Path(PLUGIN_XML_FOLDER + File.separator
                    + GMS_INFO_DEFAULT_CONFIGS_XML_FILENAME));
            
            Path xmlPath = new Path(Platform.resolve(xmlURL).getPath());
            String dataFilePath =
                CW_PROJECT_CONFIG_DIR_NAME
                + File.separator + GMS_CONFIGURATION_XML_FILENAME;
            IFile dst = _project.getFile(new Path(dataFilePath));
            dst.create(new FileInputStream(xmlPath.toOSString()), true, null);
            addVersioningToFile(dst.getLocation().toOSString());
        }
        File dbalConfigFile = _project.getLocation().append(
                CW_PROJECT_CONFIG_DIR_NAME).append(
                DBALCONFIG_XML_FILENAME).toFile();
        if (!dbalConfigFile.exists()) {
            URL xmlURL = DataPlugin.getDefault().find(
                new Path(PLUGIN_XML_FOLDER + File.separator
                    + DBALCONFIG_DEFAULT_XML_FILENAME));
            
            Path xmlPath = new Path(Platform.resolve(xmlURL).getPath());
            String dataFilePath =
                CW_PROJECT_CONFIG_DIR_NAME
                + File.separator + DBALCONFIG_XML_FILENAME;
            IFile dst = _project.getFile(new Path(dataFilePath));
            dst.create(new FileInputStream(xmlPath.toOSString()), true, null);
            addVersioningToFile(dst.getLocation().toOSString());
        }

        if(eoConfig) {
            ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
    		EObject compModel = (EObject) pdm.getComponentModel().getEList().get(0);
    		List compList = (List) compModel.eGet(compModel.eClass()
    				.getEStructuralFeature("safComponent"));

            pdm.createEOConfiguration();
    		Model eoConfiguration = pdm.getEOConfiguration();
    		EObject eoList = (EObject) eoConfiguration.getEList().get(0);

    		Iterator itr = compList.iterator();
    		while (itr.hasNext()) {
    			EObject newComp = EcoreUtils.createEObject(
    					(EClass) eoList.eClass()
    							.getEStructuralFeature("eoConfig").getEType(),
    					true);

    			EcoreUtils.setValue(newComp, "name", EcoreUtils
						.getName((EObject) EcoreUtils.getValue(
								(EObject) itr.next(), "eoProperties")));
    			((List) EcoreUtils.getValue(eoList, "eoConfig")).add(newComp);
    		}
    		EcoreModels.save(eoConfiguration.getResource());
        }

        if(eoDef) {
            ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
            pdm.createEODefinitions();
        }
    }
    /**
     * Copy Templates which are used for code generation
     * @param foldername
     * @throws IOException
     * @throws CoreException
     */
    /*private void copyTemplatesFolder(String foldername) throws IOException, CoreException {
    	URL templatesURL = WorkspacePlugin.getDefault().find(
                new Path(PROJECT_TEMPLATE_FOLDER).append(PROJECT_DEFAULT_TEMPLATE_GROUP_FOLDER).append(foldername));
        Path templatesPath = new Path(Platform.resolve(templatesURL).getPath());
        File templatesDir  = new File(templatesPath.toOSString());
        File[] templates   = templatesDir.listFiles();
        IPath dstPrefix  = new Path(PROJECT_TEMPLATE_FOLDER).append(PROJECT_DEFAULT_TEMPLATE_GROUP_FOLDER).append(foldername);
        for (int i = 0; i < templates.length; i++) {
        	if (templates[i].isDirectory()){
        		continue;
        	}
        	IFile dst = _project.getFile(
                    dstPrefix.append(templates[i].getName()));
            if (dst.exists()) {
                dst.delete(true, true, null);
            }
            dst.create(new FileInputStream(templates[i]), true, null);
        }
    }*/

	/**
	 * Adds the versioning info to the path.
	 * 
	 * @param path
	 */
	private static void addVersioningToFile(String path) {
		Document document = MigrationUtils.buildDocument(path);
		Element rootElement = document.createElement("openClovisAsp");

		Element versionElement = document.createElement("version");
		rootElement.appendChild(versionElement);

		versionElement.setAttribute("v0", ProjectConstants.ASP_VERSION);
		versionElement.appendChild(document.getDocumentElement());

		document.appendChild(rootElement);
		MigrationUtils.saveDocument(document, path);
	}
}
