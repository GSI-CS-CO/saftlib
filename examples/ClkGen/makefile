CXXFLAGS=`pkg-config etherbone saftlib --cflags` 
LDFLAGS=`pkg-config etherbone saftlib --libs`

clk-gen: clk-gen.cpp
	g++ -o $@ $< $(CXXFLAGS) $(LDFLAGS)
