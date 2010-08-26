package com.clovis.cw.workspace.project.wizard;

/**
 * 
 * @author matt
 * Class containing information about a node instance. Used in the
 * node instance table.
 */
public class NodeSlotInstanceInfo {
	private String _instanceName;
	private Integer _slotNum;
	private String _netInterface;
	
	public NodeSlotInstanceInfo(String name, Integer slot, String netInterface)
	{
		_instanceName = name;
		_slotNum = slot;
		_netInterface = netInterface;
	}
	
	public String getInstanceName() { return _instanceName; }
	
	public void setInstanceName(String name) { _instanceName = name; }
	
	public Integer getSlotNum() { return _slotNum; }
	
	public void setSlotNum(Integer slotNum) { _slotNum = slotNum; }
	
	public String getNetInterface() { return _netInterface; }
	
	public void setNetInterface(String netInterface) { _netInterface = netInterface; }
}
