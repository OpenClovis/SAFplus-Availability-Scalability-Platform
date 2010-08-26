/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace;


import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URL;

import org.eclipse.core.resources.IContainer;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.resources.IResourceChangeEvent;
import org.eclipse.core.resources.IResourceChangeListener;
import org.eclipse.core.resources.IResourceDelta;
import org.eclipse.core.resources.IWorkspace;
import org.eclipse.core.resources.ResourcesPlugin;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.core.runtime.QualifiedName;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.IEditorInput;
import org.eclipse.ui.IEditorReference;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.ResourceWorkingSetFilter;
import org.eclipse.ui.views.navigator.ResourceNavigator;
import org.eclipse.ui.views.navigator.ResourcePatternFilter;

import com.clovis.common.utils.ClovisFileUtils;
import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.editor.ca.manageability.ui.ManageabilityEditorInput;
import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.builders.ClovisConfigurator;
import com.clovis.cw.workspace.natures.SystemProjectNature;
import com.clovis.cw.workspace.project.CwProjectPropertyPage;
/**
 * @author btammanna
 *
 * Navigator Class to implement workspace which shows only Clovis Projects
 */
public class ClovisNavigator extends ResourceNavigator
{
    private ClovisNavigatorFilter _patternFilter = new ClovisNavigatorFilter();
    private final ResourceChangeListenerImpl _listener
        = new ResourceChangeListenerImpl(_patternFilter);
    private ResourceWorkingSetFilter _workingSetFilter
        = new ResourceWorkingSetFilter();
    
    /**
     * Constructor
     *
     */
    public ClovisNavigator()
    {
        //commenting the code as the implementation of the open and close 
    	//project is not complete, once Editor binding to files are done
    	//this can be uncommented
    	IWorkspace workspace = ResourcesPlugin.getWorkspace();
        workspace.addResourceChangeListener(_listener);
    }
    /**
     * @return the Pattern Filter
     */
     public ResourcePatternFilter getPatternFilter()
     {
            return this._patternFilter;
     }
     
     /*
      * (non-Javadoc)
      * 
      * @see org.eclipse.ui.views.navigator.ResourceNavigator#createViewer(org.eclipse.swt.widgets.Composite)
      */
     @Override
     protected TreeViewer createViewer(Composite parent) {
    	 TreeViewer viewer = super.createViewer(parent);
    	 viewer
				.addSelectionChangedListener(new ClovisNavigatorSelectionChangedListener());
    	 return viewer;
     }

	/**
      * @param viewer - TreeViewer
      */
     protected void initFilters(TreeViewer viewer)
     {
            viewer.addFilter(_patternFilter);
            viewer.addFilter(_workingSetFilter);
     }
     /**
      * dispose
      */
    public void dispose()
    {
        IWorkspace workspace = ResourcesPlugin.getWorkspace();
        workspace.removeResourceChangeListener(_listener);
        super.dispose();
    }
}
/**
 *
 * @author shubhada
 * Resource Change Listener which listens to the changes to Resources in
 * Clovis Workspace
 *
 */
class ResourceChangeListenerImpl implements IResourceChangeListener
{

    private ClovisNavigatorFilter _patternFilter;
    
    public ResourceChangeListenerImpl(ClovisNavigatorFilter filter)
    {
    	super();
    	_patternFilter = filter;
    }

