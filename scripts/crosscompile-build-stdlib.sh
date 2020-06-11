#!/usr/bin/env bash

set -e

SCRIPTPATH="$( cd "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
ROOT=$(dirname "$SCRIPTPATH")
CLANG_BIN="$ROOT/build/bin"
WORKDIR="$ROOT/tmp"
mkdir -p "$WORKDIR"
cd "$WORKDIR"


# prepare stuff - once
if [ ! -f gcc-8-8.3.0/gcc-8.3.0-dfsg.tar.xz ]; then
	echo "Downloading ..."
	wget http://deb.debian.org/debian/pool/main/g/gcc-8/gcc-8_8.3.0.orig.tar.gz
	tar -xf gcc-8_8.3.0.orig.tar.gz
	rm -f gcc-8_8.3.0.orig.tar.gz
fi



make_clean_libstdcxx () {
	cd "$WORKDIR"
	echo "Extracting ..."
	rm -rf gcc-8.3.0
	tar -xf gcc-8-8.3.0/gcc-8.3.0-dfsg.tar.xz
	rm -f config-ml.in
	ln -s gcc-8.3.0/config-ml.in ./
}




make_libstdcxx () {
	if [ ! -d "$SYSROOT" ]; then
		echo "ERROR: $SYSROOT does not exist"
		return 1
	fi
	cd "$WORKDIR/gcc-8.3.0"
	pushd .
	echo "Configuring (1) ..."
	CC="$CLANG_BIN/clang $FLAGS --sysroot $SYSROOT"
	CXX="$CLANG_BIN/clang++ $FLAGS --sysroot $SYSROOT"
	LD="$CLANG_BIN/ld.lld"
	CFLAGS="$FLAGS --sysroot $SYSROOT -flto -O3 -isystem $SYSROOT/usr/lib/gcc/$1/8/include -isystem $SYSROOT/usr/include/$1"
	CXXFLAGS="$FLAGS --sysroot $SYSROOT -flto -O3 -isystem $SYSROOT/usr/include/c++/8 -isystem $SYSROOT/usr/include/$1/c++/8 -isystem $SYSROOT/usr/include/c++/8/backward -isystem $SYSROOT/usr/lib/gcc/$1/8/include -isystem $SYSROOT/usr/include/$1"
	LDFLAGS="$FLAGS $LINK_FLAGS --sysroot $SYSROOT -flto -O3 -L$SYSROOT/usr/lib/$1 -L$SYSROOT/usr/lib/gcc/$1/8 -B$SYSROOT/usr/lib/$1 -B$SYSROOT/usr/lib/gcc/$1/8 -no-novt-libs"
	mkdir -p "$SYSROOT/novt-root"

	# configure parent repo
	./configure CC="$CC" CXX="$CXX" LD="$LD" CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS" LDFLAGS="$LDFLAGS" --build=x86_64-linux-gnu --prefix=$SYSROOT/novt-root --host=$1 --enable-multilib=no

	cd libstdc++-v3
	# patch (1)
	sed -i 's|ac_has_gthreads=no|ac_has_gthreads=yes|' configure
	sed -i 's|unsigned int\* uip;|unsigned int* uip = 0;|' configure

	# configure libstdc++
	echo "Configuring (2) ..."
	./configure CC="$CC" CXX="$CXX" LD="$LD" CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS" LDFLAGS="$LDFLAGS" --build=x86_64-linux-gnu --prefix=$SYSROOT/novt-root --host=$1 --with-system-libunwind=yes --enable-static --disable-shared --enable-version-specific-runtime-libs=yes --enable-multilib=no $CONFIGURE_ARGS

	# patch (2)
	# patch include/bits/c++config.h: #define _GLIBCXX_HAVE_ALIGNED_ALLOC 0
	# patch include/bits/c++config.h: #define _GLIBCXX_HAVE_POSIX_MEMALIGN 0

	sed -i 's|_GLIBCXX_HAVE_ALIGNED_ALLOC 1|_GLIBCXX_HAVE_ALIGNED_ALLOC 0|' include/*/bits/c++config.h
	sed -i 's|_GLIBCXX_HAVE_POSIX_MEMALIGN 1|_GLIBCXX_HAVE_POSIX_MEMALIGN 0|' include/*/bits/c++config.h
	sed -i 's|__builtin_sprintf|sprintf|' src/c++11/debug.cc
	sed -i 's|abs = base.is_absolute() ? base : absolute(base);|abs = base.is_absolute() ? base : fs::v1::absolute(base, current_path());|' src/filesystem/ops.cc

	# don't copy/paste this, because of tabs(!)
	cat >> src/c++11/Makefile <<'EOF'

cxx11-ios_failure-lt.ll: cxx11-ios_failure.cc
	$(LTCXXCOMPILE) -S $< -o tmp-cxx11-ios_failure-lt.s
	-test -f tmp-cxx11-ios_failure-lt.o && mv -f tmp-cxx11-ios_failure-lt.o tmp-cxx11-ios_failure-lt.s
	$(rewrite_ios_failure_typeinfo) tmp-cxx11-ios_failure-lt.s > $@
	-rm -f tmp-$@
EOF
	sed -i 's|: cxx11-ios_failure-lt.s|: cxx11-ios_failure-lt.ll|' src/c++11/Makefile

	cp "$SYSROOT/usr/lib/$1/"crt*.o src/
	cp "$SYSROOT/usr/lib/gcc/$1/8/"crt*.o src/
	sed -i 's|supc++-lm|supc++ -lm|' libtool  # fuck you libtool

	echo "Compiling ..."
	make -j `nproc` install

	mkdir -p "$SYSROOT/novt-lib"
	cp -r "$SYSROOT"/novt-root/lib/gcc/$1/*/*.a "$SYSROOT"/novt-root/lib/gcc/$1/*/include "$SYSROOT/novt-lib/"
	cd "$SYSROOT/.."
	tar -c "$(basename $SYSROOT)/novt-lib" | xz -T 0 > "$ROOT/sysroots/bundle-$1-libstdc++.tar.xz"
	echo "[OK] Bundle created. ----------------------------"
	popd >/dev/null
}



