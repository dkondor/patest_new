/*
 * iterator_zip2.h -- combine two iterators to a "zipped" iterator which
 * 	contains elements from both
 * 
 * examples:
 * std::vector<int> v1;
 * std::vector<double> v2;
 * ... // fill in vectors
 * 
 * 
 * auto itbg = make_zip_it(v1.begin(),v2.begin());
 * auto itend = make_zip_it(v1.end(),v2.end());
 * std::sort(itbg,itend,[](const auto& x, const auto& y){return x.first < y.first;});
 * 
 * // the following hold
 * itbg.first() == *(v1.begin());
 * itbg.second() == *(v2.begin());
 * 
 * // assignment
 * std::pair<int,double> val = *itbg;
 * *itbg = std::make_pair(1,2.0);
 * 
 * Copyright 2018 Daniel Kondor <kondor.dani@gmail.com>
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


#ifndef ITERATOR_ZIP_H
#define ITERATOR_ZIP_H

#include <iterator>
#include <utility>
#include <type_traits>


namespace zi {

/* 1. "reference type": an std::pair probably storing references 
 * 	or other "copmlicated" reference classes (e.g. other refpairs
 * 	recursively)*/
template<class T1, class T2>
struct refpair : std::pair<T1,T2> {
	using std::pair<T1,T2>::first;
	using std::pair<T1,T2>::second;
	
	typedef typename std::remove_reference<T1>::type T1_noref;
	typedef typename std::remove_reference<T2>::type T2_noref;
	constexpr static bool is_ref = std::is_reference<T1>::value || std::is_reference<T2>::value;
	constexpr static bool is_const = std::is_const<T1>::value || std::is_const<T2>::value;
	
	/* convert to regular pair -- this makes a copy of the elements */
	operator std::pair<T1_noref,T2_noref>() const { return std::pair<T1_noref,T2_noref>(first,second); }
	//~ refpair() = delete;
	refpair(T1 x1, T2 x2):std::pair<T1,T2>(x1,x2) { }
	refpair(const refpair<T1,T2>& pair):std::pair<T1,T2>(pair.first,pair.second) { }
	refpair(const std::pair<T1,T2>& pair):std::pair<T1,T2>(pair.first,pair.second) { }
	/* separate constructor removing references if one of the types is a reference */
	template<bool is_ref_ = is_ref>
	refpair( std::pair<T1_noref,T2_noref>&& pair,
		typename std::enable_if<is_ref_, void>::type* = 0 ) : std::pair<T1,T2>(pair.first,pair.second) { }
	/* separate constructor for making a const refpair from a non-const one 
	template<bool is_const_ = is_const>
	refpair(std::pair<typename std::remove_const<T1>::type, typename std::remove_const<T2>::type>&&  pair,
		typename std::enable_if<is_const_, int>::type* = 0):std::pair<T1,T2>(pair.first,pair.second) { }
	*/
	
	/* convert to a const version */
	operator refpair<const T1,const T2>() const { return *this; }
	
	refpair<T1,T2>& operator = (const std::pair<T1_noref,T2_noref>& pair) {
		first = pair.first;
		second = pair.second;
		return *this;
	}
	refpair<T1,T2>& operator = (std::pair<T1_noref,T2_noref>&& pair) {
		first = std::move(pair.first);
		second = std::move(pair.second);
		return *this;
	}
	
	void swap(refpair<T1,T2>& pair) {
		using std::swap;
		swap(first,pair.first);
		swap(second,pair.second);
	}
	
	/* support for it->first or it->second */
	std::pair<T1,T2>* operator ->() { return this; }
	const std::pair<T1,T2>* operator ->() const { return this; }
};

template<class T1,class T2>
void swap(refpair<T1,T2>&& p1, refpair<T1,T2>&& p2) {
	p1.swap(p2);
}

template<class T1,class T2>
void swap(refpair<T1,T2>& p1, refpair<T1,T2>& p2) {
	p1.swap(p2);
}

/* helper struct to determine if iterators are const_iterators */
template<typename It> struct is_const_iterator {
    typedef typename std::iterator_traits<It>::reference reference;
    static const bool value = std::is_const<typename std::remove_reference<reference>::type>::value;
};


