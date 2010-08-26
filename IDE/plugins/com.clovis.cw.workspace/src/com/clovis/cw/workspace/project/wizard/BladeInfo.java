/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/project/wizard/BladeInfo.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.workspace.project.wizard;

/**
 * 
 * @author pushparaj
 * Blade Info
 */
public class BladeInfo {
	private String bladeType = "Default";
	
	private String bladeName = "Blade";
	
	private int numberofBlades = 0;

	private int numberofSW = 0;

	/**
	 * Create a task with an initial description
	 * 
	 */
	public BladeInfo() {

	}

	/**
	 * Returns blade type
	 */
	public String getBladeType() {
		return bladeType;
	}
	/**
	 * Returns Blade Name
	 * @return
	 */
	public String getBladeName() {
		return bladeName;
	}
	/**
	 * Returns number of blades
	 */
	public int getNumberOfBlades() {
		return numberofBlades;
	}

	/**
	 * Returns number of SW resources
	 * 
	 */
	public int getNumberOfSW() {
		return numberofSW;
	}

	/**
	 * Set blade type
	 * 
	 * @param blade
	 *            blade type
	 */
	public void setBladeType(String blade) {
		bladeType = blade;
	}
	/**
	 * Set blade name
	 * @param name Name
	 */
	public void setBladeName(String name) {
		bladeName = name;
	}

	/**
	 * Set number of Blades
	 * 
	 * @param blades
	 *            number of blades
	 */
	public void setNumberOfBlades(int blades) {
		numberofBlades = blades;
	}

	/**
	 * Set number of SW resources
	 * 
	 * @param resources
	 *            number of resources
	 */
	public void setNumberOfSW(int resources) {
		numberofSW = resources;
	}

}
