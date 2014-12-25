#!/usr/bin/env perl
use strict;

my @logs = <obj-ia32/*_p1M.log>;
my @benches = ();
my $count = 33;

open(OUT, ">histo.csv");

foreach my $log (@logs){
    $log =~ /([0-9a-z]+)_p1M.log/;
    my $bench = $1;
    my $total = 0;
    my %values = ();

    open(IN, $log);

    while(<IN>){	
	if (/([a-z]+)\s+([0-9A-Z]+)\s+([0-9A-Z]+)/){
	    my $value = $3;
	    my $min = 0;
	    my $max = 0;
	    my $i = 1;
	    my @vals = split("", $value);
	    my $tag;

	    # calculate the taint tag
	    foreach (@vals){
		if ($_ eq '1'){
		    $max = $i;
		}
		$i++;
	    }
	    $i = 0;
	    foreach (reverse @vals){
		if ($_ eq '1'){
		    $min = int(@vals) - $i;
		}
		$i++;
	    }
	    
	    $tag = sprintf("%08X%08X", $min, $max);
	    #print "val=$value, tag=$tag\n";
	    
	    $values{$tag}++;
	    $total++;
	}
    }
    
    close(IN, $log);

    my $i=0;
    print "${bench},\n";
    print OUT "${bench},";
    foreach my $value (sort {$values{$b} <=> $values{$a} } keys %values){
	printf("$value, %1.8f\n", ${values{$value}}/$total);
	printf(OUT "%1.8f,", ${values{$value}}/$total);
	if ($i>$count){
	    last;
	}
	$i++;
    }
    print OUT "\n";
}

close(OUT);