/* 2. main iterator class */
template<class It1, class It2> /* , bool cmp_both = true> */
struct zip_it : std::iterator< typename std::iterator_traits<It1>::iterator_category,
		std::pair< typename std::iterator_traits<It1>::value_type, typename std::iterator_traits<It2>::value_type > > {
	
	protected:
		/* just store two iterators -- note: store by value, not reference,
		 * so the iterators used to create this instance are not affected by
		 * incrementing / decrementing */
		It1 it1;
		It2 it2;
	
	public:
		/* Make sure both iterators support the same operations; this might not
		 * be necessary, but not sure how to come up with restricting to traits
		 * of this iterator to the more restrictive of two different iterators
		 * used. Also, the main use case are random access iterators anyway. */
		static_assert( std::is_same< typename std::iterator_traits<It1>::iterator_category,
			typename std::iterator_traits<It2>::iterator_category>::value ,"Iterators have to have be the same kind!\n");
		
		/* typedefs -- note that the reference type is not a reference to
		 * the value_type! This is the main difference from "regular" iterators.
		 * Any algorithm that relies on that would probably not work. */
		typedef typename std::iterator_traits<It1>::value_type T1;
		typedef typename std::iterator_traits<It2>::value_type T2;
		typedef typename std::iterator_traits<It1>::reference refT1;
		typedef typename std::iterator_traits<It2>::reference refT2;
		typedef std::pair<T1,T2> value_type;
		/* flag to say if we are a const_iterator -- note that it is true even if
		 * only one of the supplied iterator is a const iterator */
		constexpr static const bool is_const = is_const_iterator<It1>::value || is_const_iterator<It2>::value;
		/* note: a const_iterator will return a "const refpair" -- does not matter as a refpair
		 * is always "const" in the sense that references cannot be reseated, but this can be used
		 * with the above struct e.g. to determine if this iterator is const */
		typedef typename std::conditional<is_const, const refpair<refT1, refT2>, refpair<refT1,refT2> >::type reference;
		typedef typename std::conditional<is_const, const refT1, refT1>::type T1_maybe_const;
		typedef typename std::conditional<is_const, const refT2, refT2>::type T2_maybe_const;
		typedef ssize_t difference_type;
		
		/* constructor -- takes two iterators */
		zip_it() = delete;
		zip_it(const It1& it1_, const It2& it2_) : it1(it1_),it2(it2_) { }
		
		/* dereferencing */
		/* Note: writing things like
		 * 	auto x = *zit;
		 * will probably NOT do what you intend (it will not copy to current
		 * element); use explicitely the above value_type or similar construct! */
		reference operator*() { return reference(*it1,*it2); }
		//~ value_type operator*() const { return std::pair<T1,T2>(*it1,*it2); }
		
		/* note: operator*() and operator->() are exactly the same since
		 * refpair<T1,T2> supports forwarding operator ->() it its underlying
		 * std::pair<T1&,T2&> */
		reference operator->() { return reference(*it1,*it2); }
		/* alternative to operator ->(): element access functions:
		* so instead of writing e.g. zit->first, one could use zit.first()
		* note: these will return const references for const_iterators,
		* mutable references otherwise*/
		T1_maybe_const first() { return *it1; }
		T2_maybe_const second() { return *it2; }
		/* T1 first() const { return *it1; }
		T2 second() const { return *it2; } */
		
		/* array access -- only works for random access iterators */
		reference operator [] (size_t i) { return reference(it1[i],it2[i]); }
		//~ value_type operator [] (size_t i) const { return std::pair<T1,T2>(it1[i],it2[i]); }
		
		
		/* increment / decrement */
		zip_it<It1,It2> operator++(int) { auto tmp = *this; it1++; it2++; return tmp; }
		zip_it<It1,It2>& operator++() { it1++; it2++; return *this; }
		zip_it<It1,It2> operator--(int) { auto tmp = *this; it1--; it2--; return tmp; }
		zip_it<It1,It2>& operator--() { it1--; it2--; return *this; }
		zip_it<It1,It2>& operator+=(ssize_t x) { it1+=x; it2+=x; return *this; }
		zip_it<It1,It2>& operator-=(ssize_t x) { it1-=x; it2-=x; return *this; }
		zip_it<It1,It2> operator+(ssize_t x) const { auto r(*this); r += x; return r; }
		zip_it<It1,It2> operator-(ssize_t x) const { auto r(*this); r -= x; return r; }
		/* distance -- return the smallest of distance among the two iterator pairs */
		//~ template<bool cmp2>
		ssize_t operator-(const zip_it<It1,It2>& r) const {
			ssize_t d1 = it1 - r.it1;
			ssize_t d2 = it2 - r.it2;
			if(d2 < d1) d1 = d2;
			return d1;
		}
		
		/* comparisons */
		/* note: two zipped iterators are equal if either of the iterator
		 * pairs compares equal -- the motivation is that it can be used to
		 * test if we reach the end of the shorter of two ranges if not equal
		 * 
		 * similarly, all other comparisons will behave similarly, e.g. 
		 * i1 < i2 if both it1 and it2 are less than their pairs, etc.
		 * 
		 * note: this changes the logic of the comparison operators, e.g.
		 * it can be that both i1 < i2 and i1 > i2 are false or both
		 * i1 > i2 and i1 < i2 are true (but i1 < i2 iff i2 > i1)
		 * to avoid this, only compare zipped iterators where both stored
		 * iterators compare similarly (e.g. create zipped iterator pairs from
		 * ranges of the same size)
		 */
		bool operator==(const zip_it<It1,It2>& r) const {
			return (it1 == r.it1 || it2 == r.it2);
		}
		bool operator!=(const zip_it<It1,It2>& r) const {
			return (it1 != r.it1 && it2 != r.it2);
		}
		bool operator< (const zip_it<It1,It2>& r) const {
			return (it1 <  r.it1 && it2 <  r.it2);
		}
		bool operator<=(const zip_it<It1,It2>& r) const {
			return (it1 <= r.it1 && it2 <= r.it2);
		}
		bool operator> (const zip_it<It1,It2>& r) const {
			return (it1 >  r.it1 || it2 >  r.it2);
		}
		bool operator>=(const zip_it<It1,It2>& r) const {
			return (it1 >= r.it1 || it2 >= r.it2);
		}
		
		/* Get access to the individual iterators;
		 * note: these functions return const references, so the individual
		 * iterators cannot be incremented / decremented,
		 * but the elements referenced by the iterators can be modified.
		 * Useful to perform operations on the original containers that the
		 * iterators belong to, but using these could easily lead to undefined
		 * behavior, so use with care :) */
		const It1& get_it1() const { return it1; }
		const It2& get_it2() const { return it2; }
};


