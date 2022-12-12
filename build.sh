#!/usr/bin/env sh

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)

gcc=false

if [[ $gcc == true ]]; then
export CC=/usr/bin/gcc
export CXX=/usr/bin/g++
export AR=/usr/bin/gcc-ar
export CXX_LD=gold
export CC_LD=gold
export LD=/usr/bin/gold
else
export CC=/usr/bin/clang
export CXX=/usr/bin/clang++
export AR=/usr/bin/llvm-ar
export CXX_LD=lld
export CC_LD=lld
export LD=/usr/bin/ld.lld
fi

export CPP=/usr/bin/cpp

srcds='/sv/tf2'

clear

if [[ -d 'builddir' ]]; then
	if [[ ! -f 'builddir/build.ninja' ]]; then
		rm -rf 'builddir'
	fi
fi

if [[ ! -f 'subprojects/libyaml.wrap' ]]; then
	meson wrap install libyaml
	if [[ $? != 0 ]]; then
		exit 1
	fi
fi

if [[ ! -d 'builddir' ]]; then
	meson setup --backend=ninja --cross-file $script_dir'/x86-linux-gnu' -Dsrcds=$srcds 'builddir'
	if [[ $? != 0 ]]; then
		exit 1
	fi
fi

if [[ ! -d 'builddir' ]]; then
	echo 'no builddir'
	exit 1
fi

samu -j8 -C'builddir' -f'build.ninja'
if [[ $? != 0 ]]; then
	exit 1
fi

meson install -C'builddir' --no-rebuild --skip-subprojects='libyaml'
if [[ $? != 0 ]]; then
	exit 1
fi

exit 0