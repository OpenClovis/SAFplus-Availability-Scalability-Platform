/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils;

import java.io.IOException;
import java.util.MissingResourceException;
import java.util.ResourceBundle;

import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.ui.plugin.AbstractUIPlugin;
import org.osgi.framework.BundleContext;

/**
 * The main plugin class to be used in the desktop.
 */
public class UtilsPlugin extends AbstractUIPlugin
{
    //The shared instance.
    private static UtilsPlugin plugin;
    //Resource bundle.
    private ResourceBundle _resourceBundle;
    
    private static IDialogSettings _dialogSettings;
    
    private static String _settingsFile; 
    
    private static Boolean _isCMDLine;
    /**
     * The constructor.
     */
    public UtilsPlugin()
    {
        super();
        plugin = this;
        try {
            _resourceBundle = ResourceBundle.getBundle(
                    "com.clovis.common.utils.UtilsPluginResources");
        } catch (MissingResourceException x) {
            _resourceBundle = null;
        }
    }
    /**
     * @return the shared instance.
     */
    public static UtilsPlugin getDefault()
    {
        return plugin;
    }
    /**
     * @return the string from the plugin's resource bundle,
     * or 'key' if not found.
     * @param key Key
     */
    /**
     * @see org.osgi.framework.BundleActivator#start(org.osgi.framework.
     * BundleContext)
     */
    public void start(BundleContext context) throws Exception
    {
        super.start(context);
        _dialogSettings = getDefault().getDialogSettings();
        _settingsFile = getDefault().getStateLocation().append(".dialog").toOSString();
    }
    public static String getResourceString(String key)
    {
        ResourceBundle bundle =
            UtilsPlugin.getDefault().getResourceBundle();
        try {
            return (bundle != null) ? bundle.getString(key) : key;
        } catch (MissingResourceException e) {
            return key;
        }
    }
    /**
     * @return the plugin's resource bundle,
     */
    public ResourceBundle getResourceBundle()
    {
        return _resourceBundle;
    }
    /**
     * @see java.lang.Object#toString()
     */
    public String toString()
    {
    	return "com.clovis.common.utils";
    }
	/**
	 * 
	 * @return true if IDE is running in CMD_TOOL mode else return false 
	 */ 
	public static boolean isCmdToolRunning() {
		// This code needs to be cleaned. This method should use some eclipse apis
		// to idendify the mode(cmd/ide)
		if (_isCMDLine != null) {
			return _isCMDLine.booleanValue();
		}
		String[] args = Platform.getCommandLineArgs();
		for (int i = 0; i < args.length; i++) {
			if (args[i].indexOf("-application") != -1) {
				for (int j = 0; j < args.length; j++) {
					if (args[j].indexOf("-perspective") != -1) {
						_isCMDLine = new Boolean(false);
						return false;
					}
				}
				_isCMDLine = new Boolean(true);
				return true;
			} else if (args[i].indexOf("-perspective") != -1) {
				_isCMDLine = new Boolean(false);
				return false;
			}
		}
		_isCMDLine = new Boolean(false);
		return false;
	}

	/**
	 * Save DialogSettings
	 * @param key
	 * @param value
	 */
	public static void saveDialogSettings(String key, String value) {
		_dialogSettings.put(key, value);
		try {
			_dialogSettings.save(_settingsFile);
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	/**
	 * Get value from DialogSettings
	 * @param key
	 * @return
	 */
	public static String getDialogSettingsValue(String key) {
		return _dialogSettings.get(key);
	}
}
