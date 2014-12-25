!#/bin/bash

../../../../pin -t dytan -o gzip.dat -p 1000000 -s 486 -q 500 -start_address main -- ./gzip ginput.program 60 > gzip_p1M.log &
../../../../pin -t dytan -o vpr.dat -p 1000000 -s 71 -q 500 -start_address main -- ./vpr net.in arch.in place.out dum.out -nodisp -place_only -init_t 5 -exit_t 0.005 -alpha_t 0.9412 -inner_num 2 > vpr_p1M.log &
../../../../pin -t dytan -o gcc.dat -p 1000000 -s 199 -q 500 -start_address main -- ./cc1 200.i -o 200.s > gcc_p1M.log &
../../../../pin -t dytan -o mcf.dat -p 1000000 -s 316 -q 500 -start_address main -- ./mcf mcf.in > mcf_p1M.log &
../../../../pin -t dytan -o parser.dat -p 1000000 -s 16 -q 500 -start_address main -- ./parser 2.1.dict -batch < pref.in > parser_p1M.log &
../../../../pin -t dytan -o perlbmk.dat -p 1000000 -s 421 -q 500 -start_address main -- ./perlbmk -I./lib splitmail.pl 850 5 19 18 1500 > 850.5.19.18.1500.out > perlbmk_p1M.log &
../../../../pin -t dytan -o bzip2.dat -p 1000000 -s 9 -q 500 -start_address main -- ./bzip2 binput.program 58 > bzip2_p1M.log &
../../../../pin -t dytan -o twolf.dat -p 1000000 -s 31 -q 500 -start_address main -- ./twolf ref > twolf_p1M.log &
../../../../pin -t dytan -o mesa.dat -p 1000000 -s 89 -q 500 -start_address main -- ./mesa -frames 1000 -meshfile mesa.in -ppmfile mesa.ppm > mesa_p1M.log &
../../../../pin -t dytan -o art.dat -p 1000000 -s 67 -q 500 -start_address main -- ./art -scanfile c756hel.in -trainfile1 a10.img -trainfile2 hc.img -stride 2 -startx 110 -starty 200 -endx 160 -endy 240 -objects 10 > art_p1M.log &
../../../../pin -t dytan -o equake.dat -p 1000000 -s 194 -q 500 -start_address main -- ./equake < equake.in > equake_p1M.log &
