/*******************************************************************************
 * ModuleName  : com
 * $File: $
 * $Author: matt $
 * $Date: 2007/10/15 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.builders;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;

import org.eclipse.ant.core.AntCorePlugin;
import org.eclipse.ant.core.AntRunner;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;

import com.clovis.cw.workspace.project.CwProjectPropertyPage;

/**
 * @author matt
 * 
 * Class for generation of Code.
 */
public class MakeImages {
	
	private Shell _shell;
	
	public static String CW = "clovisworks";

	public static final String TRAP_IP_KEY         = "TRAP_IP";
	public static final String CMM_IP_KEY          = "CMM_IP";
	public static final String CMM_AUTH_TYPE_KEY   = "CMM_AUTH_TYPE";
	public static final String CMM_USERNAME_KEY    = "CMM_USERNAME";
	public static final String CMM_PASSWORD_KEY    = "CMM_PASSWORD";
	public static final String TIPC_NETID_KEY      = "TIPC_NETID";
	public static final String CREATE_TARBALLS_KEY = "CREATE_TARBALLS";
	public static final String INSTANTIATE_IMAGES_KEY  = "INSTANTIATE_IMAGES";
	public static final String SLOT_NUM_KEY        = "SLOT_";
	public static final String LINK_INTERFACE_KEY  = "LINK_";
	
	public static final String SLOT_TABLE_KEY = "SLOTS";
	public static final String LINK_TABLE_KEY = "LINKS";
	public static final String ARCH_KEY 	  = "ARCH";
	
	public MakeImages(Shell shell) {
		_shell = shell;
	}
	public MakeImages(){
		//Added for command line tool
	}
	
	/**
	 * Makes images for a set of projects by executing the make images command
	 * through and ant script.
	 * @param projects
	 */
	public void makeImages(IProject[] projects)
	{
		for (int i = 0; i < projects.length; i++) {
			IProject project = projects[i];
			String projectAreaLocation = CwProjectPropertyPage.getProjectAreaLocation(project);

			AntRunner ant = new AntRunner();
			try {
				ant.setCustomClasspath(getClassPathURL());
				ant.addBuildLogger(ClovisMakeImagesLogger.class.getName());
			} catch (Exception e) {
				ant.addBuildLogger("org.apache.tools.ant.DefaultLogger");
			}
			
			String actionCommand = project.getLocation().append("makeImage.sh").toOSString();
			
			StringBuffer buff = new StringBuffer("-Dmakearea.loc=")
			.append(projectAreaLocation + File.separator + project.getName()).append(" -Dscript.file=")
			.append(actionCommand)
			.append(" -Dproject.name=").append(project.getName());

			ant.setBuildFileLocation(project.getLocation().append("makeImage.xml")
					.toOSString());
			ant.setArguments(buff.toString());
			ant.setMessageOutputLevel(org.apache.tools.ant.Project.MSG_INFO);
			
			AntBuildThread thread = new AntBuildThread(ant, project, actionCommand);
			Display.getDefault().syncExec(thread);
		}
	}
	
	
	/**
	 * Get URL array of classpaths. Takes classpath urls from Ant and adds
	 * current classpath to it so that CodeGenLogger class can be loaded.
	 * 
	 * @return Array of Classpath URLs
	 * @throws Exception
	 *             If URL creation failes.
	 */
	private URL[] getClassPathURL() throws Exception {
		URL[] urls = AntCorePlugin.getPlugin().getPreferences().getURLs();
		URL[] newUrls = new URL[urls.length + 1];
		System.arraycopy(urls, 0, newUrls, 1, urls.length);
		// Add the path to the Plugin classes
		String className = this.getClass().getName();
		if (!className.startsWith("/")) {
			className = "/" + className;
		}
		className = className.replace('.', '/');
		String classLoc = getClass().getClassLoader().getResource(
				className + ".class").toExternalForm();
		newUrls[0] = Platform.resolve(new URL(classLoc.substring(0, classLoc
				.indexOf(className))));
		return newUrls;
	}
	
