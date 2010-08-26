VER=5.0.0

DistDir=dist/cw/plugins

EclipseDir=3rdparty/Eclipse/eclipse3.3.2/linux/eclipse

CDTDir=3rdparty/Eclipse/eclipse3.3.2/linux/eclipse/cdt

SvnStageDir=../pkg/IDE/plugins

# If you have installed ASP somewhere, point this to your eclipse to move the compiled stuff over (and run 'make copy2eclipse').
InstalledDir=/opt/clovis/sdk-4.2/eclipse/plugins
#/opt/clovis/sdk-4.1/eclipse/plugins  

ANT:=ant

AntBuild=$(ANT) -f build-scripts/build.xml -Dplugin_name=$@ -Dversion=$(VER) -Declipse.home=$(EclipseDir) -Dcdt.home=$(CDTDir)


.PHONY: com.clovis.common.utils com.clovis.cw.clovisworks com.clovis.cw.cmdtool com.clovis.cw.data com.clovis.cw.genericeditor com.clovis.cw.editor.ca com.clovis.cw.licensing com.clovis.cw.workspace com.clovis.cw.help stage

all: com.clovis.common.utils com.clovis.cw.data com.clovis.cw.genericeditor com.clovis.cw.editor.ca com.clovis.cw.licensing com.clovis.cw.workspace com.clovis.cw.help com.clovis.cw.cmdtool stage

clean: 
	$(AntBuild) clean
	rm -f plugins/BuildIDGenerator.class plugins/com.clovis.cw.data/about.mappings
	rm -rf build dist plugins/com.clovis.common.utils/src/com/clovis/common/utils/menu/gen plugins/com.clovis.cw.genericeditor/src/com/clovis/cw/genericeditor/gen 

plugins/BuildIDGenerator.class: plugins/BuildIDGenerator.java
	(cd plugins; javac BuildIDGenerator.java)

com.clovis.common.utils: plugins/BuildIDGenerator.class
	$(AntBuild)

com.clovis.cw.clovisworks: plugins/BuildIDGenerator.class
	$(AntBuild)

com.clovis.cw.cmdtool: plugins/BuildIDGenerator.class
	$(AntBuild)

com.clovis.cw.data: plugins/BuildIDGenerator.class
	$(AntBuild)

com.clovis.cw.genericeditor: plugins/BuildIDGenerator.class
	$(AntBuild)

com.clovis.cw.editor.ca: plugins/BuildIDGenerator.class
	$(AntBuild)

com.clovis.cw.licensing: plugins/BuildIDGenerator.class
	$(AntBuild)

com.clovis.cw.workspace: plugins/BuildIDGenerator.class
	$(AntBuild)

com.clovis.cw.help: plugins/BuildIDGenerator.class
	$(AntBuild)


# Overwrite what's in the Subversion tree -- don't delete directories or we'll lose the .svn subdirectories!
stage:
	# Remove old plugin directory from svn package to ensure freshly built content
	rm -rf $(SvnStageDir)
	# Create fresh plugin directory in svn package
	mkdir -p $(SvnStageDir)
	# Copy things over
	cp -r  $(DistDir)/com.clovis.common.utils_$(VER) $(SvnStageDir)
	cp -r  $(DistDir)/com.clovis.cw.data_$(VER) $(SvnStageDir)
	cp -r  $(DistDir)/com.clovis.cw.cmdtool_$(VER) $(SvnStageDir)
	cp -r  $(DistDir)/com.clovis.cw.genericeditor_$(VER) $(SvnStageDir)
	cp -r  $(DistDir)/com.clovis.cw.editor.ca_$(VER) $(SvnStageDir)
	cp -r  $(DistDir)/com.clovis.cw.workspace_$(VER) $(SvnStageDir)
	cp -r  $(DistDir)/com.clovis.cw.help_$(VER) $(SvnStageDir)
	cp -r  $(DistDir)/com.clovis.cw.licensing_$(VER) $(SvnStageDir)

tarball:
	mkdir -p tmp
	cp -r $(DistDir)/com.clovis.common.utils_$(VER) tmp
	cp -r  $(DistDir)/com.clovis.cw.data_$(VER) tmp
	cp -r  $(DistDir)/com.clovis.cw.cmdtool_$(VER) tmp
	cp -r  $(DistDir)/com.clovis.cw.genericeditor_$(VER) tmp
	cp -r  $(DistDir)/com.clovis.cw.editor.ca_$(VER) tmp
	cp -r  $(DistDir)/com.clovis.cw.workspace_$(VER) tmp
	cp -r  $(DistDir)/com.clovis.cw.help_$(VER) tmp
	cp -r $(DistDir)/com.clovis.cw.licensing_$(VER) tmp
	(cd tmp; tar cvfz ../plugins.tgz *)

copy2eclipse:
	@mkdir -p $(InstalledDir)
	rm -rf $(InstalledDir)/com.clovis.common.utils_*
	cp -r  $(DistDir)/com.clovis.common.utils_$(VER) $(InstalledDir)
	rm -rf $(InstalledDir)/com.clovis.cw.data_*
	cp -r  $(DistDir)/com.clovis.cw.data_$(VER) $(InstalledDir)
	rm -rf $(InstalledDir)/com.clovis.cw.cmdtool_*
	cp -r  $(DistDir)/com.clovis.cw.cmdtool_$(VER) $(InstalledDir)
	rm -rf $(InstalledDir)/com.clovis.cw.genericeditor_*
	cp -r  $(DistDir)/com.clovis.cw.genericeditor_$(VER) $(InstalledDir)
	rm -rf $(InstalledDir)/com.clovis.cw.editor.ca_*
	cp -r  $(DistDir)/com.clovis.cw.editor.ca_$(VER) $(InstalledDir)
	rm -rf $(InstalledDir)/com.clovis.cw.workspace_*
	cp -r  $(DistDir)/com.clovis.cw.workspace_$(VER) $(InstalledDir)
	rm -rf $(InstalledDir)/com.clovis.cw.help_*
	cp -r  $(DistDir)/com.clovis.cw.help_$(VER) $(InstalledDir)
	rm -rf $(InstalledDir)/com.clovis.cw.licensing_*
	cp -r  $(DistDir)/com.clovis.cw.licensing_$(VER) $(InstalledDir)

.PHONY: pkg

pkg: clean all
