/*
 * @(#) $RCSfile: Log.java,v $
 * $Revision: #2 $ $Date: 2007/01/03 $
 *
 * Copyright (C) 2002 -- Clovis Solutions.
 * Proprietary and Confidential. All Rights Reserved.
 *
 * This software is the proprietary information of Clovis Solutions.
 * Use is subject to license terms.
 *
 */
/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ecore/EcoreModels.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

/*
 * @(#) $RCSfile: Log.java,v $
 * $Revision: #2 $ $Date: 2007/01/03 $
 *
 * Copyright (C) 2002 -- Clovis Solutions.
 * Proprietary and Confidential. All Rights Reserved.
 *
 * This software is the proprietary information of Clovis Solutions.
 * Use is subject to license terms.
 *
 */
package com.clovis.common.utils.ecore;

import java.io.File;
import java.io.IOException;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.emf.ecore.resource.ResourceSet;
import org.eclipse.emf.ecore.resource.impl.ResourceSetImpl;
import org.eclipse.emf.ecore.xmi.XMLResource;
import org.eclipse.emf.ecore.xmi.impl.XMIResourceFactoryImpl;
import org.eclipse.emf.ecore.xmi.impl.XMLResourceFactoryImpl;
import org.eclipse.emf.ecore.xmi.impl.XMLResourceImpl;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

import com.clovis.common.utils.ClovisDomUtils;
import com.clovis.common.utils.UtilsPlugin;
import com.clovis.common.utils.constants.ProjectConstants;
import com.clovis.common.utils.log.Log;

/**
 * @author nadeem
 * This class maintains a registry of all xmi and ecore models.
 */
public class EcoreModels
{
    private static boolean     isInitialized = false;
    //private static Hashtable   ecoreHash   = new Hashtable();
    private static ResourceSet resourceSet = new ResourceSetImpl();
    // Static Initialization Block.
    private static final Log LOG = Log.getLog(UtilsPlugin.getDefault());
    
    private static Map _registry = new HashMap();
    static {
        init();
    }
    /**
     * Init this class.
     */
    private static void init()
    {
        if (!isInitialized) {
            //Register the XML resource factory
            resourceSet.getResourceFactoryRegistry().
                getExtensionToFactoryMap().put(
                    "xml",
                        new XMLResourceFactoryImpl());
            //Register the default resource factory
            resourceSet.getResourceFactoryRegistry().
                getExtensionToFactoryMap().put(
                    Resource.Factory.Registry.DEFAULT_EXTENSION,
                        new XMIResourceFactoryImpl());
        }
    }
    /**
     * get EPackage from ECore model URI.
     * @param uriString URI for ECore model.
     * @return EPackage for this Ecore.
     * @throws IOException from EcoreModels.read
     */
    public static EPackage get(String uriString)
        throws IOException
    {
        return get(URI.createFileURI(uriString));
    }
    /**
     * get EPackage from ECore model URI.
     * @param  uri uri for ECore model.
     * @return EPackage for this Ecore.
     * @throws IOException from EcoreModels.read
     */
    public static EPackage get(URI uri)
        throws IOException
    {
        //EPackage pack = (EPackage) EPackage.Registry.INSTANCE.get(uri);
    	EPackage pack = (EPackage) _registry.get(uri);
        if (pack != null) {
            return pack;
        } else {
            Resource resource = resourceSet.getResource(uri, true);
            if (resource == null) {
                return null;
            } else {
                pack = (EPackage) resource.getContents().get(0);
                //EPackage.Registry.INSTANCE.put(uri, pack);
                EPackage.Registry.INSTANCE.put(pack.getNsURI(), pack);
                _registry.put(uri, pack);
                //_registry.put(pack.getNsURI(), pack);
                return pack;
            }
        }
    }
    /**
     * get EPackage from ECore model URI.
     * @param uriString URI for ECore model.
     * @return EPackage for this Ecore.
     * @throws IOException from EcoreModels.read
     */
    public static EPackage getUpdated(String uriString)
        throws IOException
    {
        return getUpdated(URI.createFileURI(uriString));
    }
    /**
     * get EPackage from ECore model URI.
     * Always gets from the Resource only, not from registry
     * @param  uri uri for ECore model.
     * @return EPackage for this Ecore.
     * @throws IOException from EcoreModels.read
     */
    public static EPackage getUpdated(URI uri)
        throws IOException
    {
        
        Resource resource = resourceSet.getResource(uri, true);
        if (resource == null) {
            return null;
        } else {
            EPackage pack = (EPackage) resource.getContents().get(0);
            //EPackage.Registry.INSTANCE.put(uri, pack);
            EPackage.Registry.INSTANCE.put(pack.getNsURI(), pack);
            _registry.put(uri, pack);
            //_registry.put(pack.getNsURI(), pack);
            return pack;
        }
        
    }
    /**
     * Read XMI.
     * @param uriString of XMI.
     * @return List of objects read from Resource
     * @throws IOException from URI.createFileURI
     */
    public static EList read(String uriString)
        throws IOException
    {
        return read(URI.createFileURI(uriString));
    }
    /**
     * Read XMI.
     * @param  uri  URI of the resource.
     * @return List of objects read from Resource
     * @throws IOException from resourceSet.getResource()
     */
    public static EList read(URI uri)
        throws IOException
    {
    	uri = convertURIForConfigFiles(uri);
        Resource xmi = resourceSet.getResource(uri, true);
        if (xmi != null) {
            EList eList = (EList) xmi.getContents();
            EcoreUtils.addValidationListener(eList);
            return eList;
        }
        return null;
    }
    /**
     * Creates new resource.
     * @param  uri  URI of the resource.
     * @return New resource created from uri
     */
    public static Resource create(URI uri)
    {
    	uri = convertURIForConfigFiles(uri);
        XMLResourceImpl resource =
            (XMLResourceImpl) resourceSet.createResource(uri);
        resource.setEncoding("UTF-8");
        return resource;
    }
    /**
     * Gets resource for URI
     * @param  uri URI of resource
     * @return Resource for given URI
     */
    public static Resource getResource(URI uri)
    {
    	uri = convertURIForConfigFiles(uri);
        return resourceSet.getResource(uri, true);
    }
    /**
     * Checks if the resource is already loaded in to memory
     * @param uri URI of resource
     * @return true if the Resource is loaded else return false
     */
    public static boolean isResourceLoaded(URI uri)
    {
    	uri = convertURIForConfigFiles(uri);
        Resource resource = resourceSet.getResource(uri, false);
        return (resource != null) ? true : false;
    }
    /**
     * Gets updated resource for URI by reloading the Resource
     * @param  uri URI of resource
     * @return Resource for given URI
     */
    public static Resource getUpdatedResource(URI uri)
    {
    	uri = convertURIForConfigFiles(uri);
        Resource resource = resourceSet.getResource(uri, true);
        resource.unload();
        try {
        resource.load(resourceSet.getLoadOptions());
        } catch (IOException e) {
            LOG.error("XML VALIDATION ERROR:", e);
        }
        List errorList = resource.getErrors(); 
        for (int i = 0; i < errorList.size(); i++) {
        	Object error = errorList.get(i);
        	System.err.println("XML VALIDATION ERROR:" + error);
        }
        List warningList = resource.getWarnings(); 
        for (int i = 0; i < warningList.size(); i++) {
        	Object warning = warningList.get(i);
        	System.err.println("XML VALIDATION ERROR:" + warning);
        }
        return resource;
    }
    /**
     * Save All modified Resources.
     * @throws IOException from resource.save()
     */
    public static void save()
        throws IOException
    {
        List resources = resourceSet.getResources();
        for (int i = 0; i < resources.size(); i++) {
            Resource resource = (Resource) resources.get(i);
            EcoreModels.save(resource);
        }
    }
    /**
     * Save this Resource.
     * @param resource Resource to save.
     * @throws IOException from resource.save()
     */
    public static void save(Resource resource)
        throws IOException
    {
        Map options = new HashMap();
        options.put(XMLResource.OPTION_EXTENDED_META_DATA, Boolean.TRUE);
        resource.save(options);

		String path = resource.getURI().path();
		int index = path.lastIndexOf(File.separator
				+ ProjectConstants.PROJECT_TEMP_DIR + File.separator
				+ ProjectConstants.PROJECT_CONFIG_DIR + File.separator);
		if (index != -1) {
			addVersioningToFile(path, path.substring(0, index)
					+ path.substring(index + 10));
		}
    }

