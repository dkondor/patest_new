#!/usr/bin/fish
# bitcoin_download.fsh -- download Bitcoin dataset from Dryad
# note: this script can be run from any location

# set this to the data directory to use
set DATADIR blockchain20200207


# names of file to download
set names bitcoin_2020_bh.dat.gz bitcoin_2020_txin.dat.xz bitcoin_2020_txout.dat.xz bitcoin_2020_tx.dat.xz bitcoin_2020_addresses.dat.gz

# URLs of files -- note: I don't know if these are supposed to be stable!
# please open an issue if download is not working
set urls https://datadryad.org/stash/downloads/file_stream/919157 https://datadryad.org/stash/downloads/file_stream/919161 https://datadryad.org/stash/downloads/file_stream/919162 https://datadryad.org/stash/downloads/file_stream/919221 https://datadryad.org/stash/downloads/file_stream/919156


mkdir -p $DATADIR
cd $DATADIR


for i in (seq (count $names))
if not wget -O $names[$i] $urls[$i]
echo Error downloading file $names[$i] from $urls[$i]!
break
end
end

