#!/usr/bin/env perl
use strict;

if ($#ARGV < 0){
    #print ($#ARGV)."arguments found\n";
    die "usage: spatial_locality.pl blocksize\n";
}

my $bsize = $ARGV[0];
my @logs = <obj-ia32/*_p1M.log>;
my @benches = ();

open(OUT, ">bc_sparsity_b${bsize}.csv");

foreach my $log (@logs){
    $log =~ /([0-9a-z]+)_p1M.log/;
    my $bench = $1;
    my %umem = ();
    my %nmem = ();
    my $accs = 0;

    open(IN, $log);

    while(<IN>){
	if (/([a-z]+)\s+([0-9A-Z]+)\s+([0-9A-Z]+)/){
	    my $acctype = $1;
	    my $addr = hex($2) << 1;
	    my $value = $3;

	    $addr = $addr & ($bsize - 1);

	    if (hex($value) == 0){
		$umem{$addr} = 1;
	    }else{
		$umem{$addr} = 1;
		$nmem{$addr} = 1;
	    }
	    $accs++;
	}
    }
    
    close(IN, $log);
    printf(OUT "${bench}, %d, %d, %d, %1.8f\n", $accs, (keys %nmem), (keys %umem), (keys %nmem) / (keys %umem));
}

close(OUT);
