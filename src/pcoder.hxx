/**
 * pcoder.hxx
 *
 * Shennon, Fano, Huffman Coding
 * by snovvcrash
 * 04.2017
 */

/**
 * Copyright (C) 2017 snovvcrash
 *
 * This file is part of libcoders.
 *
 * libcoders is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libcoders is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libcoders.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#ifndef PCODER_HXX
#define PCODER_HXX

#include <iostream>
#include <fstream>
#include <cstdlib> // size_t
#include <cstdint>
#include <cstring>
#include <cerrno>
#include <vector>
#include <utility> // std::pair
#include <climits> // CHAR_BIT

namespace staticcodes {

	using freq_vec_t   = typename std::vector<uint32_t>;
	using distr_vec_t  = typename std::vector<std::pair<uint8_t, double> >;
	using range_vec_t  = typename std::vector<std::pair<double, double> >;
	using scheme_vec_t = typename std::vector<std::vector<bool> >;
	using bitseq_t     = typename std::vector<bool>;

	static constexpr size_t ALPHABET = 256;

	// ------------------------------------------------------
	// --------------------- STATISTICS ---------------------
	// ------------------------------------------------------

	class Statistics {
	protected:
		freq_vec_t  m_freq_vec;
		distr_vec_t m_distr_vec;
		range_vec_t m_range_vec;
		uint64_t    m_total_chars;

		// Creates a frequency vector containing number of occurrencies of every char in the input file
		void create_freq_vector(std::ifstream& ifile);

		// Creates a probability distribution vector containing pairs <char, char_probability>
		void create_distr_vector();

		// Creates a probability ranges vector containing pairs <char_left_border, char_right_border> (acoder only)
		void create_range_vector();

		Statistics();
	};

	// ------------------------------------------------------
	// ------------------------ NODE ------------------------
	// ------------------------------------------------------

	struct Node {
		int     left;
		int     right;
		uint8_t symbol;
		double  weight;

		Node(int l, int r, uint8_t s, double w);
	};

	using tree_t = typename std::vector<Node>;

	// ------------------------------------------------------
	// --------------------- CODE-TREE ----------------------
	// ------------------------------------------------------

	class CodeTree {
	protected:

		// Creates code tree from filled code scheme
		void tree_from_scheme(tree_t& m_tree, scheme_vec_t& m_scheme_vec);

		// Recursively truncate (optimize) code tree
		void truncate_code_tree(tree_t& m_tree, int index);

		// Creates code scheme by recursively traversalling the code tree
		void traverse_code_tree(tree_t& m_tree, scheme_vec_t& m_scheme_vec, int index, bitseq_t code);

		CodeTree();
	};

	// -------------------------------------------------------
	// ----------------------- SHENNON -----------------------
	// -------------------------------------------------------

	class shennon : private CodeTree {

		// Convert a fraction "s" with "l" digits after the demical point from DEC to BIN
		std::vector<bool> dec_to_bin(double s, size_t l);

		// Creates vector containing sums of probabilities
		std::vector<double> create_prob_sum_vector(distr_vec_t& m_distr_vec);

		// Creates vector containing lengths of codes of each symbol
		std::vector<size_t> create_code_lengths_vector(distr_vec_t& m_distr_vec);
		
	public:
		tree_t       m_tree;
		scheme_vec_t m_scheme_vec;
		int          m_root;

		// Creates code scheme and code tree
		void create_code_scheme(distr_vec_t& m_distr_vec);
	};

	// -------------------------------------------------------
	// ------------------------ FANO -------------------------
	// -------------------------------------------------------

	class fano : private CodeTree {

		// Recursively splits the distribution vector into two parts (close or equal to each other by the sum of probabilities)
		// and creates a code scheme by adding 0 or 1 to it on each step of the recursion
		void create_code_scheme_helper(distr_vec_t& m_distr_vec, int begin, int end);

	public:
		tree_t       m_tree;
		scheme_vec_t m_scheme_vec;
		int          m_root;

		// Creates code scheme and code tree
		void create_code_scheme(distr_vec_t& m_distr_vec);
	};

	// -------------------------------------------------------
	// ----------------------- HUFFMAN -----------------------
	// -------------------------------------------------------

	class huffman : private CodeTree {

		// Creates code tree via Huffman method
		void create_code_tree(distr_vec_t& m_distr_vec);
		
	public:
		tree_t       m_tree;
		scheme_vec_t m_scheme_vec;
		int          m_root;

		// Creates code scheme and code tree
		void create_code_scheme(distr_vec_t& m_distr_vec);
	};

	// -------------------------------------------------------
	// ----------------------- PCODER ------------------------
	// -------------------------------------------------------

	template<typename Algorithm>
	class pcoder : private Statistics {
		Algorithm m_alg;
		bitseq_t  m_seq;

		// Creates a vector with a bit sequence containing binary code that will be written to the output file
		void create_bit_sequence(std::ifstream& ifile);

	public:

		// Encodes text and writes the final bit sequence to the output file
		void compress(std::ifstream& ifile, std::ofstream& ofile);

		void operator()(std::ifstream& ifile, std::ofstream& ofile);

		pcoder(std::ifstream& ifile, std::ofstream& ofile);

		pcoder();
	};

	// -------------------------------------------------------
	// ---------------------- PDECODER -----------------------
	// -------------------------------------------------------

	template<typename Algorithm>
	class pdecoder : private Statistics {
		Algorithm m_alg;

	public:

		// Decodes text and writes the final bit sequence to the output file
		void decompress(std::ifstream& ifile, std::ofstream& ofile);

		void operator()(std::ifstream& ifile, std::ofstream& ofile);

		pdecoder(std::ifstream& ifile, std::ofstream& ofile);

		pdecoder();
	};

	// -------------------------------------------------------
	// --------------------- PCODER IMPL ---------------------
	// -------------------------------------------------------

	template<typename Algorithm>
	void pcoder<Algorithm>::create_bit_sequence(std::ifstream& ifile) {
		m_seq.clear();

		ifile.clear();
		ifile.seekg(0, std::ios::beg);

		char c;
		while (ifile.read(&c, sizeof(c)))
			m_seq.insert(
				m_seq.end(),
				m_alg.m_scheme_vec[static_cast<uint8_t>(c)].begin(),
				m_alg.m_scheme_vec[static_cast<uint8_t>(c)].end()
			);
	}

	template<typename Algorithm>
	void pcoder<Algorithm>::compress(std::ifstream& ifile, std::ofstream& ofile) {
		create_freq_vector(ifile);
		create_distr_vector();
		m_alg.create_code_scheme(m_distr_vec);
		create_bit_sequence(ifile);

		// Writing the frequency table to file (in binary notation)
		for (const auto& freq : m_freq_vec)
			ofile.write(reinterpret_cast<const char*>(&freq), sizeof(freq));

		uint8_t bit_buffer  = 0;
		size_t  bit_counter = 0;

		// Writing the bit sequence to the output file: due to the impossibility of direct bit-by-bit data writing
		// to file we wait until an 8-bit (byte) chunk of data is filled with single bits and send it to file
		for (const auto& bit : m_seq) {
			bit_buffer |= bit << (7 - bit_counter);

			if (++bit_counter == CHAR_BIT) {
				ofile.write(reinterpret_cast<const char*>(&bit_buffer), sizeof(bit_buffer));
				bit_buffer = 0;
				bit_counter = 0;
			}
		}

		// If there are bits left -> send them with last byte (chunk) padded with zeros
		if (bit_counter) ofile.write(reinterpret_cast<const char*>(&bit_buffer), sizeof(bit_buffer));
	}

	template<typename Algorithm>
	void pcoder<Algorithm>::operator()(std::ifstream& ifile, std::ofstream& ofile) {
		compress(ifile, ofile);
	}

	template<typename Algorithm>
	pcoder<Algorithm>::pcoder(std::ifstream& ifile, std::ofstream& ofile) {
		compress(ifile, ofile);
	}

	template<typename Algorithm>
	pcoder<Algorithm>::pcoder()
	{ }

	// -------------------------------------------------------
	// -------------------- PDECODER IMPL --------------------
	// -------------------------------------------------------

	template<typename Algorithm>
	void pdecoder<Algorithm>::decompress(std::ifstream& ifile, std::ofstream& ofile) {
		m_freq_vec.clear();
		m_total_chars = 0;

		// Reading the frequency table and filling the frequency vector with it
		for (size_t i = 0; i < ALPHABET; ++i) {
			uint32_t tmp;
			ifile.read(reinterpret_cast<char*>(&tmp), sizeof(tmp));
			m_freq_vec.push_back(tmp);
			m_total_chars += tmp;
		}

		// If there is no coded text in the input file after the header (frequency table) -> exit
		if (ifile.peek() == EOF) {
			if (!ifile.eof())
				std::cerr << "pdecoder::decompress: " << std::strerror(errno) << std::endl;
			return;
		}

		create_distr_vector();
		m_alg.create_code_scheme(m_distr_vec);

		char     curr_byte;
		int      curr_index  = m_alg.m_root;
		size_t   bit_counter = 0;
		uint64_t cnt_chars   = 0;

		ifile.read(&curr_byte, sizeof(curr_byte));

		// Decoding
		while (true) {
			// Read 1 bit from current byte
			bool curr_bit = curr_byte & (1 << (7 - bit_counter));

			// Traverse the code tree
			if (!curr_bit) curr_index = m_alg.m_tree[curr_index].left;
			else           curr_index = m_alg.m_tree[curr_index].right;

			// If current node is a list -> send it to file
			if (m_alg.m_tree[curr_index].left == -1 && m_alg.m_tree[curr_index].right == -1) {
				char symbol = m_alg.m_tree[curr_index].symbol;
				ofile.write(&symbol, sizeof(symbol));
				curr_index = m_alg.m_root;
				++cnt_chars;
			}

			// If number of decoded chars != number of chars in original text (~ remove padding) -> exit loop
			if (cnt_chars == m_total_chars) break;

			// Read next data chunk
			if (++bit_counter == CHAR_BIT) {
				ifile.read(&curr_byte, sizeof(curr_byte));
				bit_counter = 0;
			}
		}
	}

	template<typename Algorithm>
	void pdecoder<Algorithm>::operator()(std::ifstream& ifile, std::ofstream& ofile) {
		decompress(ifile, ofile);
	}

	template<typename Algorithm>
	pdecoder<Algorithm>::pdecoder(std::ifstream& ifile, std::ofstream& ofile) {
		decompress(ifile, ofile);
	}

	template<typename Algorithm>
	pdecoder<Algorithm>::pdecoder()
	{ }
	
}

#endif // PCODER_HXX
