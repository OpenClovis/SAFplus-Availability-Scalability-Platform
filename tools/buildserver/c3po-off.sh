#!/bin/bash

if [[ $0 =~ '.*off.*' ]]; then

	echo "Disabling crontab activities for c3po"
	sudo crontab -u c3po -e && echo "done"

else

	echo "Enabling periodic builds and tests"
	cat | sudo crontab -u c3po - << EOF
*/5 * * * * $HOME/autobuild/autobuild -l -u -b $HOME/autobuild $HOME/autobuild/autobuild-c3po-root.conf
0 */4 * * * rm -f /tmp/autobuild/last-failed-*
EOF
	if [ $? == 0 ]; then
		echo "done"
	fi

fi

echo "This is now the content of C3PO's crontab:"
sudo crontab -u c3po -l

