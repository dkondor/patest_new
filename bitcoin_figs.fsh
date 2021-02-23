#!/usr/bin/fish
# bitcoin_figs.fsh
# create figures of main results for Bitcoin

# note: this script uses the fish shell syntax (https://fishshell.com/, https://github.com/fish-shell/fish-shell)
# also, this script is supposed to be run in the main directory of this repository

# data directory (contains the results of preprocessing); adjust as necessary
set DATADIR blockchain20200207_process

# exponents to test for -- should be the same as in bitcoin_patest_run.fsh
set AA 0.00 0.25 0.50 0.70 0.85 1.00 1.15 1.30 1.50
set colors 5aa732 005c96 e75216 009ace ffd500 8671a7 e78502 db6aa2 007939 9a2846 815234


# types of results
set titles 'new edges from new nodes' 'new edges from existing nodes' 'transactions on existing edges'
set types 3 2 4

set datalen 'Bitcoin, unlimited' 'Bitcoin, 30 day lifetime' 'Bitcoin, 1 day lifetime'
set basename $DATADIR/patest_run_output/ptrm $DATADIR/patest_run_output_30day/ptrm $DATADIR/patest_run_output_1day/ptrm


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
# 1. CDF figures for distributions over the full lifetime of Bitcoin
for i in 1 2 3
set outdir $basename[$i]-figs
mkdir -p $outdir
for j in 1 2 3
set y1 (head -n 1 $basename[$i]-summary-$types[$j].out | cut -f 2)
begin
echo se te unknown
echo se ke off
echo se yr [0:1]
echo se xr [0:1]
echo se xl \'CDF\'
echo se yl \'calculated rank\' offset 1
echo p 1/0
for k in (seq (count $AA))
echo rep \'$basename[$i]-$AA[$k]-$types[$j].cdf\' u 2:1 w l lw 3 lc rgbcolor \'\#$colors[$k]\' t \'a = $AA[$k]\'
end
echo rep \(x - $y1\) / \(1 - $y1\) w l lw 2 lc -1 not

echo se title \'$datalen[$i], $titles[$j]\'
echo se te post eps color solid size 3.2,2.1
echo se out \'$outdir/bitcoin_$types[$j].eps\'
echo rep
echo se out
end | gnuplot ^ /dev/null

epstopdf $outdir/bitcoin_$types[$j].eps
convert -density 300 $outdir/bitcoin_$types[$j].eps -bordercolor white -border 0x0 -alpha remove $outdir/bitcoin_$types[$j].png

end
end


# 1.2. figure with K-S difference as a function of the exponent
for i in 1 2 3
set outdir $basename[$i]-figs
for j in 1 2 3
begin
echo se te unknown
echo se yr [0:]
echo se xr [-0.05:1.55]
echo se xtics 0.5
echo se ytics 0.1
echo se xl \'exponent\'
echo se yl \'K-S difference\' off 1.5
echo p \'$basename[$i]-summary-$types[$j].out\' u 1:3 w lp lw 2 pt 5 not
echo se title \'$titles[$j]\' off -2,0
echo se te post eps color solid size 1.6, 1.8
echo se out \'$outdir/bitcoin_exp_$types[$j].eps\'
echo rep
echo se out
end | gnuplot ^ /dev/null
epstopdf $outdir/bitcoin_exp_$types[$j].eps
convert -density 300 $outdir/bitcoin_exp_$types[$j].eps -bordercolor white -border 0x0 -alpha remove $outdir/bitcoin_exp_$types[$j].png
end
end



########################################################################
# 2. CDF of transformed ranks in shorter (i.e. 6 month) intervals
# each interval is a separate figure

set basename2 $DATADIR/patest_run_output_6m/ptrm $DATADIR/patest_run_output_30day_6m/ptrm $DATADIR/patest_run_output_1day_6m/ptrm

for i2 in 1 2 3
set outdir $basename2[$i2]-figs
mkdir -p $outdir

set times (cut -f 1 $basename2[$i2]_times.dat)
set ntimes (count $times)

for j in (seq $ntimes)
for i in 1 2 3

set y1 (head -n 1 $basename2[$i2]-$times[$j]-summary-$types[$i].out | cut -f 2)
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
echo rep \'$basename2[$i2]-$times[$j]-$AA[$k]-$types[$i].cdf\' u 2:1 w l lw 3 lc rgbcolor \'\#$colors[$k]\' t \'a = $AA[$k]\'
end
echo rep \(x - $y1\) / \(1 - $y1\) w l lw 2 lc -1 not

