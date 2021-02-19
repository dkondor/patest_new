/*
 * numeric_join.cpp -- join two text files by a numeric field
 * 	files must be sorted by that field already
 * 	(similar to the join command line utility, but handles numeric
 * 	join fields instead of strings)
 * 
 * main motivation is to join text files containing numeric fields from the
 * command line without the need to sort them first, which might be
 * impractical if the files are very large and are already sorted by
 * numeric order
 * 
 * rewrite of the C# version in C++ for better speed / portability
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
 * 
 */


#include <iostream>
#include <vector>
#include <string.h>
#include <string>
#include "read_table_cpp.h"


	
const char usage[] = R"!!!(Usage: numjoin [OPTION]... FILE1 FILE2
For each pair of input lines with identical join fields, write a line to
standard output.  The default join field is the first, delimited by blanks.
The join field has to be an integer and both files need to be sorted on the
join fields (in numeric order).

When FILE1 or FILE2 (not both) is -, read standard input.
  (joining a file that has a literal name of '-' is not supported)

  -a FILENUM        also print unpairable lines from file FILENUM, where
                      FILENUM is 1 or 2, corresponding to FILE1 or FILE2
  -e EMPTY          replace missing input fields with EMPTY
  -1 FIELD          join on this FIELD of file 1
  -2 FIELD          join on this FIELD of file 2
  -j FIELD          equivalent to '-1 FIELD -2 FIELD'
  -t CHAR           use CHAR as input and output field separator
  -C CHAR			use CHAR as comment indicator: lines beginning with
					  CHAR are ignored
  -v FILENUM        like -a FILENUM, but suppress joined output lines
  -o1 FIELDS        output these fields from file 1 (FIELDS is a
                      comma-separated list of field)
  -o2 FIELDS        output these fields from file 2
  -c                check that the input is correctly sorted, even
                      if all input lines are pairable
  -H                treat the first line in both files as field headers,
                      print them without trying to pair them
  -h                display this help and exit

Unless -t CHAR is given, leading blanks separate fields and are ignored,
else fields are separated by CHAR.  Any FIELD is a field number counted
from 1.

Important: FILE1 and FILE2 must be sorted on the join fields.
  (use sort -n or similar to achieve this)

)!!!";

/*
 * reads one line from the given stream and splits it according to the
 * 	given separator, stores string parts in res
 * if field > 0 tries to parse the text in that field as an ID
 * 
 * returns true on success, false on errors
 */
static bool SplitLine(line_parser& sr, std::vector<std::pair<size_t,size_t> >& res) {
	for(size_t i=0;i<res.size();i++) if(!sr.read_string_view_pair(res[i])) return false;
	return true;
}

/* get the numeric ID from the current line and field */
static bool GetID(line_parser& sr, int field, int64_t& id) {
	if(field < 1) return false;
	size_t j = (size_t)(field-1);
	for(size_t i=0;i<j;i++) sr.read_skip();
	return sr.read_int64(id);
}

struct parsed_line {
	line_parser parser;
	std::vector<std::pair<size_t,size_t> > fields;
	parsed_line() = delete;
	parsed_line(const parsed_line& pl) = delete;
	parsed_line(parsed_line&& pl) = default;
	parsed_line(line_parser_params par, const std::string& line, size_t req_fields):parser(par,line) {
		fields.reserve(req_fields);
		while(true) {
			std::pair<size_t,size_t> v;
			if(!parser.read_string_view_pair(v)) break;
			fields.push_back(v);
		}
	}
	const std::string& get_line_str() const { return parser.get_line_str(); }
};


static void write_split_error(const read_table2& rt, const line_parser& lp, size_t i) {
	const char* fn = rt.get_fn();
	size_t real_line = rt.get_line() - i;
	std::cerr<<"Error parsing data in file ";
	if(fn) std::cerr<<fn;
	else std::cerr<<"<stdin>";
	std::cerr<<", line "<<real_line<<":\n";
	std::cerr<<lp.get_last_error_str()<<"\n";
}

/*
 * read the next set of lines from the sr
 * 
 * input: 
 * 	 sr -- stream to read
 *   nextid -- next id in the stream (if already read)
 *   field -- field containing the ID
 *   req_fields -- required number of fields in the file
 *   first -- true if the first data line in the file
 * 
 * output:
 *   id -- current ID
 *   lines -- collection of one or more lines with the current ID,
 *       or empty list if the file is empty
 *   nextid -- next ID in the file (if exists, unchanged otherwise)
 * 
 * returns true on success or EOF, false on format error
 * note: returning true does not mean that there are any lines, lines.size() can be checked
 *   separately by the caller
 */
