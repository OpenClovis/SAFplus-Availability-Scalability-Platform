package com.clovis.cw.workspace.codegen;

import java.io.File;
import java.io.IOException;

import org.eclipse.core.resources.IProject;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.widgets.Display;

import com.clovis.cw.licensing.LicenseUtils;
import com.clovis.cw.licensing.server.LicenseServerCommunicator;

/**
 * 
 * @author Pushparaj
 * Class which communicates with License Server to upload the
 * model and generate source
 */
public class RemoteCodeGeneration {
	/**
	 * Upload the model to license server and download the generated files
	 * @param project Project
	 * @param codegenMode code gen option
	 */
	public void generateSource(IProject project, String codeGenMode, String userName, String password) {
		LicenseServerCommunicator comm = new LicenseServerCommunicator();
		String response = comm.upLoadModel(project, codeGenMode, userName, password);
		if(response.equals("")) {
			MessageDialog.openInformation(Display.getCurrent().getActiveShell(), "Model Error", "Unable to create " + project.getName() + ".tgz file");
			return;
		}
		int errorCode = LicenseUtils.getErrorCode(response);
		if(errorCode == 1) {
			int beginIndex = response.indexOf("<a href='", response.indexOf("Success"));
			int endIndex = response.indexOf("'>here", response.indexOf("Success"));
			String remotePath = response.substring(beginIndex + 9, endIndex);	
			String downloadedFilePath = project.getLocation().append("src").append("generatedCode.tgz").toOSString();
			if(new File(downloadedFilePath).exists()) {
				new File(downloadedFilePath).delete();
			}
			comm.downloadGeneratedCode(remotePath, downloadedFilePath);
			if(new File(downloadedFilePath).exists()) {
				try {
					Process proc = Runtime.getRuntime().exec("tar xzvf " + downloadedFilePath + " -C " + project.getLocation().append("src").toOSString());
					proc.waitFor();
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				} catch (InterruptedException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
		} else if(errorCode == -3) {
			MessageDialog.openInformation(Display.getCurrent().getActiveShell(), "License Error", "Your trial has expired.  Please contact Openclovis.");
		} else if(errorCode == -5){
			MessageDialog.openInformation(Display.getCurrent().getActiveShell(), "Model Error", "Bad Model");
		} else {
			System.out.println(response);
			MessageDialog.openInformation(Display.getCurrent().getActiveShell(), "License Server Error", "UnKnown Error\n" + response);
		}
	}
}
