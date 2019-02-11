#! /bin/bash
# Copyright (c) 2017 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

S1UDEV=0000:06:00.1
SGIDEV=0000:06:00.0

USAGE="Usage: switch_ng4t_ilnperf.sh [ ng4t | ilnperf ]
	ng4t:		Binds $S1UDEV & $SGIDEV to ng40
	ilnperf:	Binds $S1UDEV & $SGIDEV to igb_uio.
	NOTE: Run this script as ROOT user."

if [ "$1" == "ng4t" ]; then
	echo "Switching $S1UDEV & $SGIDEV to ng40..."
	echo -e "\tUnbind $S1UDEV & $SGIDEV from using any driver..."
	./dpdk/usertools/dpdk-devbind.py -u $S1UDEV $SGIDEV
	read -p "Rebooting system!!! Y/N?" YN
	if [[ $YN == "Y" || $YN == "y" ]]; then
		reboot
	else 
		echo "Aborting switch to ng40..."
		exit
	fi
elif [ "$1" == "ilnperf" ]; then
	echo "Switching $S1UDEV & $SGIDEV to igb_uio..."
	echo "Step 1:"
	echo -e "\tStopping ng4t services @/etc/init.d/.depend.start..."
	ng40forcecleanup all
	service ng4t-factory stop
	service ng4t-inteldpdk stop
	service ng4t-probes stop
	service ng4t-remotecap stop
	sleep 10
	echo "Step 2:"
	echo -e "\tClear huge pages allocated @/dev/hugepages/rtemap_*..."
	rm -f /dev/hugepages/rtemap_*
	echo "Step 3:"
	echo -e "\tinstall hugepages..."
	sysctl -p
	echo "Step 4:"
	echo -e "\tRemove existing uio & igb_uio modules..."
	modprobe -q -r igb_uio
	modprobe -q -r uio
	sleep 5
	echo "Step 5:"
	echo -e "\tinsert uio & igb_uio modules..."
	source setenv.sh
	modprobe uio
	insmod dpdk/x86_64-native-linuxapp-gcc/kmod/igb_uio.ko 
	lsmod | grep uio
	echo "Step 5:"
	echo -e "\tBind $S1UDEV & $SGIDEV to igb_uio..."
	./dpdk/usertools/dpdk-devbind.py -b igb_uio $S1UDEV $SGIDEV
else
	echo "$USAGE"
fi

