/*
 * patest_balances.cpp -- preferential attachment tesztelése a címekhez bejövő összegekre
 *	egyenlegek / bejövő összegek nyilvántartása az új red-black tree-vel
 * 	egyelőre csak egy exponensre lehet egyszerre futtatni
 * 
 * Copyright 2015-2020 Kondor Dániel <kondor.dani@gmail.com>
 * 
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
 * 
 */

/*
#include "idlist.h"
#include "red_black_tree2.h"
#include "popen_noshell.h"
*/

#include "orbtree.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <time.h>

#include <unordered_map>

#include "read_table.h"

struct set_pow {
	double operator () (int64_t y, double a) const {
		double x = (double)(y >= 0L ? y : 0L);
		return pow(x,a);
	}
};

typedef orbtree::rankmultisetC<int64_t> ranktree;
/*
orbtree::orbtree< orbtree::NodeAllocatorCompact<  orbtree::KeyOnly<int64_t>, uint32_t, uint32_t, 0>,
	std::less<int64_t>, orbtree::RankFunc2<int64_t, uint32_t>, true > ranktree; */
typedef orbtree::orbmultisetC<int64_t, orbtree::NVPower2<int64_t> > exptree; /*
typedef orbtree::orbtree< orbtree::NodeAllocatorCompact<orbtree::KeyOnly<int64_t>, double, uint32_t, 0>,
		std::less<int64_t>, orbtree::NVFunc_Adapter_Vec<set_pow,double,int64_t, double >, true > exptree; */

/* maps -- for more compact representation */
struct map_rank {
	unsigned int operator () (const std::pair<int64_t, unsigned int>& p) const { return p.second; }
	typedef std::pair<int64_t, unsigned int> argument_type;
	typedef unsigned int result_type;
};
struct map_pow {
	double operator () (const std::pair<int64_t, unsigned int>&p, double a) const {
		double x = (double)(p.first >= 0L ? p.first : 0L);
		double cnt = (double)(p.second);
		return cnt*pow(x,a);
	}
};

typedef orbtree::simple_mapC<int64_t, unsigned int, map_rank> rankmap;
typedef orbtree::orbmapC<int64_t, unsigned int, orbtree::NVPowerMulti2<std::pair<int64_t, unsigned int> > > expmap;

/*
typedef orbtree::orbtree< orbtree::NodeAllocatorCompact<orbtree::KeyValue<int64_t, unsigned int>, uint32_t, uint32_t, 0>,
		std::less<int64_t>, orbtree::NVFunc_Adapter_Simple<map_rank, uint32_t, std::pair<int64_t, unsigned int> >,
		false > rankmap;
typedef orbtree::orbtree< orbtree::NodeAllocatorCompact<orbtree::KeyValue<int64_t, unsigned int>, double, uint32_t, 0>,
		std::less<int64_t>, orbtree::NVFunc_Adapter_Vec<map_pow,double,std::pair<int64_t,unsigned int>, double >,
		false > expmap;
*/


template<class tree>
inline void remove_balance_tree(tree& t, int64_t bal) {
	auto it = t.find(bal);
	if(it == t.end()) throw std::runtime_error("degree not found!\n");
	t.erase(it);
}
template<class tree>
inline void add_balance_tree(tree& t, int64_t bal) {
	t.insert(bal);
}

template<class tree>
inline void remove_balance_map(tree& t, int64_t bal) {
	auto it = t.find(bal);
	if(it == t.end()) throw std::runtime_error("balance not found!\n");
	unsigned int cnt1 = it->second;
	if(cnt1 == 1) t.erase(it);
	else it.set_value(cnt1-1);
}
template<class tree>
inline void add_balance_map(tree& t, int64_t bal) {
	/* try to insert */
	auto res = t.insert(orbtree::trivial_pair<int64_t,unsigned int>(bal,1U));
	if(!res.second) {
		/* already exists, add one */
		auto it = res.first;
		unsigned int cnt1 = it->second;
		it.set_value(cnt1+1);
	}
}



