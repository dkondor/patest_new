/*
 * edges.cpp -- élek effektív tárolása: lista az id-k szerint rendezve,
 * 	ebben gyorsan lehet keresni + binary heap az időpontok szerint, így
 * 	a legrégebbit gyorsan ki lehet választani
 * 
 * Copyright 2013 Kondor Dániel <dani@thinkpad-r9r>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */


#include "edges.h"
#include "read_table.h"

/*****************************************
 * élek rendezett tárolása (idő és élek) *
 *****************************************/

static const uint64_t pagesize = 4096;

//két él felcserélése (csak az ID-k, a sorbarendezés még a feldolgozás előtt történik)
static inline void edges_swap(edge* e, uint64_t i, uint64_t j) {
	unsigned int p1 = e[i].p1;
	unsigned int p2 = e[i].p2;
	e[i].p1 = e[j].p1;
	e[i].p2 = e[j].p2;
	e[j].p1 = p1;
	e[j].p2 = p2;
}

//quicksort az élek ID-k szerinti sorbarendezésére
static void edges_sort(edge* d, uint64_t s, uint64_t e) {
	if(s >= e) return;
	if(e-s == 1) return;
	if(e-s == 2) {
		if(cmpedge2(d,s,s+1) < 0) edges_swap(d,s,s+1);
		return;
	}
	
	uint64_t p = s + (e-s)/2;
	uint64_t i;
	edges_swap(d,p,e-1);
	p = s;
	for(i=s;i<e-1;i++) {
		int a = cmpedge2(d,e-1,i);
		if(a == -1 || (a == 0 && p<s+(e-s)/2) ) {
			if(i>p) edges_swap(d,i,p);
			p++;
		}
	}
	if(p < e-1) edges_swap(d,p,e-1);
	edges_sort(d,s,p);
	edges_sort(d,p+1,e);
}

void edges_sort1(edges* e) {
	if(!e) return;
	if(!(e->e)) return;
	edges_sort(e->e,0,e->nedges);
}



//idő szerinti rendezés
static inline int edges_tcmp(edge* e, uint64_t i, uint64_t j) {
	if(e[i].timestamp < e[j].timestamp) return 1;
	if(e[i].timestamp > e[j].timestamp) return -1;
	uint64_t eh1 = edgehash(e,i);
	uint64_t eh2 = edgehash(e,j);
	if(eh1 < eh2) return 1;
	if(eh1 > eh2) return -1;
	return 0;
}

//két él felcserélése (csak az ID-k, a sorbarendezés még a feldolgozás előtt történik)
static inline void edges_tswap(edge* e, uint64_t i, uint64_t j) {
	unsigned int p1 = e[i].p1;
	unsigned int p2 = e[i].p2;
	unsigned int t = e[i].timestamp;
	e[i].p1 = e[j].p1;
	e[i].p2 = e[j].p2;
	e[i].timestamp = e[j].timestamp;
	e[j].p1 = p1;
	e[j].p2 = p2;
	e[j].timestamp = t;
}

//quicksort az élek idő szerinti sorbarendezésére
static void edges_tsort(edge* d, uint64_t s, uint64_t e) {
	if(s >= e) return;
	if(e-s == 1) return;
	if(e-s == 2) {
		if(edges_tcmp(d,s,s+1) < 0) edges_tswap(d,s,s+1);
		return;
	}
	
	uint64_t p = s + (e-s)/2;
	uint64_t i;
	edges_tswap(d,p,e-1);
	p = s;
	for(i=s;i<e-1;i++) {
		int a = edges_tcmp(d,e-1,i);
		if(a == -1 || (a == 0 && i<s+(e-s)/2) ) {
			if(i>p) edges_tswap(d,i,p);
			p++;
		}
	}
	if(p < e-1) edges_tswap(d,p,e-1);
	edges_tsort(d,s,p);
	edges_tsort(d,p+1,e);
}

void edges_tsort1(edges* e) {
	if(!e) return;
	if(!(e->e)) return;
	edges_tsort(e->e,0,e->nedges);
}


