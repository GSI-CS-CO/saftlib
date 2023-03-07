# SimpleFirmware an instructive example of how to write an LM32 firmware driver plugin for saftlib

Saftlib version 3 introduces the possibility to extend the functionality of a running saftlib daemon
during runtime by loading shared objects as plugins. This can be used to adapt the daemon to a change
of LM32 firmware without stopping all saftlib services.