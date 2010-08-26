package com.clovis.cw.workspace.project.app;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.eclipse.cdt.core.CCProjectNature;
import org.eclipse.cdt.core.CCorePlugin;
import org.eclipse.cdt.core.CProjectNature;
import org.eclipse.cdt.core.ICDescriptor;
import org.eclipse.cdt.core.ICExtensionReference;
import org.eclipse.cdt.core.model.CoreModel;
import org.eclipse.cdt.core.settings.model.ICProjectDescription;
import org.eclipse.cdt.managedbuilder.core.BuildException;
import org.eclipse.cdt.managedbuilder.core.IConfiguration;
import org.eclipse.cdt.managedbuilder.core.IManagedBuildInfo;
import org.eclipse.cdt.managedbuilder.core.IManagedProject;
import org.eclipse.cdt.managedbuilder.core.IOption;
import org.eclipse.cdt.managedbuilder.core.ITool;
import org.eclipse.cdt.managedbuilder.core.ManagedBuildManager;
import org.eclipse.cdt.managedbuilder.core.ManagedCProjectNature;
import org.eclipse.cdt.managedbuilder.internal.core.Configuration;
import org.eclipse.cdt.managedbuilder.internal.core.ManagedProject;
import org.eclipse.cdt.managedbuilder.internal.core.ProjectType;
import org.eclipse.cdt.managedbuilder.internal.core.Tool;
import org.eclipse.cdt.managedbuilder.internal.core.ToolChain;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IProjectDescription;
import org.eclipse.core.resources.IWorkspace;
import org.eclipse.core.resources.IWorkspaceRoot;
import org.eclipse.core.resources.ResourcesPlugin;
import org.eclipse.core.runtime.Assert;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.PlatformUI;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.project.CwProjectPropertyPage;

/**
 * Class for application project creation
 * 
 * @author Pushparaj
 * 
 */
public class NewApplicationProjectCreation {

	private IProject _project;
	private Shell _shell;
	private String _srcLocation;
	private String _sdkLocation;
	private String _buildToolsLocation;
	private boolean _isSaf;
	private String _topLevelInclude;
	private String _configInclude;
	private String _ezxmlInclude;
	private List<String> _idlCompNames;
	private List<File> _buildTools;
	private Map<String,String> _toolArchDirMap = new HashMap<String, String>();
	private Map<String,String> _toolKernelDirMap = new HashMap<String, String>();
	private Map<String,String> _toolTargetDirMap = new HashMap<String, String>();
	public NewApplicationProjectCreation(IProject project, Shell shell, String srcLocation,
			String sdkLocation) {
		_buildToolsLocation = new File(sdkLocation).getParentFile().getAbsolutePath() + File.separator + "buildtools";
		_buildTools = getBuildTools();
		populateBuildTools();
		_project = project;
		_shell = shell;
		_srcLocation = srcLocation;
		_sdkLocation = sdkLocation;	
		_isSaf = new File(srcLocation + File.separator + ".saf").exists();
		
		_topLevelInclude = CwProjectPropertyPage.getSDKLocation(_project)
		+ File.separator + "include";

		_configInclude = srcLocation + File.separator + "config";

		_ezxmlInclude = sdkLocation + File.separator + "src"
		+ File.separator + "ASP" + File.separator + "3rdparty"
		+ File.separator + "ezxml" + File.separator + "stable";
		
		_idlCompNames = getIDLCompNames();
		
	}