	/**
	 * Reads information from the target.conf file for the given project
	 * and loads this data into a HashMap.
	 * @param project
	 * @return
	 */
	public static HashMap readTargetConf(IProject project)
	{
		HashMap<String, Object> settings = new HashMap<String, Object>();
		HashMap<String, String> slotConfig = new HashMap<String, String>();
		HashMap<String, String> linkConfig = new HashMap<String, String>();
		
		File targetConf = project.getLocation().append("src").append("target.conf").toFile();

		try
		{
			if (targetConf.exists())
			{
				FileReader input = new FileReader(targetConf);
				BufferedReader bufRead = new BufferedReader(input);

				String line;

				line = bufRead.readLine();

				while (line != null) {
					line = line.trim();
					
					if (!line.startsWith("#")) //ignore comment lines
					{
						if (line.startsWith(TRAP_IP_KEY + "="))
						{
							settings.put(TRAP_IP_KEY, line.substring(line.indexOf("=") + 1).trim());
						}
						
						if (line.startsWith(CMM_IP_KEY + "="))
						{
							settings.put(CMM_IP_KEY, line.substring(line.indexOf("=") + 1).trim());
						}
						
						if (line.startsWith(CMM_AUTH_TYPE_KEY + "="))
						{
							settings.put(CMM_AUTH_TYPE_KEY, line.substring(line.indexOf("=") + 1).trim());
						}
						
						if (line.startsWith(CMM_USERNAME_KEY + "="))
						{
							settings.put(CMM_USERNAME_KEY, line.substring(line.indexOf("=") + 1).trim());
						}
						
						if (line.startsWith(CMM_PASSWORD_KEY + "="))
						{
							settings.put(CMM_PASSWORD_KEY, line.substring(line.indexOf("=") + 1).trim());
						}
						
						if (line.startsWith(TIPC_NETID_KEY + "="))
						{
							settings.put(TIPC_NETID_KEY, line.substring(line.indexOf("=") + 1).trim());
						}
						
						if (line.startsWith(CREATE_TARBALLS_KEY + "="))
						{
							settings.put(CREATE_TARBALLS_KEY, line.substring(line.indexOf("=") + 1).trim());
						}
						
						if (line.startsWith(INSTANTIATE_IMAGES_KEY + "="))
						{
							settings.put(INSTANTIATE_IMAGES_KEY, line.substring(line.indexOf("=") + 1).trim());
						}
						
						if (line.startsWith(ARCH_KEY + "="))
						{
							settings.put(ARCH_KEY, line.substring(line.indexOf("=") + 1).trim());
						}
						
						if (line.startsWith(SLOT_NUM_KEY))
						{
							int equalLoc = line.indexOf("=");
							slotConfig.put(line.substring(0, equalLoc), line.substring(equalLoc + 1).trim());
						}
						
						if (line.startsWith(LINK_INTERFACE_KEY))
						{
							int equalLoc = line.indexOf("=");
							linkConfig.put(line.substring(0, equalLoc), line.substring(equalLoc + 1).trim());
						}
					}
					
					line = bufRead.readLine();
				}

				bufRead.close();
				
				settings.put(SLOT_TABLE_KEY, slotConfig);
				settings.put(LINK_TABLE_KEY, linkConfig);
			} else {
				settings = null;
			}
			
		} catch (IOException e) {
			e.printStackTrace();
		}
		
		return settings;
		
	}
	
