#!/bin/sh

Y2BASE_BINARY=/usr/lib/YaST2/bin/y2base

# any argument switches to the examples
DEFAULT_DIR=/usr/share/YaST2/clients
if test "z$1" != "z"; then
    DEFAULT_DIR=/usr/share/doc/packages/yast2-core/libyui/examples
fi

module_list=`ls $DEFAULT_DIR/*.ycp | sed 's/.*\///'`

while true; do
    module=`/opt/gnome/bin/zenity --title "Yast Control Center" \
            --list --column "Available modules:" $module_list`;
    if [ "$module" ]; then
	$Y2BASE_BINARY "$DEFAULT_DIR/$module" qt &
	$Y2BASE_BINARY "$DEFAULT_DIR/$module" gtk
    else
	exit 0;
    fi
done
