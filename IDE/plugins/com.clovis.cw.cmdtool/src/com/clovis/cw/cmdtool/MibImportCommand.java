 package com.clovis.cw.cmdtool;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.List;
import java.util.StringTokenizer;
import java.util.Vector;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IProjectDescription;
import org.eclipse.core.resources.IWorkspace;
import org.eclipse.core.resources.IWorkspaceRoot;
import org.eclipse.core.resources.ResourcesPlugin;
import org.eclipse.core.runtime.IPath;
import org.eclipse.core.runtime.IPlatformRunnable;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;

import com.clovis.common.utils.log.Log;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.editor.ca.snmp.ClovisMibUtils;
import com.clovis.cw.editor.ca.snmp.MibImportManager;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.natures.SystemProjectNature;
import com.ireasoning.protocol.snmp.MibUtil;
import com.ireasoning.util.MibParseException;
import com.ireasoning.util.MibTreeNode;

/**
 * @author ravi
 * 
 * Command line application for mib import.
 *
 */
public class MibImportCommand implements IPlatformRunnable {

	IProject _project = null;

	private List _mibNodesList = new Vector();

	private List _userSelTablList = new Vector();
	
	private List _selMibNodesList = new Vector();
	
	private String _projName = "";
	
	private String _mibFileName = "";
	
	private boolean _selAll	= false;

	private static final Log LOG = Log.getLog(CaPlugin.getDefault());

	/**
	 *  code generation application 
	 * @param args - arguments to the application
	 */
	public Object run(Object args) throws Exception {
		
		String [] cmdArgs = (String[]) args;
		
		if (2 == cmdArgs.length) {
			
			String projectPath	= cmdArgs[0];
			File file = new File(projectPath);
			_projName = file.getName();
			IPath projLocation = new Path(file.getAbsolutePath());
			_mibFileName = cmdArgs[1];
			
			IWorkspace workspace = ResourcesPlugin.getWorkspace();
			IWorkspaceRoot root = workspace.getRoot();
			_project  = root.getProject(_projName);
			
			boolean projectExist = _project.exists();
			if (!projectExist) {
				
				IProjectDescription description = workspace
						.newProjectDescription(_project.getName());
				
				description.setLocation(projLocation);
				_project.create(description,null);
				
				_project.open(null);
				boolean isOpen = true;
				
				if (!_project.hasNature(SystemProjectNature.CLOVIS_SYSTEM_PROJECT_NATURE)) {
					
					System.out.println("\n\nSpecified  project : '" +
							_project.getLocation().toOSString() + "' is not a valid clovis system project.\n\n");
					
					if(isOpen) {
						_project.close(null);
						_project.delete(false, true, null);
					}
					
					return EXIT_OK;
				}
				
			}else if(!_project.getLocation().toOSString().equals(projLocation.toOSString())) {
				
				System.out.println("\n\nWorkspace already has a project : " +
						_projName);
				
				System.out.println("\n\nKindly change the project name of " +
						"workspace or specified location.\n\n");
				return EXIT_OK;
			}
			/**
			 * Everything ok.. Now parse mib file and load the resources.
			 */
			MibTreeNode node = null;
			
			try{
				
				MibUtil.unloadAllMibs();
	        	MibUtil.loadMib2();
	        	MibUtil.setResolveSyntax(true);
	            node = MibUtil.parseMib(_mibFileName);
	            
			}catch(MibParseException mbe){
				System.out.println("Mib File Loading Error.");
				LOG.error("Error occured while parsing MIB file", mbe);
			}catch (IOException e) {
				System.out.println("Mib File Loading Error : " +
	                    "Could not load the MibFile. IO Exception has occured" + e);
	            LOG.error("IO Exception has occured", e);
	        } catch (Exception e) {
	        	System.out.println("Mib File Loading Error" +
	                    "Could not load the MibFile. Exception has occured" + e);
	            LOG.error("Exception has occured", e);
	        }
			
	        _mibNodesList.clear();
	        ClovisMibUtils.getMibObjects(_project, node, _mibNodesList);
	        
	        if(_mibNodesList.size() > 0){
	        	
	        	displayMibTables();
	        	
	        }else{
	        	
	        	System.out.print("\n\nNo new table/group/trap is present to import.\n\n");
	        	return EXIT_OK;
	        }
	        
	        if( false == takeUserInput()){
	        	
	        	return EXIT_OK;
	        }
	        
	        if( false == _selAll){
	        	
	        	for(int i = 0; i < _userSelTablList.size(); i++){
		        	
		        	int tblIdx = ((Integer)_userSelTablList.get(i)).intValue();
		        	
		        	_selMibNodesList.add(_mibNodesList.get(tblIdx - 1));
		        }
		         	
	        }else{
	        	
	        	_selMibNodesList = _mibNodesList;
	        }
	        	        
	        if( false == takeUserConfirmation() )
	        {
	        	System.out.print("\nNo Tables/Groups/Traps are imported.\n");
	        	return EXIT_OK;
	        }
	        
	        ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
	        MibImportManager mibMgr = new MibImportManager(_project);
	        
	        mibMgr.convertSelMibObjToClovisObj(pdm, _mibFileName, _selMibNodesList);
	        
	        System.out.print("\n\nSelected Tables/Groups are imported "
	        		+ "as resorce to'" +_projName + "/models/resourcedata.xml'.");
	        System.out.print("\n\nSelected Traps are imported "
	        		+ "as alarm to'" +_projName + "/models/alarmdata.xml'.\n\n");
			        
		}else if (cmdArgs.length == 0) {
			System.out.println("There is no input value provided for project name.\n\n");
		}else if (cmdArgs.length == 1) {
			System.out.println("There is no input value provided for mib file path.\n\n");
		} else {
			System.out.println("\n Usage: MibImportCommand <Project Name> <MIB File Path>\n\n");
		}
				
		return EXIT_OK;
	}

