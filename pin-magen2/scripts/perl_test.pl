#!/usr/bin/env perl
use strict;

my @array = qw(gzip gcc crafty perlbmk vortex);

for(my $i=0;$i<10;$i+=2){
    print "${array[$i>>1]}\n";
}
