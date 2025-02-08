#!/bin/bash


# 環境変数WORKDIRの設定
if [  -z $WORKDIR ] ; then
  WORKDIR=`pwd`
fi
echo "WORKDIR=$WORKDIR"

EDK2DIR="$HOME/edk2"

# build MikanLoaderPkg

LoaderPkgDir="MikanLoaderPkg"
LoaderPkgDsc="MikanLoaderPkg.dsc"

WORKSPACE="$EDK2DIR" source "$EDK2DIR/edksetup.sh"
LDFLAGS="$BINUTILS_L" CPPFLAGS="$BINUTILS_H" WORKSPACE="$EDK2DIR" build -p $LoaderPkgDir/$LoaderPkgDsc -b DEBUG -a X64 -t CLANG38


