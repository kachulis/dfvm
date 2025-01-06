CXX=g++ -std=c++17

CXXFLAG_DBG=-g
CXXFLAG=-O3
CXXFLAG_PG=-pg


LDFLAG=-lhts

UNAME_S := $(shell uname -s)

HFILE=$(shell find . -maxdepth 1 -name "*.hh")
OFILE=$(shell for file in `find . -maxdepth 1 -name "*.cpp"`; do echo $$(basename $$file .cpp).o; done)
OFILE_DBG=$(shell for file in `find . -maxdepth 1 -name "*.cpp"`; do echo debug_$$(basename $$file .cpp).o; done)
OFILE_PG=$(shell for file in `find . -maxdepth 1 -name "*.cpp"`; do echo prof_$$(basename $$file .cpp).o; done)

ifeq ($(UNAME_S),Darwin)
	LDFLAG += -L/opt/homebrew/lib -lboost_program_options -mmacosx-version-min=14.0
	INCFLAG = -I/opt/homebrew/include/
endif
ifeq ($(UNAME_S),Linux)
	LDFLAG += -L/usr/local/lib -lboost_program_options -lpthread
	INCFLAG = -I/usr/local/include/htslib
endif
.PHONY: all clean debug profile

all: dfvm

debug: dfvm_debug

profile: dfvm_prof

clean:
	rm -f dfvm dfvm_debug dfvm_prof *.o

dfvm: $(OFILE)
	$(CXX) $^ $(CXXFLAG) $(LDFLAG) $(INCFLAG)  -o dfvm

%.o:%.cpp $(HFILE)
	$(CXX) $(CXXFLAG) -c $< -o $@ $(INCFLAG)

debug_%.o:%.cpp
	$(CXX) $(CXXFLAG_DBG) -c $< -o $@ $(INCFLAG)

prof_%.o:%.cpp 
	$(CXX) $(CXXFLAG_PG) -c $< -o $@ $(INCFLAG)

dfvm_debug: $(OFILE_DBG)
	$(CXX) $^ $(LDFLAG) $(CXXFLAG_DBG) $(INCFLAG)  -o dfvm_debug

dfvm_prof: $(OFILE_PG)
	$(CXX) $^ $(LDFLAG) $(CXXFLAG_PG) $(INCFLAG)  -o dfvm_prof