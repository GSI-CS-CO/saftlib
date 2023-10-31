CXXFLAGS = --std=c++11 `pkg-config saftlib --cflags`
LDLIBS  = `pkg-config saftlib --libs`

all: serial-txrx-saftbus serial-txrx-standalone
serial-txrx-saftbus:    serial-txrx-saftbus.cpp
serial-txrx-standalone: serial-txrx-standalone.cpp

.PHONY: clean
clean:
	rm -f serial-txrx-saftbus serial-txrx-standalone