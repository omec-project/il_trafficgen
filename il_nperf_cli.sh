#!/bin/bash
source autotest/user_input.cfg
IFS='-' read -r -a cores <<< $core_list
if [[ $1 == "-g" ]] || [[ $1 == "--generator" ]]; then
	#s1u_cores="$(( ${cores[0]}+1 )).0"
	s1u_cores="[$((${cores[0]}+1)):$((${cores[0]}+2))].0"
	cmd="./app/x86_64-native-linuxapp-gcc/pktgen -l $core_list -n 4 --proc-type auto  \
		--log-level 7 --socket-mem $NUMA0_MEM,$NUMA1_MEM --file-prefix s1u_pg -w $s1u_port -- -t -T     \
		-N -m $s1u_cores -f ./autotest/input.txt -f themes/black-yellow.theme -g 127.0.0.1:2222"
elif [[ $1 == "-r" ]] || [[ $1 == "--responder" ]]; then
	#sgi_cores="[$(( ${cores[0]}+5 ))-$(( ${cores[0]}+6)):$(( ${cores[0]}+7 ))-$(( ${cores[0]}+8))].0"
	sgi_cores="[$((${cores[0]}+3)):$((${cores[0]}+4))].0"
	cmd="./app/x86_64-native-linuxapp-gcc/pktgen -l $core_list -n 4 --proc-type auto  \
		--log-level 7 --socket-mem $NUMA0_MEM,$NUMA1_MEM --file-prefix sgi_pg -w $sgi_port -- -r -T     \
		-N -m $sgi_cores -f ./autotest/input.txt -f themes/black-yellow.theme -g 127.0.0.1:3333"
elif [[ $1 == "-rs" ]] || [[ $1 == "--responder_as_reflector" ]]; then
	sgi_cores="$(( ${cores[0]}+3 )).0"
	cmd="./app/x86_64-native-linuxapp-gcc/pktgen -l $core_list -n 4 --proc-type auto  \
		--log-level 7 --socket-mem $NUMA0_MEM,$NUMA1_MEM --file-prefix sgi_ref_pg -w $sgi_port -- -x -T     \
		-N -m $sgi_cores -f ./autotest/input.txt -f themes/black-yellow.theme"
else
	echo -e "Usage: ./il_nperf.sh <option>\n"
	echo "options:"
	echo "-g, --generator			Start il_nperf as generator"
	echo "-r, --responder			Start il_nperf as responder"
	echo "-rs, --responder_as_reflector	Start il_nperf with responder as reflector"
	exit 1;
fi

$PWD/autotest/generate_cmd_file $1
if [ $? -ne 0 ]; then
	echo "Error in setting up commands for il_nperf"
	exit 1;
fi

echo "$cmd"
#eval "$cmd"

USAGE="\nUsage:\trun.sh [ debug ]
	\n\tdebug:  executes pktgen under gdb\n"

GDB_EX="-ex 'set print pretty on' "

if [ -z "$2" ]; then
	$cmd
elif [ "$2" == "debug" ]; then
	echo -e "\n\t**** @app/Makefile, /autotest/Makefile enable CFLAGS += -O0 -g ****"
	echo -e "\t$GDB_EX\n"
	gdb $GDB_EX --args $cmd
else
	echo -e $USAGE
fi

