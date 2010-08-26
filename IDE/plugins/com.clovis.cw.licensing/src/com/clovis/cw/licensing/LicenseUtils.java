package com.clovis.cw.licensing;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.security.GeneralSecurityException;
import java.util.Properties;

/**
 * 
 * @author Pushparaj
 * Utils class
 */
public class LicenseUtils {
	private static File _licenseInfoFile, _userInfoFile;
	private static File _pwdKeyFile, _licenseKeyFile;
	static {
		File ideFolder = new File(System.getProperty("user.home")
				+ File.separator + ".clovis" + File.separator + "ide");
		if(!ideFolder.exists()) {
			ideFolder.mkdirs();
		}
		_licenseInfoFile = new File(ideFolder.getAbsolutePath() + File.separator + "license.info");
		_userInfoFile = new File(ideFolder.getAbsolutePath() + File.separator + "user.info");
		_pwdKeyFile = new File(ideFolder.getAbsolutePath() + File.separator + "pwd.key");
		_licenseKeyFile = new File(ideFolder.getAbsolutePath() + File.separator + "license.key");
		if(!_licenseInfoFile.exists()) {
			try {
				_licenseInfoFile.createNewFile();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		if(!_userInfoFile.exists()) {
			try {
				_userInfoFile.createNewFile();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
	}
	/**
	 * Saves license details in properties file
	 * @param licenseInfo LicenseInfo
	 */
	public static void saveLicenseInfo(LicenseInfo licenseInfo) {
		Properties prop = new Properties();
		try {
			prop.setProperty("type", CryptoUtils.encrypt(licenseInfo.getLicenseType(), _licenseKeyFile));
			prop.store(new FileOutputStream(_licenseInfoFile), null);
		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (GeneralSecurityException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	/**
	 * Loads properties file and creates/returns LicenseInfo instance
	 * @return LicenseInfo instance
	 */
	public static LicenseInfo loadLicenseInfo() {
		LicenseInfo info = new LicenseInfo();
		if (_licenseKeyFile.exists()) {
			Properties prop = new Properties();
			try {
				prop.load(new FileInputStream(_licenseInfoFile));
				info.setLicenseType(CryptoUtils.decrypt(prop.getProperty("type"), _licenseKeyFile));
			} catch (FileNotFoundException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (GeneralSecurityException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		return info;
	}
	/**
	 * Saves user details in properties file
	 * @param userInfo UserInfo instance
	 */
	public static void saveUserInfo(UserInfo userInfo) {
		Properties prop = new Properties();
		try {
			prop.setProperty("login", userInfo.getLoginName());
			prop.setProperty("password", CryptoUtils.encrypt(userInfo
					.getPassword(), _pwdKeyFile));
			prop.setProperty("validuser", String.valueOf(userInfo
					.isVerifiedUser()));
			prop.store(new FileOutputStream(_userInfoFile), null);
		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (GeneralSecurityException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	/**
	 * Loads properties file and creates/return UserInfo instance
	 * @return UserInfo instance
	 */
	public static UserInfo loadUserInfo() {
		UserInfo info = new UserInfo();
		if (_pwdKeyFile.exists()) {
			Properties prop = new Properties();
			
			try {
				prop.load(new FileInputStream(_userInfoFile));
				info.setLoginName(prop.getProperty("login"));
				info.setPassword(CryptoUtils.decrypt(prop.getProperty("password"), _pwdKeyFile));
				info.setUserVerification(new Boolean(prop.getProperty("validuser")).booleanValue());
			} catch (FileNotFoundException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (GeneralSecurityException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		return info;
	}
	/**
	 * Parse and returns the error code from the response
	 * @param response Response from the Server 
	 * @return error code
	 */
	public static int getErrorCode(String response) {
		int errorCode = -1;
		if(response.contains(ILicensingConstants.ERROR_CODE_TAG)) {
			int beginIndex = response.indexOf(ILicensingConstants.ERROR_CODE_TAG);
			int endIndex = response.indexOf(">", beginIndex);
			errorCode = Integer.parseInt(response.substring(beginIndex, endIndex).split("\"")[1]);
		}
		return errorCode;
	}
}