const static uint64_t grow_size0 = 4194304UL; //4M darab él, 16k page, 64MiB memória
static edges* edges_grow0(edges* e, uint64_t size1) {
	edges* e2 = e;
	if(!size1) {
		if(e) size1 = e->edges_grow;
		else size1 = grow_size0;
	}
	uint64_t s1 = pagesize/sizeof(edge); //!! feltesszük, hogy a pagesize osztható az élek méretével (a jelenlegi változatban teljesül)
	uint64_t s2 = size1%s1;
	if(s2) {
		size1 -= s2;
		size1 += s1;
	}
	
	if(!e) {
		e = (edges*)malloc(sizeof(edges));
		if(!e) return 0;
		e->e = (edge*)MAP_FAILED;
		e->edges_size = 0;
		e->edges_grow = grow_size0;
		e->h = 0;
	}
	uint64_t map_size = (size1)*sizeof(edge);
	uint64_t old_size = (e->edges_size)*sizeof(edge);
	if(e->e != MAP_FAILED) {
		if(e->edges_size == 0) {
			fprintf(stderr,"edges_grow(): hibás adatok!\n");
			return 0; //hiba
		}
		void* ptr2 = mremap(e->e,old_size,old_size+map_size,MREMAP_MAYMOVE);
		if(ptr2 == MAP_FAILED) {
			fprintf(stderr,"edges_grow(): nincs elég memória!\n");
			return 0;
		}
		e->e = (edge*)ptr2;
		e->edges_size += size1;
	}
	else {
		void* ptr2 = mmap(0,map_size,PROT_READ | PROT_WRITE,MAP_ANONYMOUS | MAP_PRIVATE,-1,0);
		if(ptr2 == MAP_FAILED) {
			fprintf(stderr,"edges_grow(): nincs elég memória!\n");
			if(!e2) free(e);
			return 0;
		}
		e->e = (edge*)ptr2;
		e->edges_size = size1;
		e->nedges = 0;
	}
	return e;
}

edges* edges_grow(edges* e) {
	return edges_grow0(e,0);
}

void edges_free(edges* e) {
	if(e) {
		if(e->e != MAP_FAILED) {
			if(e->edges_size == 0) {
				fprintf(stderr,"edges_free(): hibás adatok!\n");
			}
			munmap(e->e,(e->edges_size)*sizeof(edge));
			e->e = (edge*)MAP_FAILED;
			e->edges_size = 0;
		}
		if(e->h) {
			if(e->h->index) free(e->h->index);
			if(e->h->outdeg) free(e->h->outdeg);
			free(e->h);
		}
		free(e);
	}
}

