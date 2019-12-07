
il_trafficgen setup is used to measure the sustainable frame rate for a specified number of flows, PPS and packet size.
il_trafficgen is used to transmit packets to data-plane which inturn does a packet forwarding to il_trafficgen responder/generator.

il_trafficgen test setup
============================================

                     -------------    s1u    -----------                     Aim: For a specified number of flows, PPS and pkt size,
                    |    P0-Rx    |----<----|           |<-- -                    compute Uplink(UL) and Downlink(DL) loss
                    |    P0-Tx    |---->----| P0        |---> |
                    |             |         |           |   | |                   UL loss = P0-Tx - P1-Rx
                    |             |         |           |   | |                   DL loss = P1-Tx - P0-Rx
                    |il_trafficgen|         |data-plane |   v ^
                    |             |         |           |   | |
                    |    P1-Rx    |----<----|           |<--  |
                    |    P1-Tx    |---->----| P1        |----->
                    |             |   sgi   |           |
                     -------------           -----------


il_trafficgen directory structure
==============================================
1. il_trafficgen: Download "il_trafficgen" under /opt directory from git clone https://<user-name>@ilpm.intel-research.net/bitbucket/scm/vccbbw/il_trafficgen.git
Sample: After "il_trafficgen" is downloaded, directories would be
          /opt/il_trafficgen/pktgen
          /opt/il_trafficgen/dpdk

Build il_trafficgen using script
=================================
 Inside "/opt/il_trafficgen" folder perform below steps to install il_trafficgen:
		- Execute : ./install.sh
		- Using install.sh script build pktgen and install it's dependencies.


Build Manualy
===============================
1. Build dpdk application as below:
  1.1 cd /opt/il_trafficgen/pktgen
  1.2 Execute command: "source setenv.sh"
  1.3 cd /opt/il_trafficgen/dpdk
  1.4 Execute: make config T=x86_64-native-linuxapp-gcc O=x86_64-native-linuxapp-gcc
  1.5 cd x86_64-native-linuxapp-gcc
  1.6 make -j 20

2. Build "pktgen" application by executing "make" command in /opt/il_trafficgen/pktgen

Configuration
===============================
1. Bind the DPDK ports that are to be used in il_trafficgen to forward or receive packets.
   Example:  To bind port 81:00.0 and 81:00.1, run
             /opt/il_trafficgen/dpdk/usertools/dpdk-devbind.py -b igb_uio 81:00.0 81:00.1
2. Configure /opt/il_trafficgen/pktgen/autotest/user_input.cfg

/opt/il_trafficgen/pktgen/autotest/user_input.cfg
--------------------------------------------------
Configure below parameters:

  test_duration --> Test duration (Packets would be forwarded from il_trafficgen to data-plane application for this set duration).
                      If test_duration is set to 0, the packets would be forwarded for infinite time.
                      User will have to manually execute stop commands to stop packet transmission
  flows         --> Number of flows to be tested in the test run
  ue_ip_range   --> Range of unique UE's IPs that needs to be used to create unique flows
                      Example: ue_ip_range = 16.0.0.1 to 16.255.255.100
                      NOTE: The range should be greater than or equal to the number of 'flows' field
  no_of_enb	--> Number of eNB that have to be used
					  Example: no_of_enb = 80
  enb_ip_range  --> Range of unique eNB IPs that needs to be used to create unique flows
                      Example: enb_ip_range = 11.7.1.101 to 11.7.1.255
                      NOTE: The range should be greater than or equal to the number of 'no_of_enb' field
  app_srvr_ip_range  --> Range of unique APP Server IPs that needs to be used to create unique flows
                      Example: app_srvr_ip_range = 13.7.1.110 to 13.7.1.255
                      NOTE: Current implementation considers only one dst IP for unique flows
  s1u_sgw_ip	--> IP address of s1u interface of data-plane
  gen_host_ip	--> il_trafficgen generator HOST IP address
  resp_host_ip	--> il_trafficgen responder HOST IP address
  s1u_port	--> DPDK port to be used by il_trafficgen generator(s1u)
  sgi_port	--> DPDK port to be used by il_trafficgen responder(sgi)
  p0_src_mac    --> Mac address of port 0 interface of il_trafficgen generator(s1u)
  p0_dst_mac    --> Mac address of port 0 interface of data-plane(s1u)
  p1_src_mac    --> Mac address of port 1 interface of il_trafficgen responder(sgi)
  p1_dst_mac    --> Mac address of port 1 interface of data-plane(sgi)
  pps           --> PPS(Packets Per Second) value to be used
  pkt_size      --> Packet size to be used
  protocol      --> Protocol to be used. Ex: tcp/udp
  proto_type    --> Protocol type to be used. Ex: ipv4
  core_list	--> Cores to be used by il_trafficgen application
  NUMA0/1_MEM   --> NUMA node to be used by il_trafficgen application


