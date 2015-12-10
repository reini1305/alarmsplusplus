#!/bin/bash

mkdir temp
# Build 3.7 PT only pbw
cp appinfo.json appinfo.json.old
sed /"aplite"/d appinfo.json.old > appinfo.json
pebble clean --sdk 3.7 && pebble build --sdk 3.7 && cp build/*.pbw temp/37.pbw
# Build 3.8 aplite pbw
mv appinfo.json.old appinfo.json
pebble clean --sdk 3.8-beta10 && pebble build --sdk 3.8-beta10 && cp build/*.pbw temp/38.pbw

# replace 3.8 basalt and chalk with 3.7 versions
cd temp
unzip -d 37 37.pbw
unzip -d 38 38.pbw
rm -rf 38/basalt 38/chalk
mv -f 37/basalt 37/chalk 38
cd 38
zip -r franken.zip *

# put pbw in build folder
mv franken.zip ../../build/franken.pbw
cd ../../

# clean up
rm -rf temp
