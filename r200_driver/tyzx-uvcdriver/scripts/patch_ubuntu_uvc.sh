DIR="$( cd "$( dirname "$0" )" && pwd )"

mv ../patches/*-generic ../patches/$(uname -r)

if [ ! -d "$DIR/../patches/`uname -r`" ]; then
    echo -e "\n\tNo patches found for kernel `uname -r`"
    echo -e "\tI expected patches in $DIR/../patches/`uname -r`"
    echo -e "\tMaybe this script is placed in the wrong place?"
    echo -e "\tExiting. Start this script again once the problem is fixed.\n"
    exit 1;
fi

sudo apt-get install dpkg-dev
rm -fr ~/uvc_patch
mkdir -p ~/uvc_patch
cd ~/uvc_patch
apt-get source linux-image-$(uname -r)
sudo apt-get build-dep linux-image-$(uname -r)
sudo apt-get install linux-headers-$(uname -r)
cd linux-lts-*
KBASE=`pwd`

# Look for patches for specific kernel
if [ -d "$DIR/../patches/`uname -r`" ]; then
    echo "found patches for kernel `uname -r`"
    find $DIR/../patches/`uname -r`/ -type f | xargs -I{} sh -c "patch -p1 < '{}'"
else
    echo "No patches found for kernel `uname -r`"
    exit 1
    read -p "Please merge patches into kernel $KBASE, press ENTER when done."
    # patch -p1 < $DIR/../patches/linux-lts-saucy-3_11_0.patch
    # patch -p1 < $DIR/../patches/uvc_close_bulk_stream.patch
fi

cp /boot/config-`uname -r` .config
cp  /usr/src/linux-headers-`uname -r`/Module.symvers .
make scripts oldconfig modules_prepare
cd drivers/media/usb/uvc
make -C $KBASE M=$KBASE/drivers/media/usb/uvc/ modules

## Copy module
sudo cp $KBASE/drivers/media/usb/uvc/uvcvideo.ko /lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko
## Remove old module if it's inserted
sudo rmmod uvcvideo.ko
