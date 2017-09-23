/**
 * pcoder.cxx
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

#include <iostream>
#include <fstream>
#include <cstdlib>    // size_t, std::abs
#include <cstdint>
#include <vector>
#include <queue>
#include <utility>    // std::pair
#include <algorithm>  // std::stable_sort
#include <functional> // std::greater
#include "pcoder.hxx"

namespace staticcodes {

	// ------------------------------------------------------
	// --------------------- STATISTICS ---------------------
	// ------------------------------------------------------

	void Statistics::create_freq_vector(std::ifstream& ifile) {
		m_freq_vec.clear();
		m_freq_vec.resize(ALPHABET, 0);

		m_total_chars = 0;

		char c;
		while (ifile.read(&c, sizeof(c))) {
			++m_freq_vec[static_cast<uint8_t>(c)];
			++m_total_chars;
		}
	}

	void Statistics::create_distr_vector() {
		m_distr_vec.clear();

		if (!m_total_chars) return;
		using pair_t = typename std::pair<uint8_t, double>;

		for (size_t i = 0; i < m_freq_vec.size(); ++i) {
			pair_t tmp;
			tmp.first = i;
			tmp.second = m_freq_vec[i] / static_cast<double>(m_total_chars);
			if (tmp.second > 0) m_distr_vec.push_back(tmp);
		}

		std::stable_sort(
			m_distr_vec.begin(),
			m_distr_vec.end(),
			[&](pair_t const& lhs, pair_t const& rhs) {
				return lhs.second > rhs.second;
			}
		);
	}

	Statistics::Statistics()
	{ }

	// ------------------------------------------------------
	// ------------------------ NODE ------------------------
	// ------------------------------------------------------

	Node::Node(int l, int r, uint8_t s, double w) {
		left   = l;
		right  = r;
		symbol = s;
		weight = w;
	}

	// ------------------------------------------------------
	// --------------------- CODE-TREE ----------------------
	// ------------------------------------------------------

	void CodeTree::tree_from_scheme(tree_t& m_tree, scheme_vec_t& m_scheme_vec) {
		m_tree.push_back(Node(-1, -1, 0, 0)); // ROOT

		for (size_t i = 0; i < m_scheme_vec.size(); ++i) {
			int index = 0;

			for (size_t j = 0; j < m_scheme_vec[i].size(); ++j) {
				// If 0 and there is no left node -> add left node
				if (!m_scheme_vec[i][j]) {
					if (m_tree[index].left == -1) {
						m_tree.push_back(Node(-1, -1, 0, 0));
						m_tree[index].left = m_tree.size() - 1;
					}
					index = m_tree[index].left;
				}
				// If 1 and there is no right node -> add right node
				else {
					if (m_tree[index].right == -1) {
						m_tree.push_back(Node(-1, -1, 0, 0));
						m_tree[index].right = m_tree.size() - 1;
					}
					index = m_tree[index].right;
				}
			}

			// Assign current symbol to the leaf node
			if (index != 0) m_tree[index].symbol = i;
		}
	}

	void CodeTree::truncate_code_tree(tree_t& m_tree, int index = 0) {
		if (index == -1) return; // exit if we found leaf

		int t = -1;

		if (m_tree[index].left == -1 && m_tree[index].right != -1)
			t = m_tree[index].left;
		else if (m_tree[index].left != -1 && m_tree[index].right == -1)
			t = m_tree[index].right;

		if (t != -1) {
			m_tree[index].symbol = m_tree[t].symbol;
			m_tree[index].left   = m_tree[t].left;
			m_tree[index].right  = m_tree[t].right;
		}

		truncate_code_tree(m_tree, m_tree[index].left);
		truncate_code_tree(m_tree, m_tree[index].right);
	}

	void CodeTree::traverse_code_tree(tree_t& m_tree, scheme_vec_t& m_scheme_vec, int index, bitseq_t code) {
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

	CodeTree::CodeTree()
	{ }

	// -------------------------------------------------------
	// ----------------------- SHENNON -----------------------
	// -------------------------------------------------------

	std::vector<bool> shennon::dec_to_bin(double s, size_t l) {
		std::vector<bool> code;

		for (size_t i = 1; i <= l; ++i) {
			s *= 2;
			if (s >= 1) {
				code.push_back(1);
				--s;
			}
			else code.push_back(0);
		}

		return code;
	}

	std::vector<double> shennon::create_prob_sum_vector(distr_vec_t& m_distr_vec) {
		std::vector<double> prob_sum(m_distr_vec.size(), 0);

		for (size_t i = 1; i < prob_sum.size(); ++i)
			prob_sum[i] = prob_sum[i - 1] + m_distr_vec[i - 1].second;

		return prob_sum;
	}

	std::vector<size_t> shennon::create_code_lengths_vector(distr_vec_t& m_distr_vec) {
		std::vector<size_t> code_lengths;

		size_t two_l = 2; // 2^l
		size_t l     = 1;

		for (const auto& prob : m_distr_vec) {
			double p = prob.second;
			while (!(( ( 1.0 / two_l ) <= p) && (p < ( 1.0 / ( two_l >> 1 ) ) ))) {
				two_l <<= 1;
				++l;
			}
			code_lengths.push_back(l);
		}

		return code_lengths;
	}

	void shennon::create_code_scheme(distr_vec_t& m_distr_vec) {
		m_scheme_vec.clear();
		m_scheme_vec.resize(ALPHABET);

		if (m_distr_vec.empty()) return;
		if (m_distr_vec.size() == 1) {
			m_scheme_vec[m_distr_vec.front().first] = bitseq_t(1, 0);
			return;
		}

		std::vector<double> prob_sum     = create_prob_sum_vector(m_distr_vec);
		std::vector<size_t> code_lengths = create_code_lengths_vector(m_distr_vec);

		for (size_t i = 0; i < prob_sum.size(); ++i)
			m_scheme_vec[m_distr_vec[i].first] = dec_to_bin(prob_sum[i], code_lengths[i]);

		tree_from_scheme(m_tree, m_scheme_vec);
		for (auto&& v : m_scheme_vec) v.clear();
		truncate_code_tree(m_tree);
		traverse_code_tree(m_tree, m_scheme_vec, 0, bitseq_t());

		// Tree was build from root to leaves => m_root index is 0
		m_root = 0;
	}

	// -------------------------------------------------------
	// ------------------------ FANO -------------------------
	// -------------------------------------------------------

	void fano::create_code_scheme_helper(distr_vec_t& m_distr_vec, int begin, int end) {
		if ((end - begin) <= 0) return;

		double first_sum  = 0.0;
		double second_sum = 0.0;

		for (int i = begin; i <= end; ++i) second_sum += m_distr_vec[i].second;

		double min_diff = second_sum;
		int begin_part = 0;

		for (int i = begin; i <= end; ++i) {
			first_sum  += m_distr_vec[i].second;
			second_sum -= m_distr_vec[i].second;

			double diff = std::abs(second_sum - first_sum);

			if (diff <= min_diff) {
				min_diff = diff;
				begin_part = i;
			}
			else break;
		}

		for (int i = begin; i <= begin_part; ++i)   m_scheme_vec[m_distr_vec[i].first].push_back(0);
		for (int i = begin_part + 1; i <= end; ++i) m_scheme_vec[m_distr_vec[i].first].push_back(1);

		create_code_scheme_helper(m_distr_vec, begin, begin_part);
		create_code_scheme_helper(m_distr_vec, begin_part + 1, end);
	}

	void fano::create_code_scheme(distr_vec_t& m_distr_vec) {
		m_scheme_vec.clear();
		m_scheme_vec.resize(ALPHABET);

		if (m_distr_vec.empty()) return;
		if (m_distr_vec.size() == 1) {
			m_scheme_vec[m_distr_vec.front().first] = bitseq_t(1, 0);
			return;
		}
		
		create_code_scheme_helper(m_distr_vec, 0, m_distr_vec.size() - 1);
		tree_from_scheme(m_tree, m_scheme_vec);

		// Tree was build from root to leaves => m_root index is 0
		m_root = 0;
	}

	// -------------------------------------------------------
	// ----------------------- HUFFMAN -----------------------
	// -------------------------------------------------------

	void huffman::create_code_tree(distr_vec_t& m_distr_vec) {
		using pair_t = typename std::pair<double, int>;
		std::priority_queue<pair_t, std::vector<pair_t>, std::greater<pair_t> > queue;

		for (const auto& prob : m_distr_vec)
			m_tree.push_back(Node(-1, -1, prob.first, prob.second));

		for (size_t i = 0; i < m_tree.size(); ++i)
			queue.push(pair_t(m_tree[i].weight, i));

		while (queue.size() > 1) {
			pair_t child1;
			pair_t child2;
			double parent_w;

			child1 = queue.top(); queue.pop();
			child2 = queue.top(); queue.pop();
			parent_w = child1.first + child2.first;

			m_tree.push_back(Node(child1.second, child2.second, 0, parent_w));
			queue.push(pair_t(parent_w, m_tree.size() - 1));
		}
	}

	void huffman::create_code_scheme(distr_vec_t& m_distr_vec) {
		m_scheme_vec.clear();
		m_scheme_vec.resize(ALPHABET);

		if (m_distr_vec.empty()) return;
		if (m_distr_vec.size() == 1) {
			m_scheme_vec[m_distr_vec.front().first] = bitseq_t(1, 0);
			return;
		}

		create_code_tree(m_distr_vec);
		traverse_code_tree(m_tree, m_scheme_vec, m_tree.size() - 1, bitseq_t());

		// Tree was build from leaves to root => m_root index is m_tree.size()-1
		m_root = m_tree.size() - 1;
	}
	
}
