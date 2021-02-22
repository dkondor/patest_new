# patest_new
Programs to test preferential attachment in a growing network (new implementation in c++).

This repository contains code and scripts that can be used to test for the presence of preferential attachment in growing networks such as Bitcoin and Ethereum. This code was used to create the main results in our upcoming paper:
```
Kondor D, Bulatovic N, Stéger J, Csabai I, Vattay G (2021).
The rich still get richer: Empirical comparison of preferential attachment via linking statistics in Bitcoin and Ethereum.
Under review
```

## Programs included

This repository includes various code, organized under the following subdirectories:
- patestgen: Code that reads a set of edges and nodes, and creates a set of ''events'' that can be used to generate transformed rank statistics.
- patestrun: Code that calculates transformed ranks for a set of events (`patest_ranks.cpp`) or for a set of transaction inputs and outputs, based on the balance distribution (`patest_balances.cpp`). Both are based on a [custom binary tree implementation](https://github.com/dkondor/orbtree) that allows the efficient computation of partial sums of a function over ordered sets and maps.
- misc: Additional code used during preprocessing and programs to calculate the indegree and balance distribution of the networks at given time intervals.

## Scripts included

- compile_programs.sh: Simple commands to compile all C++ code with GCC. Feel free to open an issue if you run into any issues or if you feel that using a build system should be necessary.

### Bitcoin
 
- bitcoin_preprocess.sh: Preprocessing needed to generate a list of edges from the list of transaction inputs and outputs.
- bitcoin_patest_run.fsh: Main computations for calculating the transformed rank statistics.
- bitcoin_figs.fsh: Script to generate the main figures in the paper and many additional variants.

### Ethereum

- eth_download.sh: Script to download the Ethereum dataset used in our paper from Zenodo.
- eth_preprocess.sh: Script to preprocess the Ethereum dataset to create properly sorted lists of edges.
- eth_patest_run.fsh: Main computations for calculating the transformed rank statistics for Ethereum.
- eth_figs.fsh: Script to generate the main figure in the paper and many additional variants.

Note: all scripts could be run as a whole. It might still make sense to run them step-by-step, especially the main processing scripts that calculate many variants of the main research question. Some of the steps could be run in parallel, if there is enough memory.


## Requirements

- At least 64 GiB memory to process the Bitcoin network, at least 4 GiB for the Ethereum network. 
- At least 150 GiB free disk space for the Bitcoin network (including all the raw and processed data and results); at least 20 GiB free disk space for the Ethereum network.
- A C++ compiler that supports the C++14 standard. [GCC](https://gcc.gnu.org/) is used in the example scripts, but other compilers should work as well. Feel free to open an issues if your compiler does not work. GCC is typically available as the `g++` package on Linux distributions including recent versions of Ubuntu.
- The [bash shell](http://tiswww.case.edu/php/chet/bash/bashtop.html), typically the default shell on many Linux distributions or available as a package.
- The [fish shell](https://fishshell.com/), typically available as a package (`fish`) on most Linux distributions.
- [awk](https://www.gnu.org/software/gawk/) and [mawk](https://invisible-island.net/mawk/), typically installed by default on most Linux distributions. If `mawk` is not available on your system, feel free to replace it with `awk` everywhere (`mawk` is only used as a performance optimization).

The above packages should be available by default or via Homebrew on MacOS as well. On Windows, it is recommended to use [WSL](https://docs.microsoft.com/en-us/windows/wsl/install-win10).

## Additional requirements for creating figures

The following programs are only required if you use the included scripts to create figures (i.e. `bitcoin_figs.fsh` and `eth_figs.fsh`). The main analysis can be run without these, and you can use any other software to create figures.

- [gnuplot](http://gnuplot.info/)
- The `epstopdf` script, included in the `texlive-font-utils` package on Ubuntu.
- [ImageMagick](https://imagemagick.org/), the `convert` utility is used to create PNG figures


