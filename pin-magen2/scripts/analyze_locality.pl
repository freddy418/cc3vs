#!/usr/bin/env perl
use strict;

my @logs = <obj-ia32/*_p1M.log>;
my @benches = ();
my $count = 33;

open(OUT, ">bc_histo.csv");

foreach my $log (@logs){
    $log =~ /([0-9a-z]+)_p1M.log/;
    my $bench = $1;
    my $total = 0;
    my %values = ();

    open(IN, $log);

    while(<IN>){
	if (/([a-z]+)\s+([0-9A-Z]+)\s+([0-9A-Z]+)/){
	    if (hex($3) == 0){
		$values{"0"}++;
	    }else{
		$values{$3}++;
	    }
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