	/**
	 * Creates CDT projects for the generated applications
	 * 
	 * @throws Exception
	 */
	public void createCDTProjectsForApplication() throws Exception {
		if(!new File(_sdkLocation + File.separator + "target").exists()) {
			MessageDialog.openInformation(_shell, "No Prebuild ASP", "No prebuild asp is available in sdk location");
			return;
		}
		ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
		Model model = pdm.getComponentModel();
		EObject rootObject = (EObject) model.getEList().get(0);
		EList list = (EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature("safComponent"));
		boolean isProjectCreated = false;
		boolean reCreate = false;
		for (int i = 0; i < list.size(); i++) {
			EObject comp = (EObject) list.get(i);
			boolean isSNMP = ((Boolean) EcoreUtils.getValue(comp,
					"isSNMPSubAgent")).booleanValue();
			boolean isCPP = ((Boolean) EcoreUtils.getValue(comp, "isBuildCPP"))
					.booleanValue();
			String name = EcoreUtils.getName(comp);
			EObject eoObj = (EObject) EcoreUtils.getValue(comp, "eoProperties");
			String eoName = EcoreUtils.getName(eoObj);
			File locFile = new File(_srcLocation + File.separator + "app"
					+ File.separator + name);
			IWorkspace workspace = ResourcesPlugin.getWorkspace();
			IWorkspaceRoot root = workspace.getRoot();

			// IProject newProjectHandle = root.getProject(_project.getName() +
			// "_" + name);
			IProject newProjectHandle = root.getProject(name);
			if (newProjectHandle.exists()) {
				
				if (!reCreate) {
					final Image[] image = new Image[1];
					_shell.getDisplay().syncExec(new Runnable() {
						public void run() {
							image[0] = _shell.getDisplay().getSystemImage(
									SWT.ICON_QUESTION);
						}
					});

					MessageDialog dialog = new MessageDialog(
							_shell,
							"Re-create project(s)?",
							image[0],
							"Project with the name "
									+ name
									+ " is already exists in workspace. Do you wish to re-create?",
							SWT.ICON_QUESTION, new String[] {
									IDialogConstants.YES_LABEL,
									IDialogConstants.YES_TO_ALL_LABEL,
									IDialogConstants.NO_LABEL,
									IDialogConstants.CANCEL_LABEL }, 1);
					int val = dialog.open();
					if (val == 1) {
						newProjectHandle.delete(false, true, null);
						newProjectHandle = root.getProject(name);
						reCreate = true;
					} else if (val == 0) {
						newProjectHandle.delete(false, true, null);
						newProjectHandle = root.getProject(name);
					} else if (val == 2) {
						continue;
					} else {
						break;
					}
				} else {
					newProjectHandle.delete(false, true, null);
					newProjectHandle = root.getProject(name);
				}
				//continue;
			}
			Assert.isNotNull(newProjectHandle);
			isProjectCreated = true;
			IProjectDescription description = workspace
					.newProjectDescription(newProjectHandle.getName());
			description.setLocationURI(locFile.toURI());

			newProjectHandle.create(description, null);
			newProjectHandle.open(null);
			CProjectNature.addCNature(newProjectHandle, null);
			if (isCPP)
				CCProjectNature.addCCNature(newProjectHandle, null);
			IManagedBuildInfo info = ManagedBuildManager
					.createBuildInfo(newProjectHandle);
			info.setValid(true);
			ManagedCProjectNature.addManagedNature(newProjectHandle, null);
			ManagedCProjectNature.addManagedBuilder(newProjectHandle, null);

			ICDescriptor desc = null;
			desc = CCorePlugin.getDefault().getCProjectDescription(
					newProjectHandle, true);
			desc.remove(CCorePlugin.BUILD_SCANNER_INFO_UNIQ_ID);
			desc.create(CCorePlugin.BUILD_SCANNER_INFO_UNIQ_ID,
					ManagedBuildManager.INTERFACE_IDENTITY);

			// IProjectType[] projTypes =
			// ManagedBuildManager.getDefinedProjectTypes();
			ProjectType projType = (ProjectType) ManagedBuildManager
					.getProjectType("cdt.managedbuild.target.gnu.exe");
			if (projType == null) { // TODO

			}

			IManagedProject newProject = null;
			newProject = ManagedBuildManager.createManagedProject(
					newProjectHandle, projType);

			ManagedBuildManager.setNewProjectVersion(newProjectHandle);

			IConfiguration defaultConfig = null;
			IConfiguration[] configs = projType.getConfigurations();
			for (int k = 0; k < _buildTools.size(); k++) {
				File buildFile = _buildTools.get(k);
				String toolChainName = buildFile.getName();
				if(!_toolArchDirMap.containsKey(toolChainName))
					continue;
				String arch = _toolArchDirMap.get(toolChainName);
				String kernel = _toolKernelDirMap.get(toolChainName);
				if(!new File(_sdkLocation + File.separator + "target" + File.separator + arch + File.separator + kernel).exists()) {
					continue;
				}
				for (int j = 0; j < configs.length; ++j) {
					Configuration config = new Configuration(
							(ManagedProject) newProject,
							(Configuration) configs[j], projType.getId() + "."
									+ k + j, false, false);
					config.setArtifactName(newProject.getDefaultArtifactName());
					if(toolChainName.equals("local"))
						config.setName("local" + " " + config.getName());
					else {
						config.setName(toolChainName + " " + config.getName());
					}
					if (k ==0 && j == 0)
						defaultConfig = config;
					addToolsForConfiguration(config, toolChainName, eoName, locFile, isSNMP);
					((ManagedProject) newProject).addConfiguration(config);
				}
			}
			ManagedBuildManager.setDefaultConfiguration(newProjectHandle,
					defaultConfig);

			ICProjectDescription projectDescription = CoreModel.getDefault()
					.getProjectDescription(newProjectHandle);

			CoreModel.getDefault().setProjectDescription(newProjectHandle,
					projectDescription);

			ManagedBuildManager.getBuildInfo(newProjectHandle).setValid(true);

			ManagedBuildManager.saveBuildInfo(newProjectHandle, true);

			// // Configure project to use PE debugger for windows // On linux
			// this would be ELF //
			ICExtensionReference[] binaryParsers = CCorePlugin.getDefault()
					.getBinaryParserExtensions(newProjectHandle);
			if (binaryParsers == null || binaryParsers.length == 0) {
				ICProjectDescription desc_bp = CCorePlugin.getDefault()
						.getProjectDescription(newProjectHandle);
				// if (desc_bp == null) { return false; }
				desc_bp.getDefaultSettingConfiguration().create(
						CCorePlugin.BINARY_PARSER_UNIQ_ID,
						CCorePlugin.PLUGIN_ID + "." + "ELF");

				CCorePlugin.getDefault().setProjectDescription(
						newProjectHandle, desc_bp);
			} else {

			}
		}

		/*if (isProjectCreated && !new File(_preBuildASPLibPath).exists()) {
			MessageDialog
					.openInformation(
							_shell,
							"Prebuild ASP Lib Path",
							"Prebuild ASP lib path is not specified for the created CDT projects. Please specify valid lib path before building the project(s).");
		}*/
		PlatformUI.getWorkbench().showPerspective(
				"org.eclipse.cdt.ui.CPerspective",
				PlatformUI.getWorkbench().getActiveWorkbenchWindow());
	}

