#!/usr/bin/fish
# eth_figs.fsh
# create figures of main results for Ethereum

# note: this script uses the fish shell syntax (https://fishshell.com/, https://github.com/fish-shell/fish-shell)
# also, this script is supposed to be run in the main directory of this repository

# data directory (contains the results of preprocessing); adjust as necessary
set DATADIR ethereum_data

# exponents to test for -- should be the same as in ethereum_patest_run.fsh
set AA 0.00 0.25 0.50 0.70 0.85 1.00 1.15 1.30 1.50
set colors 5aa732 005c96 e75216 009ace ffd500 8671a7 e78502 db6aa2 007939 9a2846 815234


# types of results
set titles 'new edges from new nodes' 'new edges from existing nodes' 'transactions on existing edges'
set types 3 2 4

set addrtypes 'Ethereum, addresses' 'Ethereum, contracts'
set addrbase $DATADIR/patest_addresses $DATADIR/patest_contracts

set datalen2 ', unlimited' ', 30 day lifetime' ', 1 day lifetime'
set basename2 /ptrm _30day/ptrm _1day/ptrm


########################################################################
# 0. Figure that can be used as legend
# Note: by default, none of the below figures have legend. It is recommended
# to cut the legend (e.g. with Inkscape) from this figure and paste it with
# Latex below a figure with multiple panels (thus the legend is shared).
# If this is not desired, legend can be enabled for individual figures by
# changing the "se ke off" command for individual figures.

begin
echo se te unknown
echo se ke below
echo se xr [0:1]
echo se yr [0:1]
echo p 1/0 not
for k in (seq 9)
echo rep 1/0 w l lw 3 lc rgbcolor \'\#$colors[$k]\' t \'a = $AA[$k]\'
end

echo se te post eps color solid size 6.4,2.6
echo se out \'$DATADIR/figs_legend.eps\'
echo rep
echo se out
end | gnuplot ^ /dev/null
epstopdf $DATADIR/figs_legend.eps
convert -density 300 $DATADIR/figs_legend.eps -bordercolor white -border 0x0 -alpha remove $DATADIR/figs_legend.png



########################################################################
# 1. CDF figures for distributions over the full lifetime of Ethereum
for i in 1 2
for i2 in 1 2 3
set basename $addrbase[$i]$basename2[$i2]
set outdir $basename-figs
mkdir -p $outdir
for j in 1 2 3
set y1 (head -n 1 $basename-summary-$types[$j].out | cut -f 2)
begin
echo se te unknown
echo se ke off
echo se yr [0:1]
echo se xr [0:1]
echo se xl \'CDF\'
echo se yl \'calculated rank\' offset 1
echo p 1/0
for k in (seq (count $AA))
echo rep \'$basename-$AA[$k]-$types[$j].cdf\' u 2:1 w l lw 3 lc rgbcolor \'\#$colors[$k]\' t \'a = $AA[$k]\'
end
echo rep \(x - $y1\) / \(1 - $y1\) w l lw 2 lc -1 not

echo se title \'$addrtypes[$i]$datalen2[$i2], $titles[$j]\'
echo se te post eps color solid size 3.2,2.1
echo se out \'$outdir/ethereum_$types[$j].eps\'
echo rep
echo se out
end | gnuplot ^ /dev/null

epstopdf $outdir/ethereum_$types[$j].eps
convert -density 300 $outdir/ethereum_$types[$j].eps -bordercolor white -border 0x0 -alpha remove $outdir/ethereum_$types[$j].png

end
end
end


# 1.2. figure with K-S difference as a function of the exponent
for i in 1 2
for i2 in 1 2 3
set basename $addrbase[$i]$basename2[$i2]
set outdir $basename-figs
for j in 1 2 3
begin
echo se te unknown
echo se yr [0:]
echo se xr [-0.05:1.55]
echo se xtics 0.5
echo se ytics 0.1
echo se xl \'exponent\'
echo se yl \'K-S difference\' off 1.5
echo p \'$basename-summary-$types[$j].out\' u 1:3 w lp lw 2 pt 5 not
echo se title \'$titles[$j]\' off -2,0
echo se te post eps color solid size 1.6, 1.8
echo se out \'$outdir/ethereum_exp_$types[$j].eps\'
echo rep
echo se out
end | gnuplot ^ /dev/null
epstopdf $outdir/ethereum_exp_$types[$j].eps
convert -density 300 $outdir/ethereum_exp_$types[$j].eps -bordercolor white -border 0x0 -alpha remove $outdir/ethereum_exp_$types[$j].png
end
end
end



