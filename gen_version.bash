#!/bin/bash
# update version string
VERSION='v0.4'
GITHASH=`git rev-parse --short HEAD`
cat << EOS | sed 's/\r$//' | sed 's/$/\r/' > 'version.h'
#pragma once
LPCTSTR version = _T("$VERSION ( $GITHASH )");
EOS
