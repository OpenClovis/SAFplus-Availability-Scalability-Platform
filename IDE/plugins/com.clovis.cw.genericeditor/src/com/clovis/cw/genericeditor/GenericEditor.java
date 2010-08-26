/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.genericeditor/src/com/clovis/cw/genericeditor/GenericEditor.java $
 * $Author: srajyaguru $
 * $Date: 2007/04/30 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.genericeditor;

import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import java.io.File;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.EventObject;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.gef.EditPartViewer;
import org.eclipse.gef.GraphicalViewer;
import org.eclipse.gef.KeyHandler;
import org.eclipse.gef.KeyStroke;
import org.eclipse.gef.commands.CommandStack;
import org.eclipse.gef.dnd.TemplateTransferDragSourceListener;
import org.eclipse.gef.editparts.ScalableFreeformRootEditPart;
import org.eclipse.gef.editparts.ZoomManager;
import org.eclipse.gef.palette.PaletteRoot;
import org.eclipse.gef.requests.CreationFactory;
import org.eclipse.gef.ui.actions.ActionRegistry;
import org.eclipse.gef.ui.actions.CopyTemplateAction;
import org.eclipse.gef.ui.actions.GEFActionConstants;
import org.eclipse.gef.ui.actions.ZoomInAction;
import org.eclipse.gef.ui.actions.ZoomOutAction;
import org.eclipse.gef.ui.palette.PaletteViewer;
import org.eclipse.gef.ui.palette.PaletteViewerPreferences;
import org.eclipse.gef.ui.palette.PaletteViewerProvider;
import org.eclipse.gef.ui.palette.FlyoutPaletteComposite.FlyoutPreferences;
import org.eclipse.gef.ui.parts.GraphicalEditor;
import org.eclipse.gef.ui.parts.GraphicalEditorWithFlyoutPalette;
import org.eclipse.gef.ui.parts.ScrollingGraphicalViewer;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.IEditorInput;
import org.eclipse.ui.IEditorPart;
import org.eclipse.ui.IEditorSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.actions.ActionFactory;
import org.eclipse.ui.views.contentoutline.IContentOutlinePage;

import com.clovis.common.utils.ecore.EcoreCloneUtils;
import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.menu.EnvironmentNotifier;
import com.clovis.common.utils.menu.MenuBuilder;
import com.clovis.cw.genericeditor.actions.GEAutoArrangeAction;
import com.clovis.cw.genericeditor.actions.GECollapseAction;
import com.clovis.cw.genericeditor.actions.GECopyAction;
import com.clovis.cw.genericeditor.actions.GECutAction;
import com.clovis.cw.genericeditor.actions.GEDeleteAction;
import com.clovis.cw.genericeditor.actions.GEExpandAction;
import com.clovis.cw.genericeditor.actions.GEPasteAction;
import com.clovis.cw.genericeditor.actions.GESaveAsImageAction;
import com.clovis.cw.genericeditor.editparts.GraphicalPartFactory;
import com.clovis.cw.genericeditor.engine.Parser;
import com.clovis.cw.genericeditor.model.EditorModel;
import com.clovis.cw.genericeditor.outline.OutlineViewPage;
import com.clovis.cw.project.utils.FormatConversionUtils;

/**
 *
 * @author pushparaj
 *
 * Base class for Editors.
 */
