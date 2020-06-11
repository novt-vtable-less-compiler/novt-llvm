#!/usr/bin/env bash

# Prepares the "build" folder

set -e

cd "${0%/*}"
cd ..


if [ -d "llvm-novt/tools/lld" ]; then
	echo "lld already present."
else
	echo "Downloading lld ..."
	test -f lld-10.0.0.src.tar.xz || wget https://github.com/llvm/llvm-project/releases/download/llvmorg-10.0.0/lld-10.0.0.src.tar.xz
	tar -xf lld-10.0.0.src.tar.xz
	mv lld-10.0.0.src llvm-novt/tools/lld
	rm lld-10.0.0.src.tar.xz
	echo "lld downloaded."
fi




mkdir -p build
cd build
rm -rf *
cmake -DCMAKE_BUILD_TYPE=MinSizeRel -DLLVM_TARGETS_TO_BUILD="X86;AArch64;ARM;Mips;PowerPC" -DLLVM_INCLUDE_BENCHMARKS=OFF -DLLVM_INCLUDE_TESTS=OFF -DLLVM_INCLUDE_EXAMPLES=OFF -DLLVM_ENABLE_ASSERTIONS=On ../llvm-novt/
echo ""
echo ""
echo ""
echo "Build directory prepared. To build run:"
echo "    cd `pwd`"
echo "    make -j `nproc` clang lld"
