#CW_HOME=$PWD/../../../CWorks
#P4_CLIENT_ROOT=$(echo $PWD | awk -Fdev '{print $1}' | sed -e 's/\/$//')
#THIRDPARTY_HOME=$P4_CLIENT_ROOT/dev/3rdparty
SANT_HOME=$ANT_HOME
echo $ANT_HOME
#$THIRDPARTY_HOME/common/apache-ant-1.6.4
#ECLIPSE_HOME=$THIRDPARTY_HOME/Eclipse/eclipse3.1.1/linux/eclipse
#JAVA_HOME=$THIRDPARTY_HOME/jdk/jdk1.5.0_03
export PATH=$JAVA_HOME/bin:$ANT_HOME/bin:$PATH
echo "ENvironment ; " $JAVA_HOME $ANT_HOME $PATH

#Test if sourced properly.
if [ -x $JAVA_HOME/bin/javac ]
then
    echo ""
    echo "Environmet set properly. Using following binaries:"
    echo ""
    which java
    which javac
    which ant
else
    echo "---------- ERROR ----------"
    echo "Required files not found, Please source from cwroot directory."
fi
