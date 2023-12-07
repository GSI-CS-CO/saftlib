# A standalone commandline tool for IO control

Modular design of saftlib v3 allows to use the IO controller wishbone slave in a standalone application.
This is an example of how to this. 

## Use case for the tool

For some applications, older vesions of timing receivers are in use.
MSI on USB is not yet supportet on these hardware versions and saft-io-ctl is not usable via USB.
The saft-io-standalone example tool provides access to timing receiver IOs in this situation.
