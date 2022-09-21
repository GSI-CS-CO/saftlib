# Saftlib

Saftlib provides the following
  * a C++ class library to access Fair Timing Receiver Hardware. It can be used to configure the ECA channels, create conditions and receive software Interrupts from timing events. This library can be used in stand-alone mode, or it can be dynamically loaded into a running saftbus daemon.
  * various command line tools to control and interact with attached hardware devices

## user guide

## plugin developer guide

## developer guide

### Structure of the library
#### SAFTd
The name "SAFTd" is kept for backwards compatibility with older saftlib versions, in order to keep the user facing API stable.
A better name would be EtherboneSocket, because it encapsulates an etherbone::Socket together with some additional functions.
SAFTd provides:
 * An instance of an etherbone::Socket with an eb_slave device connected to it in order to receive MSIs
 * Redistribution of incoming MSIs to callback functions
 * A container of TimingReceiver objects (std::vector<std::unique_ptr<TimingReceiver> >). 