    /**
     * @param event - IResourceChangeEvent
     */
    public void resourceChanged(IResourceChangeEvent event) {

		if (event.getSource() instanceof IWorkspace) {
			IResource resource = event.getResource();
			
			switch (event.getType()) {
			case IResourceChangeEvent.PRE_DELETE:
				break;
			case IResourceChangeEvent.PRE_CLOSE:
				ProjectDataModel.getProjectDataModel((IContainer) resource)
						.removeDependencyListeners();
				ProjectDataModel.removeProjectDataModel((IContainer) resource);
				break;
			case IResourceChangeEvent.POST_CHANGE:

				// break out if the project was opened tansiently by the filter
				if (_patternFilter._transientOpen) break;
				IResourceDelta delta = event.getDelta();
				//get the deltas for the projects
				IResourceDelta[] projectDeltas = delta.getAffectedChildren();
				for (int i = 0; i < projectDeltas.length; i++) {
					int kind = projectDeltas[i].getKind();
					int flags = projectDeltas[i].getFlags();
					if (kind == IResourceDelta.ADDED) {
						IResource res = projectDeltas[i].getResource();
						if (res instanceof IProject) {
							File linkFile = new File(res.getLocation().append(
									ICWProject.CW_PROJECT_SRC_DIR_NAME)
									.toOSString());
							try {
								// Check the source folder of the project.If it is directly under the project the
                                // set the project area location
								if (linkFile.exists()) {
									// Set the proper project area location
									if (linkFile.getCanonicalFile().getParentFile().getParentFile().exists()) {
										res.setPersistentProperty(new QualifiedName("", CwProjectPropertyPage.PROJECT_AREA_LOCATION_MODE),
							                    linkFile.getCanonicalFile().getParentFile().getParentFile().getAbsolutePath());
										ClovisConfigurator.setConfigChanges(res, "true");
									} else {
										Process proc = Runtime.getRuntime().exec("rm " + linkFile);
										proc.waitFor();
									}
								}
							} catch (CoreException e) {
								e.printStackTrace();
							} catch (IOException e1) {
								e1.printStackTrace();
							} catch (InterruptedException e) {
								e.printStackTrace();
							}
							// Check and update the SnmpSubAgent Properties
							URL caURL = DataPlugin.getDefault().find(
									new Path("model" + File.separator
											+ "compmibmap.ecore"));
							try {
								File ecoreFile = new Path(Platform.resolve(
										caURL).getPath()).toFile();
								EPackage pack = EcoreModels
										.getUpdated(ecoreFile.getAbsolutePath());
								String dataFilePath = res.getLocation()
										.toOSString()
										+ File.separator
										+ ICWProject.CW_PROJECT_MODEL_DIR_NAME
										+ File.separator + "compmibmap.xmi";
								File xmiFile = new File(dataFilePath);
								if (xmiFile.exists()) {
									URI uri = URI.createFileURI(dataFilePath);
									Resource model_res = xmiFile.exists() ? EcoreModels
											.getUpdatedResource(uri)
											: EcoreModels.create(uri);
									NotifyingList list = (NotifyingList) model_res
											.getContents();
									for (int j = 0; j < list.size(); j++) {
										EObject obj = (EObject) list.get(j);
										String mibPath = String
												.valueOf(EcoreUtils.getValue(
														obj, "mibPath"));
										if (mibPath != null
												&& !mibPath.trim().equals("")) {
											String dirs[] = mibPath.split(":");
											for (int d = 0; d < dirs.length; d++) {
												if (!new File(dirs[d]).exists()) {
													EcoreUtils.setValue(obj,
															"mibPath", "");
												}
											}
										}
									}
									model_res.save(null);
								}
							} catch (Exception e) {
								continue;
							}
						}
					} else if (kind == IResourceDelta.CHANGED) {
						if ((flags & IResourceDelta.OPEN) != 0) {
							IProject project = (IProject) projectDeltas[i]
									.getResource();
							if (project.isOpen()) {
								try {
									if (project
											.hasNature(SystemProjectNature.CLOVIS_SYSTEM_PROJECT_NATURE)) {
										ProjectDataModel
												.getProjectDataModel(project);
									}
								} catch (CoreException e) {
									// TODO Auto-generated catch block
									e.printStackTrace();
								}
							} else {
								closeClovisEditors(project);
								ProjectDataModel.getProjectDataModel(project)
										.removeDependencyListeners();
								ProjectDataModel
										.removeProjectDataModel(project);
								clearProblemView((IProject) projectDeltas[i]
								      .getResource());
							}
						}
					} else if (kind == IResourceDelta.REMOVED) {
						if (projectDeltas[i].getResource() instanceof IProject) {
							closeClovisEditors((IProject) projectDeltas[i]
							        									.getResource());
							ProjectDataModel removedPM = 
								ProjectDataModel.getExistingProjectDataModel(
										(IContainer) projectDeltas[i].getResource());
							
							if (removedPM != null)
							{
								removedPM.removeDependencyListeners();
							}
							
							ProjectDataModel
									.removeProjectDataModel((IContainer) projectDeltas[i]
											.getResource());
							clearProblemView((IProject) projectDeltas[i]
									.getResource());
						} 
					}
				}
				
				if (projectDeltas.length == 2) {
					IProject oldProject = null, newProject = null;
					/** for Renaming project **/
					if (projectDeltas[0].getKind() == IResourceDelta.REMOVED
							&& ((projectDeltas[0].getFlags() & IResourceDelta.MOVED_TO) != 0)
							&& projectDeltas[1].getKind() == IResourceDelta.ADDED
							&& ((projectDeltas[1].getFlags() & IResourceDelta.MOVED_FROM) != 0)) {

						oldProject = (IProject) projectDeltas[0].getResource();
						newProject = (IProject) projectDeltas[1].getResource();

					} else if (projectDeltas[1].getKind() == IResourceDelta.REMOVED
							&& ((projectDeltas[1].getFlags() & IResourceDelta.MOVED_TO) != 0)
							&& projectDeltas[0].getKind() == IResourceDelta.ADDED
							&& ((projectDeltas[0].getFlags() & IResourceDelta.MOVED_FROM) != 0)) {

						oldProject = (IProject) projectDeltas[1].getResource();
						newProject = (IProject) projectDeltas[0].getResource();
					}

					if (oldProject != null
							&& newProject != null
							&& !CwProjectPropertyPage.getProjectAreaLocation(
									newProject).equals("")) {

						String oldLocation = CwProjectPropertyPage
								.getProjectAreaLocation(newProject)
								+ File.separator + oldProject.getName();
						String newLocation = CwProjectPropertyPage
								.getProjectAreaLocation(newProject)
								+ File.separator + newProject.getName();

						if (new File(oldLocation).exists()) {
							new File(oldLocation)
									.renameTo(new File(newLocation));
						}

						String srcDirLocation = newLocation + File.separator
								+ ICWProject.CW_PROJECT_SRC_DIR_NAME;
						String linkFileLocation = newProject.getLocation()
								.toOSString()
								+ File.separator
								+ ICWProject.CW_PROJECT_SRC_DIR_NAME;

						if (!new File(srcDirLocation).exists()) {
							new File(srcDirLocation).mkdirs();
						}
						if (!srcDirLocation.equals(linkFileLocation))
							ClovisFileUtils.createRelativeSourceLink(
									srcDirLocation, linkFileLocation);
					} else if(oldProject == null
							&& newProject == null) {
						/** For copy project **/
						if (projectDeltas[0].getKind() == IResourceDelta.CHANGED
								&& projectDeltas[1].getKind() == IResourceDelta.ADDED) {
							oldProject = (IProject) projectDeltas[0].getResource();
							newProject = (IProject) projectDeltas[1].getResource();

						} else if (projectDeltas[1].getKind() == IResourceDelta.CHANGED
								&& projectDeltas[0].getKind() == IResourceDelta.ADDED) {
							oldProject = (IProject) projectDeltas[1].getResource();
							newProject = (IProject) projectDeltas[0].getResource();
						}
						if(oldProject != null
								&& newProject != null) {
							String projectareaLoc = CwProjectPropertyPage
							.getProjectAreaLocation(oldProject);
							if(projectareaLoc != null && !projectareaLoc.equals("")) {
								try {
									newProject.setPersistentProperty(new QualifiedName("", CwProjectPropertyPage.PROJECT_AREA_LOCATION_MODE),
											projectareaLoc);
									ClovisConfigurator.setConfigChanges(newProject, "true");
									copyDirectory(new File(projectareaLoc + File.separator + oldProject.getName() + File.separator + ICWProject.CW_PROJECT_SRC_DIR_NAME), new File(projectareaLoc + File.separator + newProject.getName() + File.separator + ICWProject.CW_PROJECT_SRC_DIR_NAME));
									String srcDirLocation = projectareaLoc
											+ File.separator
											+ newProject.getName()
											+ File.separator
											+ ICWProject.CW_PROJECT_SRC_DIR_NAME;
									String linkFileLocation = newProject
											.getLocation().toOSString()
											+ File.separator
											+ ICWProject.CW_PROJECT_SRC_DIR_NAME;
									if (!srcDirLocation
											.equals(linkFileLocation)) {

										if (new File(linkFileLocation).exists()) {
											deleteDirectory(new File(linkFileLocation));
										}
										ClovisFileUtils
												.createRelativeSourceLink(
														srcDirLocation,
														linkFileLocation);
									}
								} catch (Exception e) {
									// TODO Auto-generated catch block
									e.printStackTrace();
								}
							}
						}
					}
				}
				break;
			}
		}
	}
    /**
     * Copy files from source location to target location
     * @param sourceLocation
     * @param targetLocation
     * @throws IOException
     */
    private void copyDirectory(File sourceLocation , File targetLocation)
    throws IOException {
        
        if (sourceLocation.isDirectory()) {
            if (!targetLocation.exists()) {
                targetLocation.mkdirs();
            }
            
            String[] children = sourceLocation.list();
            for (int i=0; i<children.length; i++) {
                copyDirectory(new File(sourceLocation, children[i]),
                        new File(targetLocation, children[i]));
            }
        } else {
            
            InputStream in = new FileInputStream(sourceLocation);
            OutputStream out = new FileOutputStream(targetLocation);
            byte[] buf = new byte[1024];
            int len;
            while ((len = in.read(buf)) > 0) {
                out.write(buf, 0, len);
            }
            in.close();
            out.close();
        }
    }
    /**
     * Delete directory
     * @param file
     * @return
     */
    private void deleteDirectory(File file) {
		if (file.isDirectory()) {
			File[] files = file.listFiles();
			for (int i = 0; i < files.length; i++) {
				if (files[i].isDirectory()) {
					deleteDirectory(files[i]);
				} else {
					files[i].delete();
				}
			}
		}
		file.delete();
	}
    /**
	 * Check the source folder of the project. If it is directly under the
	 * project then move it to below the directory with the project name in the
	 * project area directory.
	 * 
	 * NOTE: This method should also create the symbolic link to the new source
	 * directory but there is a problem creating it in the same context of
	 * removing the existing src folder. For this reason the code for creating
	 * the symbolic link is in the ProjectValidator class.
	 * 
	 * @param project
	 */
    protected void fixSourceFolder(IResource project)
    {
    	try {
	    	File badSrcDir  = new File(project.getLocation().append(ICWProject.CW_PROJECT_SRC_DIR_NAME).toOSString());
			String projAreaLoc = CwProjectPropertyPage.getProjectAreaLocation(project);
			if(badSrcDir.exists() && !projAreaLoc.equals("")) {
				if (badSrcDir.isDirectory())
				{
					File goodSrcDir = new File(projAreaLoc + File.separator + project.getName()
											+ File.separator + ICWProject.CW_PROJECT_SRC_DIR_NAME);
					if (goodSrcDir.exists())
					{
						ClovisFileUtils.deleteDirectory(goodSrcDir);
					}
					
					goodSrcDir.mkdirs();
					ClovisFileUtils.copyDirectory(badSrcDir, goodSrcDir, true);
					
				} else {
					badSrcDir.delete();
				}
			}
		} catch (IOException e) {
			e.printStackTrace();
		}
    }
    
