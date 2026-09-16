#!/bin/bash
cmake -H. -Bbuild
sleep 2
if cmake --build build -- -j4; then
rm -rf koru
rm -rf koru.tar.xz
mkdir koru
cp -rf bin/koru koru/
cp -rf bin/lib koru/
cp -rf ../app koru/app
cp -rf ../katipo/apps/katipoTracker/scripts/tracker.tui koru/tracker.tui
cp -rf ../katipo/apps/katipoHost/scripts/host.tui koru/host.tui
tar -cvJf koru.tar.xz koru
echo "Release complete."
else
exit 1;
fi
