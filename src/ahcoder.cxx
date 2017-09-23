/**
 * ahcoder.cxx
 *
 * Adaptive Huffman Coding (FGK Algorithm)
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
#include <vector>
#include <algorithm>
#include <climits> // CHAR_BIT
#include "ahcoder.hxx"

namespace adaptivecodes {

	using bitseq_t  = std::vector<bool>;
	using symbseq_t = std::vector<uint8_t>;

	constexpr size_t MAX_LEAF_NUM = 256;
	constexpr size_t MAX_NODE_NUM = 512;

	// NYT = Not Yet Transmitted
	enum node_type {NYT_NODE = -1, INTERNAL_NODE = -2};

	// -------------------------------------------------------
	// ------------------------ NODE -------------------------
	// -------------------------------------------------------

	struct Node {
		int      symbol;
		int      order;
		uint64_t weight;

		Node* parent;
		Node* left;
		Node* right;

		Node(
			int symbol_,
			int order_,
			uint64_t weight_ = 0,
			Node* parent_ = nullptr,
			Node* left_   = nullptr,
			Node* right_  = nullptr
		)
			: symbol(symbol_), order(order_), weight(weight_), parent(parent_), left(left_), right(right_)
		{ }
	};

	// -------------------------------------------------------
	// ------------------------- FGK -------------------------
	// -------------------------------------------------------

	class fgk {
		Node*    m_nyt;
		Node*    m_root;
		Node*    m_leaves[MAX_LEAF_NUM];
		Node*    m_nodes[MAX_NODE_NUM + 1];
		Node*    m_dcurr;
		bitseq_t m_buf;

		void update_tree(Node* node) {
			while (node) {
				Node* highest = highest_in_class(node);
				swap(highest, node);

				++node->weight;

				node = node->parent;
			}
		}

		Node* highest_in_class(Node* node) const {
			Node* highest = node;
			int num = node->order;

			for (size_t i = num + 1; i <= MAX_NODE_NUM && m_nodes[i]->weight == node->weight; ++i)
				highest = m_nodes[i];

			return highest;
		}

		void swap(Node* a, Node* b) {
			if (a == m_root || b == m_root || a == b || a->parent == b || b->parent == a)
				return;

			if (a->parent->left == a && b->parent->left == b)
				std::swap(a->parent->left, b->parent->left);
			else if (a->parent->left == a && b->parent->right == b)
				std::swap(a->parent->left, b->parent->right);
			else if (a->parent->right == a && b->parent->left == b)
				std::swap(a->parent->right, b->parent->left);
			else if (a->parent->right == a && b->parent->right == b)
				std::swap(a->parent->right, b->parent->right);

			std::swap(m_nodes[a->order], m_nodes[b->order]);
			std::swap(a->order, b->order);
			std::swap(a->parent, b->parent);
		}

		bool is_in_tree(uint8_t byte) const {
			if (m_leaves[byte]) return 1;
			return 0;
		}

		bitseq_t get_symbol_code(const Node* symbolNode) const {
			bitseq_t code;
			code.reserve(16);

			const Node* currNode = symbolNode;
			while (currNode != m_root) {
				/* if (currNode->parent->left == currNode)
					code.push_back(0);
				else
					code.push_back(1); */
				code.push_back(currNode->parent->right == currNode);
				currNode = currNode->parent;
			}

			std::reverse(code.begin(), code.end());
			return code;
		}

		void encode_existing_byte(uint8_t byte) {
			update_tree(m_leaves[byte]);
		}

		void encode_new_byte(uint8_t byte) {
			m_nyt->symbol = INTERNAL_NODE;
			m_nyt->left   = new Node(NYT_NODE, m_nyt->order - 2, 0, m_nyt);
			m_nyt->right  = new Node(byte, m_nyt->order - 1, 0, m_nyt);

			m_leaves[byte] = m_nyt->right;
			m_nodes[m_nyt->left->order] = m_nyt->left;
			m_nodes[m_nyt->right->order] = m_nyt->right;

			m_nyt = m_nyt->left;

			update_tree(m_leaves[byte]);
		}

		void delete_tree() { delete_tree(m_root); }

		void delete_tree(const Node* m_root) {
			if (!m_root) return;

			delete_tree(m_root->left);
			delete_tree(m_root->right);
			delete m_root;
		}

	public:
		fgk() : m_nyt(new Node(NYT_NODE, MAX_NODE_NUM)), m_root(m_nyt), m_dcurr(m_root) {
			for (auto&& leaf : m_leaves)
				leaf = nullptr;
			for (auto&& node: m_nodes)
				node = nullptr;

			m_nodes[MAX_NODE_NUM] = m_nyt;
		}

		~fgk() { delete_tree(); }

		bitseq_t encode(uint8_t byte) {
			if (is_in_tree(byte)) {
				bitseq_t code = get_symbol_code(m_leaves[byte]);

				encode_existing_byte(byte);

				return code;
			}
			else {
				bitseq_t code = get_symbol_code(m_nyt);
				
				for (size_t i = 0; i < CHAR_BIT; ++i)
					code.push_back(byte & (1 << (7 - i)));

				encode_new_byte(byte);

				return code;
			}
		}

		symbseq_t decode(const bitseq_t& codeseq) {
			symbseq_t symbseq;

			std::copy(std::begin(codeseq), std::end(codeseq), std::back_inserter(m_buf));

			size_t i = 0;
			while (i < m_buf.size() || (!m_dcurr->left && !m_dcurr->right))
				if (m_dcurr->symbol == NYT_NODE) {
					if (m_buf.size() - i >= CHAR_BIT) {
						uint8_t ascii_char = 0;

						for (size_t j = 0; j < CHAR_BIT; ++j)
							ascii_char |= m_buf[i + j] << (7 - j);

						symbseq.push_back(ascii_char);

						encode_new_byte(ascii_char);

						i += CHAR_BIT;
						m_dcurr = m_root;
					}
					else break; // end of file
				}
				else if (!m_dcurr->left && !m_dcurr->right) {
					symbseq.push_back(m_dcurr->symbol);

					encode_existing_byte(m_dcurr->symbol);

					m_dcurr = m_root;
				}
				else {
					if (m_buf[i]) m_dcurr = m_dcurr->right;
					else          m_dcurr = m_dcurr->left;
					++i;
				}

			m_buf.erase(std::begin(m_buf), std::begin(m_buf) + i);

			return symbseq;
		}

		bitseq_t get_nyt_code() const {
			return get_symbol_code(m_nyt);
		}
	};

	// -------------------------------------------------------
	// ---------------------- CODERIMPL ----------------------
	// -------------------------------------------------------

	class ahcoder::CoderImpl : private fgk {
		void flush_output_buffer(std::ostream& ofile, bitseq_t& outbuf) {
			symbseq_t out_bytes;
			out_bytes.reserve((MAX_NODE_NUM * CHAR_BIT) / 2);

			size_t pos = 0;
			while (pos + CHAR_BIT <= outbuf.size()) {
				uint8_t byte = 0;

				for (size_t i = 0; i < CHAR_BIT; ++i)
					if (outbuf[pos + i])
						byte |= 1 << (7 - i);

				out_bytes.push_back(byte);

				pos += CHAR_BIT;
			}

			outbuf.erase(std::begin(outbuf), std::begin(outbuf) + pos);
			ofile.write(reinterpret_cast<const char*>(&out_bytes.front()), out_bytes.size());
		}

	public:
		void compress(std::ifstream& ifile, std::ofstream& ofile) {
			uint8_t inbuf[MAX_NODE_NUM];

			bitseq_t outbuf;
			outbuf.reserve(MAX_NODE_NUM * CHAR_BIT + 64);

			while (ifile.good()) {
				ifile.read(reinterpret_cast<char*>(inbuf), MAX_NODE_NUM);
				size_t bytes_read = ifile.gcount();

				for (size_t i = 0; i < bytes_read ; ++i) {
					bitseq_t code = encode(inbuf[i]);
					std::copy(std::begin(code), std::end(code), std::back_inserter(outbuf));
				}

				if (outbuf.size() >= MAX_NODE_NUM * CHAR_BIT)
					flush_output_buffer(ofile, outbuf);
			}

			if (!outbuf.empty()) {
				if (outbuf.size() % CHAR_BIT != 0) {
					bitseq_t nyt_code = get_nyt_code();

					size_t extra_bits = CHAR_BIT - outbuf.size() % CHAR_BIT;
					for (size_t i = 0; i < extra_bits; ++i)
						outbuf.push_back(nyt_code[i % nyt_code.size()]);
				}

				flush_output_buffer(ofile, outbuf);
			}
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

	void ahcoder::compress(std::ifstream& ifile, std::ofstream& ofile) {
		m_pImpl->compress(ifile, ofile);
	}

	void ahcoder::operator()(std::ifstream& ifile, std::ofstream& ofile) {
		m_pImpl->operator()(ifile, ofile);
	}

	ahcoder::ahcoder(std::ifstream& ifile, std::ofstream& ofile)
		: m_pImpl(new CoderImpl(ifile, ofile))
	{ }

	ahcoder::ahcoder() : m_pImpl(new CoderImpl)
	{ }

	ahcoder::~ahcoder()
	{ }

	// -------------------------------------------------------
	// --------------------- DECODERIMPL ---------------------
	// -------------------------------------------------------

	class ahdecoder::DecoderImpl : private fgk {
	public:
		void decompress(std::ifstream& ifile, std::ofstream& ofile) {
			uint8_t inbuf[MAX_NODE_NUM];

			while (ifile.good()) {
				ifile.read(reinterpret_cast<char*>(inbuf), MAX_NODE_NUM);
				size_t bytes_read = ifile.gcount();

				bitseq_t seq;
				seq.reserve(MAX_NODE_NUM * CHAR_BIT);

				for (size_t i = 0; i < bytes_read; ++i) {
					bitseq_t code(CHAR_BIT);
					for (size_t j = 0; j < CHAR_BIT; ++j)
						code[j] = inbuf[i] & (1 << (7 - j));

					std::copy(std::begin(code), std::end(code), std::back_inserter(seq));
				}

				symbseq_t outbuf = decode(seq);
				ofile.write(reinterpret_cast<const char*>(&outbuf.front()), outbuf.size());
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

	void ahdecoder::decompress(std::ifstream& ifile, std::ofstream& ofile) {
		m_pImpl->decompress(ifile, ofile);
	}

	void ahdecoder::operator()(std::ifstream& ifile, std::ofstream& ofile) {
		m_pImpl->operator()(ifile, ofile);
	}

	ahdecoder::ahdecoder(std::ifstream& ifile, std::ofstream& ofile)
		: m_pImpl(new DecoderImpl(ifile, ofile))
	{ }

	ahdecoder::ahdecoder() : m_pImpl(new DecoderImpl)
	{ }

	ahdecoder::~ahdecoder()
	{ }

}
