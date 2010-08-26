/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ecore/ViewModel.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ecore;

import java.util.HashMap;
import java.util.Map;

import org.eclipse.emf.common.notify.Adapter;
/**
 * @author manish This contains a deep clone of EList of Model. Views will work
 *         on this object
 */
class ViewModel extends Model
{
    // Actual Model.
    private Model      _model;
    private boolean    _isViewModelSaveTriggered = false;
    private HashMap    _viewModelToModelMap      = new HashMap();
    private HashMap    _modelToViewModelMap      = new HashMap();
    private Adapter    _adapter;

    /**
     * @param resource
     * @param m
     *            Actual Model.
     */
    public ViewModel(Model m)
    {
        super(null, null, m.getEPackage());
        _model = m;
        _resource = m.getResource();
        _eList = EcoreCloneUtils.cloneList(m.getEList(),
                    _viewModelToModelMap, _modelToViewModelMap);
        _adapter = new ViewModelAdapter(this);
        EcoreUtils.addListener(m.getEList(), _adapter, -1);
    }
    /**
     * Dispose.
     * Removes the Listener.
     */
    public void dispose()
    {
        EcoreUtils.removeListener(_model.getEList(), _adapter, -1);
    }
    /**
     * Get ViewModel to Model Map. This method is only for
     * ViewModel adapter.
     * @return ViewModel to Model Map.
     */
    Map getViewToModelMap()
    {
        return _viewModelToModelMap;
    }
    /**
     * Get Model to ViewModel Map. This method is only for
     * ViewModel adapter.
     * @return Model to ViewModel Map.
     */
    Map getModelToViewMap()
    {
        return _modelToViewModelMap;
    }
    /**
     * Is this ViewModel saving. This method is only for
     * ViewModel adapter.
     * @return is this viewmode saving.
     */
    boolean isSaving()
    {
        return _isViewModelSaveTriggered;
    }
    /**
     * Commits the ViewModel's EList into Model's EList. It copies the objects.
     * While copying, Model EObjects will raise events. These events will be
     * ignored by ViewModelAdapter.
     *
     * @param saveUp indicating whether it must save() its Model also not.
     *
     */
    public void save(boolean saveUp)
    {
        _isViewModelSaveTriggered = true;
        EcoreCloneUtils.copyEList(getEList(), _model.getEList(),
                  _viewModelToModelMap, _modelToViewModelMap);
        if (saveUp) {
            _model.save(true);
        }
        _isViewModelSaveTriggered = false;
    }
}
