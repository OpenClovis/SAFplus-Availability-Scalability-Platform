/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.data/src/com/clovis/cw/data/DataPlugin.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.data;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.net.URL;
import java.util.MissingResourceException;
import java.util.Properties;
import java.util.ResourceBundle;

import org.eclipse.core.runtime.IPath;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.ui.plugin.AbstractUIPlugin;
import org.osgi.framework.BundleContext;

/**
 * The main plugin class to be used in the desktop.
 */
public class DataPlugin extends AbstractUIPlugin
{
	private static final String ASP_NAME = "ASP";
	private static final String IDE_NAME = "IDE";
    //The shared instance.
    private static DataPlugin plugin;
    //Resource bundle.
    private ResourceBundle _resourceBundle;
    //Product Version 
    private static String _productVersion;
    //Product Update Version 
    private static int _productUpdateVersion = -1;
    //Default ASP Location 
    private static String _defaultSDKLocation;
    //Installed Location
    private static String _installedLocation;
    //Python Location
    private static String _pythonLocation;
    //Project Area Location
    private static String _projectAreaLocation;
    
    /**
     * The constructor.
     */
    public DataPlugin()
    {
        super();
        plugin = this;
        try {
            _resourceBundle = ResourceBundle.getBundle("com.clovis.cw.data"
                    + ".DataPluginResources");
        } catch (MissingResourceException x) {
            _resourceBundle = null;
        }
    }
    /**
     * @see org.osgi.framework.BundleActivator#start(
     * org.osgi.framework.BundleContext)
     */
    public void start(BundleContext context) throws Exception
    {
        super.start(context);
    }
    /**
     * @see org.osgi.framework.BundleActivator#stop(
     * org.osgi.framework.BundleContext)
     */
    public void stop(BundleContext context) throws Exception
    {
        super.stop(context);
    }
    /**
     * Returns the shared instance.
     * @return DataPlugin
     */
    public static DataPlugin getDefault()
    {
        return plugin;
    }
    /**
     * Returns the string from the plugin's resource bundle,
     * or 'key' if not found.
     * @param key key for resource string
     * @return resource string for the key
     */
    public static String getResourceString(String key)
    {
        ResourceBundle bundle = DataPlugin.getDefault().getResourceBundle();
        try {
            return (bundle != null) ? bundle.getString(key) : key;
        } catch (MissingResourceException e) {
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
    	return "com.clovis.cw.data";
    }
    /**
     * Returns Product version
     * @return version
     */
    public static String getProductVersion()
    {
    	if(_productVersion == null) {
    		Properties map = new Properties();
            URL url = getDefault().find(new Path("version.properties"));
            try {
                File aboutFile = new Path(Platform.resolve(url).getPath())
                        .toFile();
                FileInputStream reader = new FileInputStream(aboutFile);
                map.load(reader);
                _productVersion = map.getProperty("release.version");
            } catch (Exception e) {
                return "3.0";
            }
    	}
    	return _productVersion;
    }
    /**
     * Returns Product Update version
     * @return version
     */
    public static int getProductUpdateVersion()
    {
    	if(_productUpdateVersion == -1) {
    		Properties map = new Properties();
            URL url = getDefault().find(new Path("version.properties"));
            try {
                File aboutFile = new Path(Platform.resolve(url).getPath())
                        .toFile();
                FileInputStream reader = new FileInputStream(aboutFile);
                map.load(reader);
                _productUpdateVersion = Integer.parseInt(map.getProperty("update.version"));
            } catch (Exception e) {
                return 0;
            }
    	}
    	return _productUpdateVersion;
    }
    /**
     * Returns Installed Location. This is based on installation
     * @param res Project
     * @return path string
     */
    public static String getInstalledLocation()
    {
    	if(_installedLocation != null)
    		return _installedLocation;
   		String aspLocFile = System.getProperty("user.home") + File.separator
				+ ".clovis" + File.separator + "sdk-"
				+ getProductVersion().substring(0, 3) + File.separator
				+ "install.cache";
		if (!new File(aspLocFile).exists())
			return null;
		try {
			BufferedReader reader = new BufferedReader(new FileReader(
					aspLocFile));
			String line = reader.readLine();
			if(line != null && !line.equals("")) {
				if(new File(line).exists()) {
					return line;
				}
			}
		} catch (FileNotFoundException e) {
			return null;
		} catch (IOException e) {
			return null;
		}
    	return null;
    }
    /**
     * Returns SDK Location
     * @param res Project
     * @return path string
     */
    public static String getDefaultSDKLocation()
    {
    	if(_defaultSDKLocation != null)
    		return _defaultSDKLocation;
    	String areaLoc = getInstalledLocation();
    	if(areaLoc != null && !areaLoc.equals("")) {
    		_defaultSDKLocation = areaLoc + File.separator + "sdk-" + getProductVersion().substring(0, 3);
    		File staticFolder = new File(_defaultSDKLocation + File.separator + IDE_NAME + File.separator + ASP_NAME + File.separator + "static");
    		File templatesFolder = new File(_defaultSDKLocation + File.separator + IDE_NAME + File.separator + ASP_NAME + File.separator + "templates");
    		File buildToolFolder  = new File(new File(_defaultSDKLocation).getParentFile().getAbsolutePath() + File.separator + "buildtools");
    		if(staticFolder.exists() && templatesFolder.exists() && buildToolFolder.exists())
    			return _defaultSDKLocation;
    		else {
    			_defaultSDKLocation = null;
    		}
    	}
    	return "";
	}
    /**
     * Returns Python Location
     * @return path string
     */
    public static String getPythonLocation()
    {
    	if(_pythonLocation != null)
    		return _pythonLocation;
    	String areaLoc = getInstalledLocation();
    	if(areaLoc != null && !areaLoc.equals("")) {
    		_pythonLocation = areaLoc + File.separator + "buildtools" + File.separator + "local" + File.separator + "bin";
    		if(new File(_pythonLocation + File.separator + "python").exists()) {
    			return _pythonLocation;
    		} else {
    			_pythonLocation = null;
    		}
    	}
    	return "";
    }
    /** 	     
     * Returns Project Area Location. This is based first upon the parent of the
     * workspace location (if it is a project area) and then on the output of 
     * cl-create-project-area script
     * @param res Project 	      
     * @return Location
     */ 	      
    public static String getProjectAreaLocation() {

    	if (_projectAreaLocation != null)
			return _projectAreaLocation;

    	// get the ide workspace location's parent directory and see if it is
    	//  a project area
    	IPath workspacePath = Platform.getLocation();
    	File dir = workspacePath.toFile();
    	if (dir.isDirectory() && dir.getParentFile() != null)
    	{
    		
    		File configFile = new File(dir.getParentFile().getAbsolutePath() + File.separator + ".config");
    		if (configFile.exists())
    		{
    			_projectAreaLocation = dir.getParentFile().getAbsolutePath();
    			return _projectAreaLocation;
    		}
    	}
    	
		String aspLocFile = System.getProperty("user.home") + File.separator
				+ ".clovis" + File.separator + "sdk-"
				+ getProductVersion().substring(0, 3) + File.separator
				+ "project-area.rc";
		if (!new File(aspLocFile).exists())
			return "";
		try {
			BufferedReader reader = new BufferedReader(new FileReader(
					aspLocFile));
			String line = null;
			while ((line = reader.readLine()) != null) {
				if (line.indexOf("CL_PROJECT_AREA") != -1) {
					int index = line.indexOf("=") + 1;
					if (new File(line.substring(index, line.length())).exists()) {
						_projectAreaLocation = line.substring(index, line
								.length());
						return _projectAreaLocation;
					}
				}
			}
		} catch (FileNotFoundException e) {
		} catch (IOException e) {
		}
		return "";
	}
}
