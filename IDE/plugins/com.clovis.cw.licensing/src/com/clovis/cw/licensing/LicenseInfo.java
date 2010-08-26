package com.clovis.cw.licensing;

/**
 * 
 * @author Pushparaj
 * Class for license information
 */
public class LicenseInfo {
	private String _licenseType;
	
	/**
	 * Creates LicenseInfo instance with 'preview' license type
	 */
	public LicenseInfo() {
		_licenseType = ILicensingConstants.OPENCLOVIS_PREVIEW_LICENSE;
	}
	/**
	 * Creates LicenseInfo instance with license type
	 * @param type license type
	 */
	public LicenseInfo(String type) {
		_licenseType = type;
	}
	/**
	 * Set the license type
	 * @param type license type
	 */
	public void setLicenseType(String type) {
		_licenseType = type;
	}
	/**
	 * Returns license type
	 * @return license type
	 */
	public String getLicenseType() {
		return _licenseType;
	}
}
