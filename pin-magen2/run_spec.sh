#!/bin/sh

../../../../pin -t debugtrace -s 3750 -q 250 -o perlbench -- ./perlbench -I./lib diffmail.pl 4 800 10 17 19 300 > /dev/null &
../../../../pin -t debugtrace -s 69250 -q 250 -o bzip2 -- ./bzip2 input.source 280 > /dev/null &
../../../../pin -t debugtrace -s 31750 -q 250 -o gcc -- ./gcc 200.in -o 200.s > /dev/null &
../../../../pin -t debugtrace -s 71500 -q 250 -o mcf -- ./mcf mcf.in > /dev/null &
wait
../../../../pin -t debugtrace -s 123250 -q 250 -o milc -- ./milc < su3imp.in > /dev/null &
../../../../pin -t debugtrace -s 7750 -q 250 -o gobmk -- ./gobmk --quiet --mode gtp < 13x13.tst > /dev/null &
../../../../pin -t debugtrace -s 61500 -q 250 -o hmmer -- ./hmmer --fixed 0 --mean 500 --num 500000 --sd 350 --seed 0 retro.hmm > /dev/null &
../../../../pin -t debugtrace -s 210500 -q 250 -o sjeng -- ./sjeng ref.txt > /dev/null &
wait
../../../../pin -t debugtrace -s 608250 -q 250 -o libquantum -- ./libquantum 1397 8 > /dev/null &
../../../../pin -t debugtrace -s 77750 -q 250 -o h264ref -- ./h264ref -d foreman_ref_encoder_main.cfg > /dev/null &
../../../../pin -t debugtrace -s 287250 -q 250 -o lbm -- ./lbm 3000 reference.dat 0 0 100_100_130_ldc.of > /dev/null &
wait
