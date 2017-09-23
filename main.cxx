/**
 * main.cxx
 *
 * libcoders Library Entry Point
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
#include <cstdint>
#include <cstring>
#include <cerrno>
#include <string>
#include <chrono>
#include <unistd.h>       // getopt, usleep
#include <linux/limits.h> // PATH_MAX
#include <sys/types.h>    // S_ISREG
#include <sys/stat.h>     // struct stat
#include "src/pcoder.hxx"
#include "src/bhcoder.hxx"
#include "src/ahcoder.hxx"
#include "src/acoder.hxx"

#define ERROR_CODING_METHOD   ( -1)
#define ERROR_IFILE_PATH      ( -2)
#define ERROR_OFILE_PATH      ( -3)
#define ERROR_OPTION_TYPE     ( -4)
#define ERROR_OPTION_NUMBER   ( -5)
#define ERROR_IS_REGULAR_FILE ( -6)
#define ERROR_FILE_OPEN       ( -7)
#define ERROR_FILE_EXIST      ( -8)

using std::cout;
using std::endl;
using std::cerr;
using std::flush;
using std::string;

using staticcodes::shennon;
using staticcodes::fano;
using staticcodes::huffman;

int    is_regular_file(char const* path);
int    prepare_input_file(char const* ifilename, std::ifstream& ifile);
int    prepare_output_file(char const* ofilename, std::ofstream& ofile);
string help();

int main(int argc, char* argv[]) {
	int opt    =  0;
	int inv    = -1;
	int method =  0;
	char* ifilename = nullptr;
	char* ofilename = nullptr;

	// Command line options
	if (argc == 8 && std::strcmp(argv[1], "-h"))
		while ((opt = getopt(argc, argv, "cdi:o:m:")) != -1)  {
			switch (opt) {
				case 'c' :
					inv = 0;
					break;
				case 'd' :
					inv = 1;
					break;
				case 'i' :
					ifilename = optarg;
					if (std::strlen(ifilename) > PATH_MAX) {
						cerr << "main: Invalid input path, rerun with -h for help" << endl;
						return ERROR_IFILE_PATH;
					}
					break;
				case 'o' :
					ofilename = optarg;
					if (std::strlen(ofilename) > PATH_MAX) {
						cerr << "main: Invalid output path, rerun with -h for help" << endl;
						return ERROR_OFILE_PATH;
					}
					break;
				case 'm' :
					if      (!std::strcmp(optarg, "shennon"))    method = 1;
					else if (!std::strcmp(optarg, "fano"))       method = 2;
					else if (!std::strcmp(optarg, "huffman"))    method = 3;
					else if (!std::strcmp(optarg, "bhuffman"))   method = 4;
					else if (!std::strcmp(optarg, "ahuffman"))   method = 5;
					else if (!std::strcmp(optarg, "arithmetic")) method = 6;
					else {
						std::cerr << "main: Invalid coding method, rerun with -h for help" << std::endl;
						return ERROR_CODING_METHOD;
					}
					break;
				case '?' :
					cerr << "main: Invalid option, rerun with -h for help" << endl;
					return ERROR_OPTION_TYPE;
			}
		}
	else if (argc == 2 && !std::strcmp(argv[1], "-h")) {
		cout << help();
		return 0;
	}
	else {
		cerr << "main: Invalid number of options, rerun with -h for help" << endl;
		return ERROR_OPTION_NUMBER;
	}

	// Working with files
	std::ifstream ifile;
	if (int errcode = prepare_input_file(ifilename, ifile))
		return errcode;

	std::ofstream ofile;
	if (int errcode = prepare_output_file(ofilename, ofile))
		return errcode;

	// Main part
	if (!inv) {
		cout << "Compressing, please wait... " << flush;

		auto start = std::chrono::steady_clock::now();
		if      (method == 1) staticcodes  ::pcoder<shennon> s(ifile, ofile);
		else if (method == 2) staticcodes  ::pcoder<fano>    f(ifile, ofile);
		else if (method == 3) staticcodes  ::pcoder<huffman> h(ifile, ofile);
		else if (method == 4) contextcodes ::bhcoder         bh(ifile, ofile);
		else if (method == 5) adaptivecodes::ahcoder         ah(ifile, ofile);
		else if (method == 6) staticcodes  ::acoder          a(ifile, ofile);
		auto end  = std::chrono::steady_clock::now();
		auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

		cout << "Done" << flush;
		usleep(250000);
		cout << endl << endl;

		ifile.clear();
		ifile.seekg(0, std::ios::end);
		uint64_t isize = ifile.tellg();

		double isize_kb = static_cast<double>(isize) / 1024;
		double osize_kb = static_cast<double>(ofile.tellp()) / 1024;
		int    ratio    = (isize_kb - osize_kb) / isize_kb * 100;

		std::cout.precision(6);
		cout << "Original file:     "        << ifilename << endl;
		cout << "Compressed file:   "        << ofilename << endl;
		cout << "--------------------"       << endl;
		cout << "STATS"                      << endl;
		cout << "Original file size:     "   << isize_kb  << " Kbyte"        << endl;
		cout << "Compressed file size:   "   << osize_kb  << " Kbyte"        << endl;
		cout << "Compression ratio:      "   << ratio     << '%'             << endl;
		cout << "Time taken:             "   << diff      << " milliseconds" << endl;
		cout << std::fixed;
	}
	else {
		cout << "Decompressing, please wait... " << flush;

		auto start = std::chrono::steady_clock::now();
		if      (method == 1) staticcodes  ::pdecoder<shennon> s(ifile, ofile);
		else if (method == 2) staticcodes  ::pdecoder<fano>    f(ifile, ofile);
		else if (method == 3) staticcodes  ::pdecoder<huffman> h(ifile, ofile);
		else if (method == 4) contextcodes ::bhdecoder         bh(ifile, ofile);
		else if (method == 5) adaptivecodes::ahdecoder         ah(ifile, ofile);
		else if (method == 6) staticcodes  ::adecoder          a(ifile, ofile);
		auto end  = std::chrono::steady_clock::now();
		auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

		cout << "Done" << flush;
		usleep(250000);
		cout << endl << endl;

		cout << "Original file:       " << ifilename << endl;
		cout << "Decompressed file:   " << ofilename << endl;
		cout << "Time taken:          " << diff      << " milliseconds" << endl;
	}

	// Finish
	ifile.close();
	ofile.close();

	return 0;
}

int is_regular_file(char const* path) {
	struct stat s;
	stat(path, &s);
	return S_ISREG(s.st_mode);
}

int prepare_input_file(char const* ifilename, std::ifstream& ifile) {
	if (!is_regular_file(ifilename)) {
		cerr << "prepare_input_file: No such input file or input file is not a regular file, rerun with -h for help" << endl;
		return ERROR_IS_REGULAR_FILE;
	}

	ifile.open(ifilename, std::ios::binary);

	if (!ifile.is_open()) {
		cerr << "prepare_input_file: " << std::strerror(errno) << endl;
		return ERROR_FILE_OPEN;
	}

	return 0;
}

int prepare_output_file(char const* ofilename, std::ofstream& ofile) {
	/* if (std::ifstream(ofilename)) {
		cerr << "prepare_output_file: File already exists" << endl;
		return ERROR_FILE_EXIST;
	} */

	ofile.open(ofilename, std::ios::binary);

	if (!ofile.is_open()) {
		cerr << "prepare_output_file: " << std::strerror(errno) << endl;
		return ERROR_FILE_OPEN;
	}

	return 0;
}

string help() {
	return
		"REQUIRED OPTIONS\n"
		"	-c | -d\n"
		"	    Compressing | decompressing operation respectively\n"
		"\n"
		"	-i input\n"
		"	    Input file, i can be either a full path to a regular file\n"
		"	    or a filename of a regular file (if the file is in current directory)\n"
		"	    with maximum length of PATH_MAX (see value in <linux/limits.h>)\n"
		"\n"
		"	-o output\n"
		"	    Output file, o can be either a full path with a filename\n"
		"	    or a filename (file will be created in current directory)\n"
		"	    with maximum length of PATH_MAX (see value in <linux/limits.h>)\n"
		"\n"
		"	-m method\n"
		"	    Coding method, m can be \"shennon\", \"fano\", \"huffman\",\n"
		"	    \"bhuffman\", \"ahuffman\" or \"arithmetic\"\n"
		"\n";
}
