# Use a TimingReceiver IO as serial transceiver

The implemented serial protocol uses one start bit, and one stop bit, most significant bit first.

This examples demonstrates several things:
  - How to use signal groups to keep multiple threads decoupled with respect to saftlib signals.
  - One possibility on how to reuse an existing software action sink and a software condition from another process, and how to detect the destruction of an unowned software condition by connecting a callback to Owned::Destroyed.

## TX ouput

Sending serial data works by configuring the ECA output action channel with two conditions, one for the rising edge of the output and one for the falling edge of the output.
The byte to be sent is analyzed and events are injected such that the output emits the correct sequence of bits.


## RX input

The selected input is configured to produce an event for every detected edge on the signal. 
A software condition is configured to react on this event. 
The callback function analyses the evens and reconstructs the byte from the edge times.

