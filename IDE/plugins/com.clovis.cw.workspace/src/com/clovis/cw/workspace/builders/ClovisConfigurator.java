/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/builders/ClovisConfigurator.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.builders;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.net.URL;
import java.util.HashMap;

import org.eclipse.ant.core.AntCorePlugin;
import org.eclipse.ant.core.AntRunner;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.core.runtime.QualifiedName;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.swt.widgets.Display;

import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.workspace.project.CwProjectPropertyPage;

/**
 * @author pushparaj
 * Configurator
 */
public class ClovisConfigurator {
	
    public static final String BUILD_WITH_CROSS_MODE = "--with-cross-build";
    public static final String BUILD_TOOLCHAIN_MODE = "BUILD_TOLLCHAIN_MODE";
    
    public static final String BUILD_WITH_KERNEL_MODE = "--with-kernel-build";
    public static final String BUILD_KERNEL_VERSION_MODE = "BUILD_KERNEL_VERSION_MODE";
    
    public static final String BUILD_WITH_ASP_MODE = "--with-asp-build";
    public  static final String ASP_LOCATION_MODE = "ASP_LOCATION";
    
    public static final String BUILD_WITH_PRE_ASP_MODE = "--with-asp-installdir";
    public static final String ASP_PRE_BUILD_LOCATION_MODE = "ASP_PRE_BUILD_LOCATION";
    
    public static final String BUILD_WITH_SNMP_MODE = "--with-snmp-build";
    public static final String BUILD_WITHOUT_SNMP_MODE = "--without-snmp-build";
    
    public static final String BUILD_WITH_SIMULATION_MODE = "--with-asp-simulation";
       
    public static final String BUILD_WITH_IPC_MODE = "--with-ipc-build";
    public static final String BUILD_WITH_IPC_VALUE_MODE = "BUILD_WITH_IPC_VALUE_MODE";
    
    public static final String BUILD_WITH_CM_MODE = "--with-cm-build";
    public static final String BUILD_WITH_CM_VALUE_MODE = "BUILD_WITH_CM_VALUE_MODE";
    
    public static final String FORCE_CONFIGURE_MODE = "--force-configure-mode";
    
    public static final String BUILD_WITH_SDK_DIR = "--with-sdk-dir";
    public static final String SDK_LOCATION = "SDK_LOCATION";
    
    public static final String BUILD_WITH_BINARY_MODE = "--with-binary-mode";
    public static final String BUILD_WITH_BINARY_MODE_VALUE = "BUILD_WITH_BINARY_VALUE_MODE";
    
    public static final String CHANGES_SINCE_LAST_CONFIG = "CHANGES_SINCE_LAST_CONFIG";
    
    private boolean _configureSuccess = true;
	
	/**
	 * 
	 * @param project
	 * @param sourceLocation
	 * @param aspLocation
	 * @param buildToolsLocation
	 * @param arg1
	 * @param arg2
	 * @param arg3
	 * @param arg4
	 * @throws Exception
	 */
	public boolean configure(IProject project, String projectAreaLocation,
			String aspLocation, String arg1, String arg2, String arg3, String arg4,  String arg5,  String arg6, String arg7, String arg8, String arg9) throws Exception
	{
		boolean retVal = true;
		
		AntRunner ant = new AntRunner();
		try {
			ant.setCustomClasspath(getClassPathURL());
			ant.addBuildLogger(ClovisConfiguratorLogger.class.getName());
		} catch (Exception e) {
			ant.addBuildLogger("org.apache.tools.ant.DefaultLogger");
		}
		URL url = DataPlugin.getDefault().getBundle().getEntry("/");
		try {
			url = Platform.resolve(url);
		} catch (IOException e) {
			e.printStackTrace();
		}

		String actionCommand = project.getLocation().append("config.sh").toOSString();

		StringBuffer buff = new StringBuffer("-Dasp.loc=")
				.append(aspLocation).append(" -Dscript.file=")
				.append(actionCommand)
				.append(" -Dmodel.name=").append(project.getName()).append(
						" -Dmodel.path=").append(projectAreaLocation).append(
						" -Dproject.name=").append(project.getName()).append(
						" -Dconfig.option1=")
				.append(arg1).append(" -Dconfig.option2=").append(arg2).append(
						" -Dconfig.option3=").append(arg3).append(
						" -Dconfig.option4=").append(arg4).append(
						" -Dconfig.option5=").append(arg5).append(
						" -Dconfig.option6=").append(arg6).append(
						" -Dconfig.option7=").append(arg7).append(
						" -Dconfig.option8=").append(arg8).append(
						" -Dconfig.option9=").append(arg9);

		ant.setBuildFileLocation(project.getLocation().append("config.xml")
				.toOSString());
		ant.setArguments(buff.toString());

		ant.setMessageOutputLevel(org.apache.tools.ant.Project.MSG_INFO);

		AntBuildThread thread = new AntBuildThread(ant, project, actionCommand);
		Display.getDefault().syncExec(thread);

		if (thread.wasCancelled()) retVal = false;

		try {
			project.refreshLocal(IResource.DEPTH_INFINITE, null);
		} catch(Exception e) {
			e.printStackTrace();
		}
		return retVal;
	}

