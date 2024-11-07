CXX=g++

CXXFLAG=-g -std=c++11 -mmacosx-version-min=14.0 -O3

LDFLAG=-L/opt/homebrew/lib -lhts -lboost_program_options

#LDFLAG=/opt/homebrew/lib/libhts.a

INCFLAG=-I/opt/homebrew/include/

.PHONY: all clean

all: dfvm

clean:
	rm dfvm

dfvm: dfvm.cpp
	$(CXX) -v $(LDFLAG) $(CXXFLAG) $(INCFLAG) dfvm.cpp -o dfvm

