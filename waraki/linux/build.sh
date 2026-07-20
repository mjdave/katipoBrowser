#!/bin/bash
cmake -H. -Bbuild
sleep 2
if cmake --build build -- -j4; then
ln -fTs ../../../app bin/app
ln -fTs ../../waraki-app bin/waraki-app
ln -fTs ../../waraki-site bin/waraki-ste
echo "Build complete."
else
exit 1;
fi
