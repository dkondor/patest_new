#!/usr/bin/fish
# bitcoin_patest_run.fsh
# calculate transformed rank distribution and other measures for Bitcoin

# note: this script uses the fish shell syntax (https://fishshell.com/, https://github.com/fish-shell/fish-shell)
# also, this script is supposed to be run in the main directory of this repository

# data directory (contains the results of preprocessing); adjust as necessary
set DATADIR blockchain20200207_process
# original data directory (needed for testing preferential attachment for balances)
set INDIR blockchain20200207

# number of addresses in the data (needed some of the programs, output at the end of the preprocessing script)
set NADDR 609963452

# exponents to test for
set AA 0.00 0.25 0.50 0.70 0.85 1.00 1.15 1.30 1.50

# This script contains runs for three separate cases:
# 	1. edges have unlimited lifetime (main results in our papers)
# 	2. edges are "forgotten" after 30 days (if no transaction on them)
#	3. edges are "forgotten" after one day (i.e. 24 hours)
# In each case, two outputs are made:
# 	1. All events are considered together
#	2. Events (and the distribution of transformed ranks) are aggregated in 6 month intervals
# Technically, the first case can easily be created from the results of the second one.
# We provide both as separate output for convenience, but the below script can be easily adjusted
# to only output one of the cases.


########################################################################
# 1. Generate a list of "events", processed later
# (note: the below commands can be run in parallel; each will have an approximate
# runtime of a 1-2 hours and requires up to 65 GiB memory)
# 

# 1.1. Edges have unlimited lifetime
patestgen/ptg -Z -e $DATADIR/edges_ts.dat.gz -l $DATADIR/edges_uniq.dat.gz -i $DATADIR/addrids.dat.gz -N $NADDR -d 0 -D 1000000 | gzip -c > $DATADIR/patestgen_all.out.gz

# 1.2. Edges are forgotten after 30 days
patestgen/ptg -Z -e $DATADIR/edges_ts.dat.gz -l $DATADIR/edges_uniq.dat.gz -i $DATADIR/addrids.dat.gz -N 609963452 -d 30d -D 1000000 | gzip -c > $DATADIR/patestgen_30day.out.gz

# 1.3. Edges are forgotten after one day
patestgen/ptg -Z -e $DATADIR/edges_ts.dat.gz -l $DATADIR/edges_uniq.dat.gz -i $DATADIR/addrids.dat.gz -N $NADDR -d 1d -D 1000000 | gzip -c > $DATADIR/patestgen_1day.out.gz



########################################################################
# 2. Use the previous outputs to calculate transformed rank distributions
# each of the below will run for several hours, but needs only < 100 MiB memory

# 2.1.1. Edges have unlimited lifetime, all ranks are considered together
mkdir $DATADIR/patest_run_output
zcat $DATADIR/patestgen_all.out.gz | patestrun/ptr -o $DATADIR/patest_run_output/ptrm -m -a $AA -D 10000000 -H

# 2.1.2. Edges have unlimited lifetime, separete distribution for each 6 month interval
mkdir $DATADIR/patest_run_output_6m 
zcat $DATADIR/patestgen_all.out.gz | patestrun/ptr -o $DATADIR/patest_run_output_6m/ptrm -m -a $AA -D 10000000 -H -T 6m


# 2.2.1. Edges are forgotten after 30 days, all ranks are considered together
mkdir $DATADIR/patest_run_output_30day
zcat $DATADIR/patestgen_30day.out.gz | patestrun/ptr -o $DATADIR/patest_run_output_30day/ptrm -m -a $AA -D 10000000 -H

# 2.2.2. Edges are forgotten after 30 days, separete distribution for each 6 month interval
mkdir $DATADIR/patest_run_output_30day_6m
zcat $DATADIR/patestgen_30day.out.gz | patestrun/ptr -o $DATADIR/patest_run_output_30day_6m/ptrm -m -a $AA -D 10000000 -H -T 6m


# 2.3.1. Edges are forgotten after one day, all ranks are considered together
mkdir $DATADIR/patest_run_output_1day
zcat $DATADIR/patestgen_1day.out.gz | patestrun/ptr -o $DATADIR/patest_run_output_1day/ptrm -m -a $AA -D 10000000 -H

# 2.3.2. Edges are forgotten after one day, separete distribution for each 6 month interval
mkdir $DATADIR/patest_run_output_1day_6m
zcat $DATADIR/patestgen_1day.out.gz | patestrun/ptr -o $DATADIR/patest_run_output_1day_6m/ptrm -m -a $AA -D 10000000 -H -T 6m



########################################################################
# 3. Process the results for all cases

# 3.1. create CDF of transformed rank distributions
for basename in $DATADIR/patest_run_output/ptrm $DATADIR/patest_run_output_30day/ptrm $DATADIR/patest_run_output_1day/ptrm
for a in $AA
for i in 2 3 4
mawk 'BEGIN{OFS="\t"; print 0,0;}{s += $2; print $1,s/$3;}END{print 1,1;}' $basename-"$a"-$i.dat > $basename-"$a"-$i.cdf
end
end
end

