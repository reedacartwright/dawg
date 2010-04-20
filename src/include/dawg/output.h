#pragma once
#ifndef DAWG_OUTPUT_H
#define DAWG_OUTPUT_H
/****************************************************************************
 *  Copyright (C) 2010 Reed A. Cartwright, PhD <reed@scit.us>               *
 ****************************************************************************/

#include <ostream>
#include <fstream>
#include <utility>

#include <dawg/utils.h>
#include <boost/algorithm/string.hpp>

#include <dawg/sequence.h>

namespace dawg {

class output {
public:
	output() : p_out(NULL), format_id(0), do_op(&output::print_fasta) { }

	bool open(const char *file_name);

	inline void operator()(const alignment& aln) {
		if(p_out != NULL)
			(this->*do_op)(aln);
	}

	template<class T>
	bool set_format(T format);
	
	inline void set_ostream(std::ostream &os) {
		p_out = &os;
	}
	inline void set_ostream(std::ostream *os) {
		p_out = os;
	}

private:
	void (output::*do_op)(const alignment& aln);

	void print_aln(const alignment& aln);
	void print_poo(const alignment& aln);
	void print_fasta(const alignment& aln);
	void print_nexus(const alignment& aln);
	void print_phylip(const alignment& aln);

protected:
	std::ostream *p_out;
	std::ofstream fout;
	std::size_t format_id;
};

template<class T>
bool output::set_format(T format) {
	static const char format_keys[][10] = {
		"aln", "poo", "fasta", "fsa",
		"nexus", "phylip"
	};
	static void (output::*format_ops[])(const alignment& aln) = {
		&output::print_aln, &output::print_poo,
		&output::print_fasta, &output::print_fasta,
		&output::print_nexus, &output::print_phylip
	};
	format_id = key_switch(format, format_keys);
	if(format_id == -1) {
		format_id = 0;
		do_op = &output::print_aln;
		return false;
	}
	do_op = format_ops[format_id];
	return true;
}

} // namespace dawg 

#endif // DAWG_OUTPUT_H