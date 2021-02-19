/*
 * edgeheap.h
 * 
 * Copyright 2013 Kondor Dániel <kondor.dani@gmail.com>
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
 */


#ifndef EDGEHEAP_H
#define EDGEHEAP_H
#include "edges.h"

//élek időrendezett tárolásához szükséges heap

class edgeheap {
	public:
		uint64_t* heap; //adatokat tároló tömb
		uint64_t hsize; //heap mérete (elemek száma)
		uint64_t hn; //aktív (felhasznált) elemek száma
		uint64_t grow0; //növelés ennyivel egyszerre
		edges* e; //éleket tároló struktúra
		static constexpr uint64_t heap_max_size = UINT32_MAX;
		
		edgeheap() { heap = (uint64_t*)MAP_FAILED; hsize = 0; hn = 0; grow0 = 131072; e = 0; }
		edgeheap(uint64_t grow1) { heap = (uint64_t*)MAP_FAILED; hsize = 0; hn = 0; grow0 = grow1; e = 0; }
		edgeheap(edges* e1) { heap = (uint64_t*)MAP_FAILED; hsize = 0; hn = 0; grow0 = 131072; e = e1; }
		edgeheap(edges* e1, uint64_t grow1) { heap = (uint64_t*)MAP_FAILED; hsize = 0; hn = 0; grow0 = grow1; e = e1; }
		~edgeheap() {
			clean();
		}
		
		//memória felszabadítása
		void clean();
		//heap tömb növelése
		int grow();
		
		// szokásos heapup, elem felvitele amíg lehet
		void heapup(uint64_t i);
		
		// szokásos heapdown, elem elhelyezése a helyére
		void heapdown(uint64_t i);
		
		//i-edik elem átrendezése legfelülre -- ez nem tartja meg a heap-tulajdonságot,
		//de csak a 0. elem lesz hibás, a két al-heap jó marad; ez után a 0. elemet törölni
		//kell, és egy új elemet kell a helyére beilleszteni, majd levinni, ameddig lehet
		void shiftup(uint64_t i);
		
		//e[n] hozzáadása -- eredmény: 0, ha sikerült, >0, ha hiba történt (nem sikerült a memóriát növelni)
		int add(uint64_t n);
		
		//edges[n] törlése -- eredmény: 0, ha sikerült, >0 ha n túl nagy volt
		int del(uint64_t n);
		
		//legfelső elem (legkisebb timestamp) törlése
		void del0();
};

#endif

