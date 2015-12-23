#
#              10Mb,10ms   
#           r1 --------- r2
#

set ns [new Simulator]

#Define different colors for data flows
$ns color 1 Red


#Open the nam trace file
set nf [open out.nam w]
set tf [open out.tr w]
$ns namtrace-all $nf
$ns trace-all $tf

#Define a 'finish' procedure
proc finish {} {
        global ns nf tf
        $ns flush-trace
        #Close the trace file
        close $nf
        close $tf
        #Execute nam on the trace file
        exec nam out.nam &
        exit 0
}

set node_(r1) [$ns node]
set node_(r2) [$ns node]

$ns duplex-link $node_(r1) $node_(r2) 10Mb 10ms RED 

#Setup RED queue parameter
$ns queue-limit $node_(r1) $node_(r2) 100
Queue/RED set thresh_ 5
Queue/RED set maxthresh_ 10
Queue/RED set q_weight_ 0.002
Queue/RED set ave_ 0

$ns duplex-link-op $node_(r1) $node_(r2) queuePos 0.5
$ns duplex-link-op $node_(r1) $node_(r2) orient right

#Setup a MM UDP connection
set udp_s [new Agent/UDP/UDPvm]
set udp_r [new Agent/UDP/UDPvm]
$ns attach-agent $node_(r1) $udp_s
$ns attach-agent $node_(r2) $udp_r
$ns connect $udp_s $udp_r
$udp_s set packetSize_ 1000
$udp_r set packetSize_ 1000
$udp_s set fid_ 1
$udp_r set fid_ 1

#Setup a VM Application
set mmapp_s [new Application/VmApp]
set mmapp_r [new Application/VmApp]
$mmapp_s attach-agent $udp_s
$mmapp_r attach-agent $udp_r
$mmapp_s set frame_rate_ 30
$mmapp_s input-rate /home/saba/Desktop/SharedFolder/ns2-videosrc/samplefiles/m0-kLIMuGHbQ-vide
$mmapp_s input-target /home/saba/Desktop/SharedFolder/target-rate.txt

#Simulation Scenario

$ns at 1.0 "$mmapp_s start"
$ns at 100.0 "$mmapp_s stop"

$ns at 35.0 "finish"

$ns run


