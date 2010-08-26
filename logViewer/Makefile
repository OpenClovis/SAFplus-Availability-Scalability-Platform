DistDir=dist
SvnPackageDir=../pkg/logviewer

ANT=ant

all: clean build

build:
	$(ANT) -f build-scripts/build.xml dist

clean:
	$(ANT) -f build-scripts/build.xml clean

package:
	rm -rf $(SvnPackageDir)/*
	cp -r config $(SvnPackageDir)
	cp -r icons $(SvnPackageDir)
	cp -r lib $(SvnPackageDir)
	cp logViewer.sh $(SvnPackageDir)
	cp $(DistDir)/LogTool.jar $(SvnPackageDir)/lib
	rm $(DistDir)/LogTool.jar

pkg: all package