make_clean_libcxx () {
	cd "$WORKDIR"
	test -f libunwind-10.0.0.src.tar.xz || wget https://github.com/llvm/llvm-project/releases/download/llvmorg-10.0.0/libunwind-10.0.0.src.tar.xz
	test -f libcxxabi-10.0.0.src.tar.xz || wget https://github.com/llvm/llvm-project/releases/download/llvmorg-10.0.0/libcxxabi-10.0.0.src.tar.xz
	test -f libcxx-10.0.0.src.tar.xz || wget https://github.com/llvm/llvm-project/releases/download/llvmorg-10.0.0/libcxx-10.0.0.src.tar.xz
	test -d libunwind-10.0.0.src || tar -xf libunwind-10.0.0.src.tar.xz
	test -d libcxxabi-10.0.0.src || tar -xf libcxxabi-10.0.0.src.tar.xz
	test -d libcxx-10.0.0.src || tar -xf libcxx-10.0.0.src.tar.xz
	rm -rf libunwind-10.0.0.src/build-*
	rm -rf libcxxabi-10.0.0.src/build-*
	rm -rf libcxx-10.0.0.src/build-*
}


make_libcxx () {
	mkdir -p "$WORKDIR/libunwind-10.0.0.src/build-$1"
	mkdir -p "$WORKDIR/libcxxabi-10.0.0.src/build-$1"
	mkdir -p "$WORKDIR/libcxx-10.0.0.src/build-$1"
	mkdir -p "$SYSROOT/novt-root"

	export CC="$CLANG_BIN/clang $FLAGS --sysroot $SYSROOT"
	export CXX="$CLANG_BIN/clang++ $FLAGS --sysroot $SYSROOT"
	export LD="$CLANG_BIN/ld.lld"
	export CFLAGS="$FLAGS --sysroot $SYSROOT -flto -O3 -isystem $SYSROOT/usr/lib/gcc/$1/8/include -isystem $SYSROOT/usr/include/$1"
	export CXXFLAGS="$FLAGS --sysroot $SYSROOT -flto -O3 -isystem $SYSROOT/novt-root/include -isystem $SYSROOT/usr/include -isystem $SYSROOT/usr/include/c++/8 -isystem $SYSROOT/usr/include/$1/c++/8 -isystem $SYSROOT/usr/include/c++/8/backward -isystem $SYSROOT/usr/lib/gcc/$1/8/include -isystem $SYSROOT/usr/include/$1"
	export LDFLAGS="$FLAGS $LINK_FLAGS --sysroot $SYSROOT -flto -O3 -L$SYSROOT/novt-root/lib -L$SYSROOT/usr/lib/$1 -L$SYSROOT/usr/lib/gcc/$1/8 -B$SYSROOT/usr/lib/$1 -B$SYSROOT/usr/lib/gcc/$1/8 -no-novt-libs"

	cd "$WORKDIR/libunwind-10.0.0.src/build-$1"
	cmake .. -DCMAKE_INSTALL_PREFIX=$SYSROOT/novt-root -DLIBUNWIND_ENABLE_SHARED=Off \
		-DCMAKE_CROSSCOMPILING=True -DCMAKE_C_COMPILER_TARGET="$1" -DCMAKE_CXX_COMPILER_TARGET="$1" \
		-DCMAKE_CXX_FLAGS="$CXXFLAGS" -DCMAKE_C_FLAGS="$CFLAGS" -DCMAKE_EXE_LINKER_FLAGS="$LDFLAGS"
	make -j `nproc` install

	cd "$WORKDIR/libcxxabi-10.0.0.src/build-$1"
	cmake .. -DCMAKE_INSTALL_PREFIX=$SYSROOT/novt-root -DLIBCXXABI_USE_LLVM_UNWINDER=On -DLIBCXXABI_ENABLE_STATIC_UNWINDER=On -DLIBCXXABI_ENABLE_SHARED=Off -DLIBCXXABI_LIBCXX_INCLUDES="$WORKDIR/libcxx-10.0.0.src/include" -DLIBCXXABI_STATICALLY_LINK_UNWINDER_IN_STATIC_LIBRARY=Off \
		-DCMAKE_CROSSCOMPILING=True -DCMAKE_C_COMPILER_TARGET="$1" -DCMAKE_CXX_COMPILER_TARGET="$1" \
		-DCMAKE_CXX_FLAGS="$CXXFLAGS" -DCMAKE_C_FLAGS="$CFLAGS" -DCMAKE_EXE_LINKER_FLAGS="$LDFLAGS"
	make -j `nproc` install

	export CXXFLAGS="-isystem $WORKDIR/libcxxabi-10.0.0.src/include $CXXFLAGS"
	cd "$WORKDIR/libcxx-10.0.0.src/build-$1"
	cmake .. -DCMAKE_INSTALL_PREFIX=$SYSROOT/novt-root -DLIBCXX_ENABLE_SHARED=Off -DLIBCXXABI_USE_LLVM_UNWINDER=On -DLIBCXX_CXX_ABI=libcxxabi -DLIBCXX_ENABLE_STATIC_ABI_LIBRARY=Off \
		-DCMAKE_CROSSCOMPILING=True -DCMAKE_C_COMPILER_TARGET="$1" -DCMAKE_CXX_COMPILER_TARGET="$1" \
		-DCMAKE_CXX_FLAGS="$CXXFLAGS" -DCMAKE_C_FLAGS="$CFLAGS" -DCMAKE_EXE_LINKER_FLAGS="$LDFLAGS"
	make -j `nproc` install

	mkdir -p "$SYSROOT/novt-lib"
	cp -r "$SYSROOT"/novt-root/lib/*.a "$SYSROOT"/novt-root/include "$SYSROOT/novt-lib/"
	cd "$SYSROOT/.."
	tar -c "$(basename $SYSROOT)/novt-lib" | xz -T 0 > "$ROOT/sysroots/bundle-$1-libc++.tar.xz"
	echo "[OK] Bundle created. ----------------------------"
}




FLAGS="--target=armv7-pc-linux-gnueabi-gcc -mfloat-abi=hard"
LINK_FLAGS=""
SYSROOT="$ROOT/sysroots/arm-linux-gnueabihf"
make_clean_libstdcxx
make_libstdcxx "arm-linux-gnueabihf"


FLAGS="--target=aarch64-linux-gnu"
LINK_FLAGS=""
SYSROOT="$ROOT/sysroots/aarch64-linux-gnu"
make_clean_libcxx
make_libcxx "aarch64-linux-gnu"


FLAGS="--target=mips-mti-linux-gnu"
LINK_FLAGS="-Wl,--hash-style=sysv -Wl,-z,notext"
SYSROOT="$ROOT/sysroots/mips-linux-gnu"
make_clean_libstdcxx
make_libstdcxx "mips-linux-gnu"


FLAGS="--target=mips64el-linux-gnuabi64"
LINK_FLAGS=""
SYSROOT="$ROOT/sysroots/mips64el-linux-gnuabi64"
make_clean_libstdcxx
make_libstdcxx "mips64el-linux-gnuabi64"


FLAGS="--target=powerpc64le-linux-gnu"
LINK_FLAGS=""
SYSROOT="$ROOT/sysroots/powerpc64le-linux-gnu"
#make_clean_libstdcxx
#sed -i 's|-mno-gnu-attribute||' "$WORKDIR/gcc-8.3.0/libstdc++-v3/configure"
#make_libstdcxx "powerpc64le-linux-gnu"
make_clean_libcxx
make_libcxx "powerpc64le-linux-gnu"


FLAGS="--target=i686-linux-gnu"
LINK_FLAGS=""
SYSROOT="$ROOT/sysroots/i386-linux-gnu"
ln -s "$SYSROOT/usr/lib/gcc/i686-linux-gnu" "$SYSROOT/usr/lib/gcc/i386-linux-gnu" 2>/dev/null || true
make_clean_libstdcxx
make_libstdcxx "i386-linux-gnu"


FLAGS=""
LINK_FLAGS=""
SYSROOT="$ROOT/sysroots/x86_64-linux-gnu"
make_clean_libstdcxx
make_libstdcxx "x86_64-linux-gnu"

