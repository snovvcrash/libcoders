/**
 * acoder.hxx
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

#pragma once
#ifndef ACODER_HXX
#define ACODER_HXX

#include <fstream>
#include <memory>

namespace staticcodes {

	// -------------------------------------------------------
	// ----------------------- ACODER ------------------------
	// -------------------------------------------------------

	class acoder {
		class CoderImpl;
		std::unique_ptr<CoderImpl> m_pImpl;

	public:

		// Encodes text and writes the final bit sequence to the output file
		void compress(std::ifstream& ifile, std::ofstream& ofile);

		void operator()(std::ifstream& ifile, std::ofstream& ofile);

		acoder(std::ifstream& ifile, std::ofstream& ofile);

		acoder();

		~acoder();
	};

	// -------------------------------------------------------
	// ----------------------- DECODER -----------------------
	// -------------------------------------------------------

	class adecoder {
		class DecoderImpl;
		std::unique_ptr<DecoderImpl> m_pImpl;

	public:

		// Decodes text and writes the final bit sequence to the output file
		void decompress(std::ifstream& ifile, std::ofstream& ofile);

		void operator()(std::ifstream& ifile, std::ofstream& ofile);

		adecoder(std::ifstream& ifile, std::ofstream& ofile);

		adecoder();

		~adecoder();
	};

}

#endif // ACODER_HXX