	/**
	 * Takes HashMap of settings and writes them to the target.conf file for the project.
	 * This is done by reading the target.conf file and matching the identifiers against
	 * what is in the HashMap. At the end if there are any settings in the HashMap that
	 * have not been matched they are written out at the end of the file.
	 * @param project
	 * @param settings
	 */
	public static void writeTargetConf(IProject project, HashMap settings)
	{
		File targetConf = project.getLocation().append("src").append("target.conf").toFile();
		File tempTargetConf = project.getLocation().append("src").append("target.tmp").toFile();
		
		HashMap localSettings = (HashMap)settings.clone();
		HashMap slots = (HashMap)localSettings.get(SLOT_TABLE_KEY);
		if (slots == null) slots = new HashMap();
		localSettings.remove(SLOT_TABLE_KEY);
		HashMap links = (HashMap)localSettings.get(LINK_TABLE_KEY);
		if (links == null) links = new HashMap();
		localSettings.remove(LINK_TABLE_KEY);
		
		boolean slotsProcessed = false;
		boolean linksProcessed = false;
		boolean cmmProcessed = false;
		boolean eatLine = false;
		
		try
		{
			if (targetConf.exists())
			{
				FileReader input = new FileReader(targetConf);
				BufferedReader bufRead = new BufferedReader(input);
				
				FileWriter output = new FileWriter(tempTargetConf);
				BufferedWriter bufWrite = new BufferedWriter(output);

				String line;

				line = bufRead.readLine();

				while (line != null) {
					eatLine = false;
					line = line.trim();
					
					if (!line.startsWith("#")) //ignore comment lines
					{
						if (line.startsWith(TRAP_IP_KEY + "="))
						{
							if (localSettings.get(TRAP_IP_KEY) != null)
							{
								line = TRAP_IP_KEY + "=" + localSettings.get(TRAP_IP_KEY);
								localSettings.remove(TRAP_IP_KEY);
							} else {
								eatLine = true;
							}
						}
						
						// process all chassis management settings at once so that they
						//  are all together in the target.conf file
						if (line.startsWith(CMM_IP_KEY + "=")
							|| line.startsWith(CMM_AUTH_TYPE_KEY + "=")
							|| line.startsWith(CMM_USERNAME_KEY + "=")
							|| line.startsWith(CMM_PASSWORD_KEY + "="))
						{
							if (!cmmProcessed)
							{
								processCMMSettings(bufWrite, localSettings);
								cmmProcessed = true;
								eatLine=true;
							} else {
								eatLine=true;
							}
						}
						
						if (line.startsWith(TIPC_NETID_KEY + "="))
						{
							if (localSettings.get(TIPC_NETID_KEY) != null)
							{
								line = TIPC_NETID_KEY + "=" + localSettings.get(TIPC_NETID_KEY);
								localSettings.remove(TIPC_NETID_KEY);
							} else {
								eatLine = true;
							}
						}
						
						if (line.startsWith(CREATE_TARBALLS_KEY + "="))
						{
							if (localSettings.get(CREATE_TARBALLS_KEY) != null)
							{
								line = CREATE_TARBALLS_KEY + "=" + localSettings.get(CREATE_TARBALLS_KEY);
								localSettings.remove(CREATE_TARBALLS_KEY);
							} else {
								eatLine = true;
							}
						}
						
						if (line.startsWith(INSTANTIATE_IMAGES_KEY + "="))
						{
							if (localSettings.get(INSTANTIATE_IMAGES_KEY) != null)
							{
								line = INSTANTIATE_IMAGES_KEY + "=" + localSettings.get(INSTANTIATE_IMAGES_KEY);
								localSettings.remove(INSTANTIATE_IMAGES_KEY);
							} else {
								eatLine = true;
							}
						}
						
						// process all of the slot number assignments at one time so that 
						//  they are all together in the target.conf
						if (line.startsWith(SLOT_NUM_KEY))
						{
							if (!slotsProcessed)
							{
								if (slots != null)
								{
									Iterator iter = slots.keySet().iterator();
									while (iter.hasNext())
									{
										String key = (String)iter.next();
										String value = (String)slots.get(key);
										bufWrite.write(SLOT_NUM_KEY + key + "=" + value);
										bufWrite.newLine();
									}
									slots.clear();
								}
								slotsProcessed = true;
								eatLine = true;
							} else {
								eatLine = true;
							}
						}
						
						// process all of the link interface assignments at one time so that 
						//  they are all together in the target.conf
						if (line.startsWith(LINK_INTERFACE_KEY))
						{
							if (!linksProcessed)
							{
								if (links != null)
								{
									Iterator iter = links.keySet().iterator();
									while (iter.hasNext())
									{
										String key = (String)iter.next();
										String value = (String)links.get(key);
										bufWrite.write(LINK_INTERFACE_KEY + key + "=" + value);
										bufWrite.newLine();
									}
									links.clear();
								}
								linksProcessed = true;
								eatLine = true;
							} else {
								eatLine = true;
							}
						}
					}
					
					if (!eatLine)
					{
						bufWrite.write(line);
						bufWrite.newLine();
					}
					line = bufRead.readLine();
				}
				
				// we may have some settings that weren't found in the target.conf
				//  so output them now
				outputRemainingSettings(bufWrite, localSettings, slots, links);

				bufWrite.flush();
				output.close();
				bufRead.close();
				
				targetConf.delete();
				tempTargetConf.renameTo(project.getLocation().append("src").append("target.conf").toFile());
			}
			
		} catch (IOException e) {
			e.printStackTrace();
		}
		
	}
	