########################################################################
# 2. CDF of transformed ranks in shorter (i.e. 6 month) intervals
# each interval is a separate figure

set basename2 _6m/ptrm _30day_6m/ptrm _1day_6m/ptrm

for i1 in 1 2
for i2 in 1 2 3
set basename $addrbase[$i1]$basename2[$i2]
set outdir $basename-figs
mkdir -p $outdir

set times (cut -f 1 $basename"_"times.dat)
set ntimes (count $times)

for j in (seq $ntimes)
for i in 1 2 3

set y1 (head -n 1 $basename-$times[$j]-summary-$types[$i].out | cut -f 2)
set dt (date -d @$times[$j] -u -I)

begin
echo se te unknown
echo se ke off
echo se yr [0:1]
echo se xr [0:1]
echo se xl \'CDF\'
echo se yl \'calculated rank\' offset 1
echo p 1/0
for k in (seq (count $AA))
echo rep \'$basename-$times[$j]-$AA[$k]-$types[$i].cdf\' u 2:1 w l lw 3 lc rgbcolor \'\#$colors[$k]\' t \'a = $AA[$k]\'
end
echo rep \(x - $y1\) / \(1 - $y1\) w l lw 2 lc -1 not

echo se title \'$addrtypes[$i1]$datalen2[$i2], $dt, $titles[$i]\'
echo se te post eps color solid size 3.2,2.1
echo se out \'$outdir/ethereum_$times[$j]_$types[$i].eps\'
echo rep
echo se out
end | gnuplot ^ /dev/null

epstopdf $outdir/ethereum_$times[$j]_$types[$i].eps
convert -density 300 $outdir/ethereum_$times[$j]_$types[$i].eps -bordercolor white -border 0x0 -alpha remove $outdir/ethereum_$times[$j]_$types[$i].png

end
end
end
end


# 2.2. figure with K-S difference as a function of the exponent
for i1 in 1 2
for i2 in 1 2 3
set basename $addrbase[$i1]$basename2[$i2]
set outdir $basename-figs
set times (cut -f 1 $basename"_"times.dat)
set ntimes (count $times)
for j in (seq $ntimes)
set dt (date -d @$times[$j] -u -I)
for i in 1 2 3
begin
echo se te unknown
echo se yr [0:]
echo se xr [-0.05:1.55]
echo se xtics 0.5
echo se ytics 0.1
echo se xl \'exponent\'
echo se yl \'K-S difference\' off 1.5
echo p \'$basename-$times[$j]-summary-$types[$i].out\' u 1:3 w lp lw 2 pt 5 not
echo se title \'$dt\' off -2,0
echo se te post eps color solid size 1.6, 1.8
echo se out \'$outdir/ethereum_exp_$times[$j]_$types[$i].eps\'
echo rep
echo se out
echo se te qt
end | gnuplot  ^ /dev/null

epstopdf $outdir/ethereum_exp_$times[$j]_$types[$i].eps
convert -density 300 $outdir/ethereum_exp_$times[$j]_$types[$i].eps -bordercolor white -border 0x0 -alpha remove $outdir/ethereum_exp_$times[$j]_$types[$i].png
end
end
end
end

# 2.3. figures with the plots for exponents together for some of the times
# indices of times to include
for i1 in 1 2
for i2 in 1 2 3
set basename $addrbase[$i1]$basename2[$i2]
set outdir $basename-figs
set times (cut -f 1 $basename"_"times.dat)

for i in 1 2 3
begin
echo se te unknown
echo se yr [0:1]
echo se xr [-0.05:1.55]
echo se xtics 0.5
echo se ytics 0.1
echo se xl \'exponent\'
echo se yl \'K-S difference\' off 1.5
echo p 1/0 not

for j in (seq (math $ntimes - 1))
set dt (date -d @$times[$j] -u -I)
echo rep \'$basename-$times[$j]-summary-$types[$i].out\' u 1:3 w lp lw 2 pt 5 lc rgbcolor \'#$colors[$j]\' not
end
echo se title \'$titles[$i]\' off -2,0
echo se auto y
echo se yr [0:]
echo se te post eps color solid size 2.1, 2.1
echo se out \'$outdir/ethereum_exp_multi_$types[$i].eps\'
echo rep
echo se out
end | gnuplot ^ /dev/null