# 3.2. calculate the Kolmogorov-Smirnov differences from a uniform distribution
for basename in $DATADIR/patest_run_output/ptrm $DATADIR/patest_run_output_30day/ptrm $DATADIR/patest_run_output_1day/ptrm
for i in 2 3 4
for a in $AA
echo -en $a\t
tail -n +2 $basename-"$a"-$i.cdf | mawk -v OFS='\t' 'BEGIN{y1 = -1; max = 0; }{if(y1 == -1) y1 = $2; y = y1 + (1-y1)*$1; diff = $2 - y; if(diff < 0) diff *= -1; if(diff > max) max = diff;}END{print y1, max;}'
end > $basename-summary-$i.out
end
end


# do the above for the case of results in 6 months time bins
for basename in $DATADIR/patest_run_output_6m/ptrm $DATADIR/patest_run_output_30day_6m/ptrm $DATADIR/patest_run_output_1day_6m/ptrm
cut -f 1,4 $basename-0.00-2.dat | uniq > $basename"_times.dat"
set times (cut -f 1 $basename"_times.dat")
set ntimes (wc -l < $basename"_times.dat")
# 3.3. create CDF of transformed rank distributions
for j in (seq $ntimes)
for a in $AA
for i in 2 3 4
mawk -v t1=$times[$j] 'BEGIN{OFS="\t"; print 0,0;}{if($1 == t1) { s += $3; print $2,s/$4; }}END{print 1,1;}' $basename-"$a"-$i.dat > $basename-$times[$j]-"$a"-$i.cdf
end
end
end
# 3.4. calculate the K-S differences
for j in (seq $ntimes)
for i in 2 3 4
for a in $AA
echo -en $a\\t
tail -n +2 $basename-$times[$j]-"$a"-$i.cdf | mawk -v OFS='\t' 'BEGIN{y1 = -1; max = 0; }{if(y1 == -1) y1 = $2; y = y1 + (1-y1)*$1; diff = $2 - y; if(diff < 0) diff *= -1; if(diff > max) max = diff;}END{print y1, max;}'
end > $basename-$times[$j]-summary-$i.out
end
end
end



########################################################################
# 4. preferential attachment test for balances
mkdir -p $DATADIR/ptb_run

# Two versions: in the first case, all balances with > 2 Satoshi are used; in the second case,
# balances that exactly correspond to mining rewards are left out, since these affect the
# total balance distribution too much.
# The second version is the one used in our paper.

# The runtime of these is approximately 9 hours, and memory usage is ~1.7 GiB

# run for all balances
patestrun/ptb -i $INDIR/bitcoin_2020_txin.dat.xz -o $INDIR/bitcoin_2020_txout.dat.xz -O $DATADIR/ptb_run/ptb1 -m -f -t 2 -X -a $AA -D 1000000 -H
# same, but leave out balances exactly equal to mining rewards (these significantly affect the distributions)
patestrun/ptb -i $INDIR/bitcoin_2020_txin.dat.xz -o $INDIR/bitcoin_2020_txout.dat.xz -O $DATADIR/ptb_run/ptb1e -m -f -t 2  -X -a $AA -D 1000000 -H -e 5000000000 2500000000 1250000000 625000000


# 4.1. process these results -> CDF of transformed ranks
for a in $AA
for b in 1 1e
mawk 'BEGIN{OFS="\t"; print 0,0,0;}{s += $2; s2 += $4; print $1,s/$3,s2/$5;}END{print 1,1,1;}' $DATADIR/ptb_run/ptb"$b"-"$a".dat > $DATADIR/ptb_run/ptb"$b"-"$a".cdf
end
end

# 4.2. K-S differences
# note: we consider two additional versions, depending on whether we weight
# the probability of events with the actual balance transferred
for b in 1 1e
for a in $AA
echo -ne $a\t
tail -n +2 $DATADIR/ptb_run/ptb"$b"-"$a".cdf | awk -v OFS='\t' 'BEGIN{y1 = 0; max = 0; }{y = y1 + (1-y1)*$1; diff = $2 - y; if(diff < 0) diff *= -1; if(diff > max) max = diff;}END{print y1, max;}'
end > $DATADIR/ptb_run/ptb"$b"-summary.out
for a in $AA
echo -ne $a\\t
tail -n +2 $DATADIR/ptb_run/ptb"$b"-"$a".cdf | awk -v OFS='\t' 'BEGIN{y1 = 0; max = 0; }{y = y1 + (1-y1)*$1; diff = $3 - y; if(diff < 0) diff *= -1; if(diff > max) max = diff;}END{print y1, max;}'
end > $DATADIR/ptb_run/ptb"$b"w-summary.out
end


