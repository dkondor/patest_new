/*
 * patest_gen.c -- preferential attachment tesztelése, Pósfai Marci által javasolt verzió:
 * 	minden él csatlakozásánál a fokszám visszatranszformálása az aktuális CDF-nek megfelelően,
 * 	ahol a CDF az adott fokszámú pontok választásának a valószínűségéből jön,
 * 	ezt d^a-nak vesszük, ahol az a paraméter állítható
 * ezzel egy d fokszámú pont választásának a valószínűsége: d^a * n_d, ahol n_d a d fokszámú pontok száma
 * 
 * Preferential attachment megvalósulásának a tesztelése a Bitcoin hálózatra
 * minden új linkre (két cím / felhasználó közötti első kapcsolatra) megnézzük, hogy abban a pillanatban
 * a felhasználóknak mik a tulajdonságai: fokszám, korábbi tranzakciók száma (esetleg csak egy időablakban),
 * egyenleg, esetleg továbbiak is
 * 
 * added test mode to output all operations on the red-black tree instead of carrying out the calculations
 * output:
 * 		type	degree	timestamp	is_contract
 * where:
 * 	type == 0 if degree should be decreased
 * 	type == 1 if degree should be increased
 * 
 * types 2 -- 5: transaction to a node with a given degree, rank should be calculated
 * 	type == 2 if a transaction is made between two existing nodes, creating a new edge
 * 	type == 3 if the transaction is made by a new node
 * 	type == 4 if the transaction is made on an existing (active) edge
 * 	type == 5 if the transaction is reacitvating a previously deactivated edge
 * 
 * additions for Ethereum, separate transactions with contracts
 * 	for types 0/1, is_contract is nonzero if it refers to a degree of a contract
 * 	for types >= 2, is_contract has the following meanings:
 * 		0 if both nodes are regular addresses
 * 		1 if the sender is a regular address and the receiver is a contract
 * 		2 if the sender is a contract and the receiver is a regular address
 * 		3 if both nodes are contracts
 * 
 * these are only output if info about contracts is provided
 * 
 * Copyright 2015-2020 Kondor Dániel <kondor.dani@gmail.com>
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
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "idlist.h"
#include "edges.h"
#include "edgeheap.h"
#include "read_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <time.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


typedef struct edgerecord_t { //egy bejegyzés az eléket tartalmazó fájlban
	unsigned int in;
	unsigned int out;
	unsigned int timestamp;
} erecord;

//egy új bejegyzés beolvasása (ha van még a fájlban)
static int erecord_read(read_table* f, erecord* r, int ignore_invalid) {
	while(1) {
		if(read_table_line(f)) return -1;
		
		unsigned int in,out,timestamp;
		if(read_table_uint32(f,&in) || read_table_uint32(f,&out)) {
			if(ignore_invalid && read_table_get_last_error(f) == T_OVERFLOW) continue;
			else return -1;
		}
		if(read_table_uint32(f,&timestamp)) return -1;
		r->in = in;
		r->out = out;
		r->timestamp = timestamp;
		return 0;
	}
}

static char gzip0[] = "/usr/bin/zip";
static char zcat[] = "/bin/zcat";

typedef struct erecord_bin_t {
	const erecord* e;
	uint64_t size;
	uint64_t ix;
	int fd;
} erecord_bin;

static int erecord_bin_open(erecord_bin* r, const char* fn) {
	int fd;
	struct stat tmp;
	uint64_t fs;
	void* map;
	
	fd = open(fn,O_CLOEXEC | O_RDONLY | O_NOATIME);
	if(fd == -1) return 1;
	if(fstat(fd,&tmp) == -1 || !tmp.st_size || tmp.st_size % sizeof(erecord)) {
		close(fd);
		return 1;
	}
	fs = tmp.st_size;
	r->size = fs / sizeof(erecord);
	map = mmap(0UL,fs,PROT_READ,MAP_SHARED,fd,0UL);
	if(map == MAP_FAILED) {
		close(fd);
		return 1;
	}
	r->e = (const erecord*)map;
	r->ix = 0;
	r->fd = fd;
	return 0;
}

static int erecord_bin_read(erecord_bin* r, erecord* e) {
	if(r->ix >= r->size) return 1;
	*e = r->e[r->ix];
	r->ix++;
	return 0;
}

static void erecord_bin_close(erecord_bin* r) {
	if(r->e) {
		munmap((void*)r->e, sizeof(erecord) * (r->size));
		r->e = NULL;
	}
	if(r->fd) {
		close(r->fd),
		r->fd = -1;
	}
}

int strtodint(char* a,unsigned int* delay) {
	char* a1 = 0;
	unsigned int delay2 = strtoul(a,&a1,10);
	if(a1 == a) {
		return 1;
		
	}
	if(*a1) { switch(*a1) {
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
			*delay = delay2*2592000;
			break;
		case 'y':
			*delay = delay2*31536000;
			break;
		default:
			*delay = delay2;
	} }
	else *delay = delay2;
	return 0;
}


int main(int argc, char **argv)
{
	/*
	 * bemeneti fájlok:
	 *  0. links -- összes előforduló él egyszer, címek szerint rendezve
	 * 	1. ids -- címek / felhasználók listája
	 * 	2. txedge -- összes első tranzakció, idő szerint rendezve (címek és időpont)
	 */
	unsigned int N = 0; //id-k száma
	char* fids = 0;
	char* ftxedge = 0;
	char* flinks = 0;
	int ignore_invalid = 1; /* ignore "invalid" node IDs that cause overflow / underflow (-1 for unknown addresses typically) */
	int edges_bin = 0; // read edges from binary file
	int have_contracts = 0; // include contracts (for Ethereum)
	
	erecord edge1;
	idlist* il = 0;
	erecord_bin eb = {NULL, 0, 0, -1};
	
	unsigned int delay = 2592000; //linkek élettartama (30 nap)
	edges* ee = 0;
	edgeheap eh;
	unsigned int nedges = 0; //élek száma
	uint64_t DE1 = 100000; //ennyi él feldolgozása után kiírás
	
	time_t t1 = time(0);
	time_t t2 = 0;
	time_t t3 = 0;
	int zip = 0; //bemeneti fájlok tömörítve
	int edgehelper = 0;

	uint64_t nedges2 = 0; //feldolgozott tranzakciók száma (többszörös élek többször)
	
	uint64_t DENEXT = DE1;
	
	int i,r = 0;
	for(i=1;i<argc;i++) {
		if(argv[i][0] == '-') switch(argv[i][1]) {
			case 'e':
			case 't':
				ftxedge = argv[i+1];
				i++;
				break;
			case 'l':
				flinks = argv[i+1];
				i++;
				break;
			case 'D':
				DE1 = atoi(argv[i+1]);
				DENEXT = DE1;
				i++;
				break;
			case 'i':
				fids = argv[i+1];
				i++;
				break;
			case 'N':
			case 'n':
				N = atoi(argv[i+1]);
				i++;
				break;
			case 'd':
				if(strtodint(argv[i+1],&delay)) fprintf(stderr,"Érvénytelen paraméter: %s %s!\n",argv[i],argv[i+1]);
				i++;
				break;
			case 'Z':
				zip = 1;
				break;
			case 'H':
				edgehelper = 1;
				break;
			case 'I':
				ignore_invalid = 0;
				break;
			case 'b':
				edges_bin = 1;
				break;
			case 'c':
				have_contracts = 1;
				break;
			default:
				fprintf(stderr,"Ismeretlen paraméter: %s!\n",argv[i]);
				break;
		}
		else fprintf(stderr,"Unknown parameter: %s!\n",argv[i]);
	}
	
	if( ! (ftxedge && fids && flinks) ) {
		fprintf(stderr,"Nincsenek bemeneti fájlok megadva!\n");
		return 1;
	}
	
	FILE* fi = 0;
	FILE* e = 0;
	FILE* links = 0;
	FILE* test_out = stdout;
	if(zip) {
		size_t l1 = strlen(fids);
		size_t l2 = strlen(flinks);
		size_t l3 = strlen(ftxedge);
		if(l2 > l1) l1 = l2;
		if(l3 > l1) l1 = l3;
		char* tmp = (char*)malloc(sizeof(char)*(l1 + strlen(zcat) + 4));
		if(!tmp) {
			fprintf(stderr,"Error allocating memory!\n");
			return 1;
		}
		sprintf(tmp,"%s %s",zcat,fids);
		fi = popen(tmp,"r");
		sprintf(tmp,"%s %s",zcat,flinks);
		links = popen(tmp,"r");
		if(!edges_bin) {
			sprintf(tmp,"%s %s",zcat,ftxedge);
			e = popen(tmp,"r");
		}
		free(tmp);
	}
	else {
		fi = fopen(fids,"r");
		links = fopen(flinks,"r");
		if(!edges_bin) e = fopen(ftxedge,"r");
	}
	
	if( ! (fi && links && (e || edges_bin)) ) {
		fprintf(stderr,"Error opening input files!\n");
		if(zip) {
			if(fi) pclose(fi);
			if(e) pclose(e);
			if(links) pclose(links);
		}
		else {
			if(fi) fclose(fi);
			if(e) fclose(e);
			if(links) fclose(links);
		}
		return 1;
	}
	read_table* e_rt = NULL;
	if(!edges_bin) {
		e_rt = read_table_new(e);
		read_table_set_fn(e_rt,ftxedge);
	}
	else {
		if(erecord_bin_open(&eb, ftxedge)) {
			fprintf(stderr,"Error opening input files!\n");
			if(zip) {
				if(fi) pclose(fi);
				if(links) pclose(links);
			}
			else {
				if(fi) fclose(fi);
				if(links) fclose(links);
			}
			return 1;
		}
	}
	
	/* count the different types of output */
	size_t ntypes[6] = {0,0,0,0,0,0};
	
	il = ids_read(fi, N, have_contracts);
	if(zip) pclose(fi);
	else fclose(fi);
	if(!il) {
		fprintf(stderr,"Nem sikerült azonosítókat beolvasni a(z) %s fájlból!\n",fids);
		if(zip) pclose(links);
		else fclose(links);
		r = 3;
		goto pt6_end;
	}
	N = il->N;
	
	ee = edges_read(links);
	if(zip) pclose(links);
	else fclose(links);
	if(!ee) {
		fprintf(stderr,"Nem sikerült az éleket beolvasni a(z) %s fájlból!\n",flinks);
		r = 2;
		goto pt6_end;
	}
	eh.e = ee; //ezt valahogy automatikussá kellene tenni (esetleg az edges tömböt teljesen integrálni az edgeheap-be)
	if(edgehelper) edges_createhelper(ee,il);
	
	t3 = time(0);
	r = edges_bin ? erecord_bin_read(&eb, &edge1) : erecord_read(e_rt,&edge1,ignore_invalid);
	if(r != 0) {
		fprintf(stderr,"Nem sikerült adatokat beolvasni a bemeneti fáljokból!\n");
		if(!edges_bin) read_table_write_error(e_rt,stderr);
		r = 8;
		goto pt6_end;
	}
	
	do {
		//új rekord az edge változóban, ezt kell feldolgozni, ehhez az rin és rout változókon kell iterálni, amíg el nem érjük a tranzakció időpontját
		unsigned int timestamp = edge1.timestamp;
		unsigned int time1 = 0;
		if(delay > 0 && timestamp > delay) time1 = timestamp - delay;
		
		//régi élek törlése
		r = 0;
		if(delay > 0) while(eh.hn) {
			unsigned int ts1 = ee->e[eh.heap[0]].timestamp;
			if(ts1 >= time1) break; //összes él újabb
			
			//az eh.heap[0] él régebbi, törölni kell
			//fokszámok csökkentése először
			unsigned int idin = ids_find2(il,ee->e[eh.heap[0]].p1); //változás a korábbi programhoz képest:
			unsigned int idout = ids_find2(il,ee->e[eh.heap[0]].p2); //az éleknél az eredeti ID-ket tároljuk
			if(idin >= il->N || idout >= il->N) { //ez itt nem fordulhat elő, az összes ID-nek szerepelnie kell a felsorolásban
				fprintf(stderr,"Hiba: inkonzisztens adatok (%u)!\n",__LINE__);
				break;
			}
			//~ r = deg2_del(d1,il->inlinks[idout]);
			if(il->inlinks[idout] == 0) { // || il->outlinks[idin] == 0) { //hiba
				fprintf(stderr,"Hiba: inkonzisztens adatok (%u)!\n",__LINE__);
				r = 1;
				break;
			}
			if(have_contracts) fprintf(test_out,"0\t%u\t%u\t%d\n",il->inlinks[idout],ts1+delay,(int)il->contract[idout]);
			else fprintf(test_out,"0\t%u\t%u\n",il->inlinks[idout],ts1+delay);
			ntypes[0]++;
			
			r = 0;
			il->inlinks[idout]--;
			
			eh.del0(); //heap átrendezése (legfelső elem törlése)
		}
		if(r) break;
		
		
		//feldolgoztuk a be- és kimenő összegeket, most vethetjük össze a statisztikákkal
		unsigned int idin = ids_find2(il,edge1.in);
		unsigned int idout = ids_find2(il,edge1.out);
		
		if(idin >= il->N || idout >= il->N) { //ez itt nem fordulhat elő, az összes ID-nek szerepelnie kell a felsorolásban
			fprintf(stderr,"Hiba: inkonzisztens adatok (%u)!\n",__LINE__);
			r = 1;
			break;
		}
	
		if(edge1.in != edge1.out) { //érvényes tranzakciót olvastunk be
			//a "cél" címmel foglalkozunk
			unsigned int indeg = il->inlinks[idout];
			unsigned int type = 2; /* default: new edge */
			//~ unsigned int outdeg = il->outlinks[idin];
			r = 0;
			int new1 = 0; //0, ha aktív az él, ekkor semmit sem kell csinálni
				//1, ha már szerepelt ez az él, de nem volt aktív (delay-nél nagyobb idő óta)
				//2, ha még nem szerepelt (timestamp == 0)
			
			uint64_t eid = edges_find(ee,edge1.in,edge1.out); //feltesszük, hogy ez az él még nem szerepelt
			if(eid >= ee->nedges) {
				fprintf(stderr,"Hiba: inkonzisztens adatok (%u)!\n",__LINE__);
				r = 1;
				break;
			}
			
			{ //időpont frissítése
				unsigned int t2 = ee->e[eid].timestamp;
				ee->e[eid].timestamp = timestamp;
				if(delay == 0) { if(t2 == 0) new1 = 2; } // új él
				else { // delay > 0, a heap-et is frissíteni kell
					if(t2 < time1) { //nem aktív ez az él
						r = eh.add(eid); //hozzá kell adni a heap-hoz
						if(r) break; //hiba
						if(t2 == 0) new1 = 2; //még egyszer sem volt aktív
						else new1 = 1; //korábban már aktív volt, de már deaktiváltuk
					}
					else { //még aktív, az időpontot kell csak frissíteni
						uint64_t off = ee->e[eid].offset;
						eh.heapdown(off); //csak lefelé mehet, az új timestamp nagyobb
					}
				}
			}
			
			nedges2++;
			
			if(il->outtx[idin] == 0) {
				/* new in node */
				if(new1 != 2) {
					fprintf(stderr,"Hiba: inkonzisztens adatok (%u)!\n",__LINE__);
					r = 1;
					break;
				}
				type = 3;
			}
			else switch(new1) {
				case 0:
					type = 4;
					break;
				case 1:
					type = 5;
					break;
				case 2:
					type = 2;
					break;
			}
			if(have_contracts) {
				int contract = 0;
				if(il->contract[idout]) contract += 1;
				if(il->contract[idin]) contract += 2;
				fprintf(test_out,"%u\t%u\t%u\t%d\n",type,indeg,timestamp,contract);
			}
			else fprintf(test_out,"%u\t%u\t%u\n",type,indeg,timestamp);
			ntypes[type]++;
			
			if(new1) { //inaktív él, fokszámok frissítése
				if(have_contracts) fprintf(test_out,"1\t%u\t%u\t%d\n",il->inlinks[idout],timestamp,(int)il->contract[idout]);
				else fprintf(test_out,"1\t%u\t%u\n",il->inlinks[idout],timestamp);
				il->inlinks[idout]++;
				il->outtx[idin]++;
				r = 0;
				ntypes[1]++;
			}
		}
		
		if(DE1) if(nedges2 >= DENEXT) {
			fprintf(stderr,"%lu él feldolgozva\n",nedges2);
			DENEXT = nedges2 + DE1;
		}
		
		//új tranzakció beolvasása
		r = edges_bin ? erecord_bin_read(&eb, &edge1) : erecord_read(e_rt,&edge1,ignore_invalid);
	} while(r == 0);
	
	if(!edges_bin && read_table_get_last_error(e_rt) != T_EOF) {
		fprintf(stderr,"Nem sikerült adatokat beolvasni a bemeneti fáljokból!\n");
		read_table_write_error(e_rt,stderr);
		r = 8;
	}
	else r = 0;
	fprintf(stderr,"feldolgozott élek: %u, tranzakciók: %lu\n",nedges,nedges2);
	t2 = time(0);
	fprintf(stderr,"futásidő: %u\n",(unsigned int)(t2-t1));
	fprintf(stderr,"futásidő: %u (adatok beolvasása: %u, feldolgozás: %u)\n",(unsigned int)(t2-t1),(unsigned int)(t3-t1),(unsigned int)(t2-t3));
	
	/* output */
	fprintf(stderr,"\ntype\tcount\n");
	for(i=0;i<6;i++) fprintf(stderr,"%d\t%lu\n",i,ntypes[i]);
	
pt6_end:
	
	if(!edges_bin) {
		if(zip) pclose(e);
		else fclose(e);
		read_table_free(e_rt);
	}
	else erecord_bin_close(&eb);
	
	if(il) ids_free(il);
	if(ee) edges_free(ee);
	
	return r;
}