	/**
	 * It displays the list of table/groups/traps
	 * present in mib file.
	 *
	 */
	private void displayMibTables() {
		
		System.out.print("\n\nList of tables/groups/traps present in file " + 
				_mibFileName + " : \n\n");

		System.out.println("\n---------------------------------------------------------------------\n");
		
		for (int i = 0; i < _mibNodesList.size(); i++) {

			MibTreeNode mibNode = (MibTreeNode) _mibNodesList.get(i);

			System.out.print(i + 1 + " : ");

			if (mibNode.isSnmpV2TrapNode()) {
				System.out.println(mibNode.getName() + " ["
						+ mibNode.getOID().toString() + "] = Alarm");
			} else {
				System.out.println(mibNode.getName() + " ["
						+ mibNode.getOID().toString() + "] = Resource");
			}

		}

		System.out.println("\n---------------------------------------------------------------------\n");
	}

	/**
	 * It allows the user to select the 
	 * tables/groups/traps to be imported.
	 * 
	 * User provides the input through console.
	 *
	 */
	private boolean takeUserInput() {

		System.out
				.print("\nSelect tables/groups/traps -\n[Select table numbers separated by space" +
						", or Press 'a' for all and Enter for none] : ");

		BufferedReader bfr = new BufferedReader(
				new InputStreamReader(System.in));

		String input = "";

		try {
			input = bfr.readLine();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

		if (!input.equals("") &&
				!input.equals("a")) {

			StringTokenizer st = new StringTokenizer(input);

			while (st.hasMoreTokens()) {

				int tblNum = Integer.parseInt(st.nextToken());

				if (tblNum > _mibNodesList.size() || tblNum <= 0) {

					System.out.print("\nError: Found Invalid table number '"
							+ tblNum + "' in table number list.\n\n");
					return false;
				}

				if (_userSelTablList.contains(tblNum)) {

					System.out
							.print("\nWarning: Found Duplicate entry for table number '"
									+ tblNum + "' in table number list.\n\n");
				}

				_userSelTablList.add(tblNum);
			}
			
		}else{
			
			if(input.equals("a"))
				_selAll = true;
			
		}
		return true;
	}
	
	/**
	 * It takes the user confirmation to
	 * proceed with the mib import.
	 *  
	 * @return true when 'y' is pressed.
	 */
	private boolean takeUserConfirmation(){
		
		if(_selMibNodesList.size() > 0){
			
			if( true == _selAll){
				
				System.out.print("\nAll Tables/Groups/Traps will be imported.\n");
				
			}else{
				
				System.out.print("\nFollowing Tables/Groups/Traps will be imported:\n");
				
				System.out.println("\n---------------------------------------------------------------------\n");
				
				for (int i = 0; i < _selMibNodesList.size(); i++) {

					MibTreeNode mibNode = (MibTreeNode) _selMibNodesList.get(i);

					int tblIdx = ((Integer)_userSelTablList.get(i)).intValue();
					
					System.out.print(tblIdx + " : ");
					
					if (mibNode.isSnmpV2TrapNode()) {
						System.out.println(mibNode.getName() + " ["
								+ mibNode.getOID().toString() + "] = Alarm");
					} else {
						System.out.println(mibNode.getName() + " ["
								+ mibNode.getOID().toString() + "] = Resource");
					}

				}
				
				System.out.println("\n---------------------------------------------------------------------\n");
			
			}
				
					
			System.out.print("\n\nProceed with import[y/n]:");
			
			BufferedReader bfr = new BufferedReader(
					new InputStreamReader(System.in));

			String input = "";

			try {
				input = bfr.readLine();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}

			if (input.equals("y") || input.equals("Y") ||
					input.equals("yes") || input.equals("Yes")) {
				
				return true;
			}
			
			return false;
			
		}else{
			
			return false;
		}
		
		
	}

}