	/**
	 * Retrieves all of the configuration properties for the project, sets the
	 * appropriate configuration arguments, and then calls the configurator to
	 * configure the project.
	 * 
	 * @param project - the project that is being configured
	 */
	public boolean configureForBuild(IProject project)
	{
		try {
			// get the persisted project properties
			String aspPreBuildMode = project.getPersistentProperty(new QualifiedName("true", BUILD_WITH_PRE_ASP_MODE));
			String preASPLocation  = project.getPersistentProperty(new QualifiedName("", ASP_PRE_BUILD_LOCATION_MODE));
			String ipcMode         = project.getPersistentProperty(new QualifiedName("true", BUILD_WITH_IPC_MODE));
			String ipcValue        = project.getPersistentProperty(new QualifiedName("true", BUILD_WITH_IPC_VALUE_MODE));
			String buildMode       = project.getPersistentProperty(new QualifiedName("true", BUILD_WITH_CROSS_MODE));
			String toolChain       = project.getPersistentProperty(new QualifiedName("true", BUILD_TOOLCHAIN_MODE));
			String kernelMode      = project.getPersistentProperty(new QualifiedName("true", BUILD_WITH_KERNEL_MODE));
			String kernelName      = project.getPersistentProperty(new QualifiedName("true", BUILD_KERNEL_VERSION_MODE));
			String snmpMode        = project.getPersistentProperty(new QualifiedName("true", BUILD_WITH_SNMP_MODE));
			String simulationMode  = project.getPersistentProperty(new QualifiedName("true", BUILD_WITH_SIMULATION_MODE));
			String cmMode          = project.getPersistentProperty(new QualifiedName("true", BUILD_WITH_CM_MODE));
			String forceConfigureMode = project.getPersistentProperty(new QualifiedName("true", FORCE_CONFIGURE_MODE));
			String cmValue         = project.getPersistentProperty(new QualifiedName("true", BUILD_WITH_CM_VALUE_MODE));
			String binaryMode      = project.getPersistentProperty(new QualifiedName("true", BUILD_WITH_BINARY_MODE));
			String binaryValue     = project.getPersistentProperty(new QualifiedName("true", BUILD_WITH_BINARY_MODE_VALUE));
			String aspLocation     = project.getPersistentProperty(new QualifiedName("", SDK_LOCATION)) + File.separator + "src" + File.separator + "ASP";
			String configChanges   = project.getPersistentProperty(new QualifiedName("true", CHANGES_SINCE_LAST_CONFIG));

			HashMap targetConfSettings = MakeImages.readTargetConf(project);

			if (forceConfigureMode.equals("false") && targetConfSettings != null && configChanges != null && configChanges.equals("false")) return true;

			String projectAreaLocation = CwProjectPropertyPage.getProjectAreaLocation(project);

			// set the appropriate arguments for the configurator
            String option1 = "NO";
            String option2 = "NO";
            String option3 = BUILD_WITHOUT_SNMP_MODE;
            String option4 = "NO";
            String option5 = "NO"; // with-asp-build / with-asp-installdir
            String option6 = "NO"; // with-ipc-build
            String option7 = "NO";
            String option8 = "NO";

            if(Boolean.valueOf(aspPreBuildMode)) {
            	option5 = BUILD_WITH_PRE_ASP_MODE + "=" + preASPLocation;
            } else  {
            	option5 =  BUILD_WITH_ASP_MODE;
           	}
            if(Boolean.valueOf(ipcMode)) {
            	option6 = BUILD_WITH_IPC_MODE;
            	if(ipcValue.equals("ioc")) {
            		option6 = option6 + "=" + "ioc";
            	}
            }
            if(Boolean.valueOf(buildMode))
            	if(toolChain != null && !toolChain.equals(""))
            		option1 = BUILD_WITH_CROSS_MODE + "=" + toolChain;
            if(Boolean.valueOf(kernelMode))
            	option2 = BUILD_WITH_KERNEL_MODE + "=" + kernelName;
            if(Boolean.valueOf(snmpMode))
            	option3 = "NO";
            if(Boolean.valueOf(cmMode)) {
            	if(cmValue.equals("radisys")) {
            		option4 = BUILD_WITH_CM_MODE;
            	} else if(cmValue.equals("openhpi")) {
            		option4 = BUILD_WITH_CM_MODE + "=openhpi";
            	}
            }
            if(Boolean.valueOf(simulationMode))
            	option7 = BUILD_WITH_SIMULATION_MODE;

            if(Boolean.valueOf(binaryMode)) {
            	option8 = BUILD_WITH_BINARY_MODE;
            	if(binaryValue.equals("32")) {
            		option8 = option8 + "=32";
            	} else if(binaryValue.equals("64")) {
            		option8 = option8 + "=64";
            	}
            }
            
            String option9 = BUILD_WITH_SDK_DIR + "=" + new File(project.getPersistentProperty(new QualifiedName("", SDK_LOCATION))).getParent(); 
            // run the configuration
            boolean retVal = configure((IProject) project, projectAreaLocation,
								aspLocation, option1, option2, option3, option4,
								option5, option6, option7, option8, option9);
            if (!retVal) _configureSuccess = false;

		} catch (CoreException e) {
			e.printStackTrace();
			_configureSuccess = false;
		} catch (Exception e) {
			e.printStackTrace();
			_configureSuccess = false;
		}

		if (_configureSuccess) setConfigChanges(project, "false");

		return _configureSuccess;
	}


