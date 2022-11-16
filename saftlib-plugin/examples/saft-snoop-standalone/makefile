#PREFIX=${HOME}/local
PREFIX=/common/export/timing-rte/tg-saftlib-dev/x86_64
LDFLAGS=`PKG_CONFIG_PATH=$(PREFIX)/lib/pkgconfig pkg-config saftlib --libs --define-variable=prefix=$(PREFIX)` `pkg-config sigc++-2.0 --libs`
CXXFLAGS=`PKG_CONFIG_PATH=$(PREFIX)/lib/pkgconfig pkg-config saftlib --cflags --define-variable=prefix=$(PREFIX)` -lsaftd-service -lsaftbus-server -lltdl `pkg-config sigc++-2.0 --cflags`

all: saft-snoop-standalone

clean:
	rm saft-snoop-standalone