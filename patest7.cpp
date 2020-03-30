/*
 * patest7.cpp -- simple implementation of preferential attachment test
 * 	program, mainly to test new red-black tree implementation
 * 
 * limitation: edges have unlimited lifetime (node degrees only increase, do not decrease)
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
#include <string.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "read_table.h"
#include "orbtree.h"

template<class T, class U>
struct pair_hash {
	size_t operator()(const std::pair<T,U>& x) const {
		std::hash<T> h1;
		std::hash<U> h2;
		size_t y = h1(x.first);
		size_t z = h2(x.second);
		y ^= z + 0x9e3779b97f4a7c16UL + (y<<6) + (y>>2);
		return y;
	}
};


int main(int argc, char **argv)
{
	char* in = 0; /* input file with a list of (time-ordered) edges */
	std::vector<double> a;
	
	char* fout2 = 0; /* results go here (base filename) */
	bool foutn = false; /* if true, first edges are also written separately */
	bool out_zip = true;
	bool only_newedges = false;
	bool test_mode = false; /* if true, no calculations are done, insead the operations are written that would be done on the tree */
	
	size_t debug_out_next = 0;
	size_t debug_out = 0;
	
	for(int i=1;i<argc;i++) {
		if(argv[i][0] == '-') switch(argv[i][1]) {
			case 'i':
				in = argv[i+1];
				i++;
				break;
			case 'o':
				fout2 = argv[i+1];
				i++;
				break;
			case 'n':
				foutn = true;
				break;
			case 'a':
				for(i++;i<argc;i++) {
					if(!(isdigit(argv[i][0]) || argv[i][0] == '.')) break;
					a.push_back(atof(argv[i])); /* TODO: needs more error checking! */
				}
				i--;
				break;
			case '0':
				only_newedges = true;
				break;
			case 'D':
				debug_out = strtoul(argv[i+1],0,10);
				debug_out_next = debug_out;
				i++;
				break;
			case 'z':
				out_zip = false;
				break;
			case 'T':
				test_mode = true;
				break;
			default:
				fprintf(stderr,"Unknown parameter: %s!\n",argv[i]);
				break;
		}
		else fprintf(stderr,"Unknown parameter: %s!\n",argv[i]);
	}
	
	if(a.size() == 0 && !test_mode) {
		fprintf(stderr,"No exponents given!\n");
		return 1;
	}
	if(!fout2 && !test_mode) {
		fprintf(stderr,"No output base filename given!\n");
		return 1;
	}
	
	
	orbtree::NVPower2<unsigned int> p(a);
	
	std::unordered_map<unsigned int, unsigned int> deg; /* degrees */
	std::unordered_set<unsigned int> nodes_seen; /* source nodes that have been seen already */
	
	orbtree::orbmultisetC<unsigned int, orbtree::NVPower2<unsigned int>, uint32_t> rbtree( p );
	std::unordered_set<std::pair<unsigned int,unsigned int>, pair_hash<unsigned int,unsigned int> > edges_seen; /* already active edges */
	
	
	/* open input */
	read_table2 rt(in,stdin);
	if(rt.get_last_error() != T_OK) {
		fprintf(stderr,"Error opening input!\n");
		return 1;
	}
	
	std::vector<FILE*> out1;
	std::vector<FILE*> outn;
	
	/* open output files */
	if(!test_mode) {
		int seq = 1;
		size_t len1 = strlen(fout2);
		const char gzip0[] = "/bin/gzip -c > ";
		len1 += 50;
		char* tmp = (char*)malloc(sizeof(char)*len1);
		if(!tmp) {
			fprintf(stderr,"Error allocating memory!\n");
			return 1;
		}
		bool error = false;
		for(double aa : a) {
			int x;
			if(out_zip) x = snprintf(tmp,len1,"%s%s-%.2f-o-%d.dat.gz",gzip0,fout2,aa,seq);
			else x = snprintf(tmp,len1,"%s-%.2f-o-%d.dat",fout2,aa,seq);
			if(x < 0 || x >= len1) { error = true; break; }
			
			FILE* f;
			if(out_zip) f = popen(tmp,"w");
			else f = fopen(tmp,"w");
			if(!f) { error = true; break; }
			out1.push_back(f);
			
			if(foutn) {
				if(out_zip) x = snprintf(tmp,len1,"%s%s-%.2f-n-%d.dat.gz",gzip0,fout2,aa,seq);
				else x = snprintf(tmp,len1,"%s-%.2f-n-%d.dat",fout2,aa,seq);
				if(x < 0 || x >= len1) { error = true; break; }
				
				if(out_zip) f = popen(tmp,"w");
				else f = fopen(tmp,"w");
				if(!f) { error = true; break; }
				outn.push_back(f);
			}			
		}
		
		free(tmp);
		
		if(error) {
			fprintf(stderr,"Error opening output files!\n");
			for(FILE* f : out1) if(out_zip) pclose(f); else fclose(f);
			for(FILE* f : outn) if(out_zip) pclose(f); else fclose(f);
			return 1;
		}
	}
	
	
	int ret = 0;
	double cdf[a.size()];
	double norm[a.size()];
	
	size_t ee = 0;
	while(rt.read_line()) {
		/* input: directed edge (with timestamp) */
		unsigned int n1,n2,ts;
		if(!rt.read(n1,n2,ts)) break;
		ee++;
		bool newedge = edges_seen.insert(std::make_pair(n1,n2)).second;
		if(only_newedges && !newedge) continue;
		
		/* get degree of target + get degree of source, so we know if it's a new node or not */
		unsigned int indeg = deg[n2]; /* will add new node with zero degree if necessary */
		
		if(test_mode) {
			if(newedge) {
				if(indeg) fprintf(stdout,"%ld\n",(-1L)*indeg);
				fprintf(stdout,"%u\n",indeg+1);
				deg[n2] = indeg+1;
			}
			continue;
		}
		
		bool newnode = false;
		if(foutn) if(nodes_seen.count(n1) == 0) {
			newnode = true;
			nodes_seen.insert(n1);
		}
		if(indeg) {
			auto it = rbtree.lower_bound(indeg);
			if(it == rbtree.end() || *it != indeg) {
				fprintf(stderr,"Inconsistent rbtree()!\n");
				ret = 2;
				break;
			}
			rbtree.get_sum_fv(indeg,cdf);
			rbtree.get_norm_fv(norm);
			for(size_t i=0;i<a.size();i++) {
				if(norm[i] <= 0.0) {
					fprintf(stderr,"Inconsistent tree: sum of values <= 0!\n");
					ret = 3;
					break;
				}
				cdf[i] /= norm[i];
			}
			if(ret) break;
			if(newedge) rbtree.erase(it);
		}
		else for(size_t i=0;i<a.size();i++) cdf[i] = 0.0;
		if(newnode) for(size_t i=0;i<outn.size();i++) fprintf(outn[i],"%g\n",cdf[i]);
		else for(size_t i=0;i<out1.size();i++) fprintf(out1[i],"%g\n",cdf[i]);
		
		/* increase indegree of target node */
		if(newedge) {
			rbtree.insert(indeg+1);
			deg[n2] = indeg+1;
		}
		
		if(debug_out && ee > debug_out_next) {
			fprintf(stderr,"%lu edges processed\n",ee);
			debug_out_next += debug_out;
		}
	}
	if(rt.get_last_error() != T_EOF) {
		fprintf(stderr,"Error reading input:\n");
		rt.write_error(stderr);
		ret = 1;
	}
	
	/* close output files */
	for(FILE* f : out1) if(out_zip) pclose(f); else fclose(f);
	for(FILE* f : outn) if(out_zip) pclose(f); else fclose(f);
	
	return ret;
}

