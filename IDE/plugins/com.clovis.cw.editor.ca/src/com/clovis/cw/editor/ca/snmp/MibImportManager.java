package com.clovis.cw.editor.ca.snmp;

import java.util.ArrayList;
import java.util.Hashtable;
import java.util.List;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Path;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.IEditorPart;
import org.eclipse.ui.IWorkbenchPage;

import com.clovis.common.utils.UtilsPlugin;
import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.project.utils.FormatConversionUtils;
import com.ireasoning.util.MibTreeNode;

/**
 * 
 * @author shubhada
 *
 * This class parses the MIB and for each
 * table/group/notification to be imported,
 * it creates the corresponding resource/alarm
 * in the clovis information model.
 */
public class MibImportManager
{
	private static final Log LOG = Log.getLog(CaPlugin.getDefault());
	private IProject _project = null;
	private IProgressMonitor _monitor;
	
	/**
	 * Constructor
	 * 
	 * @param project - IProject
	 */
	public MibImportManager(IProject project)
	{
		_project = project;
	}

	/**
	 * Constructor
	 * 
	 * @param _project
	 * @param _monitor
	 */
	public MibImportManager(IProject project, IProgressMonitor monitor) {
		_project = project;
		_monitor = monitor;
	}
	
	/**
	 * This method is used to initialize certain params for the IDE. 
	 * It then calls another method which converts the  
	 * MIB table/group to resource of resource model
	 * and MIB notification to alarms.
	 * 
	 * @param mibPath - Absolute path of the selected mib
	 * @param selObjList - selected objects(table/group/notification) oids
	 * 
	 */
	public void convertMibObjToClovisObj(
			String mibPath, List selObjList)
	{
        
        try {
            ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
            final GenericEditorInput editorInput = (GenericEditorInput) pdm
					.getCAEditorInput();
			final List pageList = new ArrayList();

			Display.getDefault().syncExec(new Runnable() {
				public void run() {
					pageList.add(CaPlugin.getDefault().getWorkbench()
							.getActiveWorkbenchWindow().getActivePage());
				}
			});

			IWorkbenchPage page = null;
            boolean isDirty = true;

            if (pageList.size() == 1 && pageList.get(0) != null) {
				page = (IWorkbenchPage) pageList.get(0);
				if (editorInput != null) {
					IEditorPart editor = page.findEditor(editorInput);

					if (editor != null) {
						isDirty = editor.isDirty();
					}
				}
			}
            
			convertSelMibObjToClovisObj(pdm, mibPath, selObjList);
			
            if (!UtilsPlugin.isCmdToolRunning() && !isDirty) {
            	if(editorInput != null && editorInput.getEditor() != null)
            		Display.getDefault().syncExec(new Runnable() {

						public void run() {
							editorInput.getEditor().doSave(_monitor);							
						}});
            }
            
        } catch (Exception e) {
            LOG.error("Exception has occured while converting MIB objects to clovis objects", e);
        }
	}
	
