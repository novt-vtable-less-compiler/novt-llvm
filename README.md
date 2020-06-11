NoVT: Eliminating C++ Virtual Calls to Mitigate Vtable Hijacking
================================================================

NoVT is a compiler-based defense against vtable hijacking in C++ programs. 
NoVT replaces vtables with a switch-case construct that is inherently control-flow safe, thus preserving control flow integrity of C++ virtual dispatch.
NoVT extends Clang to perform a class hierarchy analysis on C++ source code. 
Instead of a vtable, each class gets unique identifier numbers which are used to dispatch the correct method implementation. 
Thereby, NoVT inherently protects all usages of a vtable, not just virtual dispatch.

The corresponding academic paper is currently under submission.
It has been tested with SPEC CPU 2006 and Chromium, on Ubuntu 18.04 and Debian 10.

The NoVT compiler is based on [LLVM 10, Clang 10 and LLD 10](https://releases.llvm.org/download.html). 




Dependencies
------------
The following packages are required to build NoVT. Names might slightly differ between distributions.

- `apt install build-essential cmake python` (build tools)
- `apt install libz3-dev libzip-dev libtinfo-dev libncurses-dev libxml++2.6-dev` (libraries - some of them are optional or only required for specific features)
- `apt install git wget` (additional tools)
- `apt install libgmp-dev libmpfr-dev libmpc-dev` (to build libstdc++)




Build NoVT
----------
1. Build the NoVT compiler
   - Install all the dependencies
   - Run `scripts/build-setup.sh`. It creates a `build` folder in the repository root and runs CMake for you
   - Run `make -j8 clang lld` in this `build` folder
2. Build a protected version of the C++ standard library
   - Download a source package depending on your distribution and your gcc version. We tested [Ubuntu 18.04/gcc-8](http://archive.ubuntu.com/ubuntu/pool/main/g/gcc-8/gcc-8_8.4.0.orig.tar.gz), [Ubuntu 18.04/gcc-7](http://archive.ubuntu.com/ubuntu/pool/main/g/gcc-7/gcc-7_7.5.0.orig.tar.gz) and [Debian Buster/gcc-8](http://deb.debian.org/debian/pool/main/g/gcc-8/gcc-8_8.3.0.orig.tar.gz). Others might work or not.
   - Run `scripts/build-libstdc++.sh <downloaded-gcc-xyz.orig.tar.gz>`
   - Find the compiled libraries in `/novt-lib` (repository root)
3. Install the NoVT library directory (containing protected versions of libraries)
   - `sudo ln -s $(pwd)/novt-lib /usr/novt-lib`




Test NoVT with sample applications
----------------------------------
We ship NoVT with a set of handcrafted C++ programs that test various aspects of C++, including corner-cases and strange class hierarchies. See folder `/tests` for the tests. 

To run the tests and check their output:
- first ensure you have NoVT build and a protected C++ standard library installed. 
- install `clang++-9` as reference compiler (or adjust the `COMPILER_REF` constant in `tests/test_samples.py` if you want to use another clang)
- go to directory `tests` and run `./test_samples.py`
- if you want to test other architectures, build the sysroots and install qemu, then run `./test_samples.py all`




Building Things with NoVT
-------------------------
Basically everything you have to do setting the default compiler to NoVT. 
Depending on your build system this could be something like:
- `export CC=<path>/build/bin/clang` + `export CXX=<path>/build/bin/clang++`
- `-DCMAKE_C_COMPILER=<path>/build/bin/clang -DCMAKE_CXX_COMPILER=<path>/build/bin/clang++`


### SPEC CPU 2006
To build the C++ programs of SPEC CPU 2006, you have to install two fixes for the source code: 
- [447.dealII.libcxx_pair.cpu2006.v1.2.tar.xz](https://www.spec.org/cpu2006/src.alt/447.dealII.libcxx_pair.cpu2006.v1.2.tar.xz)
- [483.xalancbmk.schemavalidator_sidecast.cpu2006.v1.2.tar.xz](extra/483.xalancbmk.schemavalidator_sidecast.cpu2006.v1.2.tar.xz)

You find a sample config file in [extra/](extra/spec-clang-lto-o3-novt-shielded.cfg).
Don't forget to edit the compiler location.

How to benchmark:
- Set CPU governor, intel pstate etc.
- `sudo cset shield -c 2,3 -k on` - create an isolated cpuset
- `sudo cset shield --exec -- sudo -u <your-user> -H bash` - get a shell in that isolated cpuset
- `. ./shrc` - enable SPEC
- `ulimit -s unlimited` - otherwise xalancbmk will crash
- `runspec -c spec-clang-lto-o3-novt-shielded.cfg --noreportable 473.astar 471.omnetpp 483.xalancbmk 444.namd 447.dealII 450.soplex 453.povray`


### LLVM
To check that protected programs do not change semantics, we compiled a protected version of LLVM and ran its unit tests.
- Download LLVM (we did with 9.0.0) and Clang from [https://releases.llvm.org/download.html](https://releases.llvm.org/download.html)
- Extract LLVM, extract clang to `<llvm-dir>/tools/clang`
- `cd <llvm-dir>/ ; mkdir build ; cd build` - create build directory
- ```
  cmake .. -DCMAKE_BUILD_TYPE=MinSizeRel -DLLVM_INCLUDE_BENCHMARKS=ON -DLLVM_INCLUDE_TESTS=ON -DLLVM_INCLUDE_EXAMPLES=OFF \
      -DLLVM_USE_LINKER=lld -DBUILD_SHARED_LIBS=OFF -DLLVM_ENABLE_ASSERTIONS=On \
      -DCMAKE_CXX_COMPILER=<novt-path>/bin/clang++ -DCMAKE_C_COMPILER=<novt-path>/bin/clang
  ```
- `make -j8 check-llvm-unit` - run the unit tests

You should see (for LLVM 9.0.0) 3874 passed tests.
One test fails (`PluginsTests.LoadPlugin`) - this is expected because NoVT does not support dynamic loading of library with C++ classes, which is exactly what this test checks.


### Chromium
First: prepare for some pain and bring some RAM - Chromium might require more than 32GB. 
Chromium relies on special compiler flags that are not present in mainline Clang 10 - they usually build using a modified version of the LLVM 11 trunk. We have to apply a few patches to get the build system compatible with Clang 10. 
We have build a release version of Chromium 83, newer versions should also work.

Check out depot_tools and the Chromium source code, as described here: [Chromium Project](https://chromium.googlesource.com/chromium/src/+/master/docs/linux/build_instructions.md) - follow the instructions until "Setting up the build". 
After you have the source code, get all tags (`git fetch --tags`) and check out the version you want to build. Run `gclient sync` afterwards to get all submodules in a consistent state. 
Before you generate a build directory (`gn gen out/YourBuildName`), you should apply these patches (inside the `src` directory):

- `sed -i 's|flto=thin|flto|' build/config/compiler/BUILD.gn` - switch thin LTO for full LTO
- `sed -i 's|"-Wno-non-c-typedef-for-linkage"|#"-Wno-non-c-typedef-for-linkage"|' build/config/compiler/BUILD.gn` - disable a recent compiler warning
- `sed -i 's|#pragma clang max_tokens_here|// #pragma clang max_tokens_here|' content/public/browser/content_browser_client.cc` - disable another warning
- Open `build/config/compiler/BUILD.gn` with a text editor, search for `"-Wno-non-c-typedef-for-linkage"` and comment out the whole block (between the surrounding `{}`)

Now you can generate a build directory, you have to configure it like this. Use `gn args out/YourBuildName` to do so:
```bash
# minimize file size
is_debug = false
enable_nacl = false
symbol_level = 0
blink_symbol_level = 0
dcheck_always_on = false
# use static libraries
is_component_build = false

# use the NoVT compiler
clang_base_path = "<novt-root>/build"
clang_use_chrome_plugins = false

# use LTO and optimization
use_thin_lto = true
thin_lto_enable_optimizations = true
```
Note that thin LTO has already been exchanged with full LTO. 
Debug builds require much more memory, we advise to build in release mode. 
Finally build Chromium: `autoninja -C out/YourBuildName chrome`

This build configuration is good enough to get a working browser, but not necessarily complete - there might be shared libraries remaining which are not supported by NoVT. 








NoVT on other architectures
---------------------------
To compile binaries for other targets than your native architecture, you first need a "sysroot", a directory containing a minimal set of libraries and include files for the desired architecture. 
Next you have to cross-compile the C++ standard library (`libstdc++` or `libc++` for aarch64 and powerpc64le). 
Finally NoVT can cross-compile binaries (without dependencies missing in those sysroots) to other architectures. 
We support cross-compilation to x86, x86_64, arm, aarch64 (64-bit arm), mips, mips64el, powerpc64le, with sysroots based on Debian Buster. 
Other architectures might work with little adaptions to the build system.

All instructions have been tested on Ubuntu 18.04, but should work on any other Debian-like system.

1. Create sysroots for all common architectures
   - Install dependencies: `apt install -y binfmt-support qemu qemu-user-static debootstrap`
   - Create sysroots: `sudo scripts/crosscompile-create-sysroots.sh` - yes you need root for this. Sorry.
   - After a while you have tarballs containing all necessary files in `/sysroots`
   - Extract the tarballs: `cd sysroots ; for file in sysroot-*.tar.xz; do tar -xf "$file"; done`
2. Compile the C++ standard libraries
   - Run `scripts/crosscompile-build-stdlib.sh`
   - Check that standard libraries have been created (in directories `/sysroots/*/novt-lib` and as tarball in `/sysroots`).
3. Test NoVT on different architectures
   - Use the test tool: `tests/test_samples.py all`
4. If you want to cross-compile other applications, you can get the necessary parameters:
   - `tests/test_samples.py all config`
   - Bear in mind that cross-compilation is a non-trivial topic on its own, with thousands of things that might go wrong. Not necessarily NoVT-related things.




NoVT Source Code
----------------
In Clang most of the magic happens in [llvm-novt/tools/clang/lib/CodeGen/NoVTCustomCXXABI.h](llvm-novt/tools/clang/lib/CodeGen/NoVTCustomCXXABI.h).

In LLD most of the magic happens in [llvm-novt/lib/Transforms/IPO/NoVTAbiBuilder.cpp](llvm-novt/lib/Transforms/IPO/NoVTAbiBuilder.cpp).

