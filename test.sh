#!/bin/bash

echo "Hackish script to run a ton of yast2-gtk test that we know"
echo "should work and layout fairly nicely."

if test "z$1" = "z"; then
    tests="Label3 HelloWorld PushButton2 RichText1"
else
    tests="$@"
fi
yast="/usr/lib/YaST2/bin/y2base"

for test in $tests; do
    $yast /usr/share/doc/packages/yast2-core/libyui/examples/$test.ycp gtk >& /dev/null &
done
