testplugin.so: Driver1.cpp  Driver1.h  Super.cpp  Super.h  plugin.cpp
	g++ -c -fPIC Driver1.cpp
	g++ -c -fPIC Super.cpp
	g++ -c -fPIC plugin.cpp
	g++ -o testplugin.so -shared -fPIC *.o

clean:
	rm *.o *.so