//élek beolvasása egy fájlból
/*beolvasás általánosan:
	flags értékei:
#define EFLAGS_T1 1 //időpontokat is beolvasunk és idő szerint rendezzük az eredményt (ha nincs megadva, akkor csak
	//éleket olvasunk be és minden élből csak egyet tartunk meg)
#define EFLAGS_SELF 2 //megengedünk önmagába mutató éleket (p1->p1)
#define EFLAGS_TXID 4 //tranzakció ID-ket is beolvasunk (legutolsó oszlop, az offset tömbbe tároljuk el) 
#define EFLAGS_ERROR_OVERFLOW 8 // overflow hibát jelent (különben csak kihagyjuk)
*/
edges* edges_read0(FILE* f, int flags) {
	edges* e = edges_grow(0);
	if(!e) {
		fprintf(stderr,"edges_read(): nem sikerült memóriát lefoglalni!\n");
		return 0;
	}
	
	read_table rt;
	read_table_init(&rt,f);
	read_table_set_comment(&rt,'#');
	
	//bemeneti fájl: egy sorban két ID csak
	uint64_t hiba = 0;
	uint64_t mh = 20;
	uint64_t sor = 0;
	uint64_t last = 0; //legutóbbi beolvasott él (minden él csak egyszer lehet)
	uint32_t tlast = 0; //legutóbbi időpont
	int sorted = 1; //ha nincs rendezve a fájl, akkor rendezzük
	int tsorted = 1; //időbeli rendezettség ellenőrzése
	while(1) {
		int a;
		uint32_t a1,a2,timestamp;
		
		if(read_table_line(&rt)) break;
		sor++;
		
		a = read_table_uint32(&rt,&a1);
		if(a == 0) a = read_table_uint32(&rt,&a2);
		if(a) {
			if(read_table_get_last_error(&rt) == T_EOF) break;
			if(read_table_get_last_error(&rt) == T_OVERFLOW && !(flags & EFLAGS_ERROR_OVERFLOW)) continue;
			if(hiba < mh) {
				fprintf(stderr,"edges_read(): hibás adatok a bement %lu. sorában!\n",sor);
				read_table_write_error(&rt,stderr);
			}
			hiba++;
			if(hiba == mh) fprintf(stderr,"\t(a további hibákat már nem írom ki)\n");
			continue;
		}
		
		if(flags & EFLAGS_T1) {
			a = read_table_uint32(&rt,&timestamp);
			if(a) {
				if(hiba < mh) {
					fprintf(stderr,"edges_read(): hibás adatok a bement %lu. sorában (nincs időpont megadva)!\n",sor);
					read_table_write_error(&rt,stderr);
				}
				hiba++;
				if(hiba == mh) fprintf(stderr,"\t(a további hibákat már nem írom ki)\n");
				continue;
			}
		}
		
		if(a1 == a2) if(!(flags & EFLAGS_SELF)) continue; //önmagába mutató éleket kihagyjuk
		
		//a1,a2 két érvényes ID
		if(e->nedges == e->edges_size) {
			edges* e2 = edges_grow(e);
			if(!e2) {
				fprintf(stderr,"edges_read(): nem sikerült memóriát lefoglalni!\n");
				break;
			}
		}
		
		edge* e1 = e->e + e->nedges;
		e1->p1 = a1;
		e1->p2 = a2;
		if(flags & EFLAGS_T1) {
			e1->timestamp = timestamp;
			if(timestamp < tlast) tsorted = 0;
		}
		else e1->timestamp = 0;
		
		if(flags & EFLAGS_TXID) { //tranzakció ID eltárolása
			unsigned int txid;
			a = read_table_uint32(&rt,&txid);
			if(a) {
				if(hiba < mh) {
					fprintf(stderr,"edges_read(): hibás adatok a bement %lu. sorában (nincs tranzakció ID megadva)!\n",sor);
					read_table_write_error(&rt,stderr);
				}
				hiba++;
				if(hiba == mh) fprintf(stderr,"\t(a további hibákat már nem írom ki)\n");
				continue;
			}
			e1->offset = txid;
		}
		
		uint64_t eh2 = edgehash(e1,0);
		if(eh2 < last) {
			sorted = 0; //megengedjük, hogy ne legyen sorbarendezve a fájl, ekkor rendezzük
		}
		if(eh2 == last) {
			if(!(flags & EFLAGS_T1)) continue; //a duplán előforduló elemeket átugorjuk
		}
		
		e->nedges += 1;
		last = eh2;		
	}
	if(hiba) fprintf(stderr,"edges_read(): összesen %lu hibás sor\n",hiba);
	
	if(!(sorted || (flags & EFLAGS_T1) )) { //ha időpontokkal együtt olvastuk be, akkor nem rendezzük újra
		edges_sort(e->e,0,e->nedges);
		//lehetnek többször előforduló élek, ezekből egyet tartunk csak meg
		uint64_t i,j=0;
		uint64_t ehl = edgehash(e->e,0);
		for(i=1;i<e->nedges;i++) {
			uint64_t eh2 = edgehash(e->e,i);
			if(eh2 != ehl) {
				j++;
				if(j!=i) {
					e->e[j].p1 = e->e[i].p1;
					e->e[j].p2 = e->e[i].p2;
				}
				ehl = eh2;
			}
		}
		e->nedges = j+1;
	}
	
	//idő szerinti sorbarendezés
	if((flags & EFLAGS_T1) && !tsorted) {
		edges_tsort(e->e,0,e->nedges);
	}
	
	//felesleges memória felszabadítása
/*	uint64_t size1 = (e->nedges)*sizeof(edge); //igazi felhasznált méret
	uint64_t size2 = (e->edges_size)*sizeof(edge);
	uint64_t p1 = size1/pagesize;
	uint64_t p2 = size1%pagesize;
	if(p2) p1++;
	uint64_t size3 = p1*pagesize;
	if(size2 > size3) {
		char* base = (char*)(e->e);
		base += (pagesize*p1);
		munmap(base,size2-size3);
	}*/
	
	fprintf(stderr,"edges_read(): összesen %lu élt olvastam be\n",e->nedges);
	
	return e;
}