	/**
	 * This method converts the MIB table/group to resource of resource model
	 * and MIB notification to alarms.
	 * 
	 * @param pdm - Project data model
	 * @param mibPath - Absolute path of the selected mib
	 * @param selObjList - selected objects(table/group/notification) oids
	 * 
	 */
	public void convertSelMibObjToClovisObj(ProjectDataModel pdm,
			String mibPath, List selObjList) {

		try {

			final Model caModel = pdm.getCAModel();
			Model alarmModel = pdm.getAlarmProfiles();
			EObject resorceInfoObj = (EObject) caModel.getEList().get(0);
			EObject chassisObj = (EObject) ((List) EcoreUtils.getValue(
					resorceInfoObj,
					ClassEditorConstants.CHASSIS_RESOURCE_REF_NAME)).get(0);
			EReference mibResourceRef = (EReference) resorceInfoObj.eClass()
					.getEStructuralFeature(
							ClassEditorConstants.MIB_RESOURCE_REF_NAME);
			EReference mibRef = (EReference) resorceInfoObj.eClass()
					.getEStructuralFeature(ClassEditorConstants.MIB_REF_NAME);
			boolean mibCreationRequired = false;
			for (int i = 0; i < selObjList.size(); i++) {
				MibTreeNode childNode = (MibTreeNode) selObjList.get(i);
				if (childNode.isTableNode() || childNode.isGroupNode()) {
					mibCreationRequired = true;
					break;
				}
			}
			final List mibList = (List) resorceInfoObj.eGet(mibRef);
			boolean mibObjExists = false;
			for (int i = 0; i < mibList.size(); i++) {
				EObject mib = (EObject) mibList.get(i);
				String mibName = EcoreUtils.getValue(mib, "name").toString();
				if (mibName.equals(new Path(mibPath).lastSegment())) {
					mibObjExists = true;
					break;
				}
			}

			if (!mibObjExists && mibCreationRequired) {
				final EObject mibObj = EcoreUtils.createEObject(mibRef
						.getEReferenceType(), true);
				EcoreUtils.setValue(mibObj, "name", new Path(mibPath)
						.lastSegment());

				Display.getDefault().syncExec(new Runnable() {
					public void run() {
						mibList.add(mibObj);
					}
				});

				addChassisConnection(resorceInfoObj, chassisObj, mibObj);
			}

			for (int i = 0; i < selObjList.size(); i++) {
				MibTreeNode childNode = (MibTreeNode) selObjList.get(i);
				if (childNode.isTableNode()) {
					final EObject mibResObj = EcoreUtils.createEObject(mibResourceRef
							.getEReferenceType(), true);
					EcoreUtils.setValue(mibResObj,
							ClassEditorConstants.CLASS_NAME, (String) childNode
									.getName());
					EcoreUtils.setValue(mibResObj,
							ClassEditorConstants.MIB_NAME_FEATURE, new Path(
									mibPath).lastSegment());
					final List mibResList = (List) resorceInfoObj
							.eGet(mibResourceRef);

					Display.getDefault().syncExec(new Runnable() {
						public void run() {
							mibResList.add(mibResObj);
						}
					});

					MibTreeNode tableEntryNode = (MibTreeNode) childNode
							.getChildNodes().get(0);
					List tableColumns = tableEntryNode.getChildNodes();
					processTableAndGroupNode(tableColumns, mibResObj);
					addChassisConnection(resorceInfoObj, chassisObj, mibResObj);

				} else if (childNode.isGroupNode()) {
					final EObject mibResObj = EcoreUtils.createEObject(mibResourceRef
							.getEReferenceType(), true);
					EcoreUtils.setValue(mibResObj,
							ClassEditorConstants.CLASS_NAME, (String) childNode
									.getName());
					EcoreUtils.setValue(mibResObj,
							ClassEditorConstants.MIB_NAME_FEATURE, new Path(
									mibPath).lastSegment());
					EcoreUtils.setValue(mibResObj,
							ClassEditorConstants.MIB_RESOURCE_SCALAR_TYPE,
							"true");
					EcoreUtils.setValue(mibResObj,
							ClassEditorConstants.CLASS_MAX_INSTANCES, "1");
					final List mibResList = (List) resorceInfoObj
							.eGet(mibResourceRef);
					Display.getDefault().syncExec(new Runnable() {
						public void run() {
							mibResList.add(mibResObj);
						}
					});
					List scalarNodes = new ArrayList();
					List children = childNode.getChildNodes();
					for (int j = 0; j < children.size(); j++) {
						MibTreeNode treeNode = (MibTreeNode) children.get(j);
						if (treeNode.isScalarNode()) {
							scalarNodes.add(treeNode);
						}
					}
					processTableAndGroupNode(scalarNodes, mibResObj);
					addChassisConnection(resorceInfoObj, chassisObj, mibResObj);

				} else if (childNode.isSnmpV2TrapNode()) {

					EObject alarmInfoObj = (EObject) alarmModel.getEList().get(
							0);
					EReference alarmProfileRef = (EReference) alarmInfoObj
							.eClass().getEStructuralFeature("AlarmProfile");
					List alarmProfiles = (List) alarmInfoObj
							.eGet(alarmProfileRef);
					boolean isAlarmExist = false;
					for (int j = 0; j < alarmProfiles.size(); j++) {
						EObject alarm = (EObject) alarmProfiles.get(j);
						String alarmId = EcoreUtils.getValue(alarm,
								ClassEditorConstants.ALARM_ID).toString();
						if (alarmId.equals(childNode.getName())) {
							isAlarmExist = true;
						}
					}
					if (!isAlarmExist) {
						EObject alarmObj = EcoreUtils.createEObject(
								alarmProfileRef.getEReferenceType(), true);
						EcoreUtils.setValue(alarmObj,
								ClassEditorConstants.ALARM_ID,
								(String) childNode.getName());
						String description = (String) childNode
								.getDescription();
						description = description.replaceAll("\n", " ");
						EcoreUtils.setValue(alarmObj, "Description",
								description);
						EcoreUtils.setValue(alarmObj, "OID", (String) childNode
								.getOID().toString());
						alarmProfiles.add(alarmObj);

					}
				}
			}

			// Convert to resource format from model and save the models
			Display.getDefault().syncExec(new Runnable() {
				public void run() {
					FormatConversionUtils.convertToResourceFormat(
							(EObject) caModel.getEList().get(0),
							"Resource Editor");
				}
			});

			caModel.save(true);
			alarmModel.save(true);

		} catch (Exception e) {
			LOG
					.error(
							"Exception has occured while converting MIB objects to clovis objects",
							e);
		}
	}
	
	
	
