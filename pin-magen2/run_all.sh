#!/bin/sh

../../../../pin -t debugtrace.so -o gzip -start_address main -- ./gzip ginput.program 60 &
../../../../pin -t debugtrace.so -o vpr -start_address main -- ./vpr net.in arch.in place.out dum.out -nodisp -place_only -init_t 5 -exit_t 0.005 -alpha_t 0.9412 -inner_num 2 &
../../../../pin -t debugtrace.so -o gcc -start_address main -- ./cc1 200.i -o 200.s &
../../../../pin -t debugtrace.so -o mcf -start_address main -- ./mcf mcf.in &
../../../../pin -t debugtrace.so -o parser -start_address main -- ./parser 2.1.dict -batch < pref.in &
../../../../pin -t debugtrace.so -o equake -start_address main -- ./equake < equake.in &
wait
