#!/usr/bin/perl
use strict;

#if ($#ARGV < 0){
    #print ($#ARGV)."arguments found\n";
#    die "usage: analyze_td bench\n";
#}

#my $pgm = $ARGV[0];

my $val1;
my $val2;
my $addr;
my @benchmarks = qw(gzip vpr gcc mcf parser perlbmk bzip2 twolf mesa art equake);

foreach my $bench (@benchmarks){

    open(OUT, ">${bench}_histo.csv") || die;
    my $tdiffs = 0;
    my $frc = 0;
    my $tmpaddr = 0;
    my $count = 0;
    my $total = 0;

    # counts capped at 200
    my @counts = qw(0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0);
    
    open(INFILE1, "${bench}_out1.out") || print "${bench}_map1.out not found\n";
    open(INFILE2, "${bench}_out2.out") || print "${bench}_map2.out not found\n";
    open(INFILE3, "${bench}_addr.out") || print "${bench}_addr.out not found\n";
    
    while(<INFILE1>){	
	chomp($val1 = $_);
	chomp($val2 = <INFILE2>);
	chomp($addr = <INFILE3>);
	$addr = hex($addr);

	#print $addr."\n";

	if ($val1 ne $val2){
	    $tdiffs++;	    
	}

	if (($val1 ne $val2) && (($addr-$tmpaddr) == 4)){
	    $count++;
	}else{
	    if($count < 199){
		if ($count > 0){
		    $counts[$count]++;		
		    $total++;
		}
	    }else{
		$total++;
		$counts[199]++;
	    }
	    $count = 0;
	}

	$tmpaddr = $addr;
	$frc++;
    }

    close(INFILE1);
    close(INFILE2);
    close(INFILE3);

    print "${bench}, ${frc}, ${tdiffs}, ${total}\n";

    #for(my $i=0;$i<@counts;$i++){
    print OUT "${bench},".(join(",", @counts))."\n";
    #}

    close(OUT);

}

