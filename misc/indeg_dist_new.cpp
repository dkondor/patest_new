/*
 * indeg_dist_new.cpp -- track an evolving network, write out the indegree
 * 	distributions at given intervals
 * 
 * use the output of the patest_gen.c program
 * 
 * Copyright 2020 Daniel Kondor <kondor.dani@gmail.com>
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
#include <map>
#include "read_table.h"

template<class cont>
void write_deg_dist(FILE* fout, const cont& deg_dist, unsigned int ts) {
	for(const auto& x : deg_dist) fprintf(fout, "%u\t%lu\t%lu\n", ts, x.first, x.second);
}

int strtodint(char* a,unsigned int* delay) {
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

int main(int argc, char **argv)
{
	/* write out degree distribution this often (default: 1 year) */
	unsigned int interval = 31536000;
	unsigned int tsnext = 0; /* starting time to write out distributions */
	bool require_sorted = true;
	for(int i = 1; i < argc; i++) if(argv[i][0] == '-') switch(argv[i][1]) {
		case 'i':
			if(strtodint(argv[i+1], &interval))
				fprintf(stderr, "Invalid value for time interval: %s!\n", argv[i+1]);
			i++;
			break;
		case 's':
			tsnext = atoi(argv[i+1]);
			i++;
			break;
		case 'S':
			require_sorted = false;
			break;
		default:
			fprintf(stderr, "Invalid value for time interval: %s!\n", argv[i+1]);
			break;
	}
	else fprintf(stderr, "Unknown parameter: %s!\n", argv[i]);
	
	/* degree distribution -- note: we don't keep track of nodes with zero degree */
	std::map<uint64_t, uint64_t> deg_dist;
	read_table2 rt(stdin);
	unsigned int ts = 0; /* current time */
	bool first = true;
	while(rt.read_line()) {
		unsigned int type, ts1;
		uint64_t deg;
		if(!rt.read(type, deg, ts1)) break;
		if(!tsnext) tsnext = ts1 + interval;
		
		if(require_sorted) {
			if(ts1 < ts) throw std::runtime_error("Input not sorted!\n");
			ts = ts1;
		}
		
		while(tsnext < ts1) {
			if(!first) write_deg_dist(stdout, deg_dist, tsnext);
			tsnext += interval;
		}
		first = false;
		
		uint64_t next_deg;
		switch(type) {
			case 0:
				/* degree should be decreased */
				if(!deg) throw std::runtime_error("Cannot decrease zero degree!\n");
				next_deg = deg - 1;
				break;
			case 1:
				next_deg = deg + 1;
				break;
			case 2:
			case 3:
			case 4:
			case 5:
				continue; /* don't care */
			default:
				throw std::runtime_error("Invalid input!\n");
		}
		if(deg > 0) {
			auto it = deg_dist.find(deg);
			if(it == deg_dist.end() || it->second == 0) throw std::runtime_error("Degree not found!\n");
			it->second--;
		}
		if(next_deg > 0) {
			auto res = deg_dist.insert(std::pair<uint64_t, uint64_t>(next_deg, 1));
			if(!res.second) res.first->second++;
		}
	}
	if(rt.get_last_error() != T_EOF) {
		rt.write_error(stderr);
		return 1;
	}
	
	write_deg_dist(stdout, deg_dist, tsnext);
	
	return 0;
}