static bool ReadNext(read_table2& sr, std::vector<parsed_line>& lines, int64_t& id,
		int64_t& nextid, int field, size_t req_fields, bool first) {
	lines.clear();
	if(first) if(! (sr.read_line() && GetID(sr,field,nextid)) ) return sr.get_last_error() == T_EOF;
	if(sr.get_last_error() == T_EOF) return true;
	id = nextid;
	lines.emplace_back(line_parser_params().set_delim(sr.get_delim()).set_comment(sr.get_comment()),sr.get_line_str(),req_fields);
	if(lines.back().fields.size() < req_fields) {
		write_split_error(sr,lines.back().parser,0);
		return false;
	}
	
	// read further lines, until we have the same ID in them
	while(true) {
		if(! (sr.read_line() && GetID(sr,field,nextid)) ) {
			if(sr.get_last_error() == T_EOF) return true;
			else return false;
		}
		if(nextid != id) break;
		lines.emplace_back(line_parser_params().set_delim(sr.get_delim()).set_comment(sr.get_comment()),sr.get_line_str(),req_fields);
		if(lines.back().fields.size() < req_fields) {
			write_split_error(sr,lines.back().parser,0);
			return false;
		}
	}
	return true;
}

static inline void OutputField(std::ostream& sw, const std::string& buf, const std::pair<size_t,size_t>& pos) {
	for(size_t i=0;i<pos.second;i++) sw<<buf[pos.first+i];
}

static void WriteFields(std::ostream& sw, const std::vector<std::pair<size_t,size_t> >& line,
		const std::string& buf, const std::vector<int>& fields, bool& firstout, char out_sep) {
	if(!fields.empty()) for(int f : fields) {
		if(firstout) { if(!line.empty()) OutputField(sw,buf,line[f-1]); }
		else {
			if(!line.empty()) { sw<<out_sep; OutputField(sw,buf,line[f-1]); }
			else sw<<out_sep;
		}
		firstout = false;
	}
	else for(const auto& s : line) {
		if(firstout) OutputField(sw,buf,s);
		else { sw<<out_sep; OutputField(sw,buf,s); }
		firstout = false;
	}
}