    /**
     * Mark the fact that configuration changes have been made to the project.
     */
    public static void setConfigChanges(IResource project, String configChanges)
    {
        try {
        	project.setPersistentProperty(
               new QualifiedName("true", CHANGES_SINCE_LAST_CONFIG), configChanges);
        } catch (CoreException e) {
        	e.printStackTrace();
        }
    }
    
    /**
     * Get ASP Location for this Project.
     * @param res Resource
     * @return ASP Location for this Project.
     */
    public static String getASPLocation(IResource res)
    {
    	String location  = null;
        try {
            location = res.getPersistentProperty(
                    new QualifiedName("", ASP_LOCATION_MODE));
        } catch (CoreException e) {
        }
        return location != null ? location : "";
    }

    /**
     * Set the ASP location for this Project.
     */
    public static void setASPLocation(IResource project, String aspLocation)
    {
        try {
            String oldAspLocation = getASPLocation(project);
            project.setPersistentProperty(
               new QualifiedName("", ASP_LOCATION_MODE), aspLocation);

            // if the value has changed then mark that configuration changes have been made
            if (checkValueChange(oldAspLocation, aspLocation, ""))
            {
            	setConfigChanges(project, "true");
            }
        } catch (CoreException e) {
        	e.printStackTrace();
        }
    }

	/**
     * Get Pre Build ASP Location for this Project.
     * @param res Resource
     * @return Pre Build ASP Location for this Project.
     */
    public static String getPreBuildASPLocation(IResource res)
    {
    	String location  = null;
        try {
            location = res.getPersistentProperty(
                    new QualifiedName("", ASP_PRE_BUILD_LOCATION_MODE));
            if (location == null || location.length() == 0)
            {
            	// if we didn't find it then default to the SDK location
            	location = res.getPersistentProperty(
                        new QualifiedName("", SDK_LOCATION));
            }
        } catch (CoreException e) {
        }
        return location != null ? location : "";
    }

