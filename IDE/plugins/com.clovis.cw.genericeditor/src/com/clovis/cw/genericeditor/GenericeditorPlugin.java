/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor;

import java.util.Locale;
import java.util.MissingResourceException;
import java.util.ResourceBundle;

import org.eclipse.ui.plugin.AbstractUIPlugin;
import org.osgi.framework.BundleContext;

import com.clovis.common.utils.log.Log;


/**
 * The main plugin class to be used in the desktop.
 */
public class GenericeditorPlugin extends AbstractUIPlugin {
	//The shared instance.
	private static GenericeditorPlugin _plugin;
	//Resource bundle.
	private ResourceBundle _resourceBundle;
    //Log
    public static Log LOG;
    
	/**
	 * The constructor.
	 */
	public GenericeditorPlugin() {
		super();
		_plugin = this;
		try {
			_resourceBundle = ResourceBundle.getBundle("com.clovis.cw.genericeditor.GenericeditorPluginResources", Locale.getDefault());
		} catch (MissingResourceException x) {
			_resourceBundle = null;
		}
		LOG = Log.getLog(getDefault());
	}

	/**
	 * This method is called upon plug-in activation
	 */
	public void start(BundleContext context) throws Exception {
		super.start(context);
	}

	/**
	 * This method is called when the plug-in is stopped
	 */
	public void stop(BundleContext context) throws Exception {
		super.stop(context);
	}

	/**
	 * Returns the shared instance.
	 */
	public static GenericeditorPlugin getDefault() {
		return _plugin;
	}

	/**
	 * Returns the string from the plugin's resource bundle,
	 * or 'key' if not found.
	 */
	public static String getResourceString(String key) {
		ResourceBundle bundle = GenericeditorPlugin.getDefault().getResourceBundle();
		try {
			return (bundle != null) ? bundle.getString(key) : key;
		} catch (MissingResourceException e) {
            GenericeditorPlugin.LOG.warn("Unable to get the value for "+key+" from resource bundle");
			return key;
		}
	}

	/**
	 * Returns the plugin's resource bundle,
	 */
	public ResourceBundle getResourceBundle() {
		return _resourceBundle;
	}
    /**
     * @see java.lang.Object#toString()
     */
	public String toString()
	{
		return "com.clovis.cw.genericeditor";
	}
}
