#!/bin/sh
# Usage: Run this script with no argument to be presented
# with Yast modules. 'z' argument for Yast-core shipped
# examples. 'x' for our own set of tests.

ZENITY_BINARY=/opt/gnome/bin/zenity
Y2BASE_BINARY=/usr/lib/YaST2/bin/y2base

# any argument switches to the examples
DEFAULT_DIR=/usr/share/YaST2/clients
if [ $# -eq 1 ]; then
    if [ "$1" == "z" ]; then
        DEFAULT_DIR=/usr/share/doc/packages/yast2-core/libyui/examples
    fi
    if [ "$1" == "x" ]; then
        DEFAULT_DIR=.
        # go to the test directory
        # (this doesn't affect user's cwd because this
        #  is running in its own shell.)
        cd "`dirname $0`/test"
    fi
fi

RUN_QT=0
zenity --question --text "Run also the modules with Qt?" && RUN_QT=1

module_list=`ls $DEFAULT_DIR/*.ycp | sed 's/.*\///'`

while true; do
    module=`$ZENITY_BINARY --title "Yast Control Center" \
            --list --column "Available modules:" $module_list`;
    if [ "$module" ]; then
        if [ $RUN_QT -eq 1 ]; then
            $Y2BASE_BINARY "$DEFAULT_DIR/$module" qt &
        fi
        $Y2BASE_BINARY "$DEFAULT_DIR/$module" gtk
    else
        exit 0;
    fi
done