    /**
     * Set the Pre Build ASP location for this Project.
     */
    public static void setPreBuildASPLocation(IResource project, String aspPreBuildLocation)
    {
        try {
            String oldPreBuildAspLocation = getPreBuildASPLocation(project);
            project.setPersistentProperty(
               new QualifiedName("", ASP_PRE_BUILD_LOCATION_MODE), aspPreBuildLocation);

            // if the value has changed then mark that configuration changes have been made
            if (checkValueChange(oldPreBuildAspLocation, aspPreBuildLocation, ""))
            {
            	setConfigChanges(project, "true");
            }
        } catch (CoreException e) {
        	e.printStackTrace();
        }
    }

    /**
	 * Get Cross Compilation mode for this Project.
	 * 
	 * @param res
	 *            Resource
	 * @return Cross Compilation mode for this Project.
	 */
    public static String getCrossBuildMode(IResource res)
    {
        String mode  = null;
        try {
            mode = res.getPersistentProperty(
                    new QualifiedName("true", BUILD_WITH_CROSS_MODE));
        } catch (CoreException e) {
        	e.printStackTrace();
        }
        return mode != null ? mode : "false";
    }

    /**
     * Set Cross Compilation mode for this Project.
     */
    public static void setCrossBuildMode(IResource project, String crossBuildMode)
    {
        try {
            String oldCrossBuildMode = getCrossBuildMode(project);
            project.setPersistentProperty(
               new QualifiedName("true", BUILD_WITH_CROSS_MODE), crossBuildMode);

            // if the value has changed then mark that configuration changes have been made
            if (checkValueChange(oldCrossBuildMode, crossBuildMode, "false"))
            {
            	setConfigChanges(project, "true");
            }
        } catch (CoreException e) {
        	e.printStackTrace();
        }
    }

    /**
     * Get Toolchain mode for this Project.
     * @param res Resource
     * @return ToolChain mode for this Project.
     */
    public static String getToolChainMode(IResource res)
    {
        String mode  = null;
        try {
            mode = res.getPersistentProperty(
                    new QualifiedName("true", BUILD_TOOLCHAIN_MODE));
        } catch (CoreException e) {
        	e.printStackTrace();
        }
        return (mode != null && !mode.equals("")) ? mode : "local";
    }

    /**
     * Set tool chain mode for this Project.
     */
    public static void setToolChainMode(IResource project, String toolChainMode)
    {
        try {
            String oldToolChainMode = getToolChainMode(project);
            project.setPersistentProperty(
               new QualifiedName("true", BUILD_TOOLCHAIN_MODE), toolChainMode);
            
            // if the value has changed then mark that configuration changes have been made
            if (checkValueChange(oldToolChainMode, toolChainMode, "local"))
            {
            	setConfigChanges(project, "true");
            }
        } catch (CoreException e) {
        	e.printStackTrace();
        }
    }

    /**
     * Get Kernel build mode for this Project.
     * @param res Resource
     * @return Kernel build mode for this Project.
     */
    public static String getKernelBuildMode(IResource res)
    {
        String mode  = null;
        try {
            mode = res.getPersistentProperty(
                    new QualifiedName("true", BUILD_WITH_KERNEL_MODE));
        } catch (CoreException e) {
        	e.printStackTrace();
        }
        return mode != null ? mode : "false";
    }

    /**
     * Set the kernel build mode for this Project.
     */
    public static void setKernelBuildMode(IResource project, String kernelBuildMode)
    {
        try {
            String oldKernelBuildMode = getKernelBuildMode(project);
            project.setPersistentProperty(
               new QualifiedName("true", BUILD_WITH_KERNEL_MODE), kernelBuildMode);
            
            // if the value has changed then mark that configuration changes have been made
            if (checkValueChange(oldKernelBuildMode, kernelBuildMode, "false"))
            {
            	setConfigChanges(project, "true");
            }
        } catch (CoreException e) {
        	e.printStackTrace();
        }
    }

