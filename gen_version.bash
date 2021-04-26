#!/bin/bash
# update version string
VERSION='v0.1'
GITHASH=`git rev-parse --short HEAD`
cat << EOS | sed 's/\r$//' | sed 's/$/\r/' > 'version.h'
#pragma once
const char *version = "$VERSION ( $GITHASH )";
EOS
