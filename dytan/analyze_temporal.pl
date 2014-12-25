#!/usr/bin/env perl
use strict;

my @logs = <obj-ia32/*_p1M.log>;
my @benches = ();
my $periods = 0;
my @data_tb;

foreach my $log (@logs){
    $log =~ /([0-9a-z]+)_p1M.log/;
    push(@benches, $1);
    open(IN, $log);

    my %taints = ();
    my @data = ();

    while(<IN>){
	if (/([01]+)/){
	    $taints{$1} = 1;
	}
	elsif (/Period ([0-9]+)/){
	    my $cp = $1;
	    if ($cp > $periods){
		$periods = $cp;
	    }
	    %taints = ();
	}
	elsif (/End Period/){
	    push(@data, scalar(keys %taints));
	}
    }
    
    close(IN, $log);
    #print join(",", @data)."\n";
    push(@data_tb, \@data);
}

#print "\n\nTemporal Data:\n\n";
print ",".join(",", @benches)."\n";
for (my $i=0;$i<$periods;$i++){
    print "${i},";
    for (my $j=0;$j<@benches;$j++){
	if (defined $data_tb[$j][$i]){
	    print $data_tb[$j][$i].",";
	}else{
	    print "0,";
	}
    }
    print "\n";
}
