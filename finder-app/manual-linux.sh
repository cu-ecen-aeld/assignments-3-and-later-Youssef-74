#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here

    # first step deep clean the jernel build tree
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper

    # second step configure the kernel for virtual arm dev board "QEMU"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig

    # third build the kernel image 
    make -j$(nproc) ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all

    # fourth step build the kernel modules and the devicetree 
    #make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs

fi

echo "Adding the Image in outdir"
# Adding the Image in outdir
cp ${OUTDIR}/linux-stable/arch/arm64/boot/Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
mkdir -p rootfs
cd ${OUTDIR}/rootfs
# creating a folder tree 
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var 
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox

    # first step clean all the previous generated files from previous builds 
    make distclean

    # second step configure the busybox for virtual arm dev board "QEMU"
    make defconfig

else
    cd busybox
fi

# TODO: Make and install busybox
#make distclean
#make defconfig
# third build the busybox image 
make -j$(nproc) ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} 

# last step define the path to the root filesystem and the install generated files in it 
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

#cd ${OUTDIR}/rootfs

echo "Library dependencies"
#${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
#${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
echo "Add library dependencies to rootfs"
aarch64_toolchain=/home/youssef/AELD/x-tools/aarch64_toolchain/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu
# copy the needed libraries from toolchain sysroot to /lib and /lib64 of created rootfs
cp ${aarch64_toolchain}/aarch64-none-linux-gnu/libc/lib/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib
cp ${aarch64_toolchain}/aarch64-none-linux-gnu/libc/lib64/libm.so.6 ${OUTDIR}/rootfs/lib64
cp ${aarch64_toolchain}/aarch64-none-linux-gnu/libc/lib64/libresolv.so.2 ${OUTDIR}/rootfs/lib64
cp ${aarch64_toolchain}/aarch64-none-linux-gnu/libc/lib64/libc.so.6 ${OUTDIR}/rootfs/lib64


# TODO: Make device nodes
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3
sudo mknod -m 600 ${OUTDIR}/rootfs/dev/console c 5 1
#sudo mknod -m 666 ${OUTDIR}/rootfs/dev/tty c 5 0

# TODO: Clean and build the writer utility
cd /home/youssef/AELD/part1_assignments/assignment-3-Youssef-74/finder-app
make clean
# edit to compile with the cross compiler aarch64-none-linux-gnu-
make CROSS_COMPILE=aarch64-none-linux-gnu-

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp writer ${OUTDIR}/rootfs/home/
cp finder.sh ${OUTDIR}/rootfs/home/
cp finder-test.sh ${OUTDIR}/rootfs/home/
cp autorun-qemu.sh ${OUTDIR}/rootfs/home/

mkdir -p ${OUTDIR}/rootfs/home/conf/
cp conf/* ${OUTDIR}/rootfs/home/conf/

# TODO: Chown the root directory
cd ${OUTDIR}/rootfs/
sudo chown -R root:root *

# TODO: Create initramfs.cpio.gz
cd ${OUTDIR}/rootfs/
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd ..
gzip -f initramfs.cpio