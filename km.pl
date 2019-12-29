#!/usr/bin/perl
#
# This script can be used to retrieve the odometer wheel
# turns and convert them into kilometers/miles the bike
# has traveled.
#
# Edit the default values for the serial port and front
# wheel size you want to use.
#

use strict;
use warnings;
use Math::Trig;
use Getopt::Std;
use Device::SerialPort;

my %args = ();

# command line arguments
getopts("hmp:d:", \%args);

# default values
$args{'d'} = 26 unless defined $args{'d'};
$args{'p'} = '/dev/tty.usbserial-A403JXK2' unless defined $args{'p'};

# print help
if(defined $args{'h'} or not $args{'d'} =~ /^\d+\.?\d*$/) {
    printf("-h             print help\n");
    printf("-m             output distance in miles (default kilometers)\n");
    printf("-p <port>      serial device port to use\n");
    printf("-d <diameter>  bicycle front wheel diameter in inches\n");
    exit 0;
}

# use SerialPort to configure
my $dev = Device::SerialPort->new($args{'p'});
$dev or die "can't open $args{'p'}\n";

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

# calculate distance
if($args{'m'}) {
    my $circumference = $args{'d'} * pi;
    my $distance = ($rotations * $circumference) / 63360;
    printf("%.1f mi\n", $distance);
} else {
    my $circumference = $args{'d'} * 2.54 / 100 * pi;
    my $distance = ($rotations * $circumference) / 1000;
    printf("%.1f km\n", $distance);
}

# finish
close FH;
$dev->close();

exit 0;