edges* edges_read(FILE* f) {
	return edges_read0(f,0);
}

edges* edges_read2(FILE* f) {
	return edges_read0(f,EFLAGS_T1);
}


//élek átmásolása egy új tömbbe, ha r == 1, akkor fordítva (p2->p1, p1->p2)
edges* edges_copy(edges* e1, int r) {
	if(!e1) return 0;
	edges* e = edges_grow0(0,e1->nedges);
	if(!e) return 0;
	uint64_t i;
	if(r) for(i=0;i<e1->nedges;i++) {
		e->e[i].p1 = e1->e[i].p2;
		e->e[i].p2 = e1->e[i].p1;
		e->e[i].timestamp = e1->e[i].timestamp;
		e->e[i].offset = 0;
	}
	else for(i=0;i<e1->nedges;i++) {
		e->e[i].p1 = e1->e[i].p1;
		e->e[i].p2 = e1->e[i].p2;
		e->e[i].timestamp = e1->e[i].timestamp;
		e->e[i].offset = 0;
	}
	
	e->nedges = e1->nedges;
	
	if(r) edges_sort1(e);
	return e;
}

//részhalmaz átmásolása
edges* edges_copy2(edges* e1, uint64_t start, uint64_t end, int r) {
	if(!e1) return 0;
	if(start >= e1->nedges || end >= e1->nedges || end <= start) return 0;
	uint64_t N = end-start;
	edges* e = edges_grow0(0,N);
	if(!e) return 0;
	uint64_t i;
	if(r) for(i=0;i<N;i++) {
		e->e[i].p1 = e1->e[i+start].p2;
		e->e[i].p2 = e1->e[i+start].p1;
		e->e[i].timestamp = e1->e[i+start].timestamp;
		e->e[i].offset = 0;
	}
	else for(i=0;i<N;i++) {
		e->e[i].p1 = e1->e[i+start].p1;
		e->e[i].p2 = e1->e[i+start].p2;
		e->e[i].timestamp = e1->e[i+start].timestamp;
		e->e[i].offset = 0;
	}
	
	e->nedges = N;
	
	//~ if(r) edges_sort1(e);
	//~ if(!(sorted || (flags & EFLAGS_T1) )) { //ha időpontokkal együtt olvastuk be, akkor nem rendezzük újra
		edges_sort(e->e,0,e->nedges);
		//lehetnek többször előforduló élek, ezekből egyet tartunk csak meg
		uint64_t j=0;
		uint64_t ehl = edgehash(e->e,0);
		for(i=1;i<e->nedges;i++) {
			uint64_t eh2 = edgehash(e->e,i);
			if(eh2 != ehl) {
				j++;
				if(j!=i) {
					e->e[j].p1 = e->e[i].p1;
					e->e[j].p2 = e->e[i].p2;
				}
				ehl = eh2;
			}
		}
		e->nedges = j+1;
	//~ }
	return e;
}

