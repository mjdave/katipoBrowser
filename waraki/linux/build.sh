#!/bin/bash
cmake -H. -Bbuild
sleep 2
if cmake --build build -- -j4; then
ln -fTs ../../../app bin/app
ln -fTs ../../waraki-app bin/waraki-app
ln -fTs ../../waraki-site bin/waraki-site
ln -fTs ../../../katipo/apps/katipoTracker/scripts/tracker.tui bin/tracker.tui
ln -fTs ../../../katipo/apps/katipoHost/scripts/host.tui bin/host.tui
echo "Build complete."
else
exit 1;
fi
