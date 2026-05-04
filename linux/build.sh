#!/bin/bash
cmake -H. -Bbuild
sleep 2
if cmake --build build -- -j4; then
#cp -f bin/katipoClient ../katipoClient
echo "Build complete."
else
exit 1;
fi
