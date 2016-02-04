#!/bin/sh
DEMO=/usr/bin/pr-demo-gui
. /etc/default/rcS
FB=fb0
while ! grep -q '^connected' /sys/class/drm/card0-HDMI-A-1/status
do
	# No external framebuffer yet, wait
	sleep 5
done
if [ "$ROOTFS_READ_ONLY" = "yes" ]
then
	export TSLIB_CALIBFILE=/tmp/pointercal
else
	export TSLIB_CALIBFILE=/etc/pointercal
fi
TS=/dev/input/touchscreen0

echo "External HDMI monitor connected."
# Locate external screen
for f in /sys/class/graphics/*
do
	if ! grep -q 'vdma-fb' $f/name
	then
		# workaround for monitor not working at startup.
		echo -n > /dev/dri/controlD64
		FB=`basename $f`
		echo "Found external display at $FB"
		break
	fi
done

TS="No Touchscreen"
for f in /sys/class/input/event*
do
	if ! grep -q AD7879 $f/device/name
	then
		if grep -q "input:.*-e0.*,3,.*a0,1,\|ads7846" $f/device/modalias
		then
			b=`basename $f`
			TS=/dev/input/$b
			echo "Found external touch screen at $b"
			TSLIB_CALIBFILE=${TSLIB_CALIBFILE}.$b
			break
		fi
	fi
done

export TSLIB_FBDEVICE=/dev/${FB}
export QWS_DISPLAY="LinuxFb:/dev/${FB}:1"
if [ -e "${TS}" ]
then
	export TSLIB_TSDEVICE="${TS}"
	export QWS_MOUSE_PROTO="Tslib:${TS}"
	export POINTERCAL_FILE="${TSLIB_CALIBFILE}"
	if [ ! -e "${TSLIB_CALIBFILE}" ]
	then
		TSLIB_CONSOLEDEVICE=none ts_calibrate < /dev/null
		sync
	fi
fi
exec $DEMO -qws
