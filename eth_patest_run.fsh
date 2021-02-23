#!/usr/bin/fish
# eth_patest_run.fsh
# calculate transformed rank distribution and other measures for Ethereum

# note: this script uses the fish shell syntax (https://fishshell.com/, https://github.com/fish-shell/fish-shell)
# also, this script is supposed to be run in the main directory of this repository

# data directory (contains the results of preprocessing); adjust as necessary
set DATADIR ethereum_data

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
# 
# In all cases, regular and contract addresses are treated separately.
# It is possible to add the results for these cases if this distinction
# is not required.


########################################################################
# 1. Generate a list of "events", processed later
# (note: the below commands can be run in parallel; each will have an approximate
# runtime of up to 1 hour and requires up to 4 GiB memory)

# 1.1. Edges have unlimited lifetime
patestgen/ptg -Z -e $DATADIR/edges_ts.dat.gz -l $DATADIR/edges_uniq.dat.gz -i $DATADIR/addresses.dat.gz -d 0 -D 1000000 -c | gzip -c > $DATADIR/patestgen_all.out.gz

# 1.2. Edges are forgotten after 30 days
patestgen/ptg -Z -e $DATADIR/edges_ts.dat.gz -l $DATADIR/edges_uniq.dat.gz -i $DATADIR/addresses.dat.gz -d 30d -D 1000000 -c | gzip -c > $DATADIR/patestgen_30d.out.gz

# 1.3. Edges are forgotten after one day
patestgen/ptg -Z -e $DATADIR/edges_ts.dat.gz -l $DATADIR/edges_uniq.dat.gz -i $DATADIR/addresses.dat.gz -d 1d -D 1000000 -c | gzip -c > $DATADIR/patestgen_1d.out.gz


########################################################################
# 2. Use the previous outputs to calculate transformed rank distributions
# each of the below will run for up to one hour, but needs only < 100 MiB memory


# 2.1.1. Edges have unlimited lifetime, all ranks are considered together
# regular addresses
mkdir -p $DATADIR/patest_addresses
zcat $DATADIR/patestgen_all.out.gz | awk '{if($4 == 0) print $1, $2, $3;}' | patestrun/ptr -o $DATADIR/patest_addresses/ptrm -m -a $AA -D 10000000 -H
# contract addresses
mkdir -p $DATADIR/patest_contracts
zcat $DATADIR/patestgen_all.out.gz | awk '{if($4 == 1) print $1, $2, $3;}' | patestrun/ptr -o $DATADIR/patest_contracts/ptrm -m -a $AA -D 10000000 -H

# 2.1.2. Edges have unlimited lifetime, separete distribution for each 6 month interval
# regular addresses
mkdir -p $DATADIR/patest_addresses_6m
zcat $DATADIR/patestgen_all.out.gz | awk '{if($4 == 0) print $1, $2, $3;}' | patestrun/ptr -o $DATADIR/patest_addresses_6m/ptrm -m -a $AA -D 10000000 -H -T 6m
# contract addresses
mkdir -p $DATADIR/patest_contracts_6m
zcat $DATADIR/patestgen_all.out.gz | awk '{if($4 == 1) print $1, $2, $3;}' | patestrun/ptr -o $DATADIR/patest_contracts_6m/ptrm -m -a $AA -D 10000000 -H -T 6m


# 2.2.1. Edges are forgotten after 30 days, all ranks are considered together
# regular addresses
mkdir -p $DATADIR/patest_addresses_30day
zcat $DATADIR/patestgen_30d.out.gz | awk '{if($4 == 0) print $1, $2, $3;}' | patestrun/ptr -o $DATADIR/patest_addresses_30day/ptrm -m -a $AA -D 10000000 -H
# contract addresses
mkdir -p $DATADIR/patest_contracts_30day
zcat $DATADIR/patestgen_30d.out.gz | awk '{if($4 == 1) print $1, $2, $3;}' | patestrun/ptr -o $DATADIR/patest_contracts_30day/ptrm -m -a $AA -D 10000000 -H

# 2.2.2. Edges are forgotten after 30 days, separete distribution for each 6 month interval
# regular addresses
mkdir -p $DATADIR/patest_addresses_30day_6m
zcat $DATADIR/patestgen_30d.out.gz | awk '{if($4 == 0) print $1, $2, $3;}' | patestrun/ptr -o $DATADIR/patest_addresses_30day_6m/ptrm -m -a $AA -D 10000000 -H -T 6m
# contract addresses
mkdir -p $DATADIR/patest_contracts_30day_6m
zcat $DATADIR/patestgen_30d.out.gz | awk '{if($4 == 1) print $1, $2, $3;}' | patestrun/ptr -o $DATADIR/patest_contracts_30day_6m/ptrm -m -a $AA -D 10000000 -H -T 6m