	/**
	 * Process the Chassis Management target.conf settings by writing them out
	 * to the output buffer.
	 * @param bufWrite
	 * @param localSettings
	 */
	private static void processCMMSettings(BufferedWriter bufWrite, HashMap localSettings)
	{
		ArrayList list = new ArrayList();
		
		try
		{
			if (localSettings != null)
			{
				Iterator iter = localSettings.keySet().iterator();
				while (iter.hasNext())
				{
					String key = (String)iter.next();
					if (key.startsWith("CMM_"))
					{
						String value = (String)localSettings.get(key);
						bufWrite.write(key + "=" + value);
						bufWrite.newLine();
						list.add(key);
					}
				}

				for (int i=0; i<list.size(); i++)
				{
					localSettings.remove(list.get(i));
				}
			}
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	
	/**
	 * Output any remaining settings in the settings map to the output buffer.
	 * @param bufWrite
	 * @param localSettings
	 * @param slots
	 * @param links
	 */
	private static void outputRemainingSettings(BufferedWriter bufWrite, HashMap localSettings,
												HashMap slots, HashMap links)
	{
		try
		{
			// add any remaining settings
			Iterator iter = localSettings.keySet().iterator();
			while (iter.hasNext())
			{
				String key = (String)iter.next();
				if (localSettings.get(key) != null)
				{
					bufWrite.write(key + "=" + localSettings.get(key));
					bufWrite.newLine();
				}
			}
		
			// add any remaining slot assignments
			iter = slots.keySet().iterator();
			while (iter.hasNext())
			{
				String key = (String)iter.next();
				if (slots.get(key) != null)
				{
					bufWrite.write(SLOT_NUM_KEY + key + "=" + slots.get(key));
					bufWrite.newLine();
				}
			}
		
			// add any remaining link interfaces
			iter = links.keySet().iterator();
			while (iter.hasNext())
			{
				String key = (String)iter.next();
				if (links.get(key) != null)
				{
					bufWrite.write(LINK_INTERFACE_KEY + key + "=" + links.get(key));
					bufWrite.newLine();
				}
			}
		
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
	
	class AntBuildThread implements Runnable {

		String _actionCommand;
		AntRunner ant = null;
		IProject project = null;
		Exception ex = null;

		public AntBuildThread(AntRunner ant, IProject project, String actionCommand)
		{
			_actionCommand = actionCommand;
			this.ant = ant;
			this.project = project;
		}
		public void run() {

			ClovisProgressMonitorDialog pmDialog = null;
			pmDialog = new ClovisProgressMonitorDialog(
					Display.getDefault().getActiveShell(), _actionCommand);
			try {
				pmDialog.run(true, true, new RunnableCode(ant, project));
			} catch (InvocationTargetException e) {
				ex = e;
			} catch (InterruptedException e) {
				ex = e;
			}

		}

		public Exception getException() {
			return this.ex;
		}
	}
	
	class RunnableCode implements IRunnableWithProgress, Runnable {
		IProject project = null;
		AntRunner ant = null;
		Exception ex = null;
		RunnableCode(AntRunner ant, IProject project) {
			this.project = project;
			this.ant = ant;
		}
		public void run(IProgressMonitor monitor)
			throws InvocationTargetException, InterruptedException {
			try {
				
					if (project != null) {
						if (monitor.isCanceled()) {
							monitor.done();
							return;
						}
						monitor.beginTask(
							"Making Images for : "
								+ project.getName(),
							IProgressMonitor.UNKNOWN);
						ant.run(monitor);
						try {
							project.refreshLocal(
								IResource.DEPTH_INFINITE,
								null);
							// donot need for monitor
						} catch (java.lang.Exception e) {
							ex = e;
						}
					}
				
			} catch (Exception e) {
				ex = e;
			}
		}
		public Exception getException() {
			return ex;
		}
		public void run() {
			
		}
	}
}
