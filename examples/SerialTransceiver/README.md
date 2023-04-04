# Use a TimingReceiver IO as serial transceiver

The implemented serial protocol uses one start bit, and one stop bit, most significant bit first.

This examples demonstrates several things:
  - How to use signal groups to keep multiple threads decoupled with respect to saftlib signals.
  - One possibility on how to reuse an existing software action sink and a software condition from another process, and how to detect the destruction of an disowned software condition by connecting a callback to Owned::Destroyed.

## TX ouput

Sending serial data works by configuring the ECA output action channel with two conditions, one for the rising edge of the output and one for the falling edge of the output.
The byte to be sent is analyzed and events are injected such that the output emits the correct sequence of bits.

## RX input

The selected input is configured to produce an event for every detected edge on the signal. 
A software condition is configured to react on this event. 
The callback function analyses the evens and reconstructs the byte from the edge times.

## Threads

RX and TX functionality both run in separate threads.
This allows the application to send and receive data streams on independent IOs.
The client side API of saftlib is safe to use from different threads.
Proxies protect shared resources with a mutex.
A saftlib signal callback function is always called from within the `wait_for_signal` function.
Resources used in signal callback functions might need manual mutex protection.
Signal groups allow to isolate the saftlib signal processing of different threads.
It is not possible to create new proxy objects with a signal group, while a signal callback is in progress on that same signal group.

In this application, each thread creates its own SignalGroup.
A SignalGroup is the entity that provides the `wait_for_signal` function, which receives from services and dispatches them to all Proxy objects that were created with this signal group.
```C++
void rx_thread(const std::string &DeviceName, uint32_t bps, const std::string &IoName) {
  uint64_t prefix = 0xffffe00000000000;
  saftbus::SignalGroup rx_group; // specify a signal group for this thread
  auto saftd = saftlib::SAFTd_Proxy::create("/de/gsi/saftlib", rx_group);
  // ...
```

proxy objects that are created and used in one thread only, the thread logic can be completely isolated from other threads. This is not really needed in the serial transceiver example application, but it is in general advisable to decouple different parts of a program as much as possible.

