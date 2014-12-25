#!/usr/bin/perl
use strict;

if ($#ARGV+1 != 1 ) {
    print "usage: find_fvs.pl <num>\n";
    exit;
}

my @benches = qw(gzip gcc mcf parser vpr);
my $fcount = $ARGV[0];
my $tfiles = 30;
my $tracedir = "/research/scrap2/dyd2/pin212/source/tools/pin-magen2/obj-ia32";

foreach my $bench (@benches){
    open(OUT, ">${bench}_fvs.csv");
    print OUT "${bench}\n";
    my %occs = ();
    my $accs = 0;
    my $fnum = 0;
    for (my $i=0;$i<$tfiles;$i++){
	open(IN, "${tracedir}/${bench}${i}.log");
	while(<IN>){
	    if (/[read|write]\s+[0-9A-Za-z]+\s+([0-9A-Za-z]+)/){
		my $value = $1;
		$occs{$value}++;
	    }
	    $accs++;
	}
	close(IN);
    }
    # print the X most frequent values and percentage of accesses encountered
    foreach my $value (sort {$occs{$b} <=> $occs{$a} }  keys %occs){
	printf(OUT "$value, $occs{$value}, %1.3f\n", (($occs{$value})/$accs));
	$fnum++;
	if ($fnum > ($fcount - 1)){
	    last;
	}
    }
    close(OUT);
}