    /**
     * Return Kernel Name
     * @param res Resource
     * @return Kernel Version
     */
    public static String getKernelName(IResource res)
    {
        String mode  = null;
        try {
            mode = res.getPersistentProperty(
                    new QualifiedName("true", BUILD_KERNEL_VERSION_MODE));
        } catch (CoreException e) {
        	e.printStackTrace();
        }
        return mode != null ? mode : System.getProperty("os.version");
    }

    /**
     * Set the kernel name for this Project.
     */
    public static void setKernelName(IResource project, String kernelName)
    {
        try {
            String oldKernelName = getKernelName(project);
            project.setPersistentProperty(
               new QualifiedName("true", BUILD_KERNEL_VERSION_MODE), kernelName);
            
            // if the value has changed then mark that configuration changes have been made
            if (checkValueChange(oldKernelName, kernelName, System.getProperty("os.version")))
            {
            	setConfigChanges(project, "true");
            }
        } catch (CoreException e) {
        	e.printStackTrace();
        }
    }

    /**
     * Get ASP build mode for this Project.
     * @param res Resource
     * @return ASP build mode for this Project.
     */
    public static String getASPBuildMode(IResource res)
    {
        String mode  = null;
        try {
            mode = res.getPersistentProperty(
                    new QualifiedName("true", BUILD_WITH_ASP_MODE));
        } catch (CoreException e) {
        	e.printStackTrace();
        }
        return mode != null ? mode : "false";
    }

    /**
     * Set the ASP build mode for this Project.
     */
    public static void setASPBuildMode(IResource project, String aspBuildMode)
    {
        try {
            String oldAspBuildMode = getASPBuildMode(project);
            project.setPersistentProperty(
               new QualifiedName("true", BUILD_WITH_ASP_MODE), aspBuildMode);
            
            // if the value has changed then mark that configuration changes have been made
            if (checkValueChange(oldAspBuildMode, aspBuildMode, "false"))
            {
            	setConfigChanges(project, "true");
            }
        } catch (CoreException e) {
        	e.printStackTrace();
        }
    }


    /**
     * Get ASP pre build mode for this Project.
     * @param res Resource
     * @return ASP pre build mode for this Project.
     */
    public static String getASPPreBuildMode(IResource res)
    {
        String mode  = null;
        try {
            mode = res.getPersistentProperty(
                    new QualifiedName("true", BUILD_WITH_PRE_ASP_MODE));
        } catch (CoreException e) {
        	e.printStackTrace();
        }
        return mode != null ? mode : "false";
    }

    /**
     * Set the pre-built ASP mode for this Project.
     */
    public static void setASPPreBuildMode(IResource project, String aspPreBuildMode)
    {
        try {
            String oldAspPreBuildMode = getASPPreBuildMode(project);
            project.setPersistentProperty(
               new QualifiedName("true", BUILD_WITH_PRE_ASP_MODE), aspPreBuildMode);
            
            // if the value has changed then mark that configuration changes have been made
            if (checkValueChange(oldAspPreBuildMode, aspPreBuildMode, "false"))
            {
            	setConfigChanges(project, "true");
            }
        } catch (CoreException e) {
        	e.printStackTrace();
        }
    }
    
    /**
     * Get ASP Simulation mode for this Project.
     * @param res Resource
     * @return ASP simulation mode for this Project.
     */
    public static String getASPSimulationMode(IResource res)
    {
        String mode  = null;
        try {
            mode = res.getPersistentProperty(
                    new QualifiedName("true", BUILD_WITH_SIMULATION_MODE));
        } catch (CoreException e) {
        	e.printStackTrace();
        }
        return mode != null ? mode : "false";
    }
    /**
     * Set the ASP Simulation mode for this Project.
     */
    public static void setASPSimulationMode(IResource project, String aspSimulationMode)
    {
        try {
            String oldaspSimulationMode = getASPSimulationMode(project);
            project.setPersistentProperty(
               new QualifiedName("true", BUILD_WITH_SIMULATION_MODE), aspSimulationMode);
            
            // if the value has changed then mark that configuration changes have been made
            if (checkValueChange(oldaspSimulationMode, aspSimulationMode, "fasle"))
            {
            	setConfigChanges(project, "true");
            }
        } catch (CoreException e) {
        	e.printStackTrace();
        }
    }
    /**
     * Get SNMP build mode for this Project.
     * @param res Resource
     * @return SNMP build mode for this Project.
     */
    public static String getSNMPBuildMode(IResource res)
    {
        String mode  = null;
        try {
            mode = res.getPersistentProperty(
                    new QualifiedName("true", BUILD_WITH_SNMP_MODE));
        } catch (CoreException e) {
        	e.printStackTrace();
        }
        return mode != null ? mode : "true";
    }
    
