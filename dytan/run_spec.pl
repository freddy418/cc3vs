#!/usr/bin/perl

use strict;

my $procs = 6;
my @children;

my $period = 1000000;
my $pname = "1M";
my @benchmarks = qw(gzip vpr gcc mcf parser mesa);
#gzip vpr gcc mcf parser perlbmk bzip2 twolf mesa art equake);

my @tasks = ("./gzip.1 ginput.program 60",
             "./gzip.2 ginput.program 60",
             "./vpr.1 net.in arch.in place.out dum.out -nodisp -place_only -init_t 5 -exit_t 0.005 -alpha_t 0.9412 -inner_num 2",
             "./vpr.2 net.in arch.in place.out dum.out -nodisp -place_only -init_t 5 -exit_t 0.005 -alpha_t 0.9412 -inner_num 2",
             "./cc1.1 200.i -o 200.s",
             "./cc1.2 200.i -o 200.s",
             "./mcf.1 mcf.in",
             "./mcf.2 mcf.in",
             "./parser.1 2.1.dict -batch < pref.in",
             "./parser.2 2.1.dict -batch < pref.in",
             "./perlbmk.1 -I./lib splitmail.pl 850 5 19 18 1500 > 850.5.19.18.1500.out",
             "./perlbmk.2 -I./lib splitmail.pl 850 5 19 18 1500 > 850.5.19.18.1500.out",
             "./bzip.1 binput.program 58",
             "./bzip.2 binput.program 58",
             "./twolf.1 ref",
             "./twolf.2 ref",
             "./mesa.1 -frames 1000 -meshfile mesa.in -ppmfile mesa.ppm",
             "./mesa.2 -frames 1000 -meshfile mesa.in -ppmfile mesa.ppm",
             "./art.1 -scanfile c756hel.in -trainfile1 a10.img -trainfile2 hc.img -stride 2 -startx 110 -starty 200 -endx 160 -endy 240 -objects 10",
             "./art.2 -scanfile c756hel.in -trainfile1 a10.img -trainfile2 hc.img -stride 2 -startx 110 -starty 200 -endx 160 -endy 240 -objects 10",
             "./equake.1 < equake.in",
             "./equake.2 < equake.in");

my @esimpts = qw(486 71 199 316 16 421 9 31 89 67 194);

print "#!/bin/bash\n\n";

# step0: run the benchmarks
# print "Running jobs\n";
for(my $i=0; $i<@benchmarks; $i++){
    my $pid = fork();

    if($pid){
	push(@children, $pid);
    }
    elsif($pid == 0){
	my $t1 = $i * 2;
	my $t2 = $t1 + 1;
	# 2 program version - find pointers
	#print("${mdir}/hd_server.pl /tmp/pipe${t1} /tmp/pipe${t2} > ${mdir}/obj-ia32/${benchmarks[$i]}_${pname}.log &\n");
	#print("/home/dyd2/pin29/pin -t ${mdir}/obj-ia32/debugtrace -p ${period} -o ${benchmarks[$i]}1.dat -f /tmp/pipe${t1} -start_address main -- ${tasks[$t1]} &\n");
	#print("/home/dyd2/pin29/pin -t ${mdir}/obj-ia32/debugtrace -p ${period} -o ${benchmarks[$i]}2.dat -f /tmp/pipe${t2} -start_address main -- ${tasks[$t2]} &\n");
	# 1 program version - track array pointers
	open(OUT, ">".$benchmarks[$i].".csh");
	print OUT "${mdir}/hd_server.pl /tmp/pipe5${t1} ${mdir}/obj-ia32/${benchmarks[$i]}.memtrace &\n";
	print OUT "sleep 5\n";
	print OUT "/home/dyd2/pin29/pin -t ${mdir}/obj-ia32/debugtrace -p ${period} -o ${benchmarks[$i]}1.dat -f /tmp/pipe5${t1} -s ${esimpts[$i]} -q 100000000 -start_address main -- ${tasks[$t1]} &\n";
	close(OUT);
	exit;
    }

    if (@children == $procs || ($i == (int(@benchmarks) - 1))){
	foreach (@children){
	    waitpid($_, 0);
	}
	@children = ();
    }
}

print "wait\n";
exit;

print "Step1: convert all the files from byte maps to word maps\n";

my @bfiles = <*_map[!w]*.out>;

foreach my $file (@bfiles){
    my $bench;
    my $phase;

    $file =~ /([A-Za-z0-9]+)_map([0-9]+).out/;
    $bench = $1;
    $phase = $2;

    print "Converting ${file}\n";

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

    # delete the byte file
    unlink($file);
}

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
