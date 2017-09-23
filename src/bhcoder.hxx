/**
 * bhcoder.hxx
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

#pragma once
#ifndef BHCODER_HXX
#define BHCODER_HXX

#include <fstream>
#include <memory>

namespace contextcodes {

	// -------------------------------------------------------
	// ----------------------- BHCODER -----------------------
	// -------------------------------------------------------

	class bhcoder {
		class CoderImpl;
		std::unique_ptr<CoderImpl> m_pImpl;

	public:

		// Encodes text and writes the final bit sequence to the output file
		void compress(std::ifstream& ifile, std::ofstream& ofile);

		void operator()(std::ifstream& ifile, std::ofstream& ofile);

		bhcoder(std::ifstream& ifile, std::ofstream& ofile);

		bhcoder();

		~bhcoder();
	};

	// -------------------------------------------------------
	// ---------------------- BHDECODER ----------------------
	// -------------------------------------------------------

	class bhdecoder {
		class DecoderImpl;
		std::unique_ptr<DecoderImpl> m_pImpl;

	public:

		// Decodes text and writes the final bit sequence to the output file
		void decompress(std::ifstream& ifile, std::ofstream& ofile);

		void operator()(std::ifstream& ifile, std::ofstream& ofile);

		bhdecoder(std::ifstream& ifile, std::ofstream& ofile);

		bhdecoder();

		~bhdecoder();
	};

}

#endif // BHCODER_HXX
