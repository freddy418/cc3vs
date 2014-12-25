#!/usr/bin/perl

use strict;

my $procs = 4;
my @children;
my $inphase = 0;
my @nums = qw(1 2);

my @benchmarks = qw(gzip vpr gcc mcf parser perlbmk bzip2 twolf mesa art equake);

print "\n** Beginning analysis of output files **\n\n";
print "Step1: convert all the files from byte maps to word maps\n";

######## start rewrite ########
foreach my $num (@nums){
    foreach my $bench (@benchmarks){
	
	print "Converting ${bench}.log\n";
	
	open(DIN, "${bench}${num}.log") || print "${bench}${num}.log not found\n";   
	
	my $phase;
	my $words = 0;
	my $laddr;
	my $val = 0;
	
	while (<DIN>){
	    if(/Period\s([0-9]+)/){ # start of a new period
		$phase = $1;
		open(DOUT, ">${bench}1_mapw${phase}.out") || print "${bench}1_mapw${phase}.out can't be created\n";
		$inphase = 1;
	    }
	    elsif(/Period\sEnd/){ # end of a period
		close(DOUT);
		$inphase = 0;
	    }
	    elsif(/([abcdef0123456789]+)\s+([0-9]+)/ && ($inphase == 1)){ 	    #if line contains a read
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
	    # else ignore everything else
	}
	
	close(DIN);
	
	# delete the byte file
	#unlink($file);
    }
}
######## stop rewrite ########

exit;

print "step2: comb each word map for differences\n";

foreach my $bench (@benchmarks){
    my @files1 = <${bench}1_mapw*.out>;
    my @files2 = <${bench}2_mapw*.out>;

    open(OUT, ">".$bench."_diffs.csv");
    print OUT "Phase, Differences,\n";

    if(int(@files1) == int(@files2)){
	print "\n** Sanity check #1: the number of output files for the benchmark match\n\n";
	print "** The number of output files match, continuing...\n\n";
    }

    for(my $i=0;$i<@files1;$i++){
	printf "Parsing ${bench}1_mapw%d.out and ${bench}2_mapw%d.out\n", $i, $i;
	open(IN1, $files1[$i]);
	open(IN2, $files2[$i]);
	
	my $count1 = `wc -l < $files1[$i]`;
	my $count2 = `wc -l < $files1[$i]`;
	if($count1 != $count2){
	    printf "** Sanity check failed: the number of lines for file1: %d, and file2: %d\n\n", $count1, $count2;
	}

	my $diffs = 0;
	
	while(<IN1>){
	    my $addr; my $val1; my $val2; my $line2;
	    
	    # first file
	    /[abcdef0123456789]+\s+([0-9]+)/;
	    $val1 = $1;

	    $line2 = <IN2>;
	    $line2 =~ /[abcdef0123456789]+\s+([0-9]+)/;
	    $val2 = $1;
	    
	    if ($val1 != $val2){
		$diffs++;
	    }
	}

	close(IN1);
	close(IN2);
	
	printf OUT "%d, %d,\n", $i, $diffs;
    }

    close(OUT);
}

print "step3: comb each word map for spatial locality differences\n";

foreach my $bench (@benchmarks){
    my @files1 = <${bench}1_mapw*.out>;
    my @files2 = <${bench}2_mapw*.out>;

    open(OUT, ">${bench}_histo.csv") || die;

    for(my $i=0;$i<@files1;$i++){
    
	my $tdiffs = 0;
	my $frc = 0;
	my $tmpaddr = 0;
	my $count = 0;
	my $total = 0;
	
	# counts capped at 200
	my @counts = qw(0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0);
    
	open(INFILE1, "${files1[$i]}") || print "${files1[$i]} not found\n";
	open(INFILE2, "${files2[$i]}") || print "${files2[$i]} not found\n";
    
	while(<INFILE1>){   
	    my $addr; my $val1; my $val2; my $line2;
	    
	    # first file
	    /([abcdef0123456789]+)\s+([0-9]+)/;
	    $addr = $1;
	    $val1 = $2;

	    $line2 = <INFILE2>;
	    $line2 =~ /([abcdef0123456789]+)\s+([0-9]+)/;
	    $val2 = $2;

	    $addr = hex($addr);	   
	    
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
	
	#print "${bench}, ${frc}, ${tdiffs}, ${total}\n";
	print OUT "P${i},".(join(",", @counts))."\n";

	#unlink($files1[$i]);
	#unlink($files2[$i]);
    }
    close(OUT);
}

print "step 4: Clean up temporary files\n";

my @files = <*_mapw*.out>;

foreach my $file (@files){
    unlink($file);
}

print "\nDone...\n";
