#!/bin/sh
# Usage: Run this script with no argument to be presented
# with Yast modules. 'z' argument for Yast-core shipped
# examples. 'x' for our own set of tests.

ZENITY_BINARY=zenity
Y2BASE_BINARY="/usr/lib/YaST2/bin/y2base -l /dev/stderr"

DEFAULT_DIR=/usr/share/YaST2/clients
FRONTEND="gtk"

if [ $# -gt 0 ]; then
    if [ "$1" == "z" ]; then
        DEFAULT_DIR=/usr/share/doc/packages/yast2-ycp-ui-bindings/examples
    fi
    if [ "$1" == "x" ]; then
        DEFAULT_DIR=.
        # go to the tests directory
        # (this doesn't affect user's cwd because this
        #  is running in its own shell.)
        cd "`dirname $0`/tests"
    fi
    if [ "$1" == "help" ]; then
        echo "ycc.sh usage:"
        echo "  \"$0\" - yast modules"
        echo "  \"$0 z\" - yast-core examples"
        echo "  \"$0 x\" - our set of tests"
        echo "  pass \"qt\" to also run the qt frontend"
        echo ""
        exit
    fi
    if [ "$1" == "qt" ]; then
        FRONTEND="qt"
    fi
fi

if [ $# -eq 2 ] && [ "$2" == "qt" ]; then
    RUN_QT=1
fi

module_list=`ls $DEFAULT_DIR/*.ycp | sed 's/.*\///'`
while true; do
    module=`$ZENITY_BINARY --title "Yast Control Center" \
            --list --column "Available modules:" $module_list`;
    if [ "$module" ]; then
        $Y2BASE_BINARY "$DEFAULT_DIR/$module" $FRONTEND
    else
        exit 0;
    fi
done
