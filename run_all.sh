#!/bin/sh

rm res.dat
touch res.dat
echo "../traces/bc/art_p1M.log" >> res.dat
./cache_sim ../traces/bc/art_p1M.log     >> res.dat
echo "../traces/bc/parser_p1M.log" >> res.dat
./cache_sim ../traces/bc/parser_p1M.log >> res.dat
echo "../traces/bc/bzip2_p1M.log" >> res.dat   
./cache_sim ../traces/bc/bzip2_p1M.log   >> res.dat
echo "../traces/bc/perlbmk_p1M.log" >> res.dat
./cache_sim ../traces/bc/perlbmk_p1M.log >> res.dat
echo "../traces/bc/equake_p1M.log" >> res.dat  
./cache_sim ../traces/bc/equake_p1M.log  >> res.dat
echo "../traces/bc/scp_p1M.log" >> res.dat
./cache_sim ../traces/bc/scp_p1M.log >> res.dat
echo "../traces/bc/gcc_p1M.log" >> res.dat     
./cache_sim ../traces/bc/gcc_p1M.log     >> res.dat
echo "../traces/bc/ssh_p1M.log" >> res.dat
./cache_sim ../traces/bc/ssh_p1M.log >> res.dat
echo "../traces/bc/gzip_p1M.log" >> res.dat    
./cache_sim ../traces/bc/gzip_p1M.log    >> res.dat
echo "../traces/bc/traceroute_p1M.log" >> res.dat
./cache_sim ../traces/bc/traceroute_p1M.log >> res.dat
echo "../traces/bc/mcf_p1M.log" >> res.dat     
./cache_sim ../traces/bc/mcf_p1M.log     >> res.dat
echo "../traces/bc/twolf_p1M.log" >> res.dat
./cache_sim ../traces/bc/twolf_p1M.log >> res.dat
echo "../traces/bc/mesa_p1M.log" >> res.dat    
./cache_sim ../traces/bc/mesa_p1M.log    >> res.dat
echo "../traces/bc/vpr_p1M.log" >> res.dat
./cache_sim ../traces/bc/vpr_p1M.log >> res.dat