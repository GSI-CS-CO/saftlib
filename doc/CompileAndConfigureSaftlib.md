# Saftlib Configuration, Compilation and Deployment

## Configuration

**Getting Interface documentation**

You need to have installed the following packages (debian) before you configure Saftlib:

* docbook-utils
* libglib2.0-dev

## Compilation

1. Check out saftlib (git clone https://github.com/GSI-CS-CO/saftlib.git)
2. Go to the project directory: `cd saftlib`
3. Run autogen: `./autogen.sh`
4. Run configure: `/configure` (important arguments may be: --prefix=/usr --sysconfdir=/etc)

## Install Locally

1. Install: `make install`

## Deployment

1. Create a tar-ball for your target system: `make dist`

## Known Problems

### Compilation: "Fatal format file error; I'm stymied"

(La)TeX applications often fail with this error when you’ve been playing with the configuration, or have just installed a new version. 

Solution: 

1. $_ `fmtutil --all`
2. $_ `make clean`
3. $_ `make`

Resources:
* http://www.tex.ac.uk/FAQ-formatstymy.html

### Compilation: "Error message: ./configure: line 16708: syntax error near unexpected token `0.23' ./configure: line 16708: `PKG_PROG_PKG_CONFIG(0.23)'"

"pkg-config" is missig.

Solution:

1. $_ `sudo apt-get install pkg-config`
2. $_ `export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig`

### Compilation: "#error This file requires compiler and library support for the ISO C+ #error This file requires compiler and library support"

You need to use the compilation flag for C++ 11

Solution:

1. $_ `./configure CPPFLAGS=-std=c++11`
2. $_ `make`

Resources:
* https://github.com/GSI-CS-CO/saftlib/issues/2

### Compilation: "configure: error: Package requirements (etherbone >= 0.1) were not met: No package 'etherbone' found"

Consider adjusting the PKG_CONFIG_PATH environment variable if you
installed software in a non-standard prefix.

Solution:

1. $_ `export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig`

Resources:
* https://github.com/GSI-CS-CO/saftlib/issues/1

### Application: "Unable to acquire name---dbus saftlib.conf installed?"

If you tried to start the saftlib daemon (i.e. saftd baseboard:dev/wbm0) and this error occurs make sure saftlib.conf is placed in the right directory.

Solution (Debian-based distribution):

By default the saftlib.conf is copied to "/usr/local/etc/dbus-1/system.d/", but Debian is expecting the file in "/etc/dbus-1/system.d/".

1. $_ `./configure --prefix=/usr --sysconfdir=/etc`
2. $_ `make install`

You should now find the file in "/etc/dbus-1/system.d/".

### Compilation: "./configure: line 16708: syntax error near unexpected token `0.23' and ./configure: line 16708: `PKG_PROG_PKG_CONFIG(0.23)'"

You need the pkgconfig/pkg-config package to proceed.

Solution:

1. $_ `sudo apt-get install pkgconfig`
