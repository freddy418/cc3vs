#!/usr/bin/perl
use strict;

#if ($#ARGV < 0){
    #print ($#ARGV)."arguments found\n";
#    die "usage: analyze_td bench\n";
#}

#my $pgm = $ARGV[0];
my $total = 0;

#print "$bench\n";

my $val1;
my $val2;
my @benchmarks = qw(gzip vpr gcc mcf parser perlbmk bzip2 twolf mesa art equake);
#($pgm);

foreach my $bench (@benchmarks){
    
    my $diffs = 0;
    my $frc = 0;
    
    open(INFILE1, "${bench}_out1.out") || print "${bench}_map1.out not found\n";
    open(INFILE2, "${bench}_out2.out") || print "${bench}_map2.out not found\n";
    
    while(<INFILE1>){	
	chomp($val1 = $_);
	chomp($val2 = <INFILE2>);
	if ($val1 ne $val2){
	    $diffs++;
	    print OUT $val1." | ".$val2."\n";
	}
	$frc++;
    }

    close(INFILE1);
    close(INFILE2);

    print "${bench}, ${frc}, ${diffs}\n";
}