template<class tree, class T>
inline void get_ranks(tree& t, int64_t bal, T* rank, T* cdf) {
	auto it = t.lower_bound(bal);
	if(it == t.end() || it.key() != bal) throw std::runtime_error("degree not found!\n");
	t.get_sum_node(it,rank);
	t.get_norm_fv(cdf);
}

struct record { //egy bejegyzés a bemeneti fájlokban (txint.txt és txoutt.txt)
	uint64_t txid;
	int64_t id;
	uint64_t value;
	//~ unsigned int timestamp; -- not used
};

bool read_in(read_table2& rt, record& r) {
	while(rt.read_line()) {
		uint64_t prev_txid;
		uint32_t seq, prev_seq;
		bool rr = rt.read(r.txid,seq,prev_txid,prev_seq,r.id,r.value);
		if(!rr) break;
		if(r.id >= 0) return true; /* we skip address IDs < 0 -- not identified */
	}
	return false;
}

bool read_out(read_table2& rt, record& r) {
	while(rt.read_line()) {
		uint32_t seq;
		bool rr = rt.read(r.txid,seq,r.id,r.value);
		if(!rr) break;
		if(r.id >= 0) return true;
	}
	return false;
}


static char gzip0[] = "/bin/gzip -c";
static char zcat[] = "/bin/zcat";
static char xzcat[] = "/usr/bin/xzcat";


