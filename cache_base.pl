#!/usr/bin/perl

use strict;
use Getopt::Long;

my $hits = 0;
my $misses = 0;
my $sets; # number of sets
my $ways; # number of associative ways
my $bsize; # block size (number of bytes per block)
my $infile;
my $numargs = $#ARGV + 1;
my $help;
my $hits = 0;
my $accs = 0;
# actual cache
my @tags = ();
my @valids = ();
my $imask;
my $ishift;
my $bshift;
my @lru = (); # 2D array for LRU replacement

sub usage
{
    print "Unknown option: @_\n" if ( @_ );
    print "usage: program [--f FILE.memtrace] [--w (ways)] [--s (set)] [--bs block size (bytes)] [--help|-?]\n";
    exit;
}

sub update_lru {
    my ($lru_ref, $usedway) = @_;

    for(my $i=0; $i<$ways;$i++){
        if ($$lru_ref[$i] == $usedway){
            splice(@$lru_ref, $i, 1);
            push(@$lru_ref, $usedway);
            last;
        }
    }
}

sub acc {
    my ($addr, $type) = @_;
    my $index = ($addr >> $bshift) & $imask;
    my $tag = ($addr >> ($bshift + $ishift));
    my $hit = 0;
    my $hitway = 0;
    my $os = $index * $ways;

    # check tags
    for (my $i=0;$i<$ways;$i++){
	if ($tags[$os+$i] == $tag && $valids[$os+$i] == 1){
	    $hit = 1;
	    $hitway = $i;
	}
    }

    # update bookkeeping
    if ($hit == 1){
	#printf "hit on ${index}, %08X, %08X\n", $addr, $tag;
	$hits++;
    }else{
	# replace the LRU way
	$hitway = $lru[$index][0];
    #printf("miss to index: ${index} on tag: %x, replaced ${hitway}\n", $tag);
	$tags[$os+$hitway] = $tag;
	$valids[$os+$hitway] = 1;

	#printf "miss on ${index}, %08X, %08X, replaced ${hitway}\n", $addr, $tag;
    }
    update_lru($lru[$index], $hitway);
    $accs++;
}

usage() if (!GetOptions('help|?' => \$help,
                        'f=s' => \$infile,
			's=i' => \$sets,
			'w=i' => \$ways,
			'bs=i' => \$bsize)
            or $numargs < 1 or defined $help );

$imask = $sets - 1;
$ishift = log($sets) / log(2);
$bshift = log($bsize) / log(2);

#my $tsize = $entries << $bshift;

#printf("Simulating a %f KB cache\n", $tsize/1024);
#print "Index bits: ${ishift}, Block offset: ${bshift}\n";

for (my $i=0;$i<$sets;$i++){
    my $os = $i * $ways;
    my @ll = ();
    for (my $j=0;$j<$ways;$j++){
	push(@ll, $j);
	$tags[$os + $j] = 0;
	$valids[$os + $j] = 0;
    }
    push(@lru, \@ll);
    #print join(",", @ll);
}

#print "Opening ${infile}\n";

open(IN, $infile);

while(<IN>){
    if (/([a-z]+)\s+([A-Z0-9]+)\s+([0-9A-Z]+)/){
	my ($t1, $t2, $t3) = ($1, $2, $3);       
	#print "Access ".$t1." to ".$t2.", value is ".$t3."\n";
	#if (hex($t2) % 4 > 0){
	#    die "Not word aligned!!\n";
	#}
	my $addr = hex($t2); #<<3;
	#print "Accessing ".$addr.", value is ".$t3."\n";
	
	if ($t1 =~ /read/){
	    acc($addr, 0); # only need address for now?
	}elsif($t1 =~ /write/){
	    acc($addr, 1); # only need address for now?
	}#else trouble
    }
    else{
	#ignore
    }

    #if (($accs % 500000 == 0) && ($accs / 500000 > 0)){
	#print "**INFO** : Analyzed ${accs} accesses\n";
    #}
}

close(IN);

printf("${infile} Acceses: %d\n", $accs);
printf("${infile} Misses: %d\n", ($accs-$hits));
printf("${infile} Hits: %d\n", $hits);
printf("${infile} Miss rate: %f\n", ($accs-$hits)/($accs));
