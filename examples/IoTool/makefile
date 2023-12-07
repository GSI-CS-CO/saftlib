CXXFLAGS = --std=c++11 `pkg-config saftlib etherbone --cflags`
LDLIBS  = `pkg-config saftlib etherbone --libs` -ldl

all: saft-io-standalone
saft-io-standalone: saft-io-standalone.cpp

.PHONY: clean
clean:
	rm -f saft-io-standalone
