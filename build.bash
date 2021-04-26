#!/bin/bash

mkdir -p Win32_Release

# copy readme
sed 's/\r$//' README.md | sed 's/$/\r/' > Win32_Release/cfs_frontend.txt
./build.bat

cd Win32_Release
zip ../cfs_frontend_wip.zip cfs_frontend.txt cfs_frontend.exe WebView2Loader.dll
