package com.clovis.cw.workspace.builders;

import java.io.File;
import java.lang.reflect.InvocationTargetException;
import java.net.URL;

import org.eclipse.ant.core.AntCorePlugin;
import org.eclipse.ant.core.AntRunner;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.swt.widgets.Display;

import com.clovis.cw.workspace.WorkspacePlugin;

public class DeployImages {

	/**
	 * UpLoad file to remote location
	 * @param userName
	 * @param password
	 * @param remoteAddress
	 * @param remoteLocation
	 * @throws Exception 
	 */
	public void upLoadFile(String nodeName, String imageName, String imagePath, String userName,
			String password, String remoteAddress, String remoteLocation,
			IProject project,boolean tarballSetting, boolean specificImages) throws Exception {
		AntRunner ant = new AntRunner();
		ant.setCustomClasspath(getClassPathURL());
		try {
			ant.addBuildLogger(ClovisDeployImageLogger.class.getName());
		} catch(Exception e) {
			ant.addBuildLogger("org.apache.tools.ant.DefaultLogger");
		}
		
		StringBuffer buff = new StringBuffer("-Dnode.name=").append(nodeName).append(" -Dimage.name=").append(imageName).append(" -Dimage.path=").append(imagePath)
				.append(" -Duser.name=").append(userName).append(" -Duser.pwd=")
				.append(password).append(" -Dremote.address=").append(
						remoteAddress).append(" -Dremote.location=").append(
						remoteLocation);
		if (tarballSetting && specificImages) {
			buff.append(" -Dspecific.tar=").append(true);
		} 
		if(tarballSetting && !specificImages) {
			buff.append(" -Dcommon.tar=").append(true);
		}
		if (!tarballSetting && specificImages) {
			buff.append(" -Dspecific.dir=").append(true);
		} 
		if(!tarballSetting && !specificImages) {
			buff.append(" -Dcommon.dir=").append(true);
		}				
		ant.setBuildFileLocation(project.getLocation().append("deployImage.xml").toOSString());
		ant.setArguments(buff.toString());
		ant.setMessageOutputLevel(org.apache.tools.ant.Project.MSG_INFO);
		AntBuildThread thread = new AntBuildThread(ant, imagePath,
				remoteAddress, remoteLocation, project);
		Display.getDefault().syncExec(thread);
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
		URL[] newUrls = new URL[urls.length + 2];
		System.arraycopy(urls, 0, newUrls, 2, urls.length);
		// Add the path to the Plugin classes
		String className = getClass().getName();
		if (!className.startsWith("/")) {
			className = "/" + className;
		}
		className = className.replace('.', '/');
		String classLoc = getClass().getClassLoader().getResource(
				className + ".class").toExternalForm();
		newUrls[0] = Platform.resolve(new URL(classLoc.substring(0, classLoc
				.indexOf(className))));
		newUrls[1] = Platform.resolve(WorkspacePlugin.getDefault().find(new Path("ant" + File.separator + "jsch-0.1.29.jar")));
		return newUrls;
	}
}

class AntBuildThread implements Runnable {

	IProject _project;
	AntRunner ant = null;
	String imagePath;
	String remoteLocation;
	String remoteAddress;

	Exception ex = null;

	public AntBuildThread(AntRunner ant, String imagePath,
			String remoteAddress, String remoteLocation,
			IProject project) {

		_project = project;
		this.ant = ant;
		this.imagePath = imagePath;
		this.remoteAddress = remoteAddress;
		this.remoteLocation = remoteLocation;
	}

	public void run() {
		ClovisProgressMonitorDialog pmDialog = null;
		pmDialog = new ClovisProgressMonitorDialog(
				Display.getDefault().getActiveShell());
		try {
			pmDialog.run(true, true, new RunnableCode(ant, imagePath, remoteAddress, remoteLocation));
		} catch (InvocationTargetException e) {
			e.printStackTrace();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}

	}

	public Exception getException() {
		return this.ex;
	}
}

class RunnableCode implements IRunnableWithProgress, Runnable {
	AntRunner ant = null;

	String imagePath;

	String remoteLocation;

	String remoteAddress;

	Exception ex = null;

	RunnableCode(AntRunner ant, String imagePath, String remoteAddress,
			String remoteLocation) {
		this.ant = ant;
		this.imagePath = imagePath;
		this.remoteAddress = remoteAddress;
		this.remoteLocation = remoteLocation;
	}

	public void run(IProgressMonitor monitor) throws InvocationTargetException,
			InterruptedException {
		try {
			if (monitor.isCanceled()) {
				monitor.done();
				return;
			}
			monitor.beginTask("Copying  " + imagePath + " to " + remoteAddress,
					IProgressMonitor.UNKNOWN);
			ant.run(monitor);
			ant.wait(20000);
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
