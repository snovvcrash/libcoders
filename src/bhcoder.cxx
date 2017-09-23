/**
 * bhcoder.cxx
 *
 * Huffman Coding on Bigrams (Markov Chain with Depth 1)
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

#include <iostream>
#include <fstream>
#include <cstdlib> // size_t
#include <cstdint>
#include <cstring>
#include <cerrno>
#include <vector>
#include <queue>
#include <utility> // std::pair
#include <climits> // CHAR_BIT
#include "bhcoder.hxx"

namespace contextcodes {

	using freq_vec_t     = typename std::vector<uint32_t>;
	using freq_table_t   = typename std::vector<freq_vec_t>;
	using scheme_vec_t   = typename std::vector<std::vector<bool> >;
	using scheme_table_t = typename std::vector<scheme_vec_t>;
	using bitseq_t       = typename std::vector<bool>;

	constexpr size_t ALPHABET = 256;

	// ------------------------------------------------------
	// --------------------- STATISTICS ---------------------
	// ------------------------------------------------------

	class Statistics {
	protected:
		freq_vec_t   m_freq_vec;
		freq_table_t m_freq_table;
		uint64_t     m_total_chars;

		void create_freq_vector(std::ifstream& ifile) {
			m_freq_vec.clear();
			m_freq_vec.resize(ALPHABET, 0);

			m_freq_table.clear();
			m_freq_table.resize(ALPHABET);

			char c;
			char context;

			ifile.read(&context, sizeof(context));
			++m_freq_vec[static_cast<uint8_t>(context)];
			m_total_chars = 1;

			while (ifile.read(&c, sizeof(c))) {
				if (m_freq_table[static_cast<uint8_t>(context)].empty()) {
					m_freq_table[static_cast<uint8_t>(context)].clear();
					m_freq_table[static_cast<uint8_t>(context)].resize(ALPHABET, 0);
				}

				++m_freq_vec[static_cast<uint8_t>(c)];
				++m_freq_table[static_cast<uint8_t>(context)][static_cast<uint8_t>(c)];

				++m_total_chars;
				context = c;
			}
		}

		Statistics()
		{ }
	};

	// ------------------------------------------------------
	// ------------------------ NODE ------------------------
	// ------------------------------------------------------

	struct Node {
		int     left;
		int     right;
		uint8_t symbol;

		Node(int l, int r, uint8_t s) {
			left   = l;
			right  = r;
			symbol = s;
		}
	};

	// ------------------------------------------------------
	// --------------------- CODE-TREE ----------------------
	// ------------------------------------------------------

	using tree_t   = typename std::vector<Node>;
	using forest_t = typename std::vector<tree_t>;

	class CodeTree {
	protected:
		void traverse_code_tree(tree_t& m_tree, scheme_vec_t& m_scheme_vec, int index, bitseq_t code) {
			if (index == -1) return;

			if (m_tree[index].left == -1 && m_tree[index].right == -1) {
				m_scheme_vec[m_tree[index].symbol] = code;
				return;
			}

			code.push_back(0);
			traverse_code_tree(m_tree, m_scheme_vec, m_tree[index].left, code);
			code.pop_back();

			code.push_back(1);
			traverse_code_tree(m_tree, m_scheme_vec, m_tree[index].right, code);
			code.pop_back();
		}

		CodeTree()
		{ }
	};

	// -------------------------------------------------------
	// ----------------------- HUFFMAN -----------------------
	// -------------------------------------------------------

	class huffman : private CodeTree {
		void create_code_tree(freq_vec_t& m_freq_vec) {
			using pair_t = typename std::pair<uint32_t, int>;
			std::priority_queue<pair_t, std::vector<pair_t>, std::greater<pair_t> > queue;

			for (size_t i = 0; i < m_freq_vec.size(); ++i) {
				if (m_freq_vec[i]) {
					m_tree.push_back(Node(-1, -1, i));
					queue.push(pair_t(m_freq_vec[i], m_tree.size() - 1));
				}
			}

			while (queue.size() > 1) {
				pair_t child1;
				pair_t child2;
				uint32_t parent_w;

				child1 = queue.top(); queue.pop();
				child2 = queue.top(); queue.pop();
				parent_w = child1.first + child2.first;

				m_tree.push_back(Node(child1.second, child2.second, 0));
				queue.push(pair_t(parent_w, m_tree.size() - 1));
			}

			if (m_tree.size() == 1) m_tree.push_back(Node(0, -1, 0));
		}
		
	public:
		tree_t       m_tree;
		scheme_vec_t m_scheme_vec;

		void create_code_scheme(freq_vec_t& m_freq_vec) {
			m_scheme_vec.clear();
			m_scheme_vec.resize(ALPHABET);

			if (m_freq_vec.empty()) return;
			if (m_freq_vec.size() == 1) {
				m_scheme_vec[m_freq_vec.front()] = bitseq_t(1, 0);
				return;
			}

			create_code_tree(m_freq_vec);
			traverse_code_tree(m_tree, m_scheme_vec, m_tree.size() - 1, bitseq_t());
		}
	};

	// -------------------------------------------------------
	// ---------------------- CODERIMPL ----------------------
	// -------------------------------------------------------

	class bhcoder::CoderImpl : private Statistics, private huffman {
		scheme_table_t m_scheme_table;
		bitseq_t       m_seq;
		char           m_context;

		void encode_first_byte(std::ifstream& ifile) {
			m_seq.clear();
			
			ifile.clear();
			ifile.seekg(0, std::ios::beg);

			char c;
			if (ifile.read(&c, sizeof(c))) {
				m_seq.insert(
					m_seq.end(),
					m_scheme_vec[static_cast<uint8_t>(c)].begin(),
					m_scheme_vec[static_cast<uint8_t>(c)].end()
				);

				m_context = c;
			}
		}

		void create_bit_sequence(std::ifstream& ifile) {
			char c;
			while (ifile.read(&c, sizeof(c))) {
				m_seq.insert(
					m_seq.end(),
					m_scheme_table[static_cast<uint8_t>(m_context)][static_cast<uint8_t>(c)].begin(),
					m_scheme_table[static_cast<uint8_t>(m_context)][static_cast<uint8_t>(c)].end()
				);

				m_context = c;
			}
		}

	public:
		void compress(std::ifstream& ifile, std::ofstream& ofile) {
			create_freq_vector(ifile);
			create_code_scheme(m_freq_vec);
			encode_first_byte(ifile);
			m_tree.clear();

			for (const auto& freq : m_freq_vec)
				ofile.write(reinterpret_cast<const char*>(&freq), sizeof(freq));

			m_scheme_table.clear();
			m_scheme_table.resize(ALPHABET);

			for (size_t i = 0; i < m_freq_table.size(); ++i) {
				if (!m_freq_table[i].empty()) {
					create_code_scheme(m_freq_table[i]);
					m_scheme_table[i].clear();
					m_scheme_table[i].insert(m_scheme_table[i].end(), m_scheme_vec.begin(), m_scheme_vec.end());
					m_tree.clear();
				}
			}

			create_bit_sequence(ifile);

			size_t num_not_empty = 0;
			for (const auto& freq_vec : m_freq_table)
				if (!freq_vec.empty())
					++num_not_empty;
			ofile.write(reinterpret_cast<const char*>(&num_not_empty), sizeof(num_not_empty));

			for (size_t context = 0; context < m_freq_table.size(); ++context) {
				if (!m_freq_table[context].empty()) {
					ofile.write(reinterpret_cast<const char*>(&context), sizeof(context));
					for (const auto& freq : m_freq_table[context])
						ofile.write(reinterpret_cast<const char*>(&freq), sizeof(freq));
				}
			}

			uint8_t bit_buffer  = 0;
			size_t  bit_counter = 0;

			for (const auto& bit : m_seq) {
				bit_buffer |= bit << (7 - bit_counter);

				if (++bit_counter == CHAR_BIT) {
					ofile.write(reinterpret_cast<const char*>(&bit_buffer), sizeof(bit_buffer));
					bit_buffer = 0;
					bit_counter = 0;
				}
			}

			if (bit_counter) ofile.write(reinterpret_cast<const char*>(&bit_buffer), sizeof(bit_buffer));
		}

		void operator()(std::ifstream& ifile, std::ofstream& ofile) {
			compress(ifile, ofile);
		}

		CoderImpl(std::ifstream& ifile, std::ofstream& ofile) {
			compress(ifile, ofile);
		}

		CoderImpl()
		{ }
	};

	void bhcoder::compress(std::ifstream& ifile, std::ofstream& ofile) {
		m_pImpl->compress(ifile, ofile);
	}

	void bhcoder::operator()(std::ifstream& ifile, std::ofstream& ofile) {
		m_pImpl->operator()(ifile, ofile);
	}

	bhcoder::bhcoder(std::ifstream& ifile, std::ofstream& ofile)
		: m_pImpl(new CoderImpl(ifile, ofile))
	{ }

	bhcoder::bhcoder() : m_pImpl(new CoderImpl)
	{ }

	bhcoder::~bhcoder()
	{ }

	// -------------------------------------------------------
	// --------------------- DECODERIMPL ---------------------
	// -------------------------------------------------------

	class bhdecoder::DecoderImpl : private Statistics, private huffman {
		forest_t m_forest;
		char     m_context;

		std::pair<char, int> decode_first_byte(std::ifstream& ifile, std::ofstream& ofile) {
			char   curr_byte;
			int    curr_index  = m_tree.size() - 1;
			size_t bit_counter = 0;

			ifile.read(&curr_byte, sizeof(curr_byte));

			while (true) {
				bool curr_bit = curr_byte & (1 << (7 - bit_counter));

				if (!curr_bit) curr_index = m_tree[curr_index].left;
				else           curr_index = m_tree[curr_index].right;

				if (m_tree[curr_index].left == -1 && m_tree[curr_index].right == -1) {
					char symbol = m_tree[curr_index].symbol;
					ofile.write(&symbol, sizeof(symbol));

					if (m_total_chars == 1) return std::make_pair(0, -1);

					m_context = symbol;
					++bit_counter;

					return std::make_pair(curr_byte, bit_counter);
				}

				if (++bit_counter == CHAR_BIT) {
					ifile.read(&curr_byte, sizeof(curr_byte));
					bit_counter = 0;
				}
			}
		}

	public:
		void decompress(std::ifstream& ifile, std::ofstream& ofile) {
			m_freq_vec.clear();
			m_total_chars = 0;

			for (size_t i = 0; i < ALPHABET; ++i) {
				uint32_t tmp;
				ifile.read(reinterpret_cast<char*>(&tmp), sizeof(tmp));
				m_freq_vec.push_back(tmp);
				m_total_chars += tmp;
			}

			m_freq_table.clear();
			m_freq_table.resize(ALPHABET);

			size_t num_not_empty;
			ifile.read(reinterpret_cast<char*>(&num_not_empty), sizeof(num_not_empty));

			for (size_t i = 0; i < num_not_empty; ++i) {
				size_t context;
				ifile.read(reinterpret_cast<char*>(&context), sizeof(context));
				for (size_t j = 0; j < ALPHABET; ++j) {
					uint32_t tmp;
					ifile.read(reinterpret_cast<char*>(&tmp), sizeof(tmp));
					m_freq_table[context].push_back(tmp);
				}
			}

			if (ifile.peek() == EOF) {
				if (!ifile.eof())
					std::cerr << "bhdecoder::decompress: " << std::strerror(errno) << std::endl;
				return;
			}

			create_code_scheme(m_freq_vec);
			std::pair<char, int> first_byte_ret = decode_first_byte(ifile, ofile);
			if (first_byte_ret.second == -1) return;
			m_tree.clear();

			m_forest.clear();
			m_forest.resize(ALPHABET);

			for (size_t i = 0; i < m_freq_table.size(); ++i) {
				if (!m_freq_table[i].empty()) {
					create_code_scheme(m_freq_table[i]);
					m_forest[i].insert(m_forest[i].end(), m_tree.begin(), m_tree.end());
					m_tree.clear();
				}
			}

			char     curr_byte   = first_byte_ret.first;
			int      curr_index  = m_forest[static_cast<uint8_t>(m_context)].size() - 1;;
			size_t   bit_counter = first_byte_ret.second;
			uint64_t cnt_chars   = 1;

			if (bit_counter == CHAR_BIT) {
				ifile.read(&curr_byte, sizeof(curr_byte));
				bit_counter = 0;
			}

			while (true) {
				bool curr_bit = curr_byte & (1 << (7 - bit_counter));

				if (!curr_bit) curr_index = m_forest[static_cast<uint8_t>(m_context)][curr_index].left;
				else           curr_index = m_forest[static_cast<uint8_t>(m_context)][curr_index].right;

				if (m_forest[static_cast<uint8_t>(m_context)][curr_index].left == -1 && 
					m_forest[static_cast<uint8_t>(m_context)][curr_index].right == -1
				) {
					char symbol = m_forest[static_cast<uint8_t>(m_context)][curr_index].symbol;
					ofile.write(&symbol, sizeof(symbol));

					m_context = symbol;
					++cnt_chars;

					curr_index = m_forest[static_cast<uint8_t>(m_context)].size() - 1;
				}

				if (cnt_chars == m_total_chars) break;

				if (++bit_counter == CHAR_BIT) {
					ifile.read(&curr_byte, sizeof(curr_byte));
					bit_counter = 0;
				}
			}
		}

		void operator()(std::ifstream& ifile, std::ofstream& ofile) {
			decompress(ifile, ofile);
		}

		DecoderImpl(std::ifstream& ifile, std::ofstream& ofile) {
			decompress(ifile, ofile);
		}

		DecoderImpl()
		{ }
	};

	void bhdecoder::decompress(std::ifstream& ifile, std::ofstream& ofile) {
		m_pImpl->decompress(ifile, ofile);
	}

	void bhdecoder::operator()(std::ifstream& ifile, std::ofstream& ofile) {
		m_pImpl->operator()(ifile, ofile);
	}

	bhdecoder::bhdecoder(std::ifstream& ifile, std::ofstream& ofile)
		: m_pImpl(new DecoderImpl(ifile, ofile))
	{ }

	bhdecoder::bhdecoder() : m_pImpl(new DecoderImpl)
	{ }

	bhdecoder::~bhdecoder()
	{ }

}
