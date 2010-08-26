/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/dialog/AssociateResourceWizardPage.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.dialog;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;

import org.eclipse.core.resources.IContainer;
import org.eclipse.core.resources.IProject;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.wizard.WizardPage;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Table;

import com.clovis.common.utils.constants.AnnotationConstants;
import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.ui.DialogValidator;
import com.clovis.common.utils.ui.table.TableUI;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.project.data.DependencyListener;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * 
 * @author shubhada
 *
 * Wizard page to capture the associated resources of the component
 */
public class AssociateResourceWizardPage extends WizardPage  
{
    private IProject _project = null;
    private EClass _eClass = null;
    private HashMap _featureValMap = null;
    private List _compsList = new ClovisNotifyingListImpl();
    private DialogValidator _dialogValidator = null;
    private DependencyListener _dependencyListener;
    private Model _model;
    /**
     * Constructor
     * @param pageName - Page Name
     */
	protected AssociateResourceWizardPage(String pageName, IProject project)
	{
		super(pageName);
		_project = project;
	}
	/**
     * Creates the page controls
     * @param parent - Parent Composite 
	 */
	public void createControl(Composite parent)
	{
        Composite container = new Composite(parent, SWT.NONE);
        container.setLayoutData(new GridData(GridData.FILL_BOTH));
        GridLayout containerLayout = new GridLayout();
        containerLayout.numColumns = 2;
        container.setLayout(containerLayout);
        
        NodeClassModelPage prevPage = (NodeClassModelPage) getPreviousPage();
        int numComponents = prevPage.getNumberOfComponents(); 
        ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
        GenericEditorInput geInput = (GenericEditorInput) pdm.
            getComponentEditorInput();
        List editorViewModelList = geInput.getModel().getEList();
        initComponentList(numComponents, editorViewModelList);
        _model = new Model(null, (NotifyingList) _compsList, null);
        _dialogValidator = new DialogValidator(this, _model);
		_dependencyListener = new DependencyListener(ProjectDataModel
				.getProjectDataModel((IContainer) _project), DependencyListener.VIEWMODEL_OBJECT);
		EcoreUtils.addListener(_model.getEList(), _dependencyListener, -1);
        
        EPackage ePackage = ((AddComponentWizard) getWizard()).
            getObjectAdditionHandler().getEPackage();
        _eClass = (EClass) ePackage.getEClassifier(
                ComponentEditorConstants.SAFCOMPONENT_NAME);
        List visibleFeatures = new ArrayList();
        visibleFeatures.add(EcoreUtils.getNameField(_eClass));
        visibleFeatures.add(ComponentEditorConstants.ASSOCIATE_RESOURCES_NAME);
        _featureValMap = EcoreUtils.processHiddenFields(_eClass, visibleFeatures);
        
        int style = SWT.MULTI | SWT.BORDER | SWT.H_SCROLL
        | SWT.V_SCROLL | SWT.FULL_SELECTION | SWT.HIDE_SELECTION;
        Table table = new Table(container, style);
        ClassLoader loader = getClass().getClassLoader();
        TableUI tableViewer = new TableUI(table, _eClass, loader);
        tableViewer.setValue("container", this);
        tableViewer.setValue("project", _project);
        tableViewer.setValue("dialogvalidator", _dialogValidator);
        
        GridData gridData1 = new GridData();
        gridData1.horizontalAlignment = GridData.FILL;
        gridData1.grabExcessHorizontalSpace = true;
        gridData1.grabExcessVerticalSpace = true;
        gridData1.verticalAlignment = GridData.FILL;
        
        gridData1.heightHint =
            container.getDisplay().getClientArea().height / 10;
        
        table.setLayoutData(gridData1);
        table.setLinesVisible(true);
        table.setHeaderVisible(true);
        table.setSelection(0);
        tableViewer.setInput(_compsList);
        
        setTitle("Associate Resources to the components");
        this.setControl(container);
	}
    /**
     * Reverts the changed annotation values
     */
    public void dispose()
    {
        Iterator iterator = _featureValMap.keySet().iterator();
        while (iterator.hasNext()) {
            EStructuralFeature feature = (EStructuralFeature) iterator.next();
            String val = (String) _featureValMap.get(feature);
            EcoreUtils.setAnnotationVal(feature, null,
                    AnnotationConstants.IS_HIDDEN, val);
        }
        EcoreUtils.removeListener(_model.getEList(), _dependencyListener, -1);
        _dialogValidator.removeListeners();
        _dialogValidator = null;
        super.dispose();
    }
    /**
     * 
     * @param numComponents
     * @param editorViewModelList
     */
    public void initComponentList(int numComponents, List editorViewModelList)
    {
        List compsList = ((AddComponentWizard) getWizard()).
            getObjectAdditionHandler().createComponents(
                numComponents, editorViewModelList);
        _compsList.clear();
        _compsList.addAll(compsList);
    }
    /**
     * 
     * @return the Components List
     */
    public List getComponentList()
    {
        return _compsList;
    }
	
}