	/**
	 * This method adds the tableColumns/groupScalars as attributes of the 
	 * Editor MIB resource
	 * 
	 * @param nodes - nodes to added as attributes to the Resource/MO
	 * @param mibResObj - Editor Mib Resource object
	 */
	private void processTableAndGroupNode(List nodes, EObject mibResObj)
	{
		EReference provRef = (EReference) mibResObj.eClass().getEStructuralFeature(
				ClassEditorConstants.RESOURCE_PROVISIONING);
		EObject provObj = (EObject) mibResObj.eGet(provRef);
		if (provObj == null) {
			provObj = EcoreUtils.createEObject(
					provRef.getEReferenceType(), true);
			mibResObj.eSet(provRef, provObj);
		}

		final EObject finalProvObj = provObj;
		Display.getDefault().syncExec(new Runnable() {
			public void run() {
				EcoreUtils.setValue(finalProvObj, "isEnabled", "true");
			}
		});

		EReference attrRef = (EReference) provObj.eClass().getEStructuralFeature(
				ClassEditorConstants.CLASS_ATTRIBUTES);
		List attrList = (List) provObj.eGet(attrRef);
		
		copyToInternalObjects(attrRef, attrList, nodes);
		
		
	}
	
	/**
     * copy the imported Attributes to our EList
     * @param attrRef - attribute reference
     * @param attrList - List of attributes of the resource
     * @param mibNodes - List of mibNodes to be converted
     */
    protected void copyToInternalObjects(EReference attrRef, final List attrList, List mibNodes)
    {
    	
        for (int i = 0; i < mibNodes.size(); i++) {
            final EObject eObject  = EcoreUtils.createEObject(
            		attrRef.getEReferenceType(), true);
            MibTreeNode mibNode = (MibTreeNode) mibNodes.get(i);
            String name = (String) mibNode.getName();
            String oid = mibNode.getOID().toString();
            String mibType = mibNode.getSyntaxType();
            String defVal = mibNode.getDefVal();

            byte mibCoreType = mibNode.getRealSyntaxType();
            if (oid.charAt(0) == '.') {
                oid = oid.substring(1);
            }
            EcoreUtils.setValue(eObject, ClassEditorConstants.ATTRIBUTE_NAME, name);
            EcoreUtils.setValue(eObject, ClassEditorConstants.ATTRIBUTE_IS_IMPORTED, "true");
            EcoreUtils.setValue(eObject, ClassEditorConstants.ATTRIBUTE_OID, oid);
            if (mibType != null && !mibType.equals("")) {
            	String size = mibNode.getSyntax().getSize();
        		Hashtable enumValues = mibNode.getSyntax().getSyntaxMap();
            	ClovisMibUtils.setTypeAndSize(mibType, mibCoreType, size, enumValues, defVal, eObject);
            }
            // Sets default properties based on access
            boolean isIndexed = mibNode.isIndexNode();
            String access = mibNode.getAccess();
            ClovisMibUtils.setDefaultPropertiesForMibAttributes(isIndexed, access, eObject);
            
			Display.getDefault().syncExec(new Runnable() {
				public void run() {
					attrList.add(eObject);
				}
			});
        }
    }
	
    /**
     * This method adds the composition connection from chassis to
     * the added mib resource 
     * 
     * @param resourceInfoObj - Top level information object
     * @param chassisObj - chassis object
     * @param mibResObj - mib resource object
     */
    private void addChassisConnection(final EObject resourceInfoObj,
    		final EObject chassisObj, final EObject mibObj)
    {
    	// Add the composition connection to chassis
		final EReference compositionRef = (EReference) resourceInfoObj.eClass().
			getEStructuralFeature(ClassEditorConstants.COMPOSITION_REF_NAME);
		final EObject compositionObj = EcoreUtils.createEObject(
				compositionRef.getEReferenceType(), true);

		Display.getDefault().syncExec(new Runnable() {
			public void run() {
				EcoreUtils.setValue(compositionObj,
						ClassEditorConstants.CONNECTION_START,
						(String) EcoreUtils.getValue(chassisObj,
								ModelConstants.RDN_FEATURE_NAME));
				EcoreUtils.setValue(compositionObj,
						ClassEditorConstants.CONNECTION_END,
						(String) EcoreUtils.getValue(mibObj,
								ModelConstants.RDN_FEATURE_NAME));
				List compositionList = (List) resourceInfoObj
						.eGet(compositionRef);
				compositionList.add(compositionObj);
			}
		});
    }
}
