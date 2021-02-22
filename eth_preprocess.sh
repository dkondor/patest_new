#!/bin/bash
# eth_preprocess.sh -- preprocess Ethereum dataset downloaded from Zenodo
# prepare input files in the format used by the later scripts

trap 'echo Error processing data' ERR

# directory with the downloaded data -- processed data is output in the same location
DATADIR=ethereum_data
# maximum memory use for sorting -- adjust as necessary
SORT_MEMORY_USE=64G

# concatenate edges and sort by time
for a in $DATADIR/edges_ts_*gz
do zcat $a
done | mawk '{if($1 != $2) print $0;}' | sort -S $SORT_MEMORY_USE -k 4n,4 -k 5n,5 | cut -f 1,2,3 | gzip -c > $DATADIR/edges_ts.dat.gz
# 628810973 lines

# create unique edges
zcat $DATADIR/edges_ts.dat.gz | cut -f 1,2 | sort -S $SORT_MEMORY_USE -k 1n,1 -k 2n,2 | uniq | gzip -c > $DATADIR/edges_uniq.dat.gz
# 189302192 lines

# note: it is OK to delete the original edge files at this point;
# only the files created by this script are needed for the next steps
