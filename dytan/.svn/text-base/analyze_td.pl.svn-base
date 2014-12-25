#!/usr/bin/env perl
use strict;

my @benchmarks = qw(gzip vpr gcc mcf parser perlbmk bzip2 twolf mesa art equake);

# counts capped at 200
my @counts = qw(0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0);

foreach (@benchmarks) {
    open(INFILE, $_.".dat") || print "${_}.dat not found\n";

    my $tmptaint;
    my $tmpaddr;
    my $count = 0;
    while(<INFILE>){
	if(/([0-9]+) ([01]+)/){
	    my $addr = $1;
	    my $taint = $2;
	    
	    if(($addr-$tmpaddr)==1){
		$count++;
	    }else{
		if($count < 199){
		    $counts[$count]++;
		}else{
		    $counts[199]++;
		}
		$count = 0;
	    }
	    
	    $tmpaddr = $addr;
	    $tmptaint = $taint;
	}
	else{
	    # ignored
	}
    }
    close(INFILE);
}

open(OUT, ">histo.csv");

for(my $i=0;$i<@counts;$i++){
    print OUT "$i, ${counts[$i]},\n";
}

close(OUT);
