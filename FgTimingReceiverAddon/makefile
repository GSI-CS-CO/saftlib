addon: addon.cpp SimpleFirmware.cpp SimpleFirmware.hpp
	g++ -o addon addon.cpp SimpleFirmware.cpp `pkg-config saftbus saftlib --libs --cflags` -lsaftbus-standalone -lsaftd-service -Wl,-rpath=/home/michael/.local/lib
