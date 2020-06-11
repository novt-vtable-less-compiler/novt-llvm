#!/usr/bin/env bash

# partially taken from https://xw.is/wiki/Create_Debian_sysroots

set -e

# run as root.
# dependencies: apt install -y binfmt-support qemu qemu-user-static debootstrap


SCRIPTPATH="$( cd "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
ROOT=$(dirname "$SCRIPTPATH")

mkdir -p /tmp/sysroots
cd /tmp/sysroots

arches="armhf,arm-linux-gnueabihf amd64,x86_64-linux-gnu arm64,aarch64-linux-gnu mips,mips-linux-gnu mips64el,mips64el-linux-gnuabi64 ppc64el,powerpc64le-linux-gnu i386,i386-linux-gnu"
packages="gdb,systemtap-sdt-dev,libgmp-dev,libmpfr-dev,libmpc-dev,libunwind-dev"

for arch in $arches; do
	IFS=","
	set $arch
	echo "==============================================================================="
	echo "Creating sysroot for architecture $1  ($2)"
	echo "==============================================================================="
	rm -rf "$2"
	qemu-debootstrap --arch "$1" --variant=buildd stable "$2" http://deb.debian.org/debian --include="$packages"
	unset IFS
done

for arch in $arches; do
	IFS=","
	set $arch
	find "$2" -type l -lname '/*' -exec sh -c 'file="$0"; dir=$(dirname "$file"); target=$(readlink "$0"); prefix=$(dirname "$dir" | sed 's@[^/]*@\.\.@g'); newtarget="$prefix$target"; ln -snf $newtarget $file' {} \;
	echo "[OK] Corrected links for $1 / $2."
	unset IFS
done

for arch in $arches; do
	IFS=","
	set $arch
	tar -c "$2/usr/include" "$2/usr/lib" "$2"/lib* "$2"/etc/ld* | xz -T 0 > "$ROOT/sysroots/sysroot-$2.tar.xz"
	echo "[OK] Created sysroot-$2.tar.xz ."
	unset IFS
done


#cd "$ROOT/sysroots"
#for arch in $arches; do
#	IFS=","
#	set $arch
#	tar -xf "$ROOT/sysroots/sysroot-$2.tar.xz"
#	echo "[OK] Extracted sysroot-$2.tar.xz ."
#	unset IFS
#done
