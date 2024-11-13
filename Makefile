CXX=g++

#CXXFLAG=-g -std=c++17 -O3
CXXFLAG=-std=c++17 -O3

LDFLAG=-lhts 

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
	LDFLAG += -L/opt/homebrew/lib -lboost_program_options -mmacosx-version-min=14.0
	INCFLAG = -I/opt/homebrew/include/
endif
ifeq ($(UNAME_S),Linux)
	LDFLAG += -L/usr/local/lib -lboost_program_options
	INCFLAG = -I/usr/local/include/htslib
endif
.PHONY: all clean

all: dfvm

clean:
	rm dfvm

dfvm: dfvm.cpp
	$(CXX) dfvm.cpp $(LDFLAG) $(CXXFLAG) $(INCFLAG)  -o dfvm

