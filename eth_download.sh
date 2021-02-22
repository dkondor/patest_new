#!/bin/bash
# eth_download.sh -- download Ethereum dataset from Zenodo
# note: this script can be run from any location

# TODO: more proper error handling!
trap 'echo Error downloading data' ERR

# set this to the data directory to use
DATADIR=ethereum_data

mkdir -p $DATADIR
cd $DATADIR

# we need all edges files and the addresses
wget https://zenodo.org/record/4543269/files/addresses.dat.gz

for i in `seq 0 93`
do wget https://zenodo.org/record/4543269/files/edges_ts_$i.dat.gz
done

