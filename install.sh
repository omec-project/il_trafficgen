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

cd $(dirname ${BASH_SOURCE[0]})
source ./services.cfg
export IL_TGEN=$PWD
echo "------------------------------------------------------------------------------"
echo " il_trafficgen exported as $IL_TGEN"
echo "------------------------------------------------------------------------------"

HUGEPGSZ=`cat /proc/meminfo  | grep Hugepagesize | cut -d : -f 2 | tr -d ' '`
IL_TGEN_MEMORY=6144*2	#il_trafficgen=6144MB + responder=6144MB Huge Page Memory
MODPROBE="/sbin/modprobe"
INSMOD="/sbin/insmod"
DPDK_DIR=$IL_TGEN/dpdk
IL_TGEN_DIR=$IL_TGEN/pktgen

#
# Sets QUIT variable so script will finish.
#
quit()
{
	QUIT=$1
}

# Shortcut for quit.
q()
{
	quit
}

setup_http_proxy()
{
	while true; do
		echo
		read -p "Enter Proxy : " proxy
		export http_proxy=$proxy
		export https_proxy=$proxy
		echo "Acquire::http::proxy \"$http_proxy\";" | sudo tee -a /etc/apt/apt.conf > /dev/null
		echo "Acquire::https::proxy \"$http_proxy\";" | sudo tee -a /etc/apt/apt.conf > /dev/null

		wget -T 20 -t 3 --spider http://www.google.com
		if [ "$?" != 0 ]; then
		  echo -e "No Internet connection. Proxy incorrect? Try again"
		  echo -e "eg: http://<proxy>:<port>"
		  exit 1
		fi
	return
	done
}

step_1()
{
        TITLE="Environment setup."
        CONFIG_NUM=1
        TEXT[1]="Check OS and network connection"
        FUNC[1]="setup_env"
}

setup_env()
{
	# a. Check for OS dependencies
	source /etc/os-release
	if [[ $VERSION_ID != "16.04" ]] ; then
		echo "WARNING: It is recommended to use Ubuntu 16.04..Your version is "$VERSION_ID
		echo "The libboost 1.58 dependency is not met by official Ubuntu PPA. Either attempt"
		echo "to find/compile boost 1.58 or upgrade your distribution by performing 'sudo do-release-upgrade'"
	else
		echo "Ubuntu 16.04 OS requirement met..."
	fi
	echo
	echo "Checking network connectivity..."
	# b. Check for internet connections
	wget -T 20 -t 3 --spider http://www.google.com
	if [ "$?" != 0 ]; then
		while true; do
			read -p "No Internet connection. Are you behind a proxy (y/n)? " yn
			case $yn in
				[Yy]* ) $SETUP_PROXY ; return;;
				[Nn]* ) echo "Please check your internet connection..." ; exit;;
				* ) "Please answer yes or no.";;
			esac
		done
	fi
}

step_2()
{
	TITLE="Download and Install"
	CONFIG_NUM=1
	TEXT[1]="Agree to download"
	FUNC[1]="get_agreement_download"
	TEXT[2]="Upgrade Ubuntu 16.04 LTS [OPTIONAL]"
	FUNC[2]="upgrade_os"
	TEXT[3]="Download packages"
	FUNC[3]="install_libs"
	TEXT[4]="Setup Huge Pages"
	FUNC[4]="setup_hugepages"
	TEXT[5]="Download DPDK submodule"
	FUNC[5]="updt_dpdk_submodule"
	TEXT[6]="Install DPDK"
	FUNC[6]="install_dpdk"
}

get_agreement_download()
{
	echo
	echo "List of packages needed for IL_TRAFFICGEN build and installation:"
	echo "-------------------------------------------------------"
	echo "1.  DPDK"
	echo "2.  build-essential"
	echo "3.  linux-headers-generic"
	echo "4.  git"
	echo "5.  unzip"
	echo "6.  libpcap-dev"
	echo "7.  make"
	echo "8.  curl"
	echo "9. openssl-dev"
	echo "10. and other library dependencies"
	while true; do
		read -p "We need download above mentioned package. Press (y/n) to continue? " yn
		case $yn in
			[Yy]* )
				touch .agree
				return;;
			[Nn]* ) exit;;
			* ) "Please answer yes or no.";;
		esac
	done
}

upgrade_os()
{
	echo "Upgrade Ubuntu 16.04 LTS to build and run IL_TRAFFICGEN..."
	file_name=".agree"
	if [ ! -e "$file_name" ]; then
		echo "Please choose option '2. Agree to download' first"
		return
	fi
	file_name=".download"
	if [ -e "$file_name" ]; then
		clear
		return
	fi
	sudo apt-get upgrade
	sudo apt-get install linux-headers-$(uname -r)
	touch .download
}

