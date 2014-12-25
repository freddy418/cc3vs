#!/usr/bin/perl
use strict;

if($#ARGV != 0){
    die("use: ./data_parse.pl <infile> <outfile>");
}

my $file = $ARGV[0];
my $bench;
my $phase;

$file =~ /([A-Za-z0-9]+)_map([0-9]+).out/;
$bench = $1;
$phase = $2;

print "reading ${file}\n";

open(DIN, $file) || print "${file} not found\n";   
open(DOUT, ">${bench}_mapw${phase}.out") || print "${bench}_mapw${phase}.out can't be created\n";

my $words = 0;
my $laddr;
my $val = 0;

while (<DIN>){
    if(/([abcdef0123456789]+)\s+([0-9]+)/){ 	    #if line contains a read
	my ($addr, $value) = ($1, $2);
	
	if ($words == 0){
	    $laddr = $addr;
	    $val = $value << 24;
	}else{
	    if ((hex($addr) - hex($laddr)) == $words){
		$val += $value << ((3-$words)<<3);
	    }else{
		print "${addr} observed for laddr: ${laddr} + ${words}\n";
		printf DOUT "0x%s\t%u\n", $laddr, $val;
		$laddr = $addr;
		$words = 0;
		$val = $value << 24;
	    }
	}
	
	$words++;
	if($words > 3){
	    printf DOUT "0x%s\t%u\n", $laddr, $val;
	    $words = 0;
	    $val = 0;
	}
    }
}

close(DIN);
close(DOUT);
