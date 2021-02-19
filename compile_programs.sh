#!/bin/sh
# compile_programs.sh -- compile C++ code needed for running preferential attachment testing
# note: this script should be run from the main directory in the repository

# 1. program to generate data for preferential attachment test
cd patestgen
g++ -o ptg patest_gen.c edgeheap.cpp edges.c idlist.cpp -O3 -march=native -lm -std=gnu++14
cd ..

# 2. programs to calculate test statistics
cd patestrun
g++ -o ptr patest_ranks.cpp -O3 -march=native -std=gnu++14
g++ -o ptb patest_balances.cpp -O3 -march=native -lm -std=gnu++14
cd ..

# 3. helper code used to preprocess data and to create degree and balance distributions
cd misc
g++ -o numjoin numeric_join.cpp -O3 -march=native -std=gnu++11
g++ -o indeg_dist indeg_dist_new.cpp -O3 -march=native -std=gnu++11
g++ -o bdist balance_dists.cpp -O3 -march=native -std=gnu++11
cd ..

