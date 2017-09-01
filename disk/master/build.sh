#########################################################################
# File Name: build.sh
# Author: Liujingrui
# mail: ylygljr@163.com
# Created Time: Wed 02 Aug 2017 08:56:45 AM CST
#########################################################################
#!/bin/bash

set -x

SOURCE_DIR=`pwd`
BUILD_DIR=${BUILD_DIR:-./build/}

mkdir -p $BUILD_DIR\
	&& cd $BUILD_DIR\
	&& cmake $SOURCE_DIR\
	&& make $*