install_libs()
{
	echo "Install libs needed to build and run IL_TRAFFICGEN..."
	file_name=".agree"
	if [ ! -e "$file_name" ]; then
		echo "Please choose option '2. Agree to download' first"
		return
	fi
	file_name=".download"
	if [ -e "$file_name" ]; then
		clear
		#return
	fi
	sudo apt-get update
	sudo apt-get -y install curl build-essential linux-headers-$(uname -r) \
		git unzip libpcap0.8-dev gcc libjson0-dev make libc6 libc6-dev \
		g++-multilib libcurl4-openssl-dev libssl-dev msr-tools libnuma-dev \
		libconfig-dev

	touch .download
}

setup_hugepages()
{
	echo -e "Setting up $(($IL_TGEN_MEMORY)) huge page memory..."
	echo -e "Hugepagesize=\t$HUGEPGSZ"

	Hugepgsz=`echo $HUGEPGSZ | tr -d 'kB'`
	Pages=$((($IL_TGEN_MEMORY*1024) / $Hugepgsz))
	echo -e "Number of huge pages=\t$Pages"

	if [ ! "`grep nr_hugepages /etc/sysctl.conf`" ]; then
		echo "vm.nr_hugepages=$Pages" | sudo tee /etc/sysctl.conf
	else
		echo "vm.nr_hugepages=$Pages"
		sudo sed -i '/^vm.nr_hugepages=/s/=.*/='$Pages'/' /etc/sysctl.conf
	fi
	sudo sysctl -p
	sudo service procps start

	grep -s '/dev/hugepages' /proc/mounts
	if [ $? -ne 0 ] ; then
		echo "Creating /mnt/huge and mounting as hugetlbfs"
		sudo mkdir -p /mnt/huge
		sudo mount -t hugetlbfs nodev /mnt/huge
		echo "nodev /mnt/huge hugetlbfs defaults 0 0" | sudo tee -a /etc/fstab > /dev/null
	fi
}

updt_dpdk_submodule()
{
	echo "Download DPDK submodule"
	file_name=".agree"
	if [ ! -e "$file_name" ]; then
		echo "Please choose option '2. Agree to download' first"
		return
	fi
	git submodule init
	git submodule update
}

install_dpdk()
{
	echo "Build DPDK"
	export RTE_TARGET=x86_64-native-linuxapp-gcc
	cp -f dpdk-16.04_common_linuxapp "$DPDK_DIR"/config/common_linuxapp

	pushd "$DPDK_DIR"
	make -j 20 install T="$RTE_TARGET"
	if [ $? -ne 0 ] ; then
		echo "Failed to build dpdk, please check the errors."
		return
	fi
	sudo modinfo igb_uio
	if [ $? -ne 0 ] ; then
		sudo $MODPROBE -v uio
		sudo $INSMOD "$RTE_TARGET"/kmod/igb_uio.ko
		sudo cp -f "$RTE_TARGET"/kmod/igb_uio.ko /lib/modules/"$(uname -r)"
		echo "uio" | sudo tee -a /etc/modules
		echo "igb_uio" | sudo tee -a /etc/modules
		sudo depmod
	fi
	popd
}

step_3()
{
        TITLE="Build IL_TRAFFICGEN"
        CONFIG_NUM=1
        TEXT[1]="Build IL_TRAFFICGEN"
        FUNC[1]="build_il_tgen"
}
build_il_tgen()
{
	source setenv.sh
	pushd $IL_TGEN_DIR
	echo "IL_TGEN_DIR= $IL_TGEN_DIR"
	echo -e "RTE_SDK=$RTE_SDK\nRTE_TARGET=$RTE_TARGET"
	make clean; make
	popd
}

SETUP_PROXY="setup_http_proxy"
STEPS[1]="step_1"
STEPS[2]="step_2"
STEPS[3]="step_3"

QUIT=0

clear

echo -n "Checking for user permission.. "
sudo -n true
if [ $? -ne 0 ]; then
   echo "Password-less sudo user must run this script" 1>&2
   exit 1
fi
echo "Done"
clear

while [ "$QUIT" == "0" ]; do
        OPTION_NUM=1
        for s in $(seq ${#STEPS[@]}) ; do
                ${STEPS[s]}

                echo "----------------------------------------------------------"
                echo " Step $s: ${TITLE}"
                echo "----------------------------------------------------------"

                for i in $(seq ${#TEXT[@]}) ; do
                        echo "[$OPTION_NUM] ${TEXT[i]}"
                        OPTIONS[$OPTION_NUM]=${FUNC[i]}
                        let "OPTION_NUM+=1"
                done

                # Clear TEXT and FUNC arrays before next step
                unset TEXT
                unset FUNC

                echo ""
        done

        echo "[$OPTION_NUM] Exit Script"
        OPTIONS[$OPTION_NUM]="quit"
        echo ""
        echo -n "Option: "
        read our_entry
        echo ""
        ${OPTIONS[our_entry]} ${our_entry}

        if [ "$QUIT" == "0" ] ; then
                echo
                echo -n "Press enter to continue ..."; read
                clear
                continue
                exit
        fi
        echo "Installation complete. Please refer to README.MD for more information"
done

