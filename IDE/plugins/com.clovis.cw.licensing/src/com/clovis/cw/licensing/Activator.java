package com.clovis.cw.licensing;

import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.ui.plugin.AbstractUIPlugin;
import org.osgi.framework.BundleContext;

/**
 * The activator class controls the plug-in life cycle
 */
public class Activator extends AbstractUIPlugin {

	// The plug-in ID
	public static final String PLUGIN_ID = "com.clovis.cw.licensing";

	// The shared instance
	private static Activator plugin;
	
	private static UserInfo _userInfo;
	
	private static LicenseInfo _licenseInfo;
	/**
	 * The constructor
	 */
	public Activator() {
	}

	/*
	 * (non-Javadoc)
	 * @see org.eclipse.ui.plugin.AbstractUIPlugin#start(org.osgi.framework.BundleContext)
	 */
	public void start(BundleContext context) throws Exception {
		super.start(context);
		plugin = this;
		_userInfo = LicenseUtils.loadUserInfo();
		_licenseInfo = LicenseUtils.loadLicenseInfo();
	}

	/*
	 * (non-Javadoc)
	 * @see org.eclipse.ui.plugin.AbstractUIPlugin#stop(org.osgi.framework.BundleContext)
	 */
	public void stop(BundleContext context) throws Exception {
		plugin = null;
		if(_userInfo.isVerifiedUser()) {
			LicenseUtils.saveUserInfo(_userInfo);
			LicenseUtils.saveLicenseInfo(_licenseInfo);
		}
		super.stop(context);
	}

	/**
	 * Returns the shared instance
	 *
	 * @return the shared instance
	 */
	public static Activator getDefault() {
		return plugin;
	}

	/**
	 * Returns an image descriptor for the image file at the given
	 * plug-in relative path
	 *
	 * @param path the path
	 * @return the image descriptor
	 */
	public static ImageDescriptor getImageDescriptor(String path) {
		return imageDescriptorFromPlugin(PLUGIN_ID, path);
	}
	
	/**
	 * Returns the UserInfo instance
	 * @return UserInfo instance
	 */
	public static UserInfo getUserInfo() {
		return _userInfo;
	}
	
	/**
	 * Returns the LicenseInfo instance
	 * @return LicenseInfo instance
	 */
	public static LicenseInfo getLicenseInfo() {
		return _licenseInfo;
	}
}