/* template function to create zip iterator deducing types automatically */
template<class It1, class It2>
zip_it<It1,It2> make_zip_it(It1 it1, It2 it2) {
	return zip_it<It1,It2>(it1,it2);
}


/* template to create a range usable in range-for (foreach) loops
 * -- C1 and C2 be classes (containers) with iterator support, i.e.
 * 	they should have iterator typedefs and begin() and end() functions */
template<class C1, class C2>
struct zip_range_t {
	protected:
		C1& c1;
		C2& c2;
		zip_range_t() = delete;
	public:
		zip_range_t(C1& c1_, C2& c2_):c1(c1_),c2(c2_) { }
		zip_it<typename C1::iterator, typename C2::iterator> begin() {
			return zip_it<typename C1::iterator, typename C2::iterator>(c1.begin(),c2.begin());
		}
		zip_it<typename C1::iterator, typename C2::iterator> end() {
			return zip_it<typename C1::iterator, typename C2::iterator>(c1.end(),c2.end());
		}
};
/* same for getting const iterators */
template<class C1, class C2>
struct zip_range_const_t {
	protected:
		const C1& c1;
		const C2& c2;
		zip_range_const_t() = delete;
	public:
		zip_range_const_t(const C1& c1_, const C2& c2_):c1(c1_),c2(c2_) { }
		zip_it<typename C1::const_iterator, typename C2::const_iterator> begin() {
			return zip_it<typename C1::const_iterator, typename C2::const_iterator>(c1.begin(),c2.begin());
		}
		zip_it<typename C1::const_iterator, typename C2::const_iterator> end() {
			return zip_it<typename C1::const_iterator, typename C2::const_iterator>(c1.end(),c2.end());
		}
};

template<class C1, class C2>
zip_range_t<C1,C2> make_zip_range(C1& c1, C2& c2) { return zip_range_t<C1,C2>(c1,c2); }
template<class C1, class C2>
zip_range_const_t<C1,C2> make_const_zip_range(C1& c1, C2& c2) { return zip_range_const_t<C1,C2>(c1,c2); }


/* Comparison operators between refpairs and std::pair<T1,T2>.
 * These are all defined similarly to std::pair to be compatible.
 * Currently, we need to provide five versions for all, so that
 * all combinations of pair / refpair / const refpair (to const) work correctly.
 * Other option is to have four template parameters and require that they
 * are the same except for optional constness, but that looks ugly as well */
