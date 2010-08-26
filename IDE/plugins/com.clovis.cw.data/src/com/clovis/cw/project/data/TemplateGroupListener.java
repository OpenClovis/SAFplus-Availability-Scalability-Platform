package com.clovis.cw.project.data;

import java.io.IOException;
import java.util.List;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.notify.impl.AdapterImpl;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EAttribute;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.resource.Resource;

import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;

/**
 * 
 * @author ravi
 *
 */
public class TemplateGroupListener extends AdapterImpl {

	private Resource _resource;

	/**
	 * Constructor
	 * @param resource
	 */

	public TemplateGroupListener(Resource resource) {
		_resource = resource;
		
	}

	/**
	 * @param notification
	 *            Notofication event
	 */
	public void notifyChanged(Notification notification) {
		
		if (notification.isTouch())
			return;
		
		switch (notification.getEventType()) {
		
		case Notification.SET:
			Object notifier = notification.getNotifier();
			if (notifier instanceof EObject) {
				boolean bol = ((EObject) notifier).eClass().getName().equals("SAFComponent");
				if (bol ) {
					EAttribute feature = (EAttribute) notification.getFeature();
					boolean isName = feature.getName().equals("name");
					if(isName){
							
						String oldSAFCompName = (String) notification.getOldValue();
						String newSAFCompName = (String) notification.getNewValue();
						if (!(oldSAFCompName.equals(newSAFCompName))) {
							EObject rootObject = (EObject) _resource.getContents().get(0); 
							List mapList = (EList) rootObject.eGet(rootObject.eClass().getEStructuralFeature("safComponent"));
							for(int i=0; i<mapList.size(); i++){
								EObject mapObj = (EObject) mapList.get(i);
								String compName = EcoreUtils.getName(mapObj);
								if(compName.equals(oldSAFCompName)){
									EcoreUtils.setValue(mapObj, "name", newSAFCompName);
								}
							}
							try {
					 			EcoreModels.save(_resource);
							} catch (IOException e) {
								e.printStackTrace();
							}
						}
					}
				}
			}
			break;		
			case Notification.ADD:
				Object value = notification.getNewValue();
				if(value instanceof EObject) {
					boolean bol = ((EObject) value).eClass().getName().equals("SAFComponent");
					if (bol) {
						EcoreUtils.addListener(value, this, 1);
					}	
				}
			break;
			case Notification.ADD_MANY:
				List objects = (List) notification.getNewValue();
				if (objects != null) {
					for (int i = 0; i < objects.size(); i++) {
						if (objects.get(i) instanceof EObject) {
							boolean bol = ((EObject)objects.get(i)).eClass().getName().equals("SAFComponent");
							if (bol) {
							EcoreUtils.addListener((EObject) objects.get(i), this, 1);
							}
						}
					}
				}
				break;
			case Notification.REMOVE:
				if (notification.getOldValue() instanceof EObject) {
					EObject obj = (EObject) notification.getOldValue();
					String delatedSAFCompName = EcoreUtils.getName(obj);
					boolean bol = obj.eClass().getName().equals("SAFComponent");
					if (bol) {
						EcoreUtils.removeListener(obj, this, 1);
						EObject rootObject = (EObject) _resource.getContents().get(0); 
						List mapList = (EList) rootObject.eGet(rootObject.eClass().getEStructuralFeature("safComponent"));
						
						for(int i=0; i<mapList.size(); i++){
							EObject mapObj = (EObject) mapList.get(i);
							String compName = EcoreUtils.getName(mapObj);
							if(compName.equals(delatedSAFCompName)){
								mapList.remove(i);
								
							}
						}
						try {
				 			EcoreModels.save(_resource);
						} catch (IOException e) {
							e.printStackTrace();
						}
					}
				}
				break;
				
			case Notification.REMOVE_MANY:
				List objectsList = (List) notification.getOldValue();
				if (objectsList != null) {
					for (int i = 0; i < objectsList.size(); i++) {
						if (objectsList.get(i) instanceof EObject) {
							EObject obj = (EObject) objectsList.get(i);
							String delatedSAFCompName = EcoreUtils.getName(obj);
							boolean bol = obj.eClass().getName().equals("SAFComponent");
							if (bol) {
								EcoreUtils.removeListener(obj, this, 1);
								EObject rootObject = (EObject) _resource.getContents().get(0); 
								List mapList = (EList) rootObject.eGet(rootObject.eClass().getEStructuralFeature("safComponent"));
								
								for(int j=0; j<mapList.size(); j++){
									EObject mapObj = (EObject) mapList.get(j);
									String compName = EcoreUtils.getName(mapObj);
									if(compName.equals(delatedSAFCompName)){
										mapList.remove(j);
										
									}
								}
							}
						}
					}
					try {
			 			EcoreModels.save(_resource);
					} catch (IOException e) {
						e.printStackTrace();
					}
				}
				
		}
	}
	
}