int main(int argc, char **argv)
{
	/*
	 * bemeneti fájlok:
	 * 	2. txout -- tranzakciók kimenetei (címekhez bejövő összegek)
	 * 	3. txin -- tranzakciók bemenetei (címektől kimenő összegek -- ha nincs megadva, akkor
	 * 		csak a címekhez kumulatíven beérkező összegeket tartjuk számon)
	 */
	char* fids = 0;
	char* ftxin = 0;
	char* ftxout = 0;
	
	char* fout2 = 0; //eredmények kiírása ide -> alapfájlnév, ebből generálás: %s-%f-n/a.gz
	char* gzip = gzip0; //tömörítéshez használt program, ha nincs megadva, akkor tömörítetlen kimenet
	
	size_t DE1 = 100000; //ennyi bemenet feldolgozása után kiírás
	size_t DENEXT;
		
	
	std::vector<double> a; /* exponents to use -- if none is given, only ranks are output (to stdout) */
	
	double a0 = 1.0;

	time_t t1 = time(0);
	bool zip = true; //kimeneti fájlok tömörítve
	bool in_zip = false; //bemeneti fájlok tömörítve
	bool in_xz = false; //input is compressed with xz
	
	int64_t thres = 1; //csak az e feletti összegekkel foglalkozunk
	
	std::vector<int64_t> excl; //a pontosan az ebben szereplő egyenleggel rendelkező címeket kihagyjuk az eloszlásból

	bool only_generate = false; /* if true, only generate "events", i.e. changes in balances */
	bool read_events = false; /* if true, instead of reading transactions, read "events" from stdin */
	bool use_map = false;
	bool forget_old = false; /* if true, "forget" addresses with zero balance (to limit the size of hashtable) */
	
	bool histogram_output = false;
	double histogram_bins = 0.0001;
	
	
	int i,r = 0;
	for(i=1;i<argc;i++) if(argv[i][0] == '-') switch(argv[i][1]) {
		case 'D':
			DE1 = atoi(argv[i+1]);
			i++;
			break;
		case 'i':
			ftxin = argv[i+1];
			i++;
			break;
		case 'o':
			ftxout = argv[i+1];
			i++;
			break;
		case 'O':
			fout2 = argv[i+1];
			i++;
			break;
		case 'z':
			zip = false;
			break;
		case 'Z':
			in_zip = true;
			break;
		case 'X':
			in_xz = true;
			break;
		case 'a':
				for( ; i+1 < argc && (isdigit(argv[i+1][0]) || argv[i+1][0] == '.'); i++ ) {
					char* tmp = argv[i+1];
					double a1 = strtod(argv[i+1],&tmp);
					if(tmp == argv[i+1]) { fprintf(stderr,"Invalid exponent value: %s!\n",argv[i+1]); break; }
					a.push_back(a1);
				}
				break;
		case 't':
			thres = strtoll(argv[i+1],0,10);
			i++;
			break;
		case 'f':
			forget_old = true;
			break;
		case 'e':
			{
				unsigned int j = i+1;
				for(;j<argc;j++) {
					if(!isdigit(argv[j][0])) break;
					char* tmp2 = 0;
					int64_t tmp = strtoll(argv[j],&tmp2,10);
					if(tmp2 == argv[j] || tmp < 0) {
						fprintf(stderr,"Hiba: érvénytelen paraméterek!\n");
						excl.clear();
						break;
					}
					excl.push_back(tmp);
				}
				i = j-1;
				break;
			}
			break;
		case 'g':
			only_generate = true;
			break;
		case 'r':
			read_events = true;
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
			fprintf(stderr,"Ismeretlen paraméter: %s!\n",argv[i]);
			break;
	}
	else fprintf(stderr,"Ismeretlen paraméter: %s!\n",argv[i]);
	
	if(read_events) {
		if(only_generate) {
			fprintf(stderr,"Inconsistent parameter combination: -g and -r should not be used together!\n");
			return 1;
		}
		if(ftxin || ftxout) fprintf(stderr,"Warning: input file names ignored!\n");
		if(ftxin) ftxin = 0;
		if(ftxout) ftxout = 0;
	}
	if(histogram_output) zip = !zip;
	
	DENEXT = DE1;
	
	char* fntmp = 0;
	if(in_zip || in_xz) {
		unsigned int l1 = ftxin ? strlen(ftxin) : 0;
		unsigned int l2 = ftxout ? strlen(ftxout) : 0;
		if(l2 > l1) l1 = l2;
		
		if(l1) {
			fntmp = (char*)malloc(sizeof(char)*(l1 + strlen(xzcat) + 4));
			if(!fntmp) {
				fprintf(stderr,"Nem sikerült memóriát lefoglalni!\n");
				return 2;
			}
		}
	}
	
	/* trees -- only one is used depending if exponents are given or not (a has >0 elements) */
	orbtree::NVPower2<int64_t> p(a);
	orbtree::NVPowerMulti2<std::pair<int64_t, unsigned int> > p2(a);
	ranktree rt;
	rankmap rmap;
	exptree et(p);
	expmap emap(p2);
	
	
	FILE* in = 0;
	if(ftxin) {
		if(in_zip || in_xz) {
			sprintf(fntmp,"%s %s",in_xz ? xzcat : zcat,ftxin);
			in = popen(fntmp,"r");
		}
		else in = fopen(ftxin,"r");
	}
	FILE* out = 0;
	if(ftxout) {
		if(in_zip || in_xz) {
			sprintf(fntmp,"%s %s",in_xz ? xzcat : zcat,ftxout);
			out = popen(fntmp,"r");
		}
		else out = fopen(ftxout,"r");
	}
	else out = stdin;
	if(fntmp) free(fntmp);
	
	//kimeneti fájlok -- minden exponenshez külön fájl, tömörítve
	FILE** out1 = 0;
	if(a.size()) {
		bool err = false;
		const char* outf_base = fout2;
		char* tmp = (char*)malloc( sizeof(char) * ( strlen(gzip) + strlen(outf_base) + 40 ) );
		if(!tmp) { fprintf(stderr,"Error allocating memory!\n"); return 1; }
		out1 = (FILE**)malloc(sizeof(FILE*)*a.size());
		if(!out1) { fprintf(stderr,"Error allocating memory!\n"); free(tmp); return 1; }
		for(size_t i=0;i<a.size();i++) {
			double a1 = a[i];
			if(zip) {
				sprintf(tmp,"%s > %s-%.2f.dat.gz",gzip,outf_base,a1);
				out1[i] = popen(tmp,"w");
			}
			else {
				sprintf(tmp,"%s-%.2f.dat",outf_base,a1);
				out1[i] = fopen(tmp,"w");
			}
			if(!out1[i]) { err = true; break; }
			if(err) break;
		}
		free(tmp);
		
		if(err) {
			fprintf(stderr,"Error opening output files!\n");
			for(size_t i=0;i<a.size();i++) {
				FILE* f = out1[i];
				if(f) {
					if(zip) pclose(f);
					else fclose(f);
				}
			}
			free(out1);
			return 1;
		}
	}
	
	
	size_t txin = 0;
	size_t txout = 0;
	
	read_table2 rtin(in);
	read_table2 rtout(out);
	if(ftxin) rtin.set_fn(ftxin);
	if(ftxout) rtout.set_fn(ftxout);
	else rtout.set_fn("<stdin>");
	
	record rin,rout;
	bool has_in = (ftxin != 0);
	if(has_in) has_in = read_in(rtin,rin);
	bool has_out = read_events ? true : read_out(rtout,rout);
	
	/* keep track of address balances */
	std::unordered_map<uint64_t,int64_t> balances;
	FILE* generate_out = stdout;
	
	std::vector<std::vector<uint64_t> > histograms;
	std::vector<std::vector<uint64_t> > histograms2;
	std::vector<uint64_t> cnts;
	std::vector<uint64_t> cnts2;
	if(histogram_output) {
		size_t nbins = (size_t)ceil(1.0 / histogram_bins);
		histograms.resize(a.size());
		histograms2.resize(a.size());
		cnts.resize(a.size(),0UL);
		cnts2.resize(a.size(),0UL);
		for(std::vector<uint64_t>& h : histograms) h.resize(nbins,0UL);
		for(std::vector<uint64_t>& h : histograms2) h.resize(nbins,0UL);
	}
	
	
	while(has_out) {
		
		int64_t old_bal = 0;
		int64_t new_bal;
		uint64_t txid;
		bool txout_event = true;
		
		if(read_events) {
			if(!rtout.read_line()) break;
			if(!rtout.read(old_bal,new_bal)) break;
			txout++;
		}
		else {
			if(has_in && rin.txid <= rout.txid) {
				auto tmp = balances.insert(std::pair<uint64_t,int64_t>((uint64_t)(rin.id),-rin.value));
				if(!tmp.second) {
					/* already seen this address */
					old_bal = tmp.first->second;
					new_bal = old_bal - rin.value;
					if(new_bal == 0L && forget_old) balances.erase(tmp.first);
					else tmp.first->second = new_bal;
				}
				else new_bal = -rin.value;
				txin++;
				txid = rin.txid;
				has_in = read_in(rtin,rin);
				txout_event = false;
			}
			else {
				auto tmp = balances.insert(std::pair<uint64_t,int64_t>((uint64_t)(rout.id),rout.value));
				if(!tmp.second) {
					/* already seen this address */
					old_bal = tmp.first->second;
					new_bal = old_bal + rout.value;
					tmp.first->second = new_bal;
				}
				else new_bal = rout.value;
				txout++;
				txid = rout.txid;
				has_out = read_out(rtout,rout);
			}
		}
		
		if(only_generate) fprintf(generate_out,"%ld\t%ld\t%lu\n",old_bal,new_bal,txid);
		else {
			if(old_bal > thres) {
				bool skip = false;
				for(auto x : excl) if(old_bal == x) { skip = true; break; }
				if(!skip) {
					/* calculate rank of target balance (only if balance increases and target balance is not excluded) */
					if(new_bal >= old_bal && txout_event) {
						if(a.size()) {
							double rank[a.size()];
							double cdf[a.size()];
							if(use_map) get_ranks(emap,old_bal,rank,cdf);
							else get_ranks(et,old_bal,rank,cdf);
							if(!old_bal) for(double& x : rank) x = 0.0;
							else for(size_t i=0;i<a.size();i++) rank[i] /= cdf[i];
							
							uint64_t change = new_bal - old_bal;
							for(size_t i=0;i<a.size();i++) {
								if(histogram_output) {
									double r1 = rank[i];
									if(r1 < 0.0 || r1 > 1.0) throw std::runtime_error("Invalid rank!\n");
									size_t b = (size_t)floor(r1 / histogram_bins);
									histograms[i][b]++;
									cnts[i]++;
									/* note: this requires that total transaction volume 
									 * is < 2^64 Satoshis
									 * check for overflow here */
									uint64_t d1 = std::numeric_limits<uint64_t>::max() - change;
									if(cnts2[i] > d1) throw std::runtime_error("Overflow in histogram values!\n");
									histograms2[i][b] += change;
									cnts2[i] += change;
								}
								else {
									FILE* f = out1[i];
									fprintf(f,"%ld\t%ld\t%f\t%lu\n",new_bal-old_bal,old_bal,rank[i],txid);
								}
							}
						}
						else {
							unsigned int rank = 0,cdf;
							if(use_map) get_ranks(rmap,old_bal,&rank,&cdf);
							else get_ranks(rt,old_bal,&rank,&cdf);
							fprintf(stdout,"%ld\t%ld\t%u\t%u\t%lu\n",new_bal-old_bal,old_bal,rank,cdf,txid);
						}
					}
					/* remove the old balance */
					if(use_map) {
						if(a.size()) remove_balance_map(emap,old_bal);
						else remove_balance_map(rmap,old_bal);
					}
					else {
						if(a.size()) remove_balance_tree(et,old_bal);
						else remove_balance_tree(rt,old_bal);
					}
				}
			}
			
			/* update the balance change -- only if above threshold and not excluded */
			if(new_bal > thres) {
				bool skip = false;
				for(auto x : excl) if(new_bal == x) { skip = true; break; }
				if(!skip) {
					if(use_map) {
						if(a.size()) add_balance_map(emap,new_bal);
						else add_balance_map(rmap,new_bal);
					}
					else {
						if(a.size()) add_balance_tree(et,new_bal);
						else add_balance_tree(rt,new_bal);
					}
				}
			}
		}
		
		if(DE1) if(txout >= DENEXT) {
			fprintf(stderr,"%lu kimenet és %lu bemenet feldolgozva\n",txout,txin);
			DENEXT = txout + DE1;
		}
	}
	
	r = 0;
	if(rtout.get_last_error() != T_EOF) {
		fprintf(stderr,"Error reading input:\n");
		rtout.write_error(stderr);
		r = 1;
	}
	if(in && !( rtin.get_last_error() == T_EOF || rtin.get_last_error() == T_OK ) ) {
		fprintf(stderr,"Error reading input:\n");
		rtin.write_error(stderr);
		r = 1;
	}
	
	if(in) {
		if(in_zip || in_xz) pclose(in);
		else fclose(in);
	}
	if(out && out != stdin) {
		if(in_zip || in_xz) pclose(out);
		else fclose(out);
	}
	
	if(a.size()) {
		/* close output files or write output */
		if(histogram_output) {
			for(size_t i=0;i<a.size();i++) {
				size_t nbins = (size_t)ceil(1.0 / histogram_bins);
				for(size_t j=0;j<nbins;j++) fprintf(out1[i],"%f\t%lu\t%lu\t%lu\t%lu\n", histogram_bins*((double)j),
					histograms[i][j], cnts[i], histograms2[i][j], cnts2[i]);
			}
		}
		for(size_t i=0;i<a.size();i++) {
			if(zip) pclose(out1[i]);
			else fclose(out1[i]);
		}
		free(out1);
	}
	
	if(read_events) fprintf(stderr,"feldolgozott események: %lu\n",txout);
	else fprintf(stderr,"feldolgozott bemenetek: %lu, kimenetek: %lu\n",txin,txout);
	time_t t2 = time(0);
	fprintf(stderr,"futásidő: %u\n",(unsigned int)(t2-t1));
	
	return r;
}


