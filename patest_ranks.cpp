/*
 * patest_ranks.cpp -- calculating ranks using red-black tree,
 * 	reading previously established "operations" from standard input
 * 
 * format of input:
 * 		type	degree
 * where:
 * 	type == 0 if degree should be decreased
 * 	type == 1 if degree should be increased
 * 	type == 2 if a transaction is made to a node with degree (rank should be calculated)
 * 	type == 3 if the transaction is made by a new node
 * 	type == 4 if the transaction is made on an existing (active) edge
 * 	type == 5 if the transaction is reacitvating a previously deactivated edge
 * 
 * types 2-5 are written to separate output files for all exponents
 * (types 0 and 1 are used only to update degrees stored in the tree)
 * 
 * Copyright 2019 Daniel Kondor <kondor.dani@gmail.com>
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above
 *   copyright notice, this list of conditions and the following disclaimer
 *   in the documentation and/or other materials provided with the
 *   distribution.
 * * Neither the name of the  nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * 
 */


#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unordered_map>
#include "read_table.h"
#include "orbtree.h"

/* trees */
typedef orbtree::rankmultisetC<unsigned int, unsigned int> ranktree;
typedef orbtree::orbmultisetC<unsigned int, orbtree::NVPower2<unsigned int>, uint32_t> exptree;
		
		
/* maps -- for more compact representation */
struct map_rank {
	typedef std::pair<unsigned int,unsigned int> argument_type;
	typedef unsigned int result_type;
	unsigned int operator () (const std::pair<unsigned int, unsigned int>& p) const { return p.second; }
};
struct map_pow {
	typedef std::pair<unsigned int,unsigned int> argument_type;
	typedef double result_type;
	typedef double ParType;
	double operator () (const std::pair<unsigned int, unsigned int>&p, double a) const {
		double x = (double)(p.first);
		double cnt = (double)(p.second);
		return cnt*pow(x,a);
	}
};

typedef orbtree::orbmapC<unsigned int, unsigned int, orbtree::NVFunc_Adapter_Simple<map_rank>, uint32_t> rankmap;
typedef orbtree::orbmapC<unsigned int, unsigned int, orbtree::NVFunc_Adapter_Vec<map_pow>, uint32_t> expmap;

template<class tree>
inline void change_deg_tree(tree& t, unsigned int old_deg, unsigned int new_deg) {
	if(old_deg) {
		auto it = t.find(old_deg);
		if(it == t.end()) throw std::runtime_error("degree not found!\n");
		t.erase(it);
	}
	t.insert(new_deg);
}

template<class tree>
inline void change_deg_map(tree& t, unsigned int old_deg, unsigned int new_deg) {
	if(old_deg) {
		auto it = t.find(old_deg);
		if(it == t.end()) throw std::runtime_error("degree not found!\n");
		unsigned int cnt1 = it->second;
		if(cnt1 == 1) t.erase(it);
		else it.set_value(cnt1-1);
	}

	/* try to insert */
	auto res = t.insert(orbtree::trivial_pair<unsigned int,unsigned int>(new_deg,1U));
	if(!res.second) {
		/* already exists, add one */
		auto it = res.first;
		unsigned int cnt1 = it->second;
		it.set_value(cnt1+1);
	}
}


template<class tree, class T>
inline void get_ranks(tree& t, unsigned int deg, T* rank, T* cdf) {
	if(deg) {
		auto it = t.lower_bound(deg);
		if(it == t.end() || it.key() != deg) throw std::runtime_error("degree not found!\n");
		t.get_sum_node(it,rank);
	}
	t.get_norm_fv(cdf);
}