echo se title \'$datalen[$i2], $dt, $titles[$i]\'
echo se te post eps color solid size 3.2,2.1
echo se out \'$outdir/bitcoin_$times[$j]_$types[$i].eps\'
echo rep
echo se out
end | gnuplot ^ /dev/null

epstopdf $outdir/bitcoin_$times[$j]_$types[$i].eps
convert -density 300 $outdir/bitcoin_$times[$j]_$types[$i].eps -bordercolor white -border 0x0 -alpha remove $outdir/bitcoin_$times[$j]_$types[$i].png

end
end
end


# 2.2. figure with K-S difference as a function of the exponent
for i2 in 1 2 3
set outdir $basename2[$i2]-figs
set times (cut -f 1 $basename2[$i2]_times.dat)
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
echo p \'$basename2[$i2]-$times[$j]-summary-$types[$i].out\' u 1:3 w lp lw 2 pt 5 not
echo se title \'$dt\' off -2,0
echo se te post eps color solid size 1.6, 1.8
echo se out \'$outdir/bitcoin_exp_$times[$j]_$types[$i].eps\'
echo rep
echo se out
echo se te qt
end | gnuplot  ^ /dev/null

epstopdf $outdir/bitcoin_exp_$times[$j]_$types[$i].eps
convert -density 300 $outdir/bitcoin_exp_$times[$j]_$types[$i].eps -bordercolor white -border 0x0 -alpha remove $outdir/bitcoin_exp_$times[$j]_$types[$i].png
end
end
end

# 2.3. figures with the plots for exponents together for some of the times
# indices of times to include
set ixt 5 8 10 12 14 16 18 20 22

for i2 in 1 2 3
set outdir $basename2[$i2]-figs
set times (cut -f 1 $basename2[$i2]_times.dat)

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

for k in (seq (count $ixt))
set j $ixt[$k]
set dt (date -d @$times[$j] -u -I)
echo rep \'$basename2[$i2]-$times[$j]-summary-$types[$i].out\' u 1:3 w lp lw 2 pt 5 lc rgbcolor \'#$colors[$k]\' not
end
echo se title \'$titles[$i]\' off -2,0
echo se auto y
echo se yr [0:]
echo se te post eps color solid size 2.1, 2.1
echo se out \'$outdir/bitcoin_exp_multi_$types[$i].eps\'
echo rep
echo se out
end | gnuplot ^ /dev/null

epstopdf $outdir/bitcoin_exp_multi_$types[$i].eps
convert -density 300 $outdir/bitcoin_exp_multi_$types[$i].eps -bordercolor white -border 0x0 -alpha remove $outdir/bitcoin_exp_multi_$types[$i].png
end
end


# common legend for the above figures -- this could be edited by e.g. Inkscape to cut the actual legend
for i2 in 1 2 3
set outdir $basename2[$i2]-figs
set times (cut -f 1 $basename2[$i2]_times.dat)
begin
echo se te unknown
echo se ke below
echo se xr [0:1]
echo se yr [0:1]
echo p 1/0 not
for k in (seq (count $ixt))
set j $ixt[$k]
set dt (date -d @$times[$j] -u -I)
echo rep 1/0 w lp lw 2 pt 5 lc rgbcolor \'#$colors[$k]\' t \'$dt\'
end
echo se te post eps color solid size 6.4, 3.2
echo se out \'$outdir/bitcoin_exp_multi_legend.eps\'
echo rep
echo se out
echo se te qt
end | gnuplot ^ /dev/null

epstopdf $outdir/bitcoin_exp_multi_legend.eps
convert -density 300 $outdir/bitcoin_exp_multi_legend.eps -bordercolor white -border 0x0 -alpha remove $outdir/bitcoin_exp_multi_legend.png
end



########################################################################
# 3. preferential attachment testing for balance dynamics

# 3.1. CDF of transformed ranks
set outdir $DATADIR/ptb_run/ptb-figs
mkdir -p $outdir

# types of results:
# 1. unweighted (i.e. each transaction is counted as one event), all addresses included in the balance distribution
# 2. unweighted, addresses with exact mining rewards are excluded from the balance distribution
# 3. weighted (each event is weighted by the amount transferred), all addresses included in the balance distribution
# 4. weighted, addresses with exact mining rewards are excluded from the balance distribution
# the last one is included in the paper
set titles 'Bitcoin, balances, unweighted' 'Bitcoin, balances, unweighted' 'Bitcoin, balances, weighted' 'Bitcoin, balances, weighted'
set names ptb1 ptb1e ptb1 ptb1e
set col 2 2 3 3
set outfn 1 1e 1w 1ew

