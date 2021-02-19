/*
 * edges.h -- élek effektív tárolása: lista az id-k szerint rendezve,
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


#ifndef EDGES_H
#define EDGES_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "idlist.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <sys/mman.h>
//~ #include <gsl/gsl_rng.h>
#include <limits.h>
//~ #include "mt19937.h"

 
//változás: hashmap használata helyett előre beolvassuk az összes lehetséges élt, sorbarendezve (feltétel:
//	a bemeneti fájlban sorbarendezve kell legyenek, minden él csak egyszer -- ezt egyszerűbb az SQL Serverrel
//	megcsináltatni), ebben tudunk már gyorsan keresni

typedef struct edgehelper_t {
	const idlist* il;
	uint64_t* index;
	uint64_t* outdeg;
} edgehelper;


//heap struktúra az élek tárolásához (összes él, idő szerint rendezve)
typedef struct edge_t { //egy él, heap ezekből, timestamp szerint rendezve
	uint32_t p1; //első pont -> itt az "igazi" ID-ket tároljuk (a hálózatbeli azonosítókat, ezeknek nem kell szekvenciálisnak lenni)
	uint32_t p2; //második pont (p1->p2 él)
	unsigned int offset; //heap-beli offset (heap[i].offset == i mindig) -- vagy: tranzakció ID-k
	unsigned int timestamp; //legutolsó aktivitás
} edge; //méret -- 16 bájt

typedef struct edges_t {
	edge* e; //élek tárolása itt, id-k szerint rendezve
	uint64_t nedges;
	uint64_t edges_size;
	uint64_t edges_grow;
	edgehelper* h;
} edges;


/*
 * egy tömb (unsigned int-ekből), amiben az edge-ekre mutató pointerek vannak, ezek binary heap-ben:
 * 	unsigned int* heap;
 * 	edges[heap[i]].timestamp < edges[heap[2*i+1 / 2*i+2]].timestamp
 * és edges[heap[i]].offset == i
 * 
 */

//hash fv. (két 32-bites id-ből egy 64-bites szám, ezeket talán gyorsabb összehasonlítani
static inline uint64_t edgehash(const edge* e, const unsigned int i) {
		uint64_t r = e[i].p1;
		r = r << 32;
		uint64_t r2 = e[i].p2;
		return r | r2;
}

//két él összehasonlítása, eredmény: -1, ha eh2-nek e[i] előtt kell lenni, 1 ha utána, 0, ha megegyeznek
static inline int cmpedge(const edge* e, uint64_t i, uint64_t eh2) {
	uint64_t eh1 = edgehash(e,i);
	if(eh1 < eh2) return 1;
	if(eh1 > eh2) return -1;
	return 0;
}

static inline int cmpedge2(const edge* e, uint64_t i, uint64_t j) {
	uint64_t eh2 = edgehash(e,j);
	return cmpedge(e,i,eh2);
}

//éleket tároló struktúra lefoglalása / növesztése:
//	ha e == 0, akkor új struktúra lefoglalása
//	ha e != 0, akkor növesztés (mremap())
//eredmény: 0, ha hiba történt (nem sikerült memóriát lefoglalni)
//ha a bemeneti e nem volt 0, akkor azt nem szabadítottuk fel
//edges* pointer, ha rendben volt (ha e != 0, akkor e-t adjuk vissza)
edges* edges_grow(edges* e);

//élek felszabadítása
void edges_free(edges* e);

//élek beolvasása egy fájlból -- a fájlban már megfelelően rendezve kell lenniük
edges* edges_read(FILE* f);

//élek beolvasása egy fájlból -- a fájlban már megfelelően rendezve kell lenniük
edges* edges_read2(FILE* f); //+ timestamp-ok is meg vannak adva

//beolvasás általánosan:
//flags értékei:
#define EFLAGS_T1 1 //időpontokat is beolvasunk és idő szerint rendezzük az eredményt (ha nincs megadva, akkor csak
	//éleket olvasunk be és minden élből csak egyet tartunk meg)
#define EFLAGS_SELF 2 //megengedünk önmagába mutató éleket (p1->p1)
#define EFLAGS_TXID 4 //tranzakció ID-ket is beolvasunk (legutolsó oszlop, az offset tömbbe tároljuk el)
#define EFLAGS_ERROR_OVERFLOW 8 // overflow hibát jelent (különben csak kihagyjuk)
edges* edges_read0(FILE* f, int flags);

//élek átmásolása egy új tömbbe, ha r == 1, akkor fordítva (p2->p1, p1->p2)
edges* edges_copy(edges* e1, int r);

//részhalmaz átmásolása
edges* edges_copy2(edges* e1, uint64_t start, uint64_t end, int r);

//él megkeresése, eredmény: a tömbön (e->e) belüli sorszám, ha megtaláltuk, e->nedges, ha nem találtuk
uint64_t edges_find(edges* e, uint32_t p1, uint32_t p2);

//olyan él keresése, ahol az első ID megegyezik a megadottal
uint64_t edges_findfirst(edges* e, uint32_t p1);

//élek sorbarendezése az ID-k szerint
void edges_sort1(edges* e);


//véletlen cserék
//a->b, c->d helyett a->d, c->b
//N darab csere
/*
void edges_rand1(edges* e, uint64_t N, gsl_rng* r);

//véletlen cserék, a sikertelen próbálkozások is beleszámítanak a cserék számába
//a->b, c->d helyett a->d, c->b
void edges_rand2(edges* e, uint64_t N, gsl_rng* r);
*/

// create a "helper" for faster edge lookups: store the starting index for each addr, making it unnecessary to perform binary
//	search for the whole set, and also return result in O(1) time for addresses with outdeg == 1
int edges_createhelper(edges* e, const idlist* il);


//ID-k generálása N db él alapján
idlist* ids_gen(const edge* e, unsigned int N);

//N db élben az ID-k kicserélése a sorszámukra
int ids_replace(const idlist* il, edge* e, uint64_t N);


#endif