    /**
     * Set the SNMP build mode for this Project.
     */
    public static void setSNMPBuildMode(IResource project, String snmpBuildMode)
    {
        try {
            String oldSnmpBuildMode = getSNMPBuildMode(project);
            project.setPersistentProperty(
               new QualifiedName("true", BUILD_WITH_SNMP_MODE), snmpBuildMode);
            
            // if the value has changed then mark that configuration changes have been made
            if (checkValueChange(oldSnmpBuildMode, snmpBuildMode, "true"))
            {
            	setConfigChanges(project, "true");
            }
        } catch (CoreException e) {
        	e.printStackTrace();
        }
    }

    /**
     * Get CM build mode for this Project.
     * @param res Resource
     * @return CM build mode for this Project.
     */
    public static String getCMBuildMode(IResource res)
    {
        String mode  = null;
        try {
            mode = res.getPersistentProperty(
                    new QualifiedName("true", BUILD_WITH_CM_MODE));
        } catch (CoreException e) {
        	e.printStackTrace();
        }
        return mode != null ? mode : "false";
    }

    /**
     * Set the CM build mode for this Project.
     */
    public static void setCMBuildMode(IResource project, String cmBuildMode)
    {
        try {
            String oldCmBuildMode = getCMBuildMode(project);
            project.setPersistentProperty(
               new QualifiedName("true", BUILD_WITH_CM_MODE), cmBuildMode);
            
            // if the value has changed then mark that configuration changes have been made
            if (checkValueChange(oldCmBuildMode, cmBuildMode, "false"))
            {
            	setConfigChanges(project, "true");
            }
        } catch (CoreException e) {
        	e.printStackTrace();
        }
    }
    
    /**
	 * Gets Force Configure mode for this Project.
	 * 
	 * @param res
	 *            Resource
	 * @return Force Configure mode for this Project.
	 */
	public static String getForceConfigureMode(IResource res) {
		String mode = null;

		try {
			mode = res.getPersistentProperty(new QualifiedName("true",
					FORCE_CONFIGURE_MODE));

		} catch (CoreException e) {
			e.printStackTrace();
		}

		return mode != null ? mode : "false";
	}

	/**
	 * Sets the Force Configure mode for this Project.
	 * 
	 * @param project
	 * @param forceConfigureMode
	 */
	public static void setForceConfigureMode(IResource project,
			String forceConfigureMode) {

		try {
			project.setPersistentProperty(new QualifiedName("true",
					FORCE_CONFIGURE_MODE), forceConfigureMode);

		} catch (CoreException e) {
			e.printStackTrace();
		}
	}

    /**
	 * Get CM build value for this Project.
	 * 
	 * @param res
	 *            Resource
	 * @return CM build value for this Project.
	 */
    public static String getCMValue(IResource res)
    {
        String mode  = null;
        try {
            mode = res.getPersistentProperty(
                    new QualifiedName("true", BUILD_WITH_CM_VALUE_MODE));
        } catch (CoreException e) {
        	e.printStackTrace();
        }
        return mode != null ? mode : "radisys";
    }

    /**
     * Set the CM build value for this Project.
     */
    public static void setCMValue(IResource project, String cmValue)
    {
        try {
            String oldCmValue = getCMValue(project);
            project.setPersistentProperty(
               new QualifiedName("true", BUILD_WITH_CM_VALUE_MODE), cmValue);
            
            // if the value has changed then mark that configuration changes have been made
            if (checkValueChange(oldCmValue, cmValue, "radisys"))
            {
            	setConfigChanges(project, "true");
            }
        } catch (CoreException e) {
        	e.printStackTrace();
        }
    }

