#!/usr/bin/env bash

set -eu

# USAGE: ./build-libstdc++.sh <ARCHIVE>



SCRIPTPATH="$( cd "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
ROOT=$(dirname "$SCRIPTPATH")

mkdir -p "$ROOT/tmp"
rm -rf "$ROOT/tmp/*"

echo "extracting (1/2) ..."
tar -xf "$1" -C "$ROOT/tmp"
cd "$ROOT/tmp"
cd gcc-?-?.?.?
echo "extracting (2/2) ..."
tar -xf gcc-?.?.?*.tar.*
cd gcc-?.?.?
rm -f ../config-ml.in
ln -s "`pwd`/config-ml.in" ../




echo "Configuring (1) ..."
CLANG_BIN="$ROOT/build/bin"
# configure parent repo
./configure CC=$CLANG_BIN/clang CXX=$CLANG_BIN/clang++ CXXFLAGS="-flto -O3" --prefix="$ROOT/tmp/install" --enable-multilib=no

cd libstdc++-v3
# patch (1)
sed -i 's|ac_has_gthreads=no|ac_has_gthreads=yes|' configure

# configure libstdc++
echo "Configuring (2) ..."
./configure CC=$CLANG_BIN/clang CXX=$CLANG_BIN/clang++ CXXFLAGS="-flto -O3" --prefix="$ROOT/tmp/install" --with-system-libunwind --enable-static --enable-version-specific-runtime-libs=yes --enable-multilib=no

# patch (2)
# patch include/bits/c++config.h: #define _GLIBCXX_HAVE_ALIGNED_ALLOC 0
# patch include/bits/c++config.h: #define _GLIBCXX_HAVE_POSIX_MEMALIGN 0
if [ -f "include/bits/c++config.h" ]; then
	F="include/bits/c++config.h"
else
	F="include/*/bits/c++config.h"
fi
sed -i 's|_GLIBCXX_HAVE_ALIGNED_ALLOC 1|_GLIBCXX_HAVE_ALIGNED_ALLOC 0|' "$F"
sed -i 's|_GLIBCXX_HAVE_POSIX_MEMALIGN 1|_GLIBCXX_HAVE_POSIX_MEMALIGN 0|' "$F"
sed -i 's|__builtin_sprintf|sprintf|' src/c++11/debug.cc
sed -i 's|abs = base.is_absolute() ? base : absolute(base);|abs = base.is_absolute() ? base : fs::v1::absolute(base, current_path());|' src/filesystem/ops.cc
sed -i 's|supc++-lm|supc++ -lm|' libtool  # f!ck you libtool

# don't copy/paste this, because of tabs(!)
cat >> src/c++11/Makefile <<'EOF'
cxx11-ios_failure-lt.ll: cxx11-ios_failure.cc
	$(LTCXXCOMPILE) -S $< -o tmp-cxx11-ios_failure-lt.s
	-test -f tmp-cxx11-ios_failure-lt.o && mv -f tmp-cxx11-ios_failure-lt.o tmp-cxx11-ios_failure-lt.s
	$(rewrite_ios_failure_typeinfo) tmp-cxx11-ios_failure-lt.s > $@
	-rm -f tmp-$@
EOF
sed -i 's|: cxx11-ios_failure-lt.s|: cxx11-ios_failure-lt.ll|' src/c++11/Makefile

echo "Compiling ..."
make -j `nproc` install


mkdir -p "$ROOT/novt-lib"
cp "$ROOT"/tmp/install/lib/gcc/*/*.a "$ROOT/novt-lib"
cp -r "$ROOT"/tmp/install/lib/gcc/*/include "$ROOT/novt-lib"

echo ""
echo ""
echo "libstdc++ has been built in installed to \"$ROOT/novt-lib\"."
rm -rf "$ROOT/tmp"
