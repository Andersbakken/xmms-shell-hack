#!/bin/sh

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have autoconf installed to compile XMMS-Shell"
	echo
	exit 1
}

echo "Generating configuration files for XMMS-Shell, please wait...."
echo

autoconf
./configure $@