for i in 1 2 3 4
set y1 (head -n 1 $DATADIR/ptb_run/$names[$i]-summary.out | cut -f 2)
begin
echo se te unknown
echo se ke off
echo se xl \'CDF\'
echo se yl \'calculated rank\' off 1
echo se xr [0:1]
echo se yr [0:1]
echo p 1 / 0 not
for k in (seq (count $AA))
echo rep \'$DATADIR/ptb_run/$names[$i]-$AA[$k].cdf\' u $col[$i]:1 w l lw 3 lc rgbcolor \'\#$colors[$k]\' t \'a = $AA[$k]\'
end
echo rep \(x - $y1\) / \(1 - $y1\) w l lw 2 lc -1 not
echo se title \'$titles[$i]\'

echo se te post eps color solid size 3.2,2.1
echo se out \'$outdir/bitcoin_balances_$outfn[$i].eps\'
echo rep
echo se out
end | gnuplot ^ /dev/null

epstopdf $outdir/bitcoin_balances_$outfn[$i].eps
convert -density 300 $outdir/bitcoin_balances_$outfn[$i].eps -bordercolor white -border 0x0 -alpha remove $outdir/bitcoin_balances_$outfn[$i].png
end



########################################################################
# 4. Indegree distributions
set times (for a in $DATADIR/indeg_dists/*
basename $a .dat
end | cut -d _ -f 2)
set ntimes (count $times)

# dates to display
set ixt 1 2 3 5 7 9 12

begin
echo se te unknown
echo se xl \'indegree\'
echo se yl \'frequency\' off 1
echo se ytics format \'%g\'
echo se log xy
echo se xr [1:]
echo se yr [1:2]
echo p 1/0 not
echo se auto y
echo se yr [:1e9]
for k in (seq (count $ixt))
set dt (date -d @$times[$ixt[$k]] -u +%Y)
echo rep \'$DATADIR/indeg_dists/dist_$times[$ixt[$k]].dat\' u \(\(\$1+\$2\)/2.0\):\(\$3/\(\$2-\$1\)\) w lp lw 3 pt 5 lc rgbcolor \'\#$colors[$k]\' t \'$dt\'
end

# note: fit done with an external utility
echo a = -2.68064
echo b = 3e11
echo rep b\*x\*\*a w l lw 2 lc -1 not

echo se te post eps color solid size 3.2,2.2
echo se out \"$DATADIR/bitcoin_indeg.eps\"
echo rep
echo se out
end | gnuplot ^ /dev/null
epstopdf $DATADIR/bitcoin_indeg.eps
convert -density 300 $DATADIR/bitcoin_indeg.eps -bordercolor white -border 0x0 -alpha remove $DATADIR/bitcoin_indeg.png



########################################################################
# 5. Balance distributions

set times (for a in $DATADIR/balance_dists/*
basename $a .dat
end | cut -d _ -f 2)
set ntimes (count $times)

# dates to display
set ixt 2 3 5 7 9 11

begin
echo se te unknown
echo se xl \'balance [BTC]\'
echo se yl \'frequency\' off 1
echo se ytics format \'%g\'
echo se log xy
echo se xr [1:]
echo se yr [1:2]
echo p 1/0 not
echo se auto y
for k in (seq (count $ixt))
set dt (date -d @$times[$ixt[$k]] -u +%Y)
echo rep \'$DATADIR/balance_dists/dist_$times[$ixt[$k]].dat\' u \(\(\$1+\$2\)/2e8\):\(\$3 \> 0 \? \(1e8\*\$3/\(\$2-\$1\)\) : 1/0\) w lp lw 3 pt 5 lc rgbcolor \'\#$colors[$k]\' t \'$dt\'
end

echo se te post eps color solid size 3.2,2.2
echo se out \"$DATADIR/bitcoin_balances.eps\"
echo rep
echo se out
end | gnuplot ^ /dev/null
epstopdf $DATADIR/bitcoin_balances.eps
convert -density 300 $DATADIR/bitcoin_balances.eps -bordercolor white -border 0x0 -alpha remove $DATADIR/bitcoin_balances.png