	/**
	 * Saves the given resource.
	 * 
	 * @param resource -
	 *            Resource to be saved
	 */
	public static void saveResource(Resource resource) {
		try {
			EcoreModels.save(resource);
		} catch (IOException e) {
			LOG.error("Resource : '" + resource.getURI()
					+ "' cannot be written");
		}
    }

    /**
	 * Test Method.
	 * 
	 * @author nadeem
	 * @param args
	 *            Program Arguments
	 * @throws Exception
	 *             on errors
	 */
    public static void main(String[] args) throws Exception
    {
        /*if (args.length != 2) {
            System.err.println("Usgae: <ecore_model> <xml_file>");
        }
        System.out.println("-------- EcoreModels: Starting  main() ---------");
        EPackage pack = EcoreModels.get(args[0]);
        List classList = pack.getEClassifiers();
        System.out.println("#Classes = " + classList.size());
        //Read Data XMI
        EObject  root = (EObject) EcoreModels.read(args[1]).get(0);
        System.out.println("Book Title is "
            + root.eGet(root.eClass().getEStructuralFeature("title")));
        System.out.println("-------- EcoreModels: Finishing main() ---------");*/
    }

	/**
	 * Adds the versioning info to the source file and saves it as target.
	 * 
	 * @param source
	 * @param target
	 */
	private static void addVersioningToFile(String source, String target) {
		Document newDocument = ClovisDomUtils.buildDocument("");
		Element rootElement = newDocument.createElement("openClovisAsp");
		newDocument.appendChild(rootElement);

		Element versionElement = newDocument.createElement("version");
		rootElement.appendChild(versionElement);

		versionElement.setAttribute("v0", ProjectConstants.ASP_VERSION);
		versionElement.appendChild(newDocument.importNode(ClovisDomUtils
				.buildDocument(source).getDocumentElement(), true));

		ClovisDomUtils.saveDocument(newDocument, target);
	}

	/**
	 * Converts the URI so that config files are handled properly.
	 * 
	 * @param uri
	 * @return
	 */
	private static URI convertURIForConfigFiles(URI uri) {
		String path = uri.path();

		int index = path.lastIndexOf(File.separator
				+ ProjectConstants.PROJECT_CONFIG_DIR + File.separator);
		if (index != -1) {
			path = path.substring(0, index) + File.separator
					+ ProjectConstants.PROJECT_TEMP_DIR + path.substring(index);
			uri = URI.createFileURI(path);
		}

		return uri;
	}
}
