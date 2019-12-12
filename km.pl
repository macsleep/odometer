#!/usr/bin/perl -w

#
# 2.11.2019 Jan Schlieper
#
# This script can be used to retrieve the odometer wheel
# turns and convert them into kilometers the bike traveled.
#
# Edit the port variable to the serial port you want to use.
# Adjust wheel_circumference to the wheel on your bike. Mine
# is a 26" wheel (55 559).
#

use strict;
use Math::Trig;
use Device::SerialPort;

my $port = '/dev/tty.usbserial-A403JXK2';
my $wheel_circumference = 26*2.54/100*pi;

# use SerialPort to configure
my $dev = Device::SerialPort->new($port);
$dev or die "can't open $port\n";

# config
$dev->baudrate(9600);
$dev->databits(8);
$dev->parity("none");
$dev->stopbits(1);
$dev->handshake("none");

# flush
$dev->purge_rx();

# get filehandle
open FH, '+<', $dev->{'NAME'};

# send print
print FH "p\r";

# dump echo
<FH>;

# get reply
my $rotations = <FH>;
chomp $rotations;

# calculate kilometers
my $km = ($rotations * $wheel_circumference)/1000;
printf("%.1f km\n", $km);

# finish
close FH;
$dev->close();

exit 0;