	/**
	 * This will close all clovis editors for respective projects
	 * 
	 * @project Project obj
	 */
	private void closeClovisEditors(final IProject project) {
		try {
			Display.getDefault().asyncExec(new Runnable() {
				public void run() {
					IWorkbenchPage page = PlatformUI.getWorkbench()
							.getActiveWorkbenchWindow().getActivePage();
					IEditorReference refs[] = page.getEditorReferences();

					for (int j = 0; j < refs.length; j++) {
						try {
							IEditorInput input = refs[j].getEditorInput();
							if (input instanceof GenericEditorInput) {
								if (project == ((GenericEditorInput) input)
										.getResource().getProject()) {
									IEditorReference ref[] = { refs[j] };
									page.closeEditors(ref, true);
								}
							} else if(input instanceof ManageabilityEditorInput) {
								if (project == ((ManageabilityEditorInput) input)
										.getProjectDataModel().getProject()) {
									IEditorReference ref[] = { refs[j] };
									page.closeEditors(ref, true);
								}
							}
						} catch (Exception e) {
							e.printStackTrace();
						}
					}
				}
			});

		} catch (Exception e) {
			e.printStackTrace();
		}
	}
	
	/**
	 * This will close all clovis editors for
	 * respective projects
	 * @project Project obj
	 */
	private void clearProblemView(final IProject project) {
		try {
			Display.getDefault().asyncExec(new Runnable() {
				public void run() {
					try {
						IWorkbenchPage page = WorkspacePlugin.getDefault().getWorkbench()
						.getActiveWorkbenchWindow().getActivePage();
						if (page != null) {
							ProblemsView problemsView = ((ProblemsView) page
						       .findView("com.clovis.cw.workspace.problemsView"));
							if(problemsView != null)
								problemsView.clearModelProblems();
						}
					} catch (Exception e) {
						e.printStackTrace();
						System.out.println(e.getMessage());
					}
				}
			});

		} catch (Exception e) {
			e.printStackTrace();
		}
	}
}