	/**
	 * Find and Returns the Arch using uname -m
	 * 
	 * @return String
	 */
	private String getSystemArch() {
		String arch = null;
		try {
			Process child = Runtime.getRuntime().exec("uname -m");
			InputStream in = child.getInputStream();
			child.waitFor();
			int availableBytes = in.available();
			byte bytes[] = new byte[availableBytes];
			in.read(bytes);
			in.close();
			return new String(bytes).replace("\n", "");
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		return arch;
	}

	/**
	 * Find and Returns the kernel version using 'uname -r'
	 * 
	 * @return String
	 */
	private String getKernelversion() {
		String kernel = "";
		try {
			Process child = Runtime.getRuntime().exec("uname -r");
			InputStream in = child.getInputStream();
			child.waitFor();
			int availableBytes = in.available();
			byte bytes[] = new byte[availableBytes];
			in.read(bytes);
			in.close();
			return new String(bytes).replace("\n", "");
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		return kernel;
	}
	/**
	 * Returns IDL Comp names
	 * @return List
	 */
	private List<String> getIDLCompNames() {
		List<String> idlCompNames = new ArrayList<String>();
		List<EObject> idlList = ProjectDataModel.getProjectDataModel(_project).
        getIDLConfigList();
		if(idlList.size() == 0)
			return idlCompNames;
		EObject idlObj = (EObject) idlList.get(0);
		List<EObject> idlEoList = (List) EcoreUtils.getValue(idlObj, "Service");
		for (int i = 0; i < idlEoList.size(); i++) {
			EObject idlEoObj =  idlEoList.get(i);
			String eoName = EcoreUtils.getValue(idlEoObj, "name").toString();
			idlCompNames.add(eoName);
		}
		return idlCompNames;
	}
	/**
	 * Adds build tools for configuration
	 * @param config
	 * @param eoName
	 * @param toolChainName
	 * @param locFile
	 * @param isSNMP
	 * @throws BuildException
	 */
	private void addToolsForConfiguration(Configuration config, String toolChainName, String eoName, File locFile, boolean isSNMP) throws BuildException {
		ToolChain toolChain = (ToolChain) config.getToolChain();
		ITool tools[] = toolChain.getTools();
		
		String buildToolsLibPath = getBuildToolsLibPath(toolChainName);
		String preBuildASPLibPath = getPreBuildASPLibPath(toolChainName);
		String configLibPath = getConfigLibPath(toolChainName);
		for (int k = 0; k < tools.length; k++) {
			Tool tool = (Tool)tools[k];
			if (tool.getId().contains(
					"cdt.managedbuild.tool.gnu.c.compiler.exe.debug")
					|| tool
							.getId()
							.contains(
									"cdt.managedbuild.tool.gnu.c.compiler.exe.release")) {
				if(!toolChainName.equals("local")) {
					String command = tool.getToolCommand();
					tool.setToolCommand(_buildToolsLocation + File.separator + toolChainName + File.separator + "bin" + File.separator + _toolTargetDirMap.get(toolChainName) + "-" + command);
				}
				IOption parent = tool
						.getOptionBySuperClassId("gnu.c.compiler.option.include.paths");
				IOption option = tool.createOption(parent, parent
						.getId()
						+ "." + ManagedBuildManager.getRandomNumber(),
						parent.getName(), false);
				if (isSNMP) {
					String buildToolsInclude = new File(
							buildToolsLibPath).getParentFile()
							.getAbsolutePath()
							+ File.separator + "include";
					String staticInclude = locFile.getAbsolutePath()
							+ File.separator + "static";
					String mdlInclude = locFile.getAbsolutePath()
							+ File.separator + "mdl_config";
					String mib2cInclude = locFile.getAbsolutePath()
							+ File.separator + "mib2c";
					if (_isSaf) {
						option.setValue(new String[] { _configInclude,
								buildToolsInclude, staticInclude,
								mdlInclude, mib2cInclude,
								_topLevelInclude,
								locFile.getAbsolutePath() });
					} else {
						option
								.setValue(new String[] { _configInclude,
										buildToolsInclude,
										staticInclude, mdlInclude,
										mib2cInclude, _topLevelInclude,
										locFile.getAbsolutePath(),
										_ezxmlInclude });
					}
				} else {
					if (_isSaf) {
						option.setValue(new String[] { _topLevelInclude,
								locFile.getAbsolutePath(),
								_configInclude });
					} else {
						option.setValue(new String[] { _topLevelInclude,
								locFile.getAbsolutePath(),
								_configInclude, _ezxmlInclude });
					}
				}
			} else if (tool.getId().contains(
					"cdt.managedbuild.tool.gnu.c.linker.exe.debug")
					|| tool
							.getId()
							.contains(
									"cdt.managedbuild.tool.gnu.c.linker.exe.release")) {
				if(!toolChainName.equals("local")) {
					String command = tool.getToolCommand();
					tool.setToolCommand(_buildToolsLocation + File.separator + toolChainName + File.separator + "bin" + File.separator + _toolTargetDirMap.get(toolChainName) + "-" + command);
				}
				IOption parent = tool
						.getOptionBySuperClassId("gnu.c.link.option.libs");
				IOption option = tool.createOption(parent, parent
						.getId()
						+ "." + ManagedBuildManager.getRandomNumber(),
						parent.getName(), false);
				if (_isSaf) {
					if (_idlCompNames.contains(eoName)) {
						option
								.setValue(new String[] { "dl", "m",
										"mw", "openhpi",
										"openhpiutils", "ClConfig",
										"Cl" + eoName + "Server",
										"Cl" + eoName + "IdlOpen",
										"ClIdlPtr" });
					} else {
						option
								.setValue(new String[] { "dl", "m",
										"mw", "openhpi",
										"openhpiutils", "ClConfig" });
					}
				} else {
					if (_idlCompNames.contains(eoName)) {
						option
								.setValue(new String[] { "dl", "m",
										"mw", "ClMain", "openhpi",
										"openhpiutils", "ClConfig",
										"Cl" + eoName + "Server",
										"Cl" + eoName + "IdlOpen",
										"ClIdlPtr" });
					} else {
						option.setValue(new String[] { "dl", "m", "mw",
								"ClMain", "openhpi", "openhpiutils",
								"ClConfig" });
					}
				}
				IOption parent1 = tool
						.getOptionBySuperClassId("gnu.c.link.option.paths");
				IOption option1 = tool.createOption(parent1, parent1
						.getId()
						+ "." + ManagedBuildManager.getRandomNumber(),
						parent1.getName(), false);
				if (new File(preBuildASPLibPath).exists()) {
					option1.setValue(new String[] { buildToolsLibPath,
							configLibPath, preBuildASPLibPath });
				} else {
					option1
							.setValue(new String[] { buildToolsLibPath });
				}

				IOption parent2 = tool
						.getOptionBySuperClassId("gnu.c.link.option.ldflags");
				IOption option2 = tool.createOption(parent2, parent2
						.getId()
						+ "." + ManagedBuildManager.getRandomNumber(),
						parent2.getName(), false);
				String linkerOptions = "-Wl,-rpath-link "
						+ buildToolsLibPath;
				if (new File(preBuildASPLibPath).exists()) {
					linkerOptions += " -Wl,-rpath-link "
							+ preBuildASPLibPath;
				}
				if (isSNMP) {
					String netsnmpconfig = "net-snmp-config";
					if(!toolChainName.equals("local")) {
						netsnmpconfig = _buildToolsLocation + File.separator + toolChainName + File.separator + "bin" + File.separator + netsnmpconfig;
					}
					linkerOptions += " `" + netsnmpconfig + " --libs`"
							+ " `" + netsnmpconfig + " --agent-libs`";
				}
				option2.setValue(linkerOptions);
			} else if (tool.getId().contains(
					"cdt.managedbuild.tool.gnu.cpp.compiler.exe.debug")
					|| tool
							.getId()
							.contains(
									"cdt.managedbuild.tool.gnu.cpp.compiler.exe.release")) {
				if(!toolChainName.equals("local")) {
					String command = tool.getToolCommand();
					tool.setToolCommand(_buildToolsLocation + File.separator + toolChainName + File.separator + "bin" + File.separator + _toolTargetDirMap.get(toolChainName) + "-" + command);
				}
				IOption parent = tool
						.getOptionBySuperClassId("gnu.cpp.compiler.option.include.paths");
				IOption option = tool.createOption(parent, parent
						.getId()
						+ "." + ManagedBuildManager.getRandomNumber(),
						parent.getName(), false);
				if (isSNMP) {
					String buildToolsInclude = new File(
							buildToolsLibPath).getParentFile()
							.getAbsolutePath()
							+ File.separator + "include";
					String staticInclude = locFile.getAbsolutePath()
							+ File.separator + "static";
					String mdlInclude = locFile.getAbsolutePath()
							+ File.separator + "mdl_config";
					String mib2cInclude = locFile.getAbsolutePath()
							+ File.separator + "mib2c";
					if (_isSaf) {
						option.setValue(new String[] { _configInclude,
								buildToolsInclude, staticInclude,
								mdlInclude, mib2cInclude,
								_topLevelInclude,
								locFile.getAbsolutePath() });
					} else {
						option
								.setValue(new String[] { _configInclude,
										buildToolsInclude,
										staticInclude, mdlInclude,
										mib2cInclude, _topLevelInclude,
										locFile.getAbsolutePath(),
										_ezxmlInclude });
					}
				} else {
					if (_isSaf) {
						option.setValue(new String[] { _topLevelInclude,
								locFile.getAbsolutePath(),
								_configInclude });
					} else {
						option.setValue(new String[] { _topLevelInclude,
								locFile.getAbsolutePath(),
								_configInclude, _ezxmlInclude });
					}
				}
			} else if (tool.getId().contains(
					"cdt.managedbuild.tool.gnu.cpp.linker.exe.debug")
					|| tool
							.getId()
							.contains(
									"cdt.managedbuild.tool.gnu.cpp.linker.exe.release")) {
				if(!toolChainName.equals("local")) {
					String command = tool.getToolCommand();
					tool.setToolCommand(_buildToolsLocation + File.separator + toolChainName + File.separator + "bin" + File.separator + _toolTargetDirMap.get(toolChainName) + "-" + command);
				}
				IOption parent = tool
						.getOptionBySuperClassId("gnu.cpp.link.option.libs");
				IOption option = tool.createOption(parent, parent
						.getId()
						+ "." + ManagedBuildManager.getRandomNumber(),
						parent.getName(), false);
				if (_isSaf) {
					if (_idlCompNames.contains(eoName)) {
						option
								.setValue(new String[] { "dl", "m",
										"mw", "openhpi",
										"openhpiutils", "ClConfig",
										"Cl" + eoName + "Server",
										"Cl" + eoName + "IdlOpen",
										"ClIdlPtr" });
					} else {
						option
								.setValue(new String[] { "dl", "m",
										"mw", "openhpi",
										"openhpiutils", "ClConfig" });
					}
				} else {
					if (_idlCompNames.contains(eoName)) {
						option
								.setValue(new String[] { "dl", "m",
										"mw", "ClMain", "openhpi",
										"openhpiutils", "ClConfig",
										"Cl" + eoName + "Server",
										"Cl" + eoName + "IdlOpen",
										"ClIdlPtr" });
					} else {
						option.setValue(new String[] { "dl", "m", "mw",
								"ClMain", "openhpi", "openhpiutils",
								"ClConfig" });
					}
				}
				IOption parent1 = tool
						.getOptionBySuperClassId("gnu.cpp.link.option.paths");
				IOption option1 = tool.createOption(parent1, parent1
						.getId()
						+ "." + ManagedBuildManager.getRandomNumber(),
						parent1.getName(), false);
				if (new File(preBuildASPLibPath).exists()) {
					option1.setValue(new String[] { buildToolsLibPath,
							configLibPath, preBuildASPLibPath });
				} else {
					option1
							.setValue(new String[] { buildToolsLibPath });
				}

				IOption parent2 = tool
						.getOptionBySuperClassId("gnu.cpp.link.option.flags");
				IOption option2 = tool.createOption(parent2, parent2
						.getId()
						+ "." + ManagedBuildManager.getRandomNumber(),
						parent2.getName(), false);
				String linkerOptions = "-Wl,-rpath-link "
						+ buildToolsLibPath;
				if (new File(preBuildASPLibPath).exists()) {
					linkerOptions += " -Wl,-rpath-link "
							+ preBuildASPLibPath;
				}
				if (isSNMP) {
					String netsnmpconfig = "net-snmp-config";
					if(!toolChainName.equals("local")) {
						netsnmpconfig = _buildToolsLocation + File.separator + toolChainName + File.separator + "bin" + File.separator + netsnmpconfig;
					}
					linkerOptions += " `" + netsnmpconfig + " --libs`"
							+ " `" + netsnmpconfig + " --agent-libs`";
				}
				option2.setValue(linkerOptions);
			}
		}
	}
	/**
	 * Returns build tools 
	 * @param buildToolsFile
	 * @return List
	 */
	private List<File> getBuildTools(){
		File buildToolsFile = new File(_buildToolsLocation);
		List<File> buidTools = new ArrayList<File>();
		File files[] =  buildToolsFile.listFiles();
		for (int i = 0; i < files.length; i++) {
			File file = files[i];
			if(file.isDirectory()) {
				if(file.getName().equals("local") || new File(file.getAbsolutePath() + File.separator + "config.mk").exists()) {
					buidTools.add(file);
				}
			}
		}
		return buidTools;
	}
	/**
	 * Populate build tools details
	 */
	private void populateBuildTools() {
		for (int i = 0; i < _buildTools.size(); i++) {
			File file = _buildTools.get(i);
			if(file.getName().equals("local")) {
				String arch = getSystemArch();
				String kernelVersion = null;
				try {
					String versionFile = "/lib/modules/"+ getKernelversion() + "/build/include/linux/version.h";
					if (new File(versionFile).exists()) {
						BufferedReader reader = new BufferedReader(
								new FileReader(versionFile));
						String readLine = null;
						while ((readLine = reader.readLine()) != null) {
							if (readLine.contains("LINUX_VERSION_CODE")) {
								int versionCode = Integer.parseInt(readLine
										.split(" ")[2].trim());
								kernelVersion = getKernelVersion(versionCode);
								_toolKernelDirMap.put("local", "linux-"
										+ kernelVersion);
							}
						}
					}
				} catch (FileNotFoundException e) {
					e.printStackTrace();
				} catch (NumberFormatException e) {
					e.printStackTrace();
				} catch (IOException e) {
					e.printStackTrace();
				}
				if(arch != null && kernelVersion != null) {
					_toolArchDirMap.put("local", arch);
					_toolKernelDirMap.put("local", "linux-" + kernelVersion);
				}
			} else {
				String configmkFile = file.getAbsolutePath() + File.separator + "config.mk";
				try {
					BufferedReader reader = new BufferedReader(new FileReader(configmkFile));
					String readLine = null;
					String arch = null;
					String target = null;
					String kernelVersion = null;
					while((readLine = reader.readLine()) != null) {
						if(readLine.contains("ARCH=")) {
							arch = readLine.split("=")[1].trim();
						}
						if(readLine.contains("TARGET=")) {
							target = readLine.split("=")[1].trim();
						}
					}
					String versionFile = file.getAbsolutePath()
							+ File.separator + "src" + File.separator + "linux"
							+ File.separator + "include" + File.separator
							+ "linux" + File.separator + "version.h";
					if(!new File(versionFile).exists()) {
						versionFile = file.getAbsolutePath()
						+ File.separator + target + File.separator + "include" + File.separator
						+ "linux" + File.separator + "version.h";
					}
					reader.close();
					if (new File(versionFile).exists()) {
						reader = new BufferedReader(new FileReader(versionFile));
						readLine = null;
						while ((readLine = reader.readLine()) != null) {
							if (readLine.contains("LINUX_VERSION_CODE")) {
								int versionCode = Integer.parseInt(readLine
										.split(" ")[2].trim());
								kernelVersion = getKernelVersion(versionCode);
							}
						}
					}
					if(arch != null && target != null && kernelVersion != null) {
						_toolArchDirMap.put(file.getName(), arch);
						_toolTargetDirMap.put(file.getName(), target);
						_toolKernelDirMap.put(file.getName(), "linux-" + kernelVersion);
					}
				} catch (FileNotFoundException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
		}
	}
	/**
	 * Parse the version code and returns the kernel version name
	 * @param versionCode
	 * @return
	 */
	private String getKernelVersion(int versionCode) {
		int major = versionCode>>16;
		int minor = (versionCode>>8)&0xff;
		int patch = versionCode&0xff;
		return major + "." + minor + "." + patch;
	}
	private String getBuildToolsLibPath(String toolChain) {
		return new File(_sdkLocation).getParentFile()
		.getAbsolutePath()
		+ File.separator
		+ "buildtools"
		+ File.separator
		+ toolChain
		+ File.separator + "lib";
	}
	private String getPreBuildASPLibPath(String toolChain) {
		return _sdkLocation + File.separator + "target"
		+ File.separator + _toolArchDirMap.get(toolChain) + File.separator
		+ _toolKernelDirMap.get(toolChain) + File.separator
		+ "lib";
	}
	private String getConfigLibPath(String toolChain) {
		return CwProjectPropertyPage
		.getProjectAreaLocation(_project)
		+ File.separator
		+ _project.getName()
		+ File.separator + "target"
		+ File.separator + _toolArchDirMap.get(toolChain) + File.separator
		+ _toolKernelDirMap.get(toolChain) + File.separator
		+ "lib";
	}
}
