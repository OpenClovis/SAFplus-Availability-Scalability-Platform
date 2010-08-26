package com.clovis.cw.workspace.builders;

import java.util.Map;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IncrementalProjectBuilder;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IProgressMonitor;

/**
 * 
 * @author Pushparaj
 * Builder for Application Source Project
 */
public class ApplicationSourceBuilder extends IncrementalProjectBuilder {

	
	@Override
	protected void clean(IProgressMonitor monitor) throws CoreException {
		
	}

	@Override
	protected IProject[] build(int kind, Map args, IProgressMonitor monitor)
			throws CoreException
	{
		return null;
	}
}