# 2.3.1. Edges are forgotten after one day, all ranks are considered together
# regular addresses
mkdir -p $DATADIR/patest_addresses_1day
zcat $DATADIR/patestgen_1d.out.gz | awk '{if($4 == 0) print $1, $2, $3;}' | patestrun/ptr -o $DATADIR/patest_addresses_1day/ptrm -m -a $AA -D 10000000 -H
# contract addresses
mkdir -p $DATADIR/patest_contracts_1day
zcat $DATADIR/patestgen_1d.out.gz | awk '{if($4 == 1) print $1, $2, $3;}' | patestrun/ptr -o $DATADIR/patest_contracts_1day/ptrm -m -a $AA -D 10000000 -H


# 2.3.2. Edges are forgotten after one day, separete distribution for each 6 month interval
# regular addresses
mkdir -p $DATADIR/patest_addresses_1day_6m
zcat $DATADIR/patestgen_1d.out.gz | awk '{if($4 == 0) print $1, $2, $3;}' | patestrun/ptr -o $DATADIR/patest_addresses_1day_6m/ptrm -m -a $AA -D 10000000 -H -T 6m
# contract addresses
mkdir -p $DATADIR/patest_contracts_1day_6m
zcat $DATADIR/patestgen_1d.out.gz | awk '{if($4 == 1) print $1, $2, $3;}' | patestrun/ptr -o $DATADIR/patest_contracts_1day_6m/ptrm -m -a $AA -D 10000000 -H -T 6m




########################################################################
# 3. Process the results for all cases

# 3.1. create CDF of transformed rank distributions
for basename in $DATADIR/patest_addresses $DATADIR/patest_contracts
for base2 in /ptrm _30day/ptrm _1day/ptrm
for a in $AA
for i in 2 3 4
mawk 'BEGIN{OFS="\t"; print 0,0;}{s += $2; print $1,s/$3;}END{print 1,1;}' $basename$base2-"$a"-$i.dat > $basename$base2-"$a"-$i.cdf
end
end
end
end

# 3.2. calculate the Kolmogorov-Smirnov differences from a uniform distribution
for basename in $DATADIR/patest_addresses $DATADIR/patest_contracts
for base2 in /ptrm _30day/ptrm _1day/ptrm
for i in 2 3 4
for a in $AA
echo -en $a\t
tail -n +2 $basename$base2-"$a"-$i.cdf | mawk -v OFS='\t' 'BEGIN{y1 = -1; max = 0; }{if(y1 == -1) y1 = $2; y = y1 + (1-y1)*$1; diff = $2 - y; if(diff < 0) diff *= -1; if(diff > max) max = diff;}END{print y1, max;}'
end > $basename$base2-summary-$i.out
end
end
end


# do the above for the case of results in 6 months time bins
for base1 in $DATADIR/patest_addresses $DATADIR/patest_contracts
for base2 in _6m/ptrm _30day_6m/ptrm _1day_6m/ptrm
set basename $base1$base2
cut -f 1,4 $basename-0.00-2.dat | uniq > $basename"_times.dat"
set times (cut -f 1 $basename"_times.dat")
set ntimes (count $times)
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
end


########################################################################
# 4. Calculate indegree distributions

# create degree distributions separately for addresses and contracts
zcat $DATADIR/patestgen_all.out.gz | awk '{if($1 == 1 && $4 == 0) print $1, $2, $3;}' | misc/indeg_dist -S | xz > $DATADIR/indeg_dist_all_addresses.out.xz
zcat $DATADIR/patestgen_all.out.gz | awk '{if($1 == 1 && $4 == 1) print $1, $2, $3;}' | misc/indeg_dist -S | xz > $DATADIR/indeg_dist_all_contracts.out.xz

# process these -- create binned distributions for plotting
mkdir $DATADIR/indeg_dist
xzcat $DATADIR/indeg_dist_all_addresses.out.xz | awk -v datadir=$DATADIR 'BEGIN{ts = 0; min1 = 1; d1 = 1;}{if(ts != $1) { if(ts > 0) { if(cnt > 0) print min,min+d,cnt >> outf; } min = min1; d = d1; cnt = 0; outf = datadir"/indeg_dist/dist_addresses_"$1".dat"; ts = $1; } if($2 > min + d) { print min, min+d, cnt >> outf; cnt = 0; min += d; d *= 2; } cnt += $3; }END{ if(cnt > 0) print min,min+d,cnt >> outf; }'
xzcat $DATADIR/indeg_dist_all_contracts.out.xz | awk -v datadir=$DATADIR 'BEGIN{ts = 0; min1 = 1; d1 = 1;}{if(ts != $1) { if(ts > 0) { if(cnt > 0) print min,min+d,cnt >> outf; } min = min1; d = d1; cnt = 0; outf = datadir"/indeg_dist/dist_contracts_"$1".dat"; ts = $1; } if($2 > min + d) { print min, min+d, cnt >> outf; cnt = 0; min += d; d *= 2; } cnt += $3; }END{ if(cnt > 0) print min,min+d,cnt >> outf; }'






