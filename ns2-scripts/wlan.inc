# return ßensuremathﬂa^x∞
proc pow {a x} {
    return [expr exp($x*log($a))]
}
# return absolute power specified in dBm notation 
proc dbm {x} {
    return [expr [pow 10.0 [expr $x/10.0]] * 0.001]
}   

# antenna model: omni-directional antenna 
set val(ant)            Antenna/OmniAntenna
# physical network type
set val(netif)          Phy/WirelessPhy
# wireless propagation model: shadowing
set val(prop)           Propagation/Shadowing
# physical channel separation: wireless
set val(chan)           Channel/WirelessChannel
# MAC: 802.11
set val(mac)            Mac/802_11
# device interface: generic link layer; drop tail queue, length 10
set val(ll)             LL
set val(ifq)            Queue/DropTail/PriQueue
set val(ifqlen)         10
# routing layer: no build-in routing, use own
set val(rp)             DumbAgent

# propagation model parameters
# path loss exponent: 3.6 (open urban area)
Propagation/Shadowing set pathlossExp_ 3.6
# standard signal distribution (4dB - medium fluctuations)
Propagation/Shadowing set std_db_      4	
# reference distance for transmitter power (1m)
Propagation/Shadowing set dist0_       1.0
Propagation/Shadowing set seed_        0

# WLAN device setup
# transmitter power: 25dBm
Phy/WirelessPhy set Pt_ [dbm 25]
# transmitter frequency: 2.472 GHz (channel 13) 
Phy/WirelessPhy set freq_ 2.472e9
# signal bandwidth: 11 Mbit/s
Phy/WirelessPhy set bandwidth_ 11e6
# carrier sense treshold
Phy/WirelessPhy set CSThresh_ [dbm -104]
# receive threshold 
Phy/WirelessPhy set RXThresh_ [dbm -94]
# minimum sinal-to-noise ratio for error free reception: 10dB
Phy/WirelessPhy set CPThresh_ 10.0
# path loss model parameter
Phy/WirelessPhy set L_ 1.0 
# debugging flag
Phy/WirelessPhy set debug_ false

# 802.11 MAC setup
# RTS/CTS threshold: 250 byte 
Mac/802_11 set RTSThreshold_ 250
# data rate (used from unicast): 11 Mbit/s
Mac/802_11 set dataRate_ 11.0e6
# basic rate (used for broadcast): 2 Mbit/s
Mac/802_11 set basicRate_ 2.0e6
# PLCP (Physical Layer Convergence Procedure)
# used for preamble 1 Mbit/s
Mac/802_11 set PLCPDataRate_  1.0e6
# number of retries for short (management) frames
Mac/802_11 set ShortRetryLimit_       7
# number of retries for data frames
Mac/802_11 set LongRetryLimit_        4
