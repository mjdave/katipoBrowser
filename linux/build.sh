#!/bin/bash
cmake -H. -Bbuild
sleep 2
if cmake --build build -- -j4; then
ln -fTs ../../app bin/app
echo "Build complete."
else
exit 1;
fi