//él megkeresése
uint64_t edges_find(edges* e, uint32_t p1, uint32_t p2) {
	edge e2;
	e2.p1 = p1;
	e2.p2 = p2;
	uint64_t eh2 = edgehash(&e2,0); //keresés ez alapján
	uint64_t i = 0; //"tipp"
	uint64_t di = e->nedges;
	if(e->h) {
		unsigned int i1 = ids_find2(e->h->il,p1);
		if(i1 < e->h->il->N) {
			i = e->h->index[i1];
			di = e->h->outdeg[i1];
		}
	}
	int found = 0;
	while(1) {
		int r = cmpedge(e->e,i,eh2);
		if(r == 0) { found = 1; break; } //megtaláltuk
		if(r < 0) { //visszafelé kell menni
			di /= 2;
			if(di == 0) di = 1;
			if(di == 1) {
				if(i==0) break;
				if(cmpedge(e->e,i-1,eh2) > 0) break; //i és i-1 között kellene lennie
			}
			if(di > i) i = 0;
			else i -= di;
		}
		else { //növelni kell i-t
			di /= 2;
			if(di == 0) di = 1;
			if(di == 1) {
				if(i == e->nedges - 1) break;
				if(cmpedge(e->e,i+1,eh2) < 0) break; //i és i+1 között kellene lennie
			}
			if(di > (e->nedges - i)) i = e->nedges - 1;
			else i += di;
		}
	}
	if(found) return i;
	return e->nedges;
}


static inline int cmpf(edge* e, uint64_t i, uint32_t p1) {
	if(p1 < e[i].p1) return -1;
	if(p1 > e[i].p1) return 1;
	return 0;
}

//olyan él keresése, ahol az első ID megegyezik a megadottal
uint64_t edges_findfirst(edges* e, uint32_t p1) {
	uint64_t i = 0; //"tipp"
	uint64_t di = e->nedges; //!! e->nedges uint64_t típus
	int found = 0;
	while(1) {
		int r = cmpf(e->e,i,p1);
		if(r == 0) { found = 1; break; } //megtaláltuk
		if(r < 0) { //visszafelé kell menni
			di /= 2;
			if(di == 0) di = 1;
			if(di == 1) {
				if(i==0) break;
				if(p1 > e->e[i-1].p1) break; //i és i-1 között kellene lennie
			}
			if(di > i) i = 0;
			else i -= di;
		}
		else { //növelni kell i-t
			di /= 2;
			if(di == 0) di = 1;
			if(di == 1) {
				if(i == e->nedges - 1) break;
				if(p1 < e->e[i+1].p1) break; //i és i+1 között kellene lennie
			}
			if(di > (e->nedges - i)) i = e->nedges - 1;
			else i += di;
		}
	}
	if(found) return i;
	return e->nedges;
}



int edges_createhelper(edges* e, const idlist* il) {
	if(!(e && il)) return 1;
	if(!(e->nedges && e->e)) return 1;
	if(e->nedges > UINT_MAX) {
		fprintf(stderr,"edges_createhelper(): too many edges!\n");
		return 4;
	}
	if(!(il->N && il->ids)) return 1;
	if(e->h) {
		if(e->h->index) { free(e->h->index); e->h->index = 0; }
		if(e->h->outdeg) { free(e->h->outdeg); e->h->outdeg = 0; }
	}
	else {
		e->h = (edgehelper*)malloc(sizeof(edgehelper));
		if(!e->h) return 2;
	}
	e->h->il = il;
	e->h->index = (uint64_t*)malloc(sizeof(uint64_t)*(il->N));
	e->h->outdeg = (uint64_t*)malloc(sizeof(uint64_t)*(il->N));
	if( ! (e->h->index && e->h->outdeg) ) {
		if(e->h->index) free(e->h->index);
		if(e->h->outdeg) free(e->h->outdeg);
		free(e->h);
		e->h = 0;
		return 3;
	}
	int32_t last = e->e[0].p1;
	uint64_t lastindex = 0;
	uint64_t deg = 1;
	uint64_t j;
	for(j=1;j<e->nedges;j++) {
		if(e->e[j].p1 != last) {
			unsigned int i = ids_find2(il,last);
			if(i < il->N) {
				e->h->index[i] = lastindex;
				e->h->outdeg[i] = deg;
			}
			last = e->e[j].p1;
			lastindex = j;
			deg = 0;
		}
		deg++;
	}
	unsigned int i2 = ids_find2(il,last);
	if(i2 < il->N) {
		e->h->index[i2] = lastindex;
		e->h->outdeg[i2] = deg;
	}
	return 0;
}

