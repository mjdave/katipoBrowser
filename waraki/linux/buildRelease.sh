#!/bin/bash
cmake -H. -Bbuild
sleep 2
if cmake --build build -- -j4; then
rm -rf waraki
rm -rf waraki.tar.xz
mkdir waraki
cp -rf bin/waraki waraki/
cp -rf bin/lib waraki/
cp -rf ../../app waraki/app
cp -rf ../waraki-app waraki/waraki-app
cp -rf ../waraki-site waraki/waraki-site
cp -rf ../../katipo/apps/katipoTracker/scripts/tracker.tui waraki/tracker.tui
cp -rf ../../katipo/apps/katipoHost/scripts/host.tui waraki/host.tui
tar -cvJf waraki.tar.xz waraki
echo "Release complete ."
else
exit 1;
fi
 