template<class T1, class T2>
bool operator == (const refpair<T1,T2>& p1, const refpair<T1,T2>& p2) {
	return p1.first == p2.first && p1.second == p2.second;
}
template<class T1, class T2, class T3, class T4>
typename std::enable_if< std::is_same<T1,typename std::remove_const<T3>::type>::value &&
	std::is_same<T2,typename std::remove_const<T4>::type>::value, bool>::type
operator == (const std::pair<T1,T2>& p1, const refpair<T3&,T4&>& p2) {
	return p1.first == p2.first && p1.second == p2.second;
}
template<class T1, class T2, class T3, class T4>
typename std::enable_if< std::is_same<T1,typename std::remove_const<T3>::type>::value &&
	std::is_same<T2,typename std::remove_const<T4>::type>::value, bool>::type
operator == (const refpair<T3&,T4&>& p1, const std::pair<T1,T2>& p2) {
	return p2 == p1;
}

template<class T1, class T2>
bool operator != (const refpair<T1,T2>& p1, const refpair<T1,T2>& p2) {
	return !(p1 == p2);
}
template<class T1, class T2>
bool operator != (const std::pair<T1,T2>& p1, const refpair<const T1&,const T2&>& p2) {
	return !(p1 == p2);
}
template<class T1, class T2>
bool operator != (const refpair<const T1&,const T2&>& p1, const std::pair<T1,T2>& p2) {
	return p2 != p1;
}
template<class T1, class T2>
bool operator != (const std::pair<T1,T2>& p1, const refpair<T1&,T2&>& p2) {
	return !(p1 == p2);
}
template<class T1, class T2>
bool operator != (const refpair<T1&,T2&>& p1, const std::pair<T1,T2>& p2) {
	return p2 != p1;
}



template<class T1, class T2>
bool operator < (const refpair<T1,T2>& p1, const refpair<T1,T2>& p2) {
	if(p1.first < p2.first) return true;
	if(p2.first < p1.first) return false;
	return p1.second < p2.second;
}
template<class T1, class T2>
bool operator < (const std::pair<T1,T2>& p1, const refpair<T1&,T2&>& p2) {
	if(p1.first < p2.first) return true;
	if(p2.first < p1.first) return false;
	return p1.second < p2.second;
}
template<class T1, class T2>
bool operator < (const refpair<T1&,T2&>& p1, const std::pair<T1,T2>& p2) {
	if(p1.first < p2.first) return true;
	if(p2.first < p1.first) return false;
	return p1.second < p2.second;
}
template<class T1, class T2>
bool operator < (const std::pair<T1,T2>& p1, const refpair<const T1&,const T2&>& p2) {
	if(p1.first < p2.first) return true;
	if(p2.first < p1.first) return false;
	return p1.second < p2.second;
}
template<class T1, class T2>
bool operator < (const refpair<const T1&,const T2&>& p1, const std::pair<T1,T2>& p2) {
	if(p1.first < p2.first) return true;
	if(p2.first < p1.first) return false;
	return p1.second < p2.second;
}




template<class T1, class T2>
bool operator <= (const refpair<T1,T2>& p1, const refpair<T1,T2>& p2) {
	return !(p2 < p1);
}
template<class T1, class T2>
bool operator <= (const std::pair<T1,T2>& p1, const refpair<const T1&,const T2&>& p2) {
	return !(p2 < p1);
}
template<class T1, class T2>
bool operator <= (const refpair<const T1&,const T2&>& p1, const std::pair<T1,T2>& p2) {
	return !(p2 < p1);
}
template<class T1, class T2>
bool operator <= (const std::pair<T1,T2>& p1, const refpair<T1&,T2&>& p2) {
	return !(p2 < p1);
}
template<class T1, class T2>
bool operator <= (const refpair<T1&,T2&>& p1, const std::pair<T1,T2>& p2) {
	return !(p2 < p1);
}

template<class T1, class T2>
bool operator > (const refpair<T1,T2>& p1, const refpair<T1,T2>& p2) {
	return p2 < p1;
}
template<class T1, class T2>
bool operator > (const std::pair<T1,T2>& p1, const refpair<T1&,T2&>& p2) {
	return p2 < p1;
}
template<class T1, class T2>
bool operator > (const refpair<T1&,T2&>& p1, const std::pair<T1,T2>& p2) {
	return p2 < p1;
}
template<class T1, class T2>
bool operator > (const std::pair<T1,T2>& p1, const refpair<const T1&,const T2&>& p2) {
	return p2 < p1;
}
template<class T1, class T2>
bool operator > (const refpair<const T1&,const T2&>& p1, const std::pair<T1,T2>& p2) {
	return p2 < p1;
}

