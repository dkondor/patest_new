/*
 * balance_dists.cpp -- calculate balance distributions at given points in time
 * 
 * Copyright 2020 Kondor Dániel <kondor.dani@gmail.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <time.h>

#include <map>
#include <unordered_map>

#include "read_table.h"


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

static int strtodint(char* a,unsigned int* delay) {
	char* a1 = 0;
	unsigned int delay2 = strtoul(a,&a1,10);
	if(a1 == a) {
		return 1;
		
	}
	if(*a1) switch(*a1) {
		case 'h':
			*delay = delay2*3600;
			break;
		case 'd':
			*delay = delay2*86400;
			break;
		case 'w':
			*delay = delay2*604800;
			break;
		case 'm':
			/* note: "average" length of a month */
			*delay = delay2*2628000;
			break;
		case 'y':
			*delay = delay2*31536000;
			break;
		default:
			*delay = delay2;
	}
	else *delay = delay2;
	return 0;
}

template<class cont>
static void write_deg_dist(FILE* fout, const cont& deg_dist, unsigned int ts) {
	for(const auto& x : deg_dist) fprintf(fout, "%u\t%ld\t%lu\n", ts, x.first, x.second);
}

int main(int argc, char **argv)
{
	/*
	 * bemeneti fájlok:
	 * 	2. txout -- tranzakciók kimenetei (címekhez bejövő összegek)
	 * 	3. txin -- tranzakciók bemenetei (címektől kimenő összegek -- ha nincs megadva, akkor
	 * 		csak a címekhez kumulatíven beérkező összegeket tartjuk számon)
	 */
	char* ftxin = 0;
	char* ftxout = 0;
	char* ftxtime = 0;
	
	char* gzip = gzip0; //tömörítéshez használt program, ha nincs megadva, akkor tömörítetlen kimenet
	
	size_t DE1 = 100000; //ennyi bemenet feldolgozása után kiírás
	size_t DENEXT;
		
	
	time_t t1 = time(0);
	bool in_zip = false; //bemeneti fájlok tömörítve
	
	/* write out balance distribution this often (default: 1 year) */
	unsigned int interval = 31536000;
	
	int i,r = 0;
	for(i=1;i<argc;i++) if(argv[i][0] == '-') switch(argv[i][1]) {
		case 'I':
			if(strtodint(argv[i+1], &interval))
				fprintf(stderr, "Invalid value for time interval: %s!\n", argv[i+1]);
			i++;
			break;
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
		case 'Z':
			in_zip = true;
			break;
		case 't':
			ftxtime = argv[i+1];
			i++;
			break;
		default:
			fprintf(stderr,"Ismeretlen paraméter: %s!\n",argv[i]);
			break;
	}
	else fprintf(stderr,"Ismeretlen paraméter: %s!\n",argv[i]);
	
	DENEXT = DE1;
	
	char* fntmp = 0;
	if(in_zip) {
		unsigned int l1 = ftxin ? strlen(ftxin) : 0;
		unsigned int l2 = ftxout ? strlen(ftxout) : 0;
		unsigned int l3 = ftxtime ? strlen(ftxtime) : 0;
		if(l2 > l1) l1 = l2;
		if(l3 > l1) l1 = l3;
		
		if(l1) {
			fntmp = (char*)malloc(sizeof(char)*(l1 + strlen(zcat) + 4));
			if(!fntmp) {
				fprintf(stderr,"Nem sikerült memóriát lefoglalni!\n");
				return 2;
			}
		}
	}
	
	FILE* in = 0;
	if(ftxin) {
		if(in_zip) {
			sprintf(fntmp,"%s %s",zcat,ftxin);
			in = popen(fntmp,"r");
		}
		else in = fopen(ftxin,"r");
	}
	else in = stdin;
	FILE* out = 0;
	if(ftxout) {
		if(in_zip) {
			sprintf(fntmp,"%s %s",zcat,ftxout);
			out = popen(fntmp,"r");
		}
		else out = fopen(ftxout,"r");
	}
	else out = stdin;
	FILE* ftt = 0;
	if(ftxtime) {
		if(in_zip) {
			sprintf(fntmp,"%s %s",zcat,ftxtime);
			ftt = popen(fntmp,"r");
		}
	}
	else ftt = stdout;
	if(fntmp) free(fntmp);
	
	size_t txin = 0;
	size_t txout = 0;
	
	read_table2 rtin(in);
	read_table2 rtout(out);
	read_table2 rttime(ftt);
	if(ftxin) rtin.set_fn(ftxin);
	else rtin.set_fn("<stdin>");
	if(ftxout) rtout.set_fn(ftxout);
	else rtout.set_fn("<stdin>");
	if(ftxtime) rttime.set_fn(ftxtime);
	else rttime.set_fn("<stdin>");
	
	record rin,rout;
	bool has_in = (ftxin != 0);
	if(has_in) has_in = read_in(rtin,rin);
	bool has_out = read_out(rtout,rout);
	
	/* keep track of address balances */
	std::unordered_map<uint64_t, int64_t> balances;
	std::map<int64_t, uint64_t> balance_hist;
	
	unsigned int txtime = 0;
	uint64_t txtime_id = 0;
	unsigned int tsnext = 0;
	
	if(!rttime.read_line()) throw std::runtime_error("Transaction timestamp file ended too early!\n");
	if(!rttime.read(txtime_id, txtime)) throw std::runtime_error("Transaction timestamp file ended too early!\n");
	tsnext = txtime + interval;
	
	while(has_out) {
		int64_t old_bal = 0;
		int64_t new_bal;
		uint64_t txid;
		bool txout_event = true;
		
		if(has_in && rin.txid <= rout.txid) {
			auto tmp = balances.insert(std::pair<uint64_t,int64_t>((uint64_t)(rin.id),-rin.value));
			if(!tmp.second) {
				/* already seen this address */
				old_bal = tmp.first->second;
				new_bal = old_bal - rin.value;
				if(new_bal == 0L) balances.erase(tmp.first);
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
		
		while(txtime_id < txid) {
			if(!rttime.read_line()) throw std::runtime_error("Transaction timestamp file ended too early!\n");
			if(!rttime.read(txtime_id, txtime)) throw std::runtime_error("Transaction timestamp file ended too early!\n");
			if(txtime_id > txid) throw std::runtime_error("Transaction timestamp not found!\n");
		}
		
		while(txtime > tsnext) {
			write_deg_dist(stdout, balance_hist, tsnext);
			tsnext += interval;
		}
		
		if(old_bal > 0) {
			auto it = balance_hist.find(old_bal);
			if(it == balance_hist.end() || it->second == 0) throw std::runtime_error("Balance not found!\n");
			it->second--;
		}
		
		if(new_bal > 0) {
			auto res = balance_hist.insert(std::pair<int64_t, uint64_t>(new_bal, 1));
			if(!res.second) res.first->second++;
		}
		
		if(DE1) if(txout >= DENEXT) {
			fprintf(stderr,"%lu kimenet és %lu bemenet feldolgozva\n",txout,txin);
			DENEXT = txout + DE1;
		}
	}
	
	write_deg_dist(stdout, balance_hist, tsnext);
	
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
		if(in_zip) pclose(in);
		else fclose(in);
	}
	if(out && out != stdin) {
		if(in_zip) pclose(out);
		else fclose(out);
	}
	
	fprintf(stderr,"feldolgozott bemenetek: %lu, kimenetek: %lu\n",txin,txout);
	time_t t2 = time(0);
	fprintf(stderr,"futásidő: %u\n",(unsigned int)(t2-t1));
	
	return r;
}


