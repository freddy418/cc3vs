#!/bin/sh

../../../../pin -t debugtrace -o gzip_p1M.log -p 1000000 -q 500 -start_address main -- ./gzip ginput.program 60 &
../../../../pin -t debugtrace -o vpr_p1M.log -p 1000000 -q 500 -start_address main -- ./vpr net.in arch.in place.out dum.out -nodisp -place_only -init_t 5 -exit_t 0.005 -alpha_t 0.9412 -inner_num 2 &
../../../../pin -t debugtrace -o gcc_p1M.log -p 1000000 -q 500 -start_address main -- ./cc1 200.i -o 200.s &
../../../../pin -t debugtrace -o mcf_p1M.log -p 1000000 -q 500 -start_address main -- ./mcf mcf.in &
../../../../pin -t debugtrace -o parser_p1M.log -p 1000000 -q 500 -start_address main -- ./parser 2.1.dict -batch < pref.in &
../../../../pin -t debugtrace -o perlbmk_p1M.log -p 1000000 -q 500 -start_address main -- ./perlbmk -I./lib splitmail.pl 850 5 19 18 1500 > 850.5.19.18.1500.out &
../../../../pin -t debugtrace -o bzip2_p1M.log -p 1000000 -q 500 -start_address main -- ./bzip2 binput.program 58 &
../../../../pin -t debugtrace -o twolf_p1M.log -p 1000000 -q 500 -start_address main -- ./twolf ref &
../../../../pin -t debugtrace -o mesa_p1M.log -p 1000000 -q 500 -start_address main -- ./mesa -frames 1000 -meshfile mesa.in -ppmfile mesa.ppm &
../../../../pin -t debugtrace -o art_p1M.log -p 1000000 -q 500 -start_address main -- ./art -scanfile c756hel.in -trainfile1 a10.img -trainfile2 hc.img -stride 2 -startx 110 -starty 200 -endx 160 -endy 240 -objects 10 &
../../../../pin -t debugtrace -o equake_p1M.log -p 1000000 -q 500 -start_address main -- ./equake < equake.in &
wait