int main(int argc, char** args) {
	const char* file1 = 0;
	const char* file2 = 0;
	
	int field1 = 1;
	int field2 = 1;
	int req_fields1 = 1;
	int req_fields2 = 1;
	
	std::vector<int> outfields1; bool outfields1_empty = false;
	std::vector<int> outfields2; bool outfields2_empty = false;
	
	char delim = 0;
	char comment = 0;
	//~ const char* empty = 0;
	
	int unpaired = 0; // if 1 or 2, print unpaired lines from the given file
	bool only_unpaired = false;
	bool header = false;
	bool strict_order = false;
	
	// process option arguments
	int i=1;
	for(;i<argc;i++) if(args[i][0] == '-' && args[i][1] != 0) switch(args[i][1]) {
		case '1':
			field1 = atoi(args[i+1]);
			i++;
			break;
		case '2':
			field2 = atoi(args[i+1]);
			i++;
			break;
		case 'j':
			field1 = atoi(args[i+1]);
			field2 = field1;
			i++;
			break;
		case 't':
			delim = args[i+1][0];
			i++;
			break;
		case 'C':
			comment = args[i+1][0];
			i++;
			break;
/*		case 'e':
			empty = args[i+1];
			i++;
			break; */
		case 'a':
			unpaired = atoi(args[i+1]);
			if( ! (unpaired == 1 || unpaired == 2) ) { std::cerr<<"-a parameter has to be either 1 or 2\n  use numjoin -h for help\n"; return 1; }
			i++;
			break;
		case 'v':
			unpaired = atoi(args[i+1]);
			if( ! (unpaired == 1 || unpaired == 2) ) { std::cerr<<"-a parameter has to be either 1 or 2\n  use numjoin -h for help\n"; return 1; }
			i++;
			only_unpaired = true;
			break;
		case 'o':
			if(!(args[i][2] == '1' || args[i][2] == '2')) { std::cerr<<"Invalid parameter: "<<args[i]<<"\n  (use -o1 or -o2)\n  use numjoin -h for help\n"; return 1; }
			{
				std::vector<int> tmp;
				bool valid = true;
				bool empty = true;
				int max = 0;
				// it is valid to give zero output columns from one of the files
				// (e.g. to filter the other file)
				// this case it might be necessary to give an empty string
				// as the argument (i.e. -o1 "")
				if( !(args[i+1] == 0 || args[i+1][0] == 0 || args[i+1][0] == '-') ) {
					line_parser lp(line_parser_params().set_delim(','),args[i+1]);
					int x;
					while(lp.read(x)) { tmp.push_back(x); if(x > max) max = x; if(x < 1) { valid = false; break; } }
					if(lp.get_last_error() != T_EOL || tmp.empty()) valid = false;
					empty = false;
				}
				if(!valid) { std::cerr<<"Invalid parameter: "<<args[i]<<" "<<args[i+1]<<"\n  use numjoin -h for help\n"; return 1; }
				if(args[i][2] == '1') {
					outfields1 = std::move(tmp);
					if(max > req_fields1) req_fields1 = max;
					outfields1_empty = empty;
				}
				if(args[i][2] == '2') {
					outfields2 = std::move(tmp);
					if(max > req_fields2) req_fields2 = max;
					outfields2_empty = empty;
				}
				if(!(args[i+1] == 0 || args[i+1][0] == '-')) i++;
			}
			break;
		case 'H':
			header = true;
			break;
		case 'c':
			strict_order = true;
			break;
		case 'h':
			std::cout<<usage;
			return 0;
		default:
			std::cerr<<"Unknown parameter: "<<args[i]<<"\n  use numjoin -h for help\n";
			return 1;
	}
	else break; // non-option argument, means the filenames
	// i now points to the first filename
	if(i + 1 >= argc) { std::cerr<<"Error: expecting two input filenames\n  use numjoin -h for help\n"; return 1; }
	file1 = args[i];
	file2 = args[i+1];
	if(!strcmp(file1,file2)) { std::cerr<<"Error: input files have to be different!\n"; return 1; }
	if(file1[0] == '-' && file1[1] == 0) file1 = 0;
	if(file2[0] == '-' && file2[1] == 0) file2 = 0;
	if(field1 < 1 || field2 < 1) { std::cerr<<"Error: field numbers have to be >= 1!\n"; return 1; }
	
	auto& sw = std::cout;
	read_table2 s1(file1,std::cin,line_parser_params().set_delim(delim).set_comment(comment));
	read_table2 s2(file2,std::cin,line_parser_params().set_delim(delim).set_comment(comment));
	
	int64_t id1 = INT64_MIN;
	int64_t id2 = INT64_MIN;
	
	std::vector<parsed_line> lines1;
	int64_t nextid1 = INT64_MIN;
	std::vector<parsed_line> lines2;
	int64_t nextid2 = INT64_MIN;
	
	char out_sep = '\t';
	if(delim) out_sep = delim;
	
	std::vector<std::pair<size_t,size_t> > line1_fields(req_fields1);
	std::vector<std::pair<size_t,size_t> > line2_fields(req_fields2);
	
	if(header) {
		// read and write output header
		if( ! (s1.read_line() && SplitLine(s1,line1_fields) ) ) {
			std::cerr<<"Error reading header in file 1:\n";
			s1.write_error(std::cerr);
			return 1;
		}
		if( ! (s2.read_line() && SplitLine(s2,line2_fields) ) ) {
			std::cerr<<"Error reading header in file 2:\n";
			s2.write_error(std::cerr);
			return 1;
		}
		
			
		bool firstout = true;
		if(!outfields1_empty) WriteFields(sw,line1_fields,s1.get_line_str(),outfields1,firstout,out_sep);
		if(!outfields2_empty) WriteFields(sw,line2_fields,s2.get_line_str(),outfields2,firstout,out_sep);
		
	}
	
	// read first lines
	if( !ReadNext(s1,lines1,id1,nextid1,field1,req_fields1,true) ) {
		std::cerr<<"Error reading data from file 1:\n";
		s1.write_error(std::cerr);
		return 1;
	}
	if( !ReadNext(s2,lines2,id2,nextid2,field2,req_fields2,true) ) {
		std::cerr<<"Error reading data from file 2:\n";
		s2.write_error(std::cerr);
		return 1;
	}
	
	size_t out_lines = 0;
	size_t matched1 = 0;
	size_t matched2 = 0;
	size_t unmatched = 0;
	while(true) {
		if(lines1.empty() && lines2.empty()) break; // end of both files
		if(lines1.empty() && unpaired != 2) break; // end of first file and we don't care about unpaired
		if(lines2.empty() && unpaired != 1) break; // end of second file and we don't care about unpaired
		if(lines1.size() > 0 && lines2.size() > 0 && id1 == id2) {
			// match, write out (if needed -- not only_unpaired)
			// there could be several lines from both files, iterate
			// over the cross product
			if(!only_unpaired) {
				matched1 += lines1.size();
				matched2 += lines2.size();
				for(size_t j=0;j<lines1.size();j++)
				for(size_t k=0;k<lines2.size();k++) {
					// write out fields from the first file
					bool firstout = true;
					if(!outfields1_empty) WriteFields(sw,lines1[j].fields,lines1[j].get_line_str(),outfields1,firstout,out_sep);
					if(!outfields2_empty) WriteFields(sw,lines2[k].fields,lines2[k].get_line_str(),outfields2,firstout,out_sep);
					sw<<'\n';
					out_lines++;
				}
			}
			lines1.clear();
			lines2.clear();
			
			if(strict_order) {
				// check order
				if(nextid1 < id1) {
					std::cerr<<"Error: input file 1 ("<<(file1?file1:"<stdin>");
					std::cerr<<") not sorted on line "<<s1.get_line()<<" ( "<<nextid1<<" < "<<id1<<")!\n";
					break;
				}
				if(nextid2 < id2) {
					std::cerr<<"Error: input file 2 ("<<(file2?file2:"<stdin>");
					std::cerr<<") not sorted on line "<<s2.get_line()<<" ( "<<nextid2<<" < "<<id2<<")!\n";
					break;
				}
			}
			
			// read next lines
			if( !ReadNext(s1,lines1,id1,nextid1,field1,req_fields1,false) ) {
				std::cerr<<"Error reading data from file 1:\n";
				s1.write_error(std::cerr);
				return 1;
			}
			if( !ReadNext(s2,lines2,id2,nextid2,field2,req_fields2,false) ) {
				std::cerr<<"Error reading data from file 2:\n";
				s2.write_error(std::cerr);
				return 1;
			}
			continue; // skip following section, the next lines might be a match as well
		} // write out one match
		
		// no match
		
		// need to advance file1
		if(lines1.size() > 0 && (lines2.empty() || id1 < id2)) {
			// check if lines from file 1 should be output if not matched
			if(unpaired == 1) for(size_t j=0;j<lines1.size();j++) {
				// still print unpaired lines from file 1
				bool firstout = true;
				if(!outfields1_empty) WriteFields(sw,lines1[j].fields,lines1[j].get_line_str(),outfields1,firstout,out_sep);
				// note: we write empty fields for file 2
				if(!outfields2.empty()) WriteFields(sw,std::vector<std::pair<size_t,size_t> >(),std::string(),outfields2,firstout,out_sep);
				sw<<'\n';
				out_lines++;
				unmatched++;
			}
			lines1.clear();
			
			// first check sort order, that could be a problem here
			if(nextid1 < id1) {
				std::cerr<<"Error: input file 1 ("<<(file1?file1:"<stdin>");
				std::cerr<<") not sorted on line "<<s1.get_line()<<" ( "<<nextid1<<" < "<<id1<<")!\n";
				break;
			}
			// then advance file 1
			if( !ReadNext(s1,lines1,id1,nextid1,field1,req_fields1,false) ) {
				std::cerr<<"Error reading data from file 1:\n";
				s1.write_error(std::cerr);
				return 1;
			}
		}
		else {
			// here id2 < id1 or end of file1 already
			// check if lines from file 2 should be output if not matched
			if(unpaired == 2) for(size_t j=0;j<lines2.size();j++) {
				// still print unpaired lines from file 2
				bool firstout = true;
				// note: we write empty fields for file 1
				if(!outfields1.empty()) WriteFields(sw,std::vector<std::pair<size_t,size_t> >(),std::string(),outfields1,firstout,out_sep);
				if(!outfields2_empty) WriteFields(sw,lines2[j].fields,lines2[j].get_line_str(),outfields2,firstout,out_sep);
				sw<<'\n';
				out_lines++;
				unmatched++;
			}
			lines2.clear();
			
			// first check sort order, that could be a problem here
			if(nextid2 < id2) {
				std::cerr<<"Error: input file 2 ("<<(file2?file2:"<stdin>");
				std::cerr<<") not sorted on line "<<s2.get_line()<<" ( "<<nextid2<<" < "<<id2<<")!\n";
				break;
			}
			
			if( !ReadNext(s2,lines2,id2,nextid2,field2,req_fields2,false) ) {
				std::cerr<<"Error reading data from file 2:\n";
				s2.write_error(std::cerr);
				return 1;
			}
		}
	} // main loop
	
	
	sw.flush(); // flush output
	
	std::cerr<<"Matched lines from file 1: "<<matched1<<'\n';
	std::cerr<<"Matched lines from file 2: "<<matched2<<'\n';
	if(unmatched > 0) switch(unpaired) {
		case 1:
			std::cerr<<"Unmatched lines from file 1: "<<unmatched<<'\n';
			break;
		case 2:
			std::cerr<<"Unmatched lines from file 2: "<<unmatched<<'\n';
			break;
	}
	std::cerr<<"Total lines output: "<<out_lines<<'\n';
}





