package com.clovis.cw.workspace.dialog;

import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;
import java.util.regex.Pattern;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.notify.impl.AdapterImpl;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.ComboBoxCellEditor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CCombo;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.ui.table.FormView;
import com.clovis.cw.editor.ca.ComponentDataUtils;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.editor.ca.dialog.NodeProfileDialog;
import com.clovis.cw.editor.ca.manageability.ui.ManageabilityEditor;
import com.clovis.cw.editor.ca.manageability.ui.RessourceEditingComposite.ResourceTableViewer;
/**
 *
 * @author Pushparaj
 * Combo Box Cell Editor to display the Resource Instance IDs
 */
public class ResourceInstanceIDComboBoxCellEditor extends ComboBoxCellEditor
{
    private List _compositionList;
    private List _moList;
    private List _associatedRes = new Vector();
    private CCombo _combo;
    private EStructuralFeature _feature = null;
    private Environment _env = null;
    private HashMap _posValueMap = new HashMap();
    private HashMap _cwkeyObjectMap = new HashMap();
    private HashMap _nameResourceMap = new HashMap();
    private String _lastSetItem = " ";
    private List _resourceList;
    private ManageabilityEditor _editor;
    /**
     * @param parent - parent Composite
     * @param feature - EStructuralFeature
     * @param env - Environment
     */
    public ResourceInstanceIDComboBoxCellEditor(Composite parent,
            EStructuralFeature feature, Environment env)
    {
        super(parent, new String[0], SWT.BORDER);
        _feature = feature;
        _env = env;
        _combo.removeAll();
        setItems(getComboValues());
    }
    /**
     * Create Editor Instance.
     * @param parent  Composite
     * @param feature EStructuralFeature
     * @param env     Environment
     * @return cell Editor
     */
    public static CellEditor createEditor(Composite parent,
            EStructuralFeature feature, Environment env)
    {
        return new ResourceInstanceIDComboBoxCellEditor(parent, feature, env);
    }
    /**
     *@param value - Value to be set
     */
    protected void doSetValue(Object value)
    {
        if (_combo.indexOf((String) value) == -1
            && getIndex((String) value) != -1) {
            _combo.setItem(getIndex((String) value), (String) value);
        }
        setItems(_combo.getItems());
        int index = -1;
        String[] items = this.getItems();
        for (int i = 0; i < items.length; i++) {
            if (items[i].equals(value)) {
                index = i;
            }
        }
        super.doSetValue(new Integer(index));
    }
    /**
     *@return the Value
     */
    protected Object doGetValue()
    {
       int selIndex = _combo.getSelectionIndex();
       setItems(_combo.getItems());
       _combo.select(selIndex);
        if (((Integer) super.doGetValue()).intValue() == -1 || selIndex == -1) {
            return _lastSetItem;
        } else {
            return getItems()[selIndex];
        }
    }
    /**
     * When Cell Editor is activated, the Tooltip message
     * is displayed on the Message Area of the Containing 
     * dialog 
     */
 	public void activate()
 	{
 		super.activate();
 	}
    /**
     * @return Combo Values
     */
    protected String[] getComboValues()
    {
        List comboValues = new Vector();
        if (NodeProfileDialog.getInstance() != null) {
			if (_feature.getName().equals("moID")) {
				List inputList = NodeProfileDialog.getInstance()
						.getResourceList();
				initCompositionList(inputList);
				initMaps(inputList);
				Environment parentEnv = _env.getParentEnv();
				EObject compObj = (EObject) ((FormView) parentEnv)
						.getValue("model");
				String compName = (String) EcoreUtils.getValue(compObj, "type");
				List compList = NodeProfileDialog.getInstance()
						.getComponentsList();
				IProject project = (IProject) NodeProfileDialog.getInstance()
						.getProject();
				for (int i = 0; i < compList.size(); i++) {
					EObject cObj = (EObject) compList.get(i);
					if (compName.equals(EcoreUtils.getName(cObj))) {
						List resList = (List) ComponentDataUtils
								.getAssociatedResources(project, cObj);
						for (int j = 0; resList != null && j < resList.size(); j++) {
							EObject resObj = (EObject) _nameResourceMap
									.get(resList.get(j));
							if (resObj != null) {
								_associatedRes.add(resObj);
							}
						}

						EObject containerNode = compObj.eContainer()
								.eContainer().eContainer().eContainer();
						String containerNodeMoID = EcoreUtils.getValue(
								containerNode, "nodeMoId").toString();
						comboValues = processResourcePaths(containerNodeMoID,
								_associatedRes);

					}
				}
			}
		} else {
			ResourceTableViewer viewer = (ResourceTableViewer) _env;
			_editor = viewer.getManageabilityEditor();
			_resourceList = _editor.getResourceList();
			final EAdapter listener = new EAdapter();
			EcoreUtils.addListener(_resourceList, listener, -1);
			getControl().addDisposeListener(new DisposeListener(){

				public void widgetDisposed(DisposeEvent e) {
					EcoreUtils.removeListener(_resourceList, listener, -1);
					
				}});
			EObject rootObject = (EObject) _resourceList.get(0);
			List<EObject> mibList = (EList)rootObject.eGet(rootObject.eClass()
					.getEStructuralFeature(
							ClassEditorConstants.MIB_RESOURCE_REF_NAME));
			for (int i = 0; i < mibList.size(); i++) {
				EObject obj = (EObject) mibList.get(i);
				comboValues.add("\\Chassis:*\\" + EcoreUtils.getName(obj) + ":*" );
			}
		}
		String[] values = new String[comboValues.size()];
		for (int i = 0; i < comboValues.size(); i++) {
			values[i] = (String) comboValues.get(i);
			_posValueMap.put(new Integer(i), getDefaultMoIdVal(values[i]));
		}
		return values;
	}
    /**
	 * 
	 * @param inputList
	 *            Resource Editor input.
	 * 
	 */
    private void initBladeLevelMoList(List inputList)
    {
        _moList = new Vector();
        EObject rootObject = (EObject) inputList.get(0);
        List list = (EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ClassEditorConstants.NODE_HARDWARE_RESOURCE_REF_NAME));
        for (int i = 0; i < list.size(); i ++) {
        	EObject eObject = (EObject) list.get(i);
        	_moList.add(eObject);
            String cwkey = (String) EcoreUtils.getValue(eObject,
            		ModelConstants.RDN_FEATURE_NAME);
            _cwkeyObjectMap.put(cwkey, eObject);
        }
        list = (EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ClassEditorConstants.CHASSIS_RESOURCE_REF_NAME));
        for (int i = 0; i < list.size(); i ++) {
        	EObject eObject = (EObject) list.get(i);
        	_moList.add(eObject);
            String cwkey = (String) EcoreUtils.getValue(eObject,
            		ModelConstants.RDN_FEATURE_NAME);
            _cwkeyObjectMap.put(cwkey, eObject);
        }
        list = (EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ClassEditorConstants.SYSTEM_CONTROLLER_REF_NAME));
        for (int i = 0; i < list.size(); i ++) {
        	EObject eObject = (EObject) list.get(i);
        	_moList.add(eObject);
            String cwkey = (String) EcoreUtils.getValue(eObject,
            		ModelConstants.RDN_FEATURE_NAME);
            _cwkeyObjectMap.put(cwkey, eObject);
        }
    }
    /**
     *
     * @param inputList Resource Editor Input
     */
    private void initCompositionList(List inputList)
    {
    	_compositionList = new Vector();
        EObject rootObject = (EObject) inputList.get(0);
		_compositionList.addAll((EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ClassEditorConstants.COMPOSITION_REF_NAME)));
    }
    /**
     *
     * @param inputList Resource Editor input.
     *
     */
    private void initMaps(List inputList)
    {
        _moList = new Vector();
        EObject rootObject = (EObject) inputList.get(0);
        List list = (EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ClassEditorConstants.HARDWARE_RESOURCE_REF_NAME));
        for (int i = 0; i < list.size(); i ++) {
        	EObject eObject = (EObject) list.get(i);
        	_moList.add(eObject);
        	 _nameResourceMap.put(EcoreUtils.getName(eObject), eObject);
            String cwkey = (String) EcoreUtils.getValue(eObject,
            		ModelConstants.RDN_FEATURE_NAME);
            _cwkeyObjectMap.put(cwkey, eObject);
        }
        list = (EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ClassEditorConstants.SOFTWARE_RESOURCE_REF_NAME));
        for (int i = 0; i < list.size(); i ++) {
        	EObject eObject = (EObject) list.get(i);
        	_moList.add(eObject);
        	 _nameResourceMap.put(EcoreUtils.getName(eObject), eObject);
            String cwkey = (String) EcoreUtils.getValue(eObject,
            		ModelConstants.RDN_FEATURE_NAME);
            _cwkeyObjectMap.put(cwkey, eObject);
        }
        list = (EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ClassEditorConstants.MIB_RESOURCE_REF_NAME));
        for (int i = 0; i < list.size(); i ++) {
        	EObject eObject = (EObject) list.get(i);
        	_moList.add(eObject);
        	 _nameResourceMap.put(EcoreUtils.getName(eObject), eObject);
            String cwkey = (String) EcoreUtils.getValue(eObject,
            		ModelConstants.RDN_FEATURE_NAME);
            _cwkeyObjectMap.put(cwkey, eObject);
        }
        list = (EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ClassEditorConstants.NODE_HARDWARE_RESOURCE_REF_NAME));
        for (int i = 0; i < list.size(); i ++) {
        	EObject eObject = (EObject) list.get(i);
        	_moList.add(eObject);
        	 _nameResourceMap.put(EcoreUtils.getName(eObject), eObject);
            String cwkey = (String) EcoreUtils.getValue(eObject,
            		ModelConstants.RDN_FEATURE_NAME);
            _cwkeyObjectMap.put(cwkey, eObject);
        }
        list = (EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ClassEditorConstants.CHASSIS_RESOURCE_REF_NAME));
        for (int i = 0; i < list.size(); i ++) {
        	EObject eObject = (EObject) list.get(i);
        	_moList.add(eObject);
        	 _nameResourceMap.put(EcoreUtils.getName(eObject), eObject);
            String cwkey = (String) EcoreUtils.getValue(eObject,
            		ModelConstants.RDN_FEATURE_NAME);
            _cwkeyObjectMap.put(cwkey, eObject);
        }
        list = (EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ClassEditorConstants.SYSTEM_CONTROLLER_REF_NAME));
        for (int i = 0; i < list.size(); i ++) {
        	EObject eObject = (EObject) list.get(i);
        	_moList.add(eObject);
        	 _nameResourceMap.put(EcoreUtils.getName(eObject), eObject);
            String cwkey = (String) EcoreUtils.getValue(eObject,
            		ModelConstants.RDN_FEATURE_NAME);
            _cwkeyObjectMap.put(cwkey, eObject);
        }
    }
    /**
     *
     * @param resourceList List of Resources
     * @return All Possible MO Paths
     */
    private List processMoPaths(List resourceList)
    {
        List mopathList = new Vector();
        for (int i = 0; i < resourceList.size(); i++) {
            EObject eobj = (EObject) resourceList.get(i);
            if (eobj.eClass().getName().equals("ChassisResource")) {
                expand(eobj, "", mopathList, true);
                return mopathList;
            }
        }
        return mopathList;
    }
    /**
    *@param containerNodeMoID the moId for the container node
     * @param resNamesList List of Associated Resource Names
    * @return All Possible MO Paths for the Associated MO's for the component
    */
   private List processResourcePaths(String containerNodeMoID, List resList)
   {
       List mopathList = new Vector();
       if (resList == null) {
           return mopathList;
       } else {
           for (int i = 0; i < resList.size(); i++) {
               EObject resObj = (EObject) resList.get(i);
               getResourcePathList(containerNodeMoID, resObj, mopathList);
           }
           return mopathList;
       }
   }
   /**
	 * 
	 * @param containerNodeMoID
	 *            the moId for the container node
	 * @param resObj
	 *            Resource EObject
	 * @param mopathList
	 *            List
	 */
    private void getResourcePathList(String containerNodeMoID, EObject resObj, List mopathList)
    {
		EObject parentObj = getParent(resObj);
		
		if(parentObj == null)	return;
		
		String resName = EcoreUtils.getName(resObj);
		
		String rootName = containerNodeMoID.substring(
				containerNodeMoID.indexOf("\\") + 1 , 
				containerNodeMoID.indexOf(":"));
		
		String containerResName = containerNodeMoID.substring(
				containerNodeMoID.lastIndexOf("\\") + 1,
				containerNodeMoID.lastIndexOf(":"));
		
		if( null == rootName || null == containerResName)
			return;
		
		if( parentObj.eClass().getName().equals(
				ClassEditorConstants.CHASSIS_RESOURCE_NAME) ){
			
			if( resObj.eClass().getName().equals(
					ClassEditorConstants.HARDWARE_RESOURCE_NAME) ||
					resObj.eClass().getName().equals(
							ClassEditorConstants.SOFTWARE_RESOURCE_NAME) ||
					resObj.eClass().getName().equals(
							ClassEditorConstants.MIB_RESOURCE_NAME)){
				
				String fullPath = "\\" + rootName + ":*\\" + resName + ":*"; 
				
				mopathList.add(fullPath);
				
			}
			else
				if( resObj.eClass()
						.getName().equals(
								ClassEditorConstants.NODE_HARDWARE_RESOURCE_NAME) ){
				
					if (!(resName.equals(containerResName)) ) {
						
						return;
					}
					else{
						String fullPath = "\\" + rootName + ":*\\" + 
										containerResName + ":*";
						mopathList.add(fullPath);
					}
				}
		}
		else{
			String fullPath = "";
			
			getPathFromRoot(resObj, rootName, containerResName,
					mopathList, fullPath);
							
		}
			
    }
    
    /**
     * Finds MoPath from root(chassis) to the resourse
     * and adds the full path to mopathList
     * 
     * @param resObj Resource EObject
     * @param rootName Name of the root resource
     * @param containerResName Name of container resource name
     * @param mopathList List 
     * @param fullPath - path caontaining resource(s) name 
     * 			separated by \ and :*
     * @return true if that resource belongs to 
     * 			the same path as container node moId
     */
    private boolean getPathFromRoot(EObject resObj, String rootName, String containerResName,  
    		List mopathList, String fullPath){
    	
    	EObject parentObj = getParent(resObj);
    	
    	if(parentObj == null)	return false;
    	
    	String resName = EcoreUtils.getName(resObj);
    	
    	if( parentObj.eClass().getName().equals(
    			ClassEditorConstants.CHASSIS_RESOURCE_NAME)) {
    		
    		if( resName.equals(containerResName) || 
    				resObj.eClass().getName().equals(
    						ClassEditorConstants.HARDWARE_RESOURCE_NAME) ||
    				resObj.eClass().getName().equals(
    						ClassEditorConstants.SOFTWARE_RESOURCE_NAME) ||
    				resObj.eClass().getName().equals(
    	    				ClassEditorConstants.MIB_RESOURCE_NAME)){
    			
    			fullPath = "\\" + rootName + ":*\\" + resName + ":*" + fullPath;
				
				mopathList.add(fullPath);
				
    			return true;
    		}
    		else
    		{
    			return false;
    		}
    	}
    	else{
    		
    		fullPath = "\\" + resName + ":*" + fullPath;
    		
    		return getPathFromRoot(parentObj, rootName, containerResName, 
    				mopathList, fullPath);   	
    		
    	}
    	
    }
    
    /**
	 * 
	 * @param eobj
	 *            EObject - Chassis EObject
	 * @param parentPath -
	 *            parentPath
	 * @param pathList -
	 *            List of processed MO Paths
	 * @param tillEnd -
	 *            boolean value to indicate whether it should expand till the
	 *            end.
	 */
    private void expand(EObject eobj, String parentPath, List pathList,
            boolean tillEnd)
    {
        String resName = EcoreUtils.getName(eobj);
        if (!tillEnd) {
			String containerResName = parentPath.substring(parentPath
					.lastIndexOf("\\") + 1, parentPath.lastIndexOf(":"));
			if (!resName.equals(containerResName)) {
				parentPath += "\\" + resName + ":*";
			}
			pathList.add(parentPath);

			List children = getChildren(eobj);
			if (children != null) {
				for (int i = 0; i < children.size(); i++) {
					EObject childObj = (EObject) children.get(i);
					expand(childObj, parentPath, pathList, tillEnd);
				}
			}
        } else {
            parentPath +=  "\\" + resName + ":*";
            pathList.add(parentPath);
            List children = getChildren(eobj);
            if (children != null) {
                for (int i = 0; i < children.size(); i++) {
                    EObject childObj = (EObject) children.get(i);
                    expand(childObj, parentPath, pathList, tillEnd);
                }
            }
        }
    }
    /**
     *
     * @param resourceObj EObject
     * @return the Parent EObject of resourceObj
     */
    private EObject getParent(EObject resourceObj)
    {
        String cwkey = (String) EcoreUtils.getValue(resourceObj,
        		ModelConstants.RDN_FEATURE_NAME);
        for (int i = 0; i < _compositionList.size(); i++) {
            EObject compositionObj = (EObject) _compositionList.get(i);
            String targetKey = (String) EcoreUtils.getValue(
                    compositionObj, "target");
            if (targetKey.equals(cwkey)) {
                return (EObject) _cwkeyObjectMap.get(EcoreUtils.
                        getValue(compositionObj, "source"));
            }
        }
        return null;
    }
    /**
     *
     * @param resourceObj EObject
     * @return the Children of the resourceObj
     */
    private List getChildren(EObject resourceObj)
    {
        List children = new Vector();
        for (int i = 0; i < _moList.size(); i++) {
            EObject moObj = (EObject) _moList.get(i);
            EObject parentObj = getParent(moObj);
            if (parentObj != null && parentObj.equals(resourceObj)) {
                children.add(moObj);
            }
        }
        return children;
    }
    /**
     * @param parent Composite
     * @return Control
     */
    protected Control createControl(Composite parent)
    {
        CCombo comboBox = (CCombo) super.createControl(parent);
        comboBox.setEditable(true);
        ComboTextModifyListener listener = new ComboTextModifyListener();
        comboBox.addModifyListener(listener);
        _combo = comboBox;
        return comboBox;
    }
    /**
     *
     * @param newVal Modified Value of the Combo Box
     * @return the index of the modified value
     */
    private int getIndex(String newVal)
    {
        String value = getDefaultMoIdVal(newVal);
        Iterator iterator = _posValueMap.keySet().iterator();
        while (iterator.hasNext()) {
            Integer index = (Integer) iterator.next();
            String val = (String) _posValueMap.get(index);
            if (val.equals(value)) {
                return index.intValue();
            }
        }
        return -1;
    }

    public static String getDefaultMoIdVal(String val) {
        char [] array = val.toCharArray();
        int num = 0;
        for (int i = 0; i < array.length; i++) {
            if (array[i] == ':') {
                array[i + 1] = '*';
                int index = i + 2;
                while (index < array.length && (array[index] == '0'
                    || array[index] == '1'
                    || array[index] == '2'
                    || array[index] == '3'
                    || array[index] == '4'
                    || array[index] == '5'
                    || array[index] == '6'
                    || array[index] == '7'
                    || array[index] == '8'
                    || array[index] == '9')) {
                    array[index] = '#';
                    num++;
                    index = index + 1;
                }
            }
        }

        char [] temp = new char[array.length - num];
        int ind = 0;
        for (int i = 0; i < array.length; i++) {
            if (array[i] != '#') {
                temp[ind] = array[i];
                ind++;
            }
        }
        return new String(temp);
    }

    /**
     *
     * @author shubhada
     *Modify Listener to listen on the change in the combo box text
     */
    private class ComboTextModifyListener implements ModifyListener
    {
        /**
         * @param e - ModifyEvent
         */
        public void modifyText(ModifyEvent e)
        {
            CCombo combo = (CCombo) e.getSource();
            String newVal = combo.getText();
            if (_feature != null) {
                String pattern =
                    EcoreUtils.getAnnotationVal(_feature, null, "pattern");
                if (pattern != null) {
                    if (Pattern.compile(pattern).
                            matcher(newVal).matches()) {
                        if (getIndex(newVal) != -1) {
                        combo.setItem(getIndex(newVal), newVal);
                        _lastSetItem = newVal;
                        }
                    }
                }
            }
        }
    }
    /**
     * Dont Deactivate editor in FormView.
     */
    public void deactivate()
    {
        if (_env instanceof FormView) {
            fireCancelEditor();
        } else {
            super.deactivate();
        }
    }
    
    public class EAdapter extends AdapterImpl {
		public void notifyChanged(Notification notification) {
			if (notification.isTouch()
					|| NodeProfileDialog.getInstance() != null) {
				return;
			}
			switch (notification.getEventType()) {
			case Notification.SET:
			case Notification.ADD:
			case Notification.ADD_MANY:
			case Notification.REMOVE:
			case Notification.REMOVE_MANY:
				updateComboItems();
			}
		}
		private void updateComboItems() {
			Display.getDefault().syncExec(new Runnable() {

				public void run() {
					_combo.removeAll();
					List comboValues = new Vector();
					_resourceList = _editor.getResourceList();
					EObject rootObject = (EObject) _resourceList.get(0);
					List<EObject> mibList = (EList) rootObject
							.eGet(rootObject.eClass().getEStructuralFeature(
									ClassEditorConstants.MIB_RESOURCE_REF_NAME));
					for (int i = 0; i < mibList.size(); i++) {
						EObject obj = (EObject) mibList.get(i);
						comboValues.add("\\Chassis:*\\"
								+ EcoreUtils.getName(obj) + ":*");
					}
					String[] values = new String[comboValues.size()];
					_posValueMap.clear();
					for (int i = 0; i < comboValues.size(); i++) {
						values[i] = (String) comboValues.get(i);
						_posValueMap.put(new Integer(i),
								getDefaultMoIdVal(values[i]));
					}
					setItems(values);
				}
			});
		}
	}
}
