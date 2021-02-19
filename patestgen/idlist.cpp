/*
 * idlist.c -- id-k tárolása, gyors keresés
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

#include "idlist.h"
#include "edges.h"
#include "iterator_zip.h"

void ids_free(idlist* id) {
	if(id) {
		if(id->ids) free(id->ids);
		if(id->inlinks) free(id->inlinks);
		if(id->outtx) free(id->outtx);
		if(id->contract) free(id->contract);
		delete id;
	}
}


static bool ids_grow(idlist* l, size_t& current_size, size_t new_size, bool have_contracts) {
	const size_t grow_size = 4194304UL; // allocate space in 4M chunks -- 52M memory
	if(!l) return false;
	if(!new_size || new_size <= current_size) new_size = current_size + grow_size;
	unsigned int* tmp = (unsigned int*)realloc(l->ids, sizeof(unsigned int)*new_size);
	if(!tmp) return false;
	l->ids = tmp;
	if(have_contracts) {
		char* contract = (char*)realloc(l->contract, sizeof(char)*new_size);
		if(!contract) return false;
		l->contract = contract;
	}
	current_size = new_size;
	return true;
}


//id-k beolvasása a megadott fájlból (maximum N darab)
idlist* ids_read(read_table2& rt, unsigned int N, bool have_contracts) {
	idlist* ret = new idlist;
	if(!ret) return nullptr;

	size_t current_size = 0;
	if(!ids_grow(ret, current_size, N, have_contracts)) {
		delete ret;
		return nullptr;
	}
	
	size_t i = 0;
	for(; rt.read_line(); i++) {
		if(i >= current_size && !ids_grow(ret, current_size, 0, have_contracts)) {
			ids_free(ret);
			return nullptr;
		}
		if(!rt.read(ret->ids[i])) break;
		if(have_contracts) {
			int tmp;
			if(!rt.read(read_table_skip(), tmp)) break;
			ret->contract[i] = !!tmp;
		}
	}
	if(rt.get_last_error() != T_EOF) {
		rt.write_error(stderr);
		ids_free(ret);
		return nullptr;
	}
	
	ret->N = i;
	
	if(have_contracts) std::sort(zi::make_zip_it(ret->ids, ret->contract),
		zi::make_zip_it(ret->ids + i, ret->contract + i),
		[](const auto& x, const auto& y) { return x.first < y.first; });
	else std::sort(ret->ids, ret->ids + i);
	
	ret->inlinks = (unsigned int*)calloc(i, sizeof(unsigned int));
	ret->outtx = (unsigned int*)calloc(i, sizeof(unsigned int));
	if(!(ret->inlinks && ret->outtx)) {
		ids_free(ret);
		return nullptr;
	}
	
	return ret;
}


