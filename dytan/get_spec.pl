#!/usr/bin/perl

use strict;

my $specdir = "/ufs/lockbox/deng/benches/SPEC2006/CPU2006";
my @bdirs = qw(400.perlbench 401.bzip2 403.gcc 429.mcf 433.milc 445.gobmk 456.hmmer 458.sjeng 462.libquantum 464.h264ref 470.lbm);
my @bches = qw(perlbench bzip2 gcc mcf milc gobmk hmmer sjeng libquantum h264ref lbm);

chdir("obj-ia32");

# grab binaries
for (my $i=0;$i<@bches;$i++){
    system("cp ${specdir}/${bdirs[$i]}/src/${bches[$i]}.1 ${bches[$i]}");
}

# grab input sets
# perlbench
system("cp -r ${specdir}/400.perlbench/data/all/input/lib .");
system("cp -r ${specdir}/400.perlbench/data/all/input/diffmail.pl .");

# bzip2
system("cp -r ${specdir}/401.bzip2/data/ref/input/input.source .");

# gcc
system("cp ${specdir}/403.gcc/data/ref/input/200.in .");

# mcf
system("cp ${specdir}/429.mcf/data/ref/input/inp.in mcf.in");

# milc
system("cp ${specdir}/433.milc/data/ref/input/su3imp.in .");

# gobmk
system("cp -r ${specdir}/445.gobmk/data/all/input/games .");
system("cp ${specdir}/445.gobmk/data/ref/input/13x13.tst .");

# hmmer
system("cp ${specdir}/456.hmmer/data/ref/input/retro.hmm .");

# sjeng
system("cp ${specdir}/458.sjeng/data/ref/input/ref.txt .");

# h264ref
system("cp ${specdir}/464.h264ref/data/all/input/foreman_qcif.yuv .");
system("cp ${specdir}/464.h264ref/data/ref/input/foreman_ref_encoder_main.cfg .");
system("cp ${specdir}/464.h264ref/data/ref/input/sss.yuv .");

# lbm
system("cp ${specdir}/470.lbm/data/ref/input/lbm.in .");
system("cp ${specdir}/470.lbm/data/ref/input/100_100_130_ldc.of .");

# run commands
# ./perlbench -I./lib diffmail.pl 4 800 10 17 19 300
# ./bzip2 input.source 280
# ./gcc 200.in -o 200.s
# ./mcf mcf.in > mcf.ref.out
# ./gobmk --quiet --mode gtp < 13x13.tst

#system("cp ../Makefile .");
#system("make");
