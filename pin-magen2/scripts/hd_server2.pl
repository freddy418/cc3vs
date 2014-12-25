#!/usr/bin/perl

use strict;

my $numargs = $#ARGV + 1;

if ($numargs != 2){
    print "Usage: hd_sever.pl pipe1 name.memtrace";
    exit;
}

my $pipe1 = $ARGV[0];
my $outfile = $ARGV[1];

if (-p $pipe1){    unlink $pipe1; }

system('mknod', $pipe1, 'p') && die "can't mknod $pipe1: $!";

open(PIPE1, "${pipe1}");

#open(OUT, ">".$outfile);

while(1){
    my $line1;
    while(!(defined $line1)){
	$line1 = <PIPE1>;
    }

    chomp($line1);

    #print "Read: ${line1} ***and*** ${line2}\n";
    
    if ($line1 =~ /End Program/){
	last;
    }
    elsif ($line1 =~ /Period\s([0-9]+)/){
	#start of a new period
    }
    elsif ($line1 =~ /[A-Za-z]+\s+[0-9A-Za-z]+\s+[0-9A-Za-z]+/){
	my @t1 = split(" ", $line1);
	my $type1 = $t1[0];
	my $addr1 = $t1[1];
	my $val1 = $t1[2];
	#print "$val1 *****and***** $val2\n";
	
	print "${type1}, ${addr1}, ${val1}\n";
    }
    else{
	next;
	#print "Unrecognized: ${line1}, ${line2}\n";
	#exit;
    }
}

close(PIPE1);
unlink $pipe1;

#close(OUT);
