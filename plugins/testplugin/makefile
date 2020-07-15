CXXFLAGS+=`pkg-config --cflags sigc++-2.0 saftlib`
DLFLAGS+=`pkg-config --libs sigc++-2.0 saftlib`

testplugin.so: Driver1.cpp  Driver1.h  Super.cpp  Super.h  plugin.cpp
	g++ $(CXXFLAGS) -c -fPIC Driver1.cpp
	g++ $(CXXFLAGS) -c -fPIC Super.cpp
	g++ $(CXXFLAGS) -c -fPIC plugin.cpp
	g++ -o testplugin.so -shared -fPIC *.o $(LDFLAGS)

clean:
	rm *.o *.so