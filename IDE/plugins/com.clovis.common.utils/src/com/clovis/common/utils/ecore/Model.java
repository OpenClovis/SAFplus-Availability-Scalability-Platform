/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ecore/Model.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ecore;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.emf.common.notify.NotifyingList;

import com.clovis.common.utils.log.Log;
import com.clovis.common.utils.UtilsPlugin;
/**
 * @author manish
 * This is the main Data Model class. It keeps the data in the form of EList
 */
public class Model
{
    protected NotifyingList    _eList;
    protected Resource         _resource;
    protected EPackage         _pack;
    protected boolean          _isDirty;
    private static final Log LOG = Log.getLog(UtilsPlugin.getDefault());
    /**
     * Creates a model from an EObject.
     * @param resource Resource where the Object belongs to.
     * @param eObj EObject
     */
    public Model(Resource resource, EObject eObj)
    {
        this(resource,
            new ClovisNotifyingListImpl(), eObj.eClass().getEPackage());
        _eList.add(eObj);
    }
    /**
     * Constructor.
     * @param resource Resource for the List
     * @param list     List
     * @param pack     EPackage
     */
    public Model(Resource resource, NotifyingList list, EPackage pack)
    {
        _resource = resource;
        _eList    = list;
        _pack     = pack;
    }
    /**
     * Save the Model into the XMI file.
     * @param saveUp to save models above this model.
     */
    public void save(boolean saveUp)
    {
        if (_resource != null) {
            try {
                EcoreModels.save(_resource);
            } catch (Exception ex) {
                LOG.error("Model Saving Failed.", ex);
            }
            _isDirty = false;
        }
    }
    /**
     * Disposes the Model.
     */
    public void dispose()
    {
    }
    /**
     * Set dirty flag of Model
     * @return Returns the _isDirty.
     */
    public boolean isDirty()
    {
        return _isDirty;
    }
    /**
     * Set dirty flag on Model
     * @param dirty The _isDirty to set.
     */
    public void setDirty(boolean dirty)
    {
        _isDirty = dirty;
    }
    /**
     * Resturn the Resource
     * @return Resource
     */
    public Resource getResource()
    {
        return _resource;
    }
    /**
     * Return the EList
     * @return EList
     */
    public NotifyingList getEList()
    {
        return _eList;
    }
    /**
     * Return the EObject
     * @return EObject
     */
    public EObject getEObject()
    {
        return (EObject) _eList.get(0);
    }
    /**
     * Gets Package.
     * @return EPckage for this Model.
     */
    public EPackage getEPackage()
    {
        return _pack;
    }
    /**
     * Returns a View Model containing clone of Model's EList.
     * @return View Model
     */
    public Model getViewModel() { return new ViewModel(this); }
}