Execution
=========================
Note: Configure and Run data-plane application before start sending traffic by il_trafficgen generator.

SCREEN MODE
Run 'il_nperf.sh' located in /opt/il_trafficgen/pktgen

	1. Start il_trafficgen (as generator)in one terminal
	cd /opt/il_trafficgen/pktgen
	./il_nperf.sh –g
	
	wait for the il_trafficgen command prompt to appear on screen.

	2. Start il_trafficgen (as responder)in another terminal
	cd /opt/il_trafficgen/pktgen
	./il_nperf.sh –r
	
	Wait for the il_trafficgen command prompt to appear on screen.


	3. To send uplink traffic
	Execute "start 0" to generate Uplink traffic(i.e., P0-Tx --> P1-Rx) on GENERATOR prompt.
	>start 0

	4. To send downlink traffic
	Execute "start 0" to generate Downlink traffic(i.e., P0-Rx <-- P1-Tx) on RESPONDER prompt.
	>start 0

	5. To send traffic uplink and downlink run step 3 and 4 simultaneously.

il_trafficgen stops automatically, after test-duration.
If test-duration is set to 0, packet transmission happens indefinitely. So, use 'stp' command to stop packet tranmission
Logs for the current run will be stored in /opt/il_trafficgen/pktgen/autotest/log/il_trafficgen-data-plane_debug_<DD_MM_YYYY_HH_MM_S>.log

CLI MODE
Run 'il_nperf_cli.sh' located in /opt/il_trafficgen/pktgen

	1. Start il_trafficgen (as generator)in one terminal
	cd /opt/il_trafficgen/pktgen
	./il_nperf_cli.sh –g

	wait for the il_trafficgen command prompt to appear on screen.

	2. Start il_trafficgen (as responder)in another terminal
	cd /opt/il_trafficgen/pktgen
	./il_nperf_cli.sh –r

	Wait for the il_trafficgen command prompt to appear on screen.

	3. To send uplink traffic
	Send the following command to generate Uplink traffic(i.e., P0-Tx --> P1-Rx) on GENERATOR.
	$ socat - TCP4:localhost:2222 < start_il_trafficgen.lua

	4. To send downlink traffic
	Send the following command to generate Downlink traffic(i.e., P0-Rx <-- P1-Tx) on RESPONDER.
	$ socat - TCP4:localhost:3333 < start_il_trafficgen.lua

	5. To send traffic uplink and downlink run step 3 and 4 simultaneously.

	6. Send the following command to quit both GENERATOR and RESPONDER.
	$ socat - TCP4:localhost:2222 < quit_il_trafficgen.lua ; socat - TCP4:localhost:3333 < quit_il_trafficgen.lua

Uninstall il_trafficgen
=========================
  1. cd /opt/il_trafficgen/pktgen
  2. Execute command: "make clean"

Known Issues and Limitations
============================
  1. while switching from ng4t to il_trafficgen using switch_ng4t_ilnperf.sh script, traffic will not flow out of il_trafficgen's
     generator/responder. We need to restart both generator and responder to be able to send traffic successfully.