epstopdf $outdir/ethereum_exp_multi_$types[$i].eps
convert -density 300 $outdir/ethereum_exp_multi_$types[$i].eps -bordercolor white -border 0x0 -alpha remove $outdir/ethereum_exp_multi_$types[$i].png
end
end
end



# common legend for the above figures -- this could be edited by e.g. Inkscape to cut the actual legend
for i1 in 1 2
for i2 in 1 2 3
set basename $addrbase[$i1]$basename2[$i2]
set outdir $basename-figs
set times (cut -f 1 $basename"_"times.dat)
begin
echo se te unknown
echo se ke below
echo se xr [0:1]
echo se yr [0:1]
echo p 1/0 not
for j in (seq (math $ntimes - 1))
set dt (date -d @$times[$j] -u -I)
echo rep 1/0 w lp lw 2 pt 5 lc rgbcolor \'#$colors[$j]\' t \'$dt\'
end
echo se te post eps color solid size 6.4, 3.2
echo se out \'$outdir/ethereum_exp_multi_legend.eps\'
echo rep
echo se out
echo se te qt
end | gnuplot ^ /dev/null

epstopdf $outdir/ethereum_exp_multi_legend.eps
convert -density 300 $outdir/ethereum_exp_multi_legend.eps -bordercolor white -border 0x0 -alpha remove $outdir/ethereum_exp_multi_legend.png
end
end




########################################################################
# 3. indegree distributions, separately for contracts and addresses



set times (for a in $DATADIR/indeg_dist/dist_addresses*
basename $a .dat
end | cut -d _ -f 3)
set ntimes (count $times)

begin
echo se te unknown
echo se xl \'address indegree\'
echo se yl \'frequency\' off 1
echo se ytics format \'%g\'
echo se xr [1:]
echo se yr [1:2]
echo p 1/0 not
echo se auto y
echo se yr [:1e8]
echo se log xy
for k in (seq $ntimes)
set dt (date -d @$times[$k] -u +%Y)
echo rep \'$DATADIR/indeg_dist/dist_addresses_$times[$k].dat\' u \(\(\$1+\$2\)/2.0\):\(\$3/\(\$2-\$1\)\) w lp lw 3 pt 5 lc rgbcolor \'\#$colors[$k]\' t \'$dt\'
end

# note: fit done externally
echo a = -2.53542
echo b = 5e9
echo rep b\*x\*\*a w l lw 2 lc -1 not

echo se te post eps color solid size 3.2,2.2
echo se out \"$DATADIR/eth_indeg_addr.eps\"
echo rep
echo se out
end | gnuplot ^ /dev/null

epstopdf $DATADIR/eth_indeg_addr.eps
convert -density 300 $DATADIR/eth_indeg_addr.eps -bordercolor white -border 0x0 -alpha remove $DATADIR/eth_indeg_addr.png


set times (for a in $DATADIR/indeg_dist/dist_contracts*
basename $a .dat
end | cut -d _ -f 3)
set ntimes (count $times)

begin
echo se te unknown
echo se xl \'contract indegree\'
echo se yl \'frequency\' off 1
echo se ytics format \'%g\'
echo se xr [1:]
echo se yr [1:2]
echo p 1/0 not
echo se auto y
echo se yr [:1e6]
echo se log xy
for k in (seq $ntimes)
set dt (date -d @$times[$k] -u +%Y)
echo rep \'$DATADIR/indeg_dist/dist_contracts_$times[$k].dat\' u \(\(\$1+\$2\)/2.0\):\(\$3/\(\$2-\$1\)\) w lp lw 3 pt 5 lc rgbcolor \'\#$colors[$k]\' t \'$dt\'
end

# note: fit done externally
echo a = -2.19452
echo b = 2e8
echo rep b\*x\*\*a w l lw 2 lc -1 not

echo se te post eps color solid size 3.2,2.2
echo se out \"$DATADIR/eth_indeg_contracts.eps\"
echo rep
echo se out
end | gnuplot ^ /dev/null

epstopdf $DATADIR/eth_indeg_contracts.eps
convert -density 300 $DATADIR/eth_indeg_contracts.eps -bordercolor white -border 0x0 -alpha remove $DATADIR/eth_indeg_contracts.png