template<class T1, class T2>
bool operator >= (const refpair<T1,T2>& p1, const refpair<T1,T2>& p2) {
	return !(p1 < p2);
}
template<class T1, class T2>
bool operator >= (const std::pair<T1,T2>& p1, const refpair<T1&,T2&>& p2) {
	return !(p1 < p2);
}
template<class T1, class T2>
bool operator >= (const refpair<T1&,T2&>& p1, const std::pair<T1,T2>& p2) {
	return !(p1 < p2);
}
template<class T1, class T2>
bool operator >= (const std::pair<T1,T2>& p1, const refpair<const T1&,const T2&>& p2) {
	return !(p1 < p2);
}
template<class T1, class T2>
bool operator >= (const refpair<const T1&,const T2&>& p1, const std::pair<T1,T2>& p2) {
	return !(p1 < p2);
}



/* Comparisons for case when only the first argument should be compared
 * (e.g. when the first argument is a key, and the second is a value,
 * which in that case need not have comparison operators defined) */


template<class T1, class T2>
struct cmp_less_first {
	bool operator () (const refpair<T1&,T2&>& p1, const refpair<T1&,T2&>& p2) const {
		return p1.first < p2.first;
	}
	bool operator () (const refpair<const T1&,const T2&>& p1, const refpair<const T1&,const T2&>& p2) const {
		return p1.first < p2.first;
	}
	bool operator () (const refpair<T1&,T2&>& p1, const std::pair<T1,T2>& p2) const {
		return p1.first < p2.first;
	}
	bool operator () (const std::pair<T1,T2>& p1, const refpair<T1&,T2&>& p2) const {
		return p1.first < p2.first;
	}
	bool operator () (const refpair<const T1&,const T2&>& p1, const std::pair<T1,T2>& p2) const {
		return p1.first < p2.first;
	}
	bool operator () (const std::pair<T1,T2>& p1, const refpair<const T1&,const T2&>& p2) const {
		return p1.first < p2.first;
	}
	bool operator () (const std::pair<T1,T2>& p1, const std::pair<T1,T2>& p2) const {
		return p1.first < p2.first;
	}
};

template<class T1, class T2>
struct cmp_eq_first {
	bool operator () (const refpair<T1&,T2&>& p1, const refpair<T1&,T2&>& p2) const {
		return p1.first == p2.first;
	}
	bool operator () (const refpair<const T1&,const T2&>& p1, const refpair<const T1&,const T2&>& p2) const {
		return p1.first == p2.first;
	}
	bool operator () (const refpair<T1&,T2&>& p1, const std::pair<T1,T2>& p2) const {
		return p1.first == p2.first;
	}
	bool operator () (const std::pair<T1,T2>& p1, const refpair<T1&,T2&>& p2) const {
		return p1.first == p2.first;
	}
	bool operator () (const refpair<const T1&,const T2&>& p1, const std::pair<T1,T2>& p2) const {
		return p1.first == p2.first;
	}
	bool operator () (const std::pair<T1,T2>& p1, const refpair<const T1&,const T2&>& p2) const {
		return p1.first == p2.first;
	}
	bool operator () (const std::pair<T1,T2>& p1, const std::pair<T1,T2>& p2) const {
		return p1.first == p2.first;
	}
};



/* functions to create the previous structs from anything that has a value_type defined
 * (e.g. containers or iterators) */
template<class C1, class C2>
cmp_less_first<typename C1::value_type, typename C2::value_type>
make_cmp_less_first(const C1& x, const C2& y) {
	return cmp_less_first<typename C1::value_type, typename C2::value_type>();
}

template<class C1, class C2>
cmp_eq_first<typename C1::value_type, typename C2::value_type>
make_cmp_eq_first(const C1& x, const C2& y) {
	return cmp_eq_first<typename C1::value_type, typename C2::value_type>();
}

template<class It1, class It2>
cmp_less_first<typename zip_it<It1,It2>::T1, typename zip_it<It1,It2>::T2>
make_cmp_less_first(const zip_it<It1,It2>& zit) {
	return cmp_less_first<typename zip_it<It1,It2>::T1, typename zip_it<It1,It2>::T2>();
}

template<class It1, class It2>
cmp_eq_first<typename zip_it<It1,It2>::T1, typename zip_it<It1,It2>::T2>
make_cmp_eq_first(const zip_it<It1,It2>& zit) {
	return cmp_eq_first<typename zip_it<It1,It2>::T1, typename zip_it<It1,It2>::T2>();
}


}


#endif