    /**
     * Get IPC build mode for this Project.
     * @param res Resource
     * @return IPC build mode for this Project.
     */
    public static String getIPCBuildMode(IResource res)
    {
        String mode  = null;
        try {
            mode = res.getPersistentProperty(
                    new QualifiedName("true", BUILD_WITH_IPC_MODE));
        } catch (CoreException e) {
        	e.printStackTrace();
        }
        return mode != null ? mode : "false";
    }

    /**
     * Set the IPC build mode for this Project.
     */
    public static void setIPCBuildMode(IResource project, String ipcBuildMode)
    {
        try {
            String oldIpcBuildMode = getIPCBuildMode(project);
            project.setPersistentProperty(
               new QualifiedName("true", BUILD_WITH_IPC_MODE), ipcBuildMode);
            
            // if the value has changed then mark that configuration changes have been made
            if (checkValueChange(oldIpcBuildMode, ipcBuildMode, "false"))
            {
            	setConfigChanges(project, "true");
            }
        } catch (CoreException e) {
        	e.printStackTrace();
        }
    }

    /**
     * Get IPC value for this Project.
     * @param res Resource
     * @return CM build value for this Project.
     */
    public static String getIPCValue(IResource res)
    {
        String mode  = null;
        try {
            mode = res.getPersistentProperty(
                    new QualifiedName("true", BUILD_WITH_IPC_VALUE_MODE));
        } catch (CoreException e) {
        	e.printStackTrace();
        }
        return mode != null ? mode : "radisys";
    }

    /**
     * Set the IPC value for this Project.
     */
    public static void setIPCValue(IResource project, String ipcValue)
    {
        try {
            String oldIpcValue = getIPCValue(project);
            project.setPersistentProperty(
               new QualifiedName("true", BUILD_WITH_IPC_VALUE_MODE), ipcValue);
            
            // if the value has changed then mark that configuration changes have been made
            if (checkValueChange(oldIpcValue, ipcValue, "radisys"))
            {
            	setConfigChanges(project, "true");
            }
        } catch (CoreException e) {
        	e.printStackTrace();
        }
    }

    /**
     * Get Binary build mode for this Project.
     * @param res Resource
     * @return Binary build mode for this Project.
     */
    public static String getBinaryBuildMode(IResource res)
    {
        String mode  = null;
        try {
            mode = res.getPersistentProperty(
                    new QualifiedName("true", BUILD_WITH_BINARY_MODE));
        } catch (CoreException e) {
        	e.printStackTrace();
        }
        return mode != null ? mode : "false";
    }

    /**
     * Set the Binary build mode for this Project.
     */
    public static void setBinaryBuildMode(IResource project, String binaryBuildMode)
    {
        try {
            String oldBinaryBuildMode = getBinaryBuildMode(project);
            project.setPersistentProperty(
               new QualifiedName("true", BUILD_WITH_BINARY_MODE), binaryBuildMode);
            
            // if the value has changed then mark that configuration changes have been made
            if (checkValueChange(oldBinaryBuildMode, binaryBuildMode, "false"))
            {
            	setConfigChanges(project, "true");
            }
        } catch (CoreException e) {
        	e.printStackTrace();
        }
    }

    /**
	 * Get Binary mode build value for this Project.
	 * 
	 * @param res
	 *            Resource
	 * @return Binary mode build value for this Project.
	 */
    public static String getBinaryModeValue(IResource res)
    {
        String mode  = null;
        try {
            mode = res.getPersistentProperty(
                    new QualifiedName("true", BUILD_WITH_BINARY_MODE_VALUE));
        } catch (CoreException e) {
        	e.printStackTrace();
        }
        return mode != null ? mode : "32";
    }

