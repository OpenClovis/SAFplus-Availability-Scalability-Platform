/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/


package com.clovis.cw.editor.ca;

import java.util.MissingResourceException;
import java.util.ResourceBundle;

import org.eclipse.ui.plugin.AbstractUIPlugin;
import org.osgi.framework.BundleContext;

import com.clovis.common.utils.log.Log;


/**
 * @author pushparaj
 *
 *  The main plugin class to be used in the desktop.
 */
public class CaPlugin extends AbstractUIPlugin
{
    //The shared instance.
    private static CaPlugin plugin;

    //Resource bundle.
    private ResourceBundle  _resourceBundle;
    
    //Log
    public static Log LOG = null;
    
    /**
     * The constructor.
     */
    public CaPlugin()
    {
        super();
        plugin = this;
        try {
            _resourceBundle = ResourceBundle
                    .getBundle("com.clovis.cw.editor.ca.CaPluginResources");
        } catch (MissingResourceException x) {
            _resourceBundle = null;
        }
        LOG = Log.getLog(getDefault());
    }
    /**
     * @see org.osgi.framework.BundleActivator#start(org.osgi.framework.
     * BundleContext)
     */
    public void start(BundleContext context) throws Exception
    {
        super.start(context);
    }
    /**
     * @see org.osgi.framework.BundleActivator#stop(org.osgi.framework.
     * BundleContext)
     */
    public void stop(BundleContext context) throws Exception
    {
        super.stop(context);
    }
    /**
     * Returns the shared instance.
     * @return Plugin
     */
    public static CaPlugin getDefault()
    {
        return plugin;
    }
    /**
     * Returns the string from the plugin's resource bundle, or 'key' if not
     * found.
     * @param key key for resource
     * @return Resource String
     */
    public static String getResourceString(String key)
    {
        ResourceBundle bundle = CaPlugin.getDefault().getResourceBundle();
        try {
            return (bundle != null) ? bundle.getString(key) : key;
        } catch (MissingResourceException e) {
            LOG.warn("Unable to get the value for " + key
                     + " from resource bundle");
            return key;
        }
    }

    /**
     * Returns the plugin's resource bundle,
     * @return ResourceBundle
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
	   return "com.clovis.cw.editor.ca";
   }
}