public abstract class GenericEditor extends GraphicalEditorWithFlyoutPalette
        implements Environment, PropertyChangeListener
{
    private KeyHandler               _sharedKeyHandler;
    private ScrollingGraphicalViewer _graphicalViewer;
    private EditorModel              _editorModel          = null;
    private EditorPropertiesUtil     _propertiesUtil;
    private static final String      PALETTE_DOCK_LOCATION = "Dock location";
    private static final String      PALETTE_SIZE          = "Palette Size";
    private static final String      PALETTE_STATE         = "Palette state";
    private static final int         DEFAULT_PALETTE_SIZE  = 130;
    private GenericEditorInput       _geInput;
    private OutlineViewPage          _fOutlinePage         =  null;
    protected Parser                 _parser;
    protected Map                    _elementFactoriesMap  = new HashMap();
    protected Model                    _model                = null;
    private Map                    _propertyValMap = new HashMap();
    private GraphicalPartFactory     _grapFactory = new GraphicalPartFactory();
    private Thread         _backupThread;
    private boolean     _isBackupNeeded = true;
    private String        _backupFilePath = null;
    private String         _xmiFilePath = null;
    private Map			_toolEntriesMap = new HashMap();
    private Point       _cursorLocation = new Point(1,1);
    
    static {
        GenericeditorPlugin.getDefault().getPreferenceStore().setDefault(
                PALETTE_SIZE, DEFAULT_PALETTE_SIZE);
    }
    /**
     * Constructs the editor part.
     */
    protected GenericEditor()
    {
    	
    }
    /**
     * Gets Values from Environment.
     * @param key Key for which value is required.
     * @return Object
     */
    public Object getValue(Object key)
    {
        if (key.equals("selection")) {
            StructuredSelection selection = (StructuredSelection)
            _graphicalViewer.getSelection();
            return selection;
        } else if (key.equals("shell")) {
            return _graphicalViewer.getControl().getShell();
        } else if (key.equals("classloader")) {
            return getEditorClassLoader();
        } else if (key.equals("model")) {
            return getEditorModel().getEList();
        } else if (key.equals("project")) {
            return _geInput.getResource();
        } else if (key.equals("input")) {
            return _geInput;
        } else {
            return _propertyValMap.get(key);
        }
    }
    /**
     *
     * @param property - property to be set
     * @param value - Value to be set on the property
     */
    public void setValue(Object property, Object value)
    {
        _propertyValMap.put(property, value);
    }
    /**
     *
     * @return the notifier instance
     */
    public EnvironmentNotifier getNotifier()
    {
        return null;
    }
    /**
     * Returns Parent Environment.
     * @return Parent Environment.
     */
    public Environment getParentEnv()
    {
        return null;
    }
    /**
     * This will update all actions in action registry. This method is invoked
     * from markSaveLocation().
     *
     * @param event EventObject
     */
    public void commandStackChanged(EventObject event)
    {
        firePropertyChange(IEditorPart.PROP_DIRTY);
        super.commandStackChanged(event);
    }
    /**
     * @see org.eclipse.gef.ui.parts.GraphicalEditor#configureGraphicalViewer()
     */
    protected void configureGraphicalViewer()
    {
        super.configureGraphicalViewer();
        _graphicalViewer = (ScrollingGraphicalViewer) getGraphicalViewer();
        ScalableFreeformRootEditPart root = new ScalableFreeformRootEditPart();
        List zoomLevels = new ArrayList(3);
        zoomLevels.add(ZoomManager.FIT_ALL);
        zoomLevels.add(ZoomManager.FIT_WIDTH);
        zoomLevels.add(ZoomManager.FIT_HEIGHT);
        root.getZoomManager().setZoomLevelContributions(zoomLevels);
        IAction zoomIn = new ZoomInAction(root.getZoomManager());
        IAction zoomOut = new ZoomOutAction(root.getZoomManager());
        getActionRegistry().registerAction(zoomIn);
        getActionRegistry().registerAction(zoomOut);
        getSite().getKeyBindingService().registerAction(zoomIn);
        getSite().getKeyBindingService().registerAction(zoomOut);
        createContextMenu();
        _graphicalViewer.setRootEditPart(root);
        _grapFactory.setClassLoader(getEditorClassLoader());
        _grapFactory.setEditParts(getEditParts());
        _graphicalViewer.setEditPartFactory(_grapFactory);
        /*_graphicalViewer.setKeyHandler(new GraphicalViewerKeyHandler(
                _graphicalViewer).setParent(getCommonKeyHandler()));*/
        _graphicalViewer.setKeyHandler(new GEKeyHandler(
                _graphicalViewer, this).setParent(getCommonKeyHandler()));
        loadProperties();
    }
    /**
     * Create Context Menu. If metaclass info has menu xml, it creates context
     * provider from menubuilder and adds default menus to it.
     */
    private void createContextMenu()
    {
        MenuBuilder builder = null;
        String file = _parser.getMenuXmlPath();
        if (file != null && !file.equals("")) {
            try {
                InputStream is = getEditorClassLoader().getResourceAsStream(
                        file);
                if (is != null) {
                    InputStreamReader reader = new InputStreamReader(is);
                    builder = new MenuBuilder(reader, this);
                }
            } catch (Exception e) {
                GenericeditorPlugin.LOG
                        .error("Unable to create MenuBuilder", e);
            }
        }
        ActionRegistry reg = getActionRegistry();
        _graphicalViewer.setContextMenu(new GEContextMenuProvider(
                _graphicalViewer, builder, reg));
    }
    /**
     * @see org.eclipse.gef.ui.parts.GraphicalEditorWithFlyoutPalette
     * #createPaletteViewerProvider()
     */
    protected PaletteViewerProvider createPaletteViewerProvider()
    {
        return new GEPaletteViewerProvider(getEditDomain(),
				new GEPaletteViewer(this));
    }
    /**
     * Saves UI and Data.
     *@param progressMonitor monitor to show for long process.
     */
    public void doSave(IProgressMonitor progressMonitor)
    {
        try {
            saveProperties();
            _propertiesUtil.save();
            _model.save(true);
            _editorModel.setDirty(false);

            Display.getDefault().syncExec(new Runnable() {
				public void run() {
					getCommandStack().markSaveLocation();
				}
			});
        } catch (Exception e) {
            GenericeditorPlugin.LOG.error(
                    "Unable to save EObjects into xmi file", e);
        }
    }
    /**
     * @see org.eclipse.ui.ISaveablePart#doSaveAs()
     */
    public void doSaveAs()
    {
    }
    /**
     * @see org.eclipse.core.runtime.IAdaptable#getAdapter(java.lang.Class)
     */
    public Object getAdapter(Class type)
    {
        if (type == ZoomManager.class) {
            return getGraphicalViewer().getProperty(
                    ZoomManager.class.toString());
        } else if (type == IContentOutlinePage.class) {
            if (_fOutlinePage == null) {
                _fOutlinePage = createOutlinePage();
            }
            return _fOutlinePage;
        }
        return super.getAdapter(type);
    }
    /**
     * @return ResourceOutlineViewPage
     */
    private OutlineViewPage createOutlinePage()
    {
        _fOutlinePage = new OutlineViewPage((EList) ((GenericEditorInput)
                getEditorInput()).getModel().getEList(), this);
        return _fOutlinePage;
    }
    /**
     * Returns the KeyHandler with common bindings for both the Outline and
     * Graphical Views. For example, delete is a common action.
     * @return KeyHandler
     */
    protected KeyHandler getCommonKeyHandler()
    {
        if (_sharedKeyHandler == null) {
            _sharedKeyHandler = new KeyHandler();
            _sharedKeyHandler
                    .put(KeyStroke.getPressed(SWT.DEL, 127, 0),
                            getActionRegistry().getAction(
                                    ActionFactory.DELETE.getId()));
            _sharedKeyHandler.put(KeyStroke.getPressed(SWT.F2, 0),
                    getActionRegistry().getAction(
                            GEFActionConstants.DIRECT_EDIT));
        }
        return _sharedKeyHandler;
    }
    /**
     * returns editor model.
     *
     * @return EditorModel
     */
    public EditorModel getEditorModel()
    {
        return _editorModel;
    }
    /**
     * @return the FlyoutPreferences object used to save the flyout palette's
     * preferences
     */
    protected FlyoutPreferences getPalettePreferences()
    {

        return new FlyoutPreferences() {
            public int getDockLocation()
            {
                return GenericeditorPlugin.getDefault().getPreferenceStore()
                        .getInt(PALETTE_DOCK_LOCATION);
            }

            public int getPaletteState()
            {
                return GenericeditorPlugin.getDefault().getPreferenceStore()
                        .getInt(PALETTE_STATE);
            }

            public int getPaletteWidth()
            {
                return GenericeditorPlugin.getDefault().getPreferenceStore()
                        .getInt(PALETTE_SIZE);
            }

            public void setDockLocation(int location)
            {
                GenericeditorPlugin.getDefault().getPreferenceStore().setValue(
                        PALETTE_DOCK_LOCATION, location);
            }

            public void setPaletteState(int state)
            {
                GenericeditorPlugin.getDefault().getPreferenceStore().setValue(
                        PALETTE_STATE, state);
            }

            public void setPaletteWidth(int width)
            {
                GenericeditorPlugin.getDefault().getPreferenceStore().setValue(
                        PALETTE_SIZE, width);
            }
        };
    }
    /**
     * @see org.eclipse.gef.ui.parts.GraphicalEditorWithFlyoutPalette
     * #getPaletteRoot()
     */
    protected PaletteRoot getPaletteRoot()
    {
    	 GEPaletteRoot palette = new GEPaletteRoot(this, _model.getEPackage(),
                _geInput.getMetaObjects(), _geInput.getPaletteOption(), _toolEntriesMap);
        return palette;
    }
    public Map getToolEntries()
    {
    	return _toolEntriesMap;
    }
    /**
     * @see GraphicalEditor#initializeGraphicalViewer()
     */
    protected void initializeGraphicalViewer()
    {
        super.initializeGraphicalViewer();
        PaletteViewer viewer = getEditDomain().getPaletteViewer();
        viewer.addDragSourceListener(
                new TemplateTransferDragSourceListener(viewer));
        
        /* Yusuf - changes start*/
    	PaletteViewerPreferences pPref = viewer.getPaletteViewerPreferences();
    	pPref.setAutoCollapseSetting(1); /* COLLAPSE_NEVER */
    	viewer.setPaletteViewerPreferences(pPref);
    	/* Yusuf - changes end*/
        
        getEditorModel().setEditorModelValidator(getEditorModelValidator());
        getGraphicalViewer().setContents(getEditorModel());
        getGraphicalViewer()
                .addDropTargetListener(
                        (org.eclipse.jface.util.TransferDropTargetListener)
                        new GETemplateTransferDropTargetListener(this));
    }
    /**
     * @see org.eclipse.gef.ui.parts.GraphicalEditor#getGraphicalViewer()
     */
    public GraphicalViewer getGraphicalViewer() {
    	return super.getGraphicalViewer();
    }

    /**
     * @see org.eclipse.gef.ui.parts.GraphicalEditor#createActions()
     */
    protected void createActions()
    {
        super.createActions();
        ActionRegistry registry = getActionRegistry();
        IAction action;
        action = new CopyTemplateAction(this);
        registry.registerAction(action);

        action = new GECutAction(this);
        registry.registerAction(action);
        getSelectionActions().add(action.getId());
        action = new GECopyAction(this);
        registry.registerAction(action);
        getSelectionActions().add(action.getId());
        action = new GEPasteAction(this);
        registry.registerAction(action);
        getSelectionActions().add(action.getId());
        action = new GEDeleteAction(this);
        registry.registerAction(action);
        getSelectionActions().add(action.getId());
        action = new GEExpandAction(this);
        registry.registerAction(action);
        getSelectionActions().add(action.getId());
        action = new GECollapseAction(this);
        registry.registerAction(action);
        getSelectionActions().add(action.getId());
        action = new GEAutoArrangeAction(this);
        registry.registerAction(action);
        getSelectionActions().add(action.getId());

        action = new GESaveAsImageAction(this);
        registry.registerAction(action);
    }
    /**
     * Returns whether the contents of this part have changed
     * since the last save operation. If this value changes
     * the part must fire a property listener
     * event with <code>PROP_DIRTY</code>.
     * <p>
     *
     * @return <code>true</code> if the contents have been modified and need
     * saving, and <code>false</code> if they have not changed since the last
     *  save
     */
    public boolean isDirty()
    {
        return getCommandStack().isDirty() || _editorModel.isDirty();
    }
    /**
     * Returns whether the "Save As" operation is supported by this part.
     *
     * @return <code>true</code> if "Save As" is supported, and
     * <code>false</code> if not supported
     */
    public boolean isSaveAsAllowed()
    {
        return false;
    }
    /**
     * loads the editor properties like zoom.
     */
    protected void loadProperties()
    {
        ZoomManager manager = (ZoomManager) getGraphicalViewer().getProperty(
                ZoomManager.class.toString());
        if (manager != null) {
            manager.setZoom(getEditorModel().getZoom());
        }
    }
    /**
     * saves the properties like zoom.
     */
    protected void saveProperties()
    {
        ZoomManager manager = (ZoomManager) getGraphicalViewer().getProperty(
                ZoomManager.class.toString());
        if (manager != null) {
            getEditorModel().setZoom(manager.getZoom());
        }
    }
    /**
     * sets the input for the editor.
     * @param input EditorInput
     */
    public void setInput(IEditorInput input)
    {
        super.setInput(input);
        _parser = new Parser(getMetaClassFile());
        if (input instanceof GenericEditorInput) {
            _geInput = (GenericEditorInput) input;
            File uiFile = _geInput.getUIPropertyFile();
            _model = _geInput.getModel();
            EList eObjects = (EList) _geInput.getModel().getEList();
            GEEditDomain editDomain = new GEEditDomain(this);
            editDomain.setCommandStack(new GECommandStack());
            setEditDomain(editDomain);
            GEUtils utils = getUtils(eObjects.toArray());
            GEDataUtils dataUtils = getDataUtils(eObjects);
            ArrayList nodeNames = _parser.getNodeNamesList();
            _propertiesUtil = new EditorPropertiesUtil(uiFile, eObjects,
                    utils, nodeNames);
            _editorModel = _propertiesUtil.getEditorModel();
            _editorModel.setModelInfo(eObjects, dataUtils, nodeNames, _parser
                    .getEdgeNamesList());
            _editorModel.setProject((IProject) _geInput.getResource());
            _editorModel.addPropertyChangeListener(this);
            _geInput.setEditor(this);
        }
    }
    /**
     *
     * @return graphical viewer.
     */
    public EditPartViewer getViewer()
    {
        return getGraphicalViewer();
    }
    /**
     *
     * @return edittpart's class names
     */
    protected Map getEditParts()
    {
        return _parser.getEditPartsMap();
    }
    /**
     * @param obj meta-object
     * @return creation factory for eClass.
     */
    public CreationFactory getCreationFactory(EObject obj)
    {
        return (CreationFactory) _elementFactoriesMap.get(obj);
    }
    /**
     * @param meta-class Name
     * @return creation factory for meta-class Name
     */
    public CreationFactory getCreationFactory(String metaClassName)
    {
    	Object [] keys = _elementFactoriesMap.keySet().toArray();
    	for (int i = 0; i < keys.length; i++) {
    		EObject eobj = (EObject) keys[i];
    		if (metaClassName.equals(eobj.eClass().getName())) {
    			return (CreationFactory) _elementFactoriesMap.get(eobj);
    		}
    	}
    	return null;
    }
    /**
     * returns class loader for specific editors. This needs to be implemented
     * in sub classes.
     * @return classLoader.
     */
    protected ClassLoader getEditorClassLoader()
    {
        return this.getClass().getClassLoader();
    }
    /**
     * Returns Meta-Class file. Child classes should implement
     * this method
     * @return Meta-Class file
     */
    protected abstract File getMetaClassFile();
    /**
     * Returns Utils class for UI.
     * @param objs array of objects
     * @return Utils
     */
    protected abstract GEUtils getUtils(Object[] objs);
    /**
     * Returns Utils class for data.
     * @param objs list of objects
     * @return Utils
     */
    protected abstract GEDataUtils getDataUtils(EList objs);
    /**
     * Returns true if Editor Model is valid. Child classes should implement
     * this method
     * @return boolean
     */
    protected abstract boolean isValid();
    /**
     * Returns ModelValidator or null
     * @return ModelValidator
     */
    protected abstract EditorModelValidator getEditorModelValidator();
    /**
     * Returns Type of the Editor
     * @return type
     */
    public abstract String getEditorType();
    /**
     * Init: Set the title and tooltip.
     * @param site Editor Site
     * @param input Editor Input
     * @throws PartInitException from super implementation
     */
    public void init(IEditorSite site, IEditorInput input)
            throws PartInitException
    {
        super.init(site, input);
        GenericEditorInput genInput = (GenericEditorInput) input;
        String title = genInput.getResource().getName() + " - " + getPartName();
        setPartName(title);
        genInput.setToolTipText(title);
        _xmiFilePath = _model.getResource().getURI().devicePath();
        int index = _xmiFilePath.indexOf(".xml");
        _backupFilePath = _xmiFilePath.substring(0, index) + "_bak.xml";

        /*_backupThread = new AutoBackupThread();
        _backupThread.start();*/
    }

    /**
     * dispose:Stop the auto backup thread
     */
    public void dispose()
    {
        _isBackupNeeded = false;
        deleteBackupFile(_backupFilePath);
        _model.dispose();
        if(_fOutlinePage != null) {
        	_fOutlinePage.removeListeners();
        }
        _fOutlinePage = null;
    }

    /**
     * deletes the backup file
     *
     * @path location of backup file
     */
    private void deleteBackupFile(String path)
    {
        File backupFile = new File(path);
        if (backupFile.exists()) {
            backupFile.delete();
        }
    }
    /**
     * Property change callback
     * @param evt PropertyChangeEvent
     */
    public void propertyChange(PropertyChangeEvent evt)
    {
        _editorModel.setDirty(true);
        firePropertyChange(IEditorPart.PROP_DIRTY);
    }
    /**
     * @return the Command Stack
     */
    public CommandStack getCommandStack()
    {
        return super.getCommandStack();
    }
    /**
     * Set cursor location
     * @param p
     */
    public void setCursorLocation(Point p)
    {	  
    	double zoom = 1;
    	ZoomManager manager = (ZoomManager) getGraphicalViewer().getProperty(
                ZoomManager.class.toString());
    	if(manager != null) {
    		zoom = manager.getZoom();
    	} else {
    		zoom = _editorModel.getZoom();
    	}
    	int x = (int) ((1/zoom) * p.x);
    	int y = (int) ((1/zoom) * p.y);
    	_cursorLocation = new Point(x,y);
    }
    /**
     * Returns Cursor Location
     * @return
     */
    public Point getCursorLocation()
    {
    	return _cursorLocation;
    }
    /**
     * Autobackup thread. It periodically copies the view model data into a new
     * resource and saves the resource as a backup file.
     */
    /*private class AutoBackupThread extends Thread {
        public void run() {
            try {
                while (_isBackupNeeded) {
                    if (new File(_xmiFilePath).exists() && isValid()) {
                        URI uri = URI.createFileURI(_backupFilePath);
                        Resource resource = new File(_backupFilePath).exists()
                                ? EcoreModels
                                .getResource(uri): EcoreModels.create(uri);

                        NotifyingList listToBeBackedUp =
                            (NotifyingList) resource.getContents();
                        listToBeBackedUp.clear();
                        EcoreCloneUtils.copyEList(_model.getEList(),
                                listToBeBackedUp, new HashMap(), new HashMap());
                        FormatConversionUtils.convertToResourceFormat((EObject) listToBeBackedUp.
                    			get(0), getEditorType());
                        EcoreModels.save(resource);

                    }
                    try {
                        Thread.sleep(10000);
                    } catch (InterruptedException e) {
                        GenericeditorPlugin.
                        LOG.error("Unable to backup resource ["
                                + _backupFilePath + "] ", e);
                    }
                }
            } catch (Exception e) {
                GenericeditorPlugin.LOG.error("Unable to backup resource ["
                        + _backupFilePath + "] ", e);
            }
        }
    }*/
}
