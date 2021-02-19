/*
 * idlist.h -- id-k tárolása, gyors keresés
 * 
 * Copyright 2013, 2020 Kondor Dániel <dani@thinkpad-r9r>
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


#ifndef IDLIST_H
#define IDLIST_H

#include <stdio.h>
#include <stdlib.h>
#include <algorithm>

#include "read_table.h"

struct idlist {
	unsigned int N = 0; //id-k száma
	unsigned int* ids = nullptr; //id-k tárolása
	unsigned int* inlinks = nullptr; //bejövő linkek száma
	unsigned int* outtx = nullptr; //kimenő tranzakciók száma (txin-beli tranzakciók szerint)
	char* contract = nullptr; //flag to store which address is a contract (only for Ethereum)
};

//id-k felszabadítása
void ids_free(idlist* id);

//id-k beolvasása a megadott fájlból (N egy tipp a méretre)
idlist* ids_read(read_table2& rt, unsigned int N = 0, bool have_contracts = false);
static inline idlist* ids_read(read_table2&& rt, unsigned int N = 0, bool have_contracts = false) {
	return ids_read(rt, N, have_contracts);
}
static inline idlist* ids_read(FILE* f, unsigned int N = 0, bool have_contracts = false) {
	return f ? ids_read(read_table2(f), N, have_contracts) : nullptr;
}

//id megkeresése: eredmény a tömbbeli hely, vagy l->N, ha nem találtuk
static inline unsigned int ids_find(const idlist* l, unsigned int id) {
	if(!l) return 0;
	unsigned int* res = std::lower_bound(l->ids, l->ids + l->N, id);
	unsigned int ret = (unsigned int)(res - l->ids);
	if(ret < l->N && *res != id) ret = l->N;
	return ret;
}

static inline unsigned int ids_find2(const idlist* l, unsigned int id) {
	if(id && id <= l->N && l->ids[id-1] == id) return id-1;
	else if(id < l->N && l->ids[id] == id) return id;
	return ids_find(l,id);
}

#endif

