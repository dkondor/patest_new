time ../patest/pt6r3 -e edges_ts_100M.dat -i addrids_100M.dat -N 14691303 -O2 patest_100M_out1/pt6r10y2 -0 -l edges_100M.dat -a 0.7 0.85 1.0 1.1 1.2 1.35 0.0 0.25 0.5 1.5 -d 10y -D 5000000


awk '{if($1 >= 0 && $2 >= 0 && $1 != $2) print $0;}' edges_ts_100M.dat | time ../orbtree/pt7 -o patest_100M_out4/pt7 -a 0.7 0.85 1.0 1.1 1.2 1.35 0.0 0.25 0.5 1.5 -0 -D 5000000
1987.50user 27.73system 21:07.77elapsed 158%CPU (0avgtext+0avgdata 4013960maxresident)k


awk '{if($1 >= 0 && $2 >= 0 && $1 != $2) print $0;}' edges_ts_100M.dat | time ../patest_test/pt7 -o patest_100M_out5/pt7 -a 0.7 0.85 1.0 1.1 1.2 1.35 0.0 0.25 0.5 1.5 -0 -D 5000000
1953.65user 27.10system 20:36.84elapsed 160%CPU (0avgtext+0avgdata 4014260maxresident)k


awk '{if($1 >= 0 && $2 >= 0 && $1 != $2) print $0;}' edges_ts_100M.dat | time ../patest_test/pt7s -o patest_100M_out5/pt7s -a 0.7 0.85 1.0 1.1 1.2 1.35 0.0 0.25 0.5 1.5 -0 -D 5000000
2835.52user 26.14system 35:56.07elapsed 132%CPU (0avgtext+0avgdata 4014060maxresident)k


awk '{if($1 >= 0 && $2 >= 0 && $1 != $2) print $0;}' edges_ts_100M.dat | time ../patest_test/pt7sd -o patest_100M_out5/pt7sd -a 0.7 0.85 1.0 1.1 1.2 1.35 0.0 0.25 0.5 1.5 -0 -D 5000000
1995.40user 23.02system 21:17.86elapsed 157%CPU (0avgtext+0avgdata 4014180maxresident)k


    zcat patest_100M_out2/pt6r10y2-$a"-o-1".dat.gz | sort -S 10G -g | awk '{if(NR%10000 == 0) print $0;}' > patest_100M_out2/pt6r10y2-$a"-o-1".cdf

for a in 0.70 0.85 1.00 1.10 1.20 1.35 0.00 0.25 0.50 1.50
    zcat patest_100M_out4/pt7-$a"-o-1".dat.gz | sort -S 10G -g | awk '{if(NR%10000 == 0) print $0;}' > patest_100M_out4/pt7-$a"-o-1".cdf
    echo $a
end

for a in 0.70 0.85 1.00 1.10 1.20 1.35 0.00 0.25 0.50 1.50
    zcat patest_100M_out5/pt7-$a"-o-1".dat.gz | sort -S 10G -g | awk '{if(NR%10000 == 0) print $0;}' > patest_100M_out5/pt7-$a"-o-1".cdf
    echo $a
end

for a in 0.70 0.85 1.00 1.10 1.20 1.35 0.00 0.25 0.50 1.50
    zcat patest_100M_out5/pt7s-$a"-o-1".dat.gz | sort -S 10G -g | awk '{if(NR%10000 == 0) print $0;}' > patest_100M_out5/pt7s-$a"-o-1".cdf
    echo $a
end

for a in 0.70 0.85 1.00 1.10 1.20 1.35 0.00 0.25 0.50 1.50
    zcat patest_100M_out5/pt7sd-$a"-o-1".dat.gz | sort -S 10G -g | awk '{if(NR%10000 == 0) print $0;}' > patest_100M_out5/pt7sd-$a"-o-1".cdf &
    echo $a
end