    /**
     * Set the Binary mode build value for this Project.
     */
    public static void setBinaryModeValue(IResource project, String binaryValue)
    {
        try {
            String oldBinaryValue = getBinaryModeValue(project);
            project.setPersistentProperty(
               new QualifiedName("true", BUILD_WITH_BINARY_MODE_VALUE), binaryValue);
            
            // if the value has changed then mark that configuration changes have been made
            if (checkValueChange(oldBinaryValue, binaryValue, "32"))
            {
            	setConfigChanges(project, "true");
            }
        } catch (CoreException e) {
        	e.printStackTrace();
        }
    }

    /**
     * Returns Build Tools Locations
     * @param res Project
     * @return Location
     */
    public static String getBuildToolsLocation(IResource res)
    {
    	String sdkLocation = CwProjectPropertyPage.getSDKLocation(res);
		if(sdkLocation != null && !sdkLocation.equals("")) {
			String buildToolsLoc = new File(sdkLocation).getParent() + File.separator + "buildtools";
			if(new File(buildToolsLoc).exists()) {
				return buildToolsLoc;
			}
		}
    	return "";
    }
    
    /**
     * Checks two strings to see if the new value is different from the old value. Also,
     *  if new value is null check to see if old value is equal to the default value in
     *  which case the new value will NOT be considered as a change.
     * @param oldValue the existing value
     * @param newValue the value to be set
     * @param defaultValue the default value to check if new value is null
     * @return true if the value has changed, false otherwise
     */
    private static boolean checkValueChange(String oldValue, String newValue, String defaultValue)
    {
    	boolean changed = false;
    	
        if (newValue == null)
        {
        	if (!oldValue.equals(defaultValue)) changed = true;
        } else if (!oldValue.equals(newValue)) {
        	changed = true;
        }

        return changed;
    }

    class AntBuildThread implements Runnable {

		String _actionCommand;
		AntRunner ant = null;
		IProject project = null;
		private boolean _isCancelled = false;
		
		public AntBuildThread(AntRunner ant, IProject project, String actionCommand)
		{
			_actionCommand = actionCommand;
			this.ant = ant;
			this.project = project;
		}

		public void run() {
			try {
				ClovisProgressMonitorDialog pmDialog = null;
				pmDialog = new ClovisProgressMonitorDialog(
						Display.getDefault().getActiveShell(), _actionCommand);
				pmDialog.run(true, true, new RunnableCode(ant, project));
				if (pmDialog.getReturnCode() == ClovisProgressMonitorDialog.CANCEL)
				{
					_isCancelled = true;
				}
			} catch (InterruptedException ie) {
				_isCancelled = true;
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
		
		public boolean wasCancelled() { return _isCancelled; }
		
	}
	class RunnableCode implements IRunnableWithProgress, Runnable {

		IProject project = null;
		AntRunner ant = null;

		RunnableCode(AntRunner ant, IProject project) {
			this.ant = ant;
			this.project = project;
		}

		public void run(IProgressMonitor monitor)
				throws InvocationTargetException, InterruptedException {
			try {

				if (monitor.isCanceled()) {
					monitor.done();
					return;
				}
				monitor.beginTask("Configuring : "
						+ project.getName(), IProgressMonitor.UNKNOWN);
				ant.run(monitor);
			} catch (Exception e) {
				_configureSuccess = false;
				e.printStackTrace();
			}
		}

		public void run() {
			// TODO Auto-generated method stub
		}
		
	}

	/**
     * Get URL array of classpaths.
     * Takes classpath urls from Ant and adds current classpath to it
     * so that ClovisConfiguratorLogger class can be loaded.
     * @return Array of Classpath URLs
     * @throws Exception If URL creation failes.
     */
    private URL[] getClassPathURL()
        throws Exception
    {
        URL[] urls = AntCorePlugin.getPlugin().getPreferences().getURLs();
        URL[] newUrls = new URL[urls.length + 1];
        System.arraycopy(urls, 0, newUrls, 1, urls.length);
        //Add the path to the Plugin classes
        String className = this.getClass().getName();
        if (!className.startsWith("/")) {
            className = "/" + className;
        }
        className = className.replace('.', '/');
        String classLoc = getClass().getClassLoader().
            getResource(className + ".class").toExternalForm();
        newUrls[0] = Platform.resolve(new URL(
                classLoc.substring(0, classLoc.indexOf(className))));
        return newUrls;
    }
}
