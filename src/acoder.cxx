/**
 * acoder.cxx
 *
 * Arithmetic Coding
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
#include <cstdlib>    // size_t
#include <cstdint>
#include <cstring>
#include <cerrno>
#include <vector>
#include <utility>    // std::pair
#include <algorithm>  // std::stable_sort
#include <functional> // std::greater
#include <cmath>      // std::floor
#include "pcoder.hxx"
#include "acoder.hxx"

namespace staticcodes {

	static constexpr size_t EOT = ALPHABET; // End of Transmission

	// ------------------------------------------------------
	// --------------------- STATISTICS ---------------------
	// ------------------------------------------------------

	void Statistics::create_range_vector() {
		std::vector<std::pair<uint32_t, int> > sorted_freq;

		for(size_t i = 0; i < m_freq_vec.size(); ++i)
			if (m_freq_vec[i])
				sorted_freq.push_back(std::pair<uint32_t, int>(m_freq_vec[i], i));

		std::stable_sort(sorted_freq.begin(), sorted_freq.end(), std::greater<std::pair<uint32_t, int> >());

		sorted_freq.push_back(std::pair<uint32_t, int>(sorted_freq.back().first, EOT));
		++m_total_chars;

		std::vector<double> sum_ranges(sorted_freq.size() + 1, 0.0);
		for (size_t i = 1; i < sum_ranges.size(); ++i)
			sum_ranges[i] = sum_ranges[i-1] + sorted_freq[i-1].first / static_cast<double>(m_total_chars);

		m_range_vec.clear();
		m_range_vec.resize(ALPHABET + 1);

		for (size_t i = 0; i < sorted_freq.size(); ++i)
			m_range_vec[sorted_freq[i].second] = std::pair<double, double>(sum_ranges[i], sum_ranges[i+1]);
	}

	// ------------------------------------------------------
	// --------------------- ARITHMETIC ---------------------
	// ------------------------------------------------------

	class arithmetic {
	protected:
		size_t m_Max;
		size_t m_half;
		size_t m_quarter;
		size_t m_three_quarters;

		bool scaling(size_t& Low, size_t& High, int& count, bitseq_t& seq) {
			if (High <= m_half) {
				Low  = 2*Low;
				High = 2*High;

				seq.push_back(0);

				for (int i = 0; i < count; ++i)
					seq.push_back(1);
				count = 0;

				return true;
			}
			else if (m_half <= Low) {
				Low  = 2*Low  - m_Max;
				High = 2*High - m_Max;

				seq.push_back(1);

				for (int i = 0; i < count; ++i)
					seq.push_back(0);
				count = 0;

				return true;
			}
			else return false;
		}

		bool expansion(size_t& Low, size_t& High, int& count) {
			if ((m_half < High) && (High <= m_three_quarters) && (m_quarter <= Low) && (Low < m_half)) {
				Low  = 2*Low  - m_half;
				High = 2*High - m_half;
				++count;
				return true;
			}
			else return false;
		}

		void encode_symbol(int symbol, size_t& Low, size_t& High, int& count, range_vec_t const& m_range_vec, bitseq_t& seq) {
			size_t range = High - Low;

			int add_high = std::floor(m_range_vec[symbol].second * range);
			int add_low  = std::floor(m_range_vec[symbol].first  * range);

			High = Low + add_high;
			Low  = Low + add_low;

			while(scaling(Low, High, count, seq) || expansion(Low, High, count))
				;
		}

		bool scaling(size_t& Low, size_t& High, int& l_index, int& r_index, bitseq_t& seq) {
			if ((High <= m_half) || (m_half <= Low)) {
				bool b = seq[l_index];
				Low  = 2*Low  - b*m_Max;
				High = 2*High - b*m_Max;
				++l_index; ++r_index;
				return true;
			}
			else return false;
		}

		bool expansion(size_t& Low, size_t& High, int& l_index, int& r_index, bitseq_t& seq) {
			if ((m_quarter <= Low) && (Low < m_half) && (m_half < High) && (High <= m_three_quarters)) {
				Low  = 2*Low  - m_half;
				High = 2*High - m_half;
				seq[l_index + 1] = seq[l_index];
				++l_index; ++r_index;
				return true;
			}
			else return false;
		}

		size_t get_value(int l_index, int r_index, bitseq_t& seq) {
			size_t value = 0;
			size_t i     = 1;

			for (int index = r_index; index >= l_index; index--) {
				if (seq[index]) value += i;
				i <<= 1;
			}

			return value;
		}

		arithmetic(size_t M)
			: m_Max(M), m_half(M / 2), m_quarter(M / 4), m_three_quarters(m_quarter * 3)
		{ }
	};

	// -------------------------------------------------------
	// ---------------------- CODERIMPL ----------------------
	// -------------------------------------------------------

	class acoder::CoderImpl : private Statistics, private arithmetic {
		bitseq_t m_seq;

		void create_bit_sequence(std::ifstream& ifile) {
			m_seq.clear();

			ifile.clear();
			ifile.seekg(0, std::ios::beg);

			size_t Low  = 0;
			size_t High = m_Max;

			int count = 0;

			char c;
			while (ifile.read(&c, sizeof(c)))
				encode_symbol(static_cast<uint8_t>(c), Low, High, count, m_range_vec, m_seq);
		
			encode_symbol(EOT, Low, High, count, m_range_vec, m_seq);

			if ((Low < m_quarter) && (m_half < High)) {
				m_seq.push_back(0);
				for (int i = 0; i < count + 1; ++i)
					m_seq.push_back(1);
			}

			if ((m_quarter <= Low) && (Low < m_half) && (m_three_quarters <= High)) {
				m_seq.push_back(1);
				for (int i = 0; i < count + 1; ++i)
					m_seq.push_back(0);
			}
		}

	public:
		void compress(std::ifstream& ifile, std::ofstream& ofile) {
			create_freq_vector(ifile);
			create_range_vector();
			create_bit_sequence(ifile);

			for (const auto& freq : m_freq_vec)
				ofile.write(reinterpret_cast<const char*>(&freq), sizeof(freq));

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

		CoderImpl(std::ifstream& ifile, std::ofstream& ofile) : arithmetic(2147483648) {
			compress(ifile, ofile);
		}

		CoderImpl() : arithmetic(2147483648)
		{ }
	};

	void acoder::compress(std::ifstream& ifile, std::ofstream& ofile) {
		m_pImpl->compress(ifile, ofile);
	}

	void acoder::operator()(std::ifstream& ifile, std::ofstream& ofile) {
		m_pImpl->operator()(ifile, ofile);
	}

	acoder::acoder(std::ifstream& ifile, std::ofstream& ofile)
		: m_pImpl(new CoderImpl(ifile, ofile))
	{ }

	acoder::acoder() : m_pImpl(new CoderImpl)
	{ }

	acoder::~acoder()
	{ }

	// -------------------------------------------------------
	// --------------------- DECODERIMPL ---------------------
	// -------------------------------------------------------

	class adecoder::DecoderImpl : private Statistics, private arithmetic {
		bitseq_t m_seq;

		void read_bit_sequence(std::ifstream& ifile) {
			m_seq.clear();

			char curr_byte;
			while (ifile.read(&curr_byte, sizeof(curr_byte)))
				for (size_t bit_counter = 0; bit_counter < CHAR_BIT; ++bit_counter)
					m_seq.push_back(curr_byte & (1 << (7 - bit_counter)));
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

			if (ifile.peek() == EOF) {
				if (!ifile.eof())
					std::cerr << "adecoder::decompress: " << std::strerror(errno) << std::endl;
				return;
			}

			create_range_vector();
			read_bit_sequence(ifile);

			size_t N = std::ceil(std::log2(static_cast<double>(m_Max)));
			int l_index = 0;
			int r_index = N - 1;

			std::vector<bool> temp(r_index, false);
			m_seq.insert(m_seq.end(), temp.begin(), temp.end());

			size_t Low = 0;
			size_t High = m_Max;

			int symbol = 0;
			size_t value;

			uint64_t cnt_chars = 0;

			while (true) {			
				value = get_value(l_index, r_index, m_seq);

				size_t range = High - Low;

				for (size_t i = 0; i < m_range_vec.size(); ++i) {
					double H_i = Low + std::floor(m_range_vec[i].second * range);
					double L_i = Low + std::floor(m_range_vec[i].first * range);

					if ((L_i <= value) && (value < H_i)) {
						symbol = i;
						High = H_i;
						Low  = L_i;
						break;
					}
				}

				while(scaling(Low, High, l_index, r_index, m_seq) || expansion(Low, High, l_index, r_index, m_seq))
					;

				if (symbol == EOT || cnt_chars == m_total_chars - 1)
					break;

				char c = symbol;
				ofile.write(&c, sizeof(c));
				++cnt_chars;
			}
		}

		void operator()(std::ifstream& ifile, std::ofstream& ofile) {
			decompress(ifile, ofile);
		}

		DecoderImpl(std::ifstream& ifile, std::ofstream& ofile) : arithmetic(2147483648) {
			decompress(ifile, ofile);
		}

		DecoderImpl() : arithmetic(2147483648)
		{ }
	};

	void adecoder::decompress(std::ifstream& ifile, std::ofstream& ofile) {
		m_pImpl->decompress(ifile, ofile);
	}

	void adecoder::operator()(std::ifstream& ifile, std::ofstream& ofile) {
		m_pImpl->operator()(ifile, ofile);
	}

	adecoder::adecoder(std::ifstream& ifile, std::ofstream& ofile)
		: m_pImpl(new DecoderImpl(ifile, ofile))
	{ }

	adecoder::adecoder() : m_pImpl(new DecoderImpl)
	{ }

	adecoder::~adecoder()
	{ }

}
