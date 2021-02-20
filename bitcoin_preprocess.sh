#!/bin/bash
# bitcoin_preprocess.sh
# script to preprocess Bitcoin data for preferential attachment testing
# this includes creating a list of edges (both sorted by time and node ID)
# that can be used as input in the later steps

# note: working directory should be the root directory of the git repository
# compile helper programs first with compile_programs.sh

set -e

# input data dir
DIR_IN=blockchain20200207
# preprocessed data dir
DIR_OUT=blockchain20200207_process
# maximum memory use for sorting -- adjust as necessary
SORT_MEMORY_USE=64G

echo "Join transaction inputs and outputs to create edges"
misc/numjoin -j 1 -o1 1,5 -o2 3 <(xzcat $DIR_IN/bitcoin_2020_txin.dat.xz) <(xzcat $DIR_IN/bitcoin_2020_txout.dat.xz) | gzip -c > $DIR_OUT/edges.dat.gz

echo "Create unique edges"
zcat $DIR_OUT/edges.dat | awk 'BEGIN{OFS="\t";}{if($2!=$3) print $2,$3;}' | sort -k 1n,1 -k 2n,2 -S $SORT_MEMORY_USE | uniq | gzip -c > $DIR_OUT/edges_uniq.dat.gz

echo "Add timestamps to the edges"
misc/numjoin -1 2 -o1 1 -2 1 -o2 3 <(xzcat $DIR_IN/bitcoin_2020_tx.dat.xz) <(zcat $DIR_IN/bitcoin_2020_bh.dat.gz) | gzip -c > $DIR_OUT/txtime.dat.gz
misc/numjoin -j 1 -o1 2,3 -o2 2 <(zcat $DIR_OUT/edges.dat.gz) <(zcat $DIR_OUT/txtime.dat.gz) | gzip -c > $DIR_OUT/edges_ts.dat.gz
rm $DIR_OUT/edges.dat.gz

echo "Cut all address IDs"
echo "Number of addresses:"
zcat $DIR_IN/bitcoin_2020_addresses.dat.gz | cut -f 1 | tee >(gzip -c > $DIR_OUT/addrids.dat.gz) | wc -l
# result: 609963452
