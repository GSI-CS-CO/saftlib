# Saftbus

The saftbus backage provides 
  - a deamon that provides services which can be accessed through a UNIX domain socket. New services can be added by plugins at startup or during runtime of the daemon.
  - a library and a code generator that facilitates developments of plugins for the daemon.
  - a command line tool to control the daemon.

## user guide

To get something like saftlib-v2, the following 2 steps are needed:
  - compile and install saftbus
    - ./autogen.sh
    - ./configure --prefix=/install/directory
    - make install
  - compile and install saftlib-plugin
    - cd saftlib-plugin
    - ./autogen.sh
    - ./configure --prefix=/install/directory
    - make install

In saftlib-v2, saftd was started like this for example:

    safd tr0:dev/wbm0

To do the same in saftlib-v3, the following is needed:

    saftbusd libsaftd-service.la tr0:dev/wbm0 &

The saftbusd programm is not a deamon yet and it has to be detached from the terminal manually.

The saftlib-plugin installs a wrapper script "saftd" that allows to use the saftlib-v2 syntax to start saftbusd with the saftlib-plugin.

## use saftlib in an application

the saftlib-plugin installs a pkg-config file. In order to compile an application, use the following:

    g++ `pkg-config saftlib --libs --cflags` -o application application.cpp
    

