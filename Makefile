#
# Makefile
#
# libcoders Builder
# by snovvcrash
# 04.2017
#

#
# Copyright (C) 2017 snovvcrash
#
# This file is part of libcoders.
#
# libcoders is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# libcoders is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with libcoders.  If not, see <http://www.gnu.org/licenses/>.
#

CXXTARGET = libcoders
CTARGET   =

CXX = g++
CC  = gcc

CXXFLAGS += -Wall -c -std=c++11 -O2
CFLAGS   += -Wall -c
LDFLAGS  += -Wall

CXXHEADERS = $(wildcard *.hxx) $(wildcard */*.hxx)
CXXSOURCES = $(wildcard *.cxx) $(wildcard */*.cxx)
CXXOBJECTS = $(patsubst %.cxx, %.o, $(CXXSOURCES))
CHEADERS   = $(wildcard *.h) $(wildcard */*.h)
CSOURCES   = $(wildcard *.c) $(wildcard */*.c)
COBJECTS   = $(patsubst %.c, %.o, $(CSOURCES))

.PHONY: cxxbuild cbuild all default clean
.PRECIOUS: $(CXXTARGET) $(CTARGET) $(CXXOBJECTS) $(COBJECTS)

all: clean default
default: cxxbuild
cxxbuild: $(CXXTARGET)
	@echo "Build cxx-project"
cbuild: $(CTARGET)
	@echo "Build c-project"

$(CXXTARGET): $(CXXOBJECTS)
	@echo "(CXX) $?"
	@$(CXX) $(CXXOBJECTS) $(LDFLAGS) -o $@

$(CTARGET): $(COBJECTS)
	@echo "(CC) $?"
	@$(CC) $(COBJECTS) $(LDFLAGS) -o $@
	
%.o: %.cxx $(CXXHEADERS)
	@echo "(CXX) $<"
	@$(CXX) $(CXXFLAGS) $< -o $@

%.o: %.c $(CHEADERS)
	@echo "(CC) $<"
	@$(CC) $(CFLAGS) $< -o $@

debug: CXXFLAGS += -DDEBUG -g -O0
debug: CFLAGS   += -DDEBUG -g -O0
debug: all
	@echo "DEBUG MODE"

clean:
	@echo "Clean project"
	@rm -rfv *.o */*.o $(CXXTARGET) $(CTARGET)
