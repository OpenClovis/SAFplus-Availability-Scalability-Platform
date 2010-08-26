#!/usr/bin/env perl
use File::Basename;

$SCRIPT_PATH=$0;
$SCRIPT_DIR=dirname($SCRIPT_PATH);
$CLASSPATH=$SCRIPT_DIR."/checkstyle-all-4.0-beta2.jar";
$JAVA=$SCRIPT_DIR."/../external-binaries/linux/jdk1.5.0_03/jre/bin/java";
$MAIN_CLASS="com.puppycrawl.tools.checkstyle.Main";
$CONF_XML=$SCRIPT_DIR."/checkstyle_configuration.xml";
$CMD="$JAVA -classpath $CLASSPATH $MAIN_CLASS -c $CONF_XML";
$error_count=0;
$max_error_count=4;
foreach $fileName (@ARGV) {
    #Only .java files.
    if ($fileName =~ m/\.java$/i) {
        print "\n\nFile: $fileName\n";
        system("$CMD $fileName | grep \"warning:\" > /tmp/cwcs.tmp");
        #my $new_count = `cat /tmp/cwcs.tmp | | wc -l`;
        open IN, "/tmp/cwcs.tmp";
        while (<IN>) {
            my $line  = $_;
            ($fld1, $fld2) = split("java:", $line, -1);
            ($rowcol, $errstr) = split(" warning: ", $fld2, -1);
            print "$rowcol	 $errstr";
            $error_count = $error_count + 1;
        }
        close IN;
    }
}
if ($max_error_count < $error_count) {
    print "\n-------------------------------------------------\n";
    print "Number of violation = ".$error_count."\n";
    print "Too many Coding violation. Please correct these errors and try again.";
    print "\n-------------------------------------------------\n";
    exit 1;
}