int main(int argc, char **argv)
{
	char* outf_base = 0; /* output base filename */
	std::vector<double> a; /* exponents to use -- if none is given, only ranks are output (to stdout) */
	bool zip = true; /* should compress output files */
	size_t debug_out = 0;
	size_t debug_out_next = 0;
	bool use_map = false;
	
	bool histogram_output = false;
	double histogram_bins = 0.0001;
	
	for(int i=1;i<argc;i++) {
		if(argv[i][0] == '-') switch(argv[i][1]) {
			case 'a':
				for( ; i+1 < argc && (isdigit(argv[i+1][0]) || argv[i+1][0] == '.'); i++ ) {
					char* tmp = argv[i+1];
					double a1 = strtod(argv[i+1],&tmp);
					if(tmp == argv[i+1]) { fprintf(stderr,"Invalid exponent value: %s!\n",argv[i+1]); break; }
					a.push_back(a1);
				}
				break;
			case 'o':
				outf_base = argv[i+1];
				i++;
				break;
			case 'z':
				zip = false;
				break;
			case 'D':
				debug_out = strtoul(argv[i+1],0,10);
				debug_out_next = debug_out;
				i++;
				break;
			case 'm':
				use_map = true;
				break;
			case 'h':
				histogram_bins = atof(argv[i+1]);
				i++;
			case 'H':
				histogram_output = true;
				break;
			default:
				fprintf(stderr,"Unknown parameter: %s!\n",argv[i]);
				break;
			
		}
		else fprintf(stderr,"Unknown parameter: %s!\n",argv[i]);
	}
	
	/* open output files */
	/* map type ids to output file indexes */
	const  unsigned int ntypes = 4;
	const  std::unordered_map<unsigned int,unsigned int> types = { {2,0}, {3,1}, {4,2}, {5,3} };
	const  char gzip[] = "/bin/gzip -c";
	
	if(histogram_output) zip = !zip;
	
	FILE** out = 0;
	if(a.size()) {
		bool err = false;
		char* tmp = (char*)malloc( sizeof(char) * ( strlen(gzip) + strlen(outf_base) + 40 ) );
		if(!tmp) { fprintf(stderr,"Error allocating memory!\n"); return 1; }
		out = (FILE**)malloc(sizeof(FILE*)*ntypes*a.size());
		if(!out) { fprintf(stderr,"Error allocating memory!\n"); free(tmp); return 1; }
		for(size_t i=0;i<a.size();i++) {
			double a1 = a[i];
			for(auto t : types) {
				if(zip) {
					sprintf(tmp,"%s > %s-%.2f-%u.dat.gz",gzip,outf_base,a1,t.first);
					out[i*ntypes + t.second] = popen(tmp,"w");
				}
				else {
					sprintf(tmp,"%s-%.2f-%u.dat",outf_base,a1,t.first);
					out[i*ntypes + t.second] = fopen(tmp,"w");
				}
				if(!out[i*ntypes + t.second]) { err = true; break; }
			}
			if(err) break;
		}
		free(tmp);
		
		if(err) {
			fprintf(stderr,"Error opening output files!\n");
			for(size_t i=0;i<ntypes*a.size();i++) {
				FILE* f = out[i];
				if(f) {
					if(zip) pclose(f);
					else fclose(f);
				}
			}
			free(out);
			return 1;
		}
	}
	
	
	/* trees -- only one is used depending if exponents are given or not (a has >0 elements) */
	orbtree::NVPower2<unsigned int> p(a);
	orbtree::NVFunc_Adapter_Vec<map_pow> p2(a);
	ranktree rt;
	rankmap rmap;
	exptree et(p);
	expmap emap(p2);
	
	read_table2 rt2(stdin);
	
	size_t lines = 0;
	size_t l1 = 0;
	size_t l2 = 0;
	
	std::vector<std::vector<uint64_t> > histograms;
	std::vector<uint64_t> cnts;
	if(histogram_output) {
		size_t nbins = (size_t)ceil(1.0 / histogram_bins);
		histograms.resize(ntypes * a.size());
		cnts.resize(ntypes * a.size(),0UL);
		for(std::vector<uint64_t>& h : histograms) h.resize(nbins,0UL);
	}
	
	while(rt2.read_line()) {
		unsigned int type, deg;
		if(!rt2.read( type, deg )) break;
		
		if(type == 0 || type == 1) {
			/* decrease / increase degree */
			unsigned int old_deg = deg;
			unsigned int new_deg;
			if(type == 0) {
				if(!old_deg) throw std::runtime_error("Invalid input: cannot decrease zero degree!\n");
				new_deg = old_deg-1;
			}
			else new_deg = old_deg+1;
			
			if(use_map) {
				if(a.size()) change_deg_map(emap,old_deg,new_deg);
				else change_deg_map(rmap,old_deg,new_deg);
			}
			else {
				if(a.size()) change_deg_tree(et,old_deg,new_deg);
				else change_deg_tree(rt,old_deg,new_deg);
			}
			l1++;
		}
		else {
			/* calculate rank, write output */
			unsigned int o = types.at(type); /* will throw if type is invalid */
			
			if(a.size()) {
				double rank[a.size()];
				double cdf[a.size()];
				if(use_map) get_ranks(emap,deg,rank,cdf);
				else get_ranks(et,deg,rank,cdf);
				if(!deg) for(double& x : rank) x = 0.0;
				else for(size_t i=0;i<a.size();i++) rank[i] /= cdf[i];
								
				for(size_t i=0;i<a.size();i++) {
					size_t idx = i*ntypes + o;
					if(histogram_output) {
						double r1 = rank[i];
						if(r1 < 0.0 || r1 > 1.0) throw std::runtime_error("Invalid rank!\n");
						size_t b = (size_t)floor(r1 / histogram_bins);
						histograms[idx][b]++;
						cnts[idx]++;
					}
					else {
						FILE* f = out[idx];
						fprintf(f,"%g\n",rank[i]);
					}
				}
			}
			else {
				unsigned int rank = 0,cdf;
				if(use_map) get_ranks(rmap,deg,&rank,&cdf);
				else get_ranks(rt,deg,&rank,&cdf);
				fprintf(stdout,"%u\t%u\t%u\t%u\n",type,deg,rank,cdf);
			}
			l2++;
		}
		lines++;
		
		if(debug_out && lines >= debug_out_next) {
			fprintf(stderr,"%lu lines processed, %lu degree changes, %lu rank calculations\n",lines,l1,l2);
			debug_out_next += debug_out;
		}
	}
	
	if(rt2.get_last_error() != T_EOF) rt2.write_error(stderr);
	else fprintf(stderr,"%lu lines processed, %lu degree changes, %lu rank calculations\n",lines,l1,l2);
	
	if(a.size()) {
		/* close output files or write output */
		if(histogram_output) {
			for(size_t i=0;i<ntypes*a.size();i++) {
				size_t nbins = (size_t)ceil(1.0 / histogram_bins);
				for(size_t j=0;j<nbins;j++) fprintf(out[i],"%f\t%lu\t%lu\n",histogram_bins*((double)j),
					histograms[i][j],cnts[i]);
			}
		}
		for(size_t i=0;i<ntypes*a.size();i++) {
			FILE* f = out[i];
			if(zip) pclose(f);
			else fclose(f);
		}
		free(out);
	}
	
	return 0;
}

