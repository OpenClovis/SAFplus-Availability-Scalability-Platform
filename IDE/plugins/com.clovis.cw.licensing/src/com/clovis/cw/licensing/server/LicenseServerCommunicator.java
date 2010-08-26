package com.clovis.cw.licensing.server;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URL;

import org.eclipse.core.resources.IProject;

import com.clovis.cw.licensing.LicenseUtils;

import HTTPClient.Codecs;
import HTTPClient.HTTPConnection;
import HTTPClient.HTTPResponse;
import HTTPClient.ModuleException;
import HTTPClient.NVPair;
import HTTPClient.ParseException;

/**
 * 
 * @author Pushparaj
 * Class to communicate with License Server
 */
public class LicenseServerCommunicator{
	
	/**
	 * Creates instance
	 */
	public LicenseServerCommunicator() {
		
	}
	/**
	 * UpLoad the model to License Server
	 * @param project IProject
	 * @return String
	 */
	public String upLoadModel(IProject project, String codegenMode, String userName, String password){
		try {
			String tarFilePath = project.getLocation().append(project.getName() + ".tgz").toOSString();
			if(new File(tarFilePath).exists()) {
				new File(tarFilePath).delete();
			}
			Process proc = Runtime.getRuntime().exec("tar czvf " + tarFilePath + " -C " + project.getLocation().toOSString() + " models/ configs/");
			proc.waitFor();
			if(!new File(tarFilePath).exists()) {
				return "";
			}
			HTTPConnection con = new HTTPConnection("ide.openclovis.com");
			// create the NVPair's for the form data to be submitted and do the
			// encoding
			NVPair[] files = { new NVPair("model", tarFilePath) };
			NVPair[] opts = { new NVPair("username", userName),
					new NVPair("password", password) };
			NVPair[] hdrs = new NVPair[1];
			byte[] form_data = null;
			form_data = Codecs.mpFormDataEncode(opts, files, hdrs, null);
			HTTPResponse rsp = con.Post("/generate", form_data, hdrs);
			return rsp.getText();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			return e.getMessage();
		} catch (ModuleException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			return e.getMessage();
		} catch (ParseException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			return e.getMessage();
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			return e.getMessage();
		} 
	}
	/**
	 * Downloads generated code from license server
	 * @param remotePath path for the generated code
	 * @param destination local path
	 */
	public void downloadGeneratedCode(String remotePath, String destination) {
		BufferedInputStream in = null;
        FileOutputStream fout = null;
        try
        {
                in = new BufferedInputStream(new URL("http://ide.openclovis.com" + remotePath).openStream());
                fout = new FileOutputStream(destination);

                byte data[] = new byte[1024];
                int count;
                while ((count = in.read(data, 0, 1024)) != -1)
                {
                        fout.write(data, 0, count);
                }
                in.close();
                fout.close();
        } catch (MalformedURLException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
        finally
        {
                if (in != null)
					try {
						in.close();
					} catch (IOException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
                if (fout != null)
					try {
						fout.close();
					} catch (IOException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
        }
	}
	/**
	 * Verifys the user login info
	 * @param loginName username
	 * @param password pwd
	 * @return
	 */
	public int login(String loginName, String password) {
		int errorCode = 0;
		try {
			HTTPConnection con = new HTTPConnection("ide.openclovis.com");
			NVPair[] form_data = { new NVPair("email", loginName),
					new NVPair("password", password) };
			HTTPResponse rsp = con.Post("/login", form_data);
			String message = rsp.getText();
			errorCode = LicenseUtils.getErrorCode(message);
		} catch (ModuleException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (ParseException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		return errorCode;
	}
}
