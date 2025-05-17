Just some small webservers running on ESP8266.

This started as my desktop computer did not have Wake on Lan functionality, so I took an ESP8266, a relay, and other necessary components (see wiring diagram) and put in a small 3D-printed boxed, connecting to a USB header on the motherboard for power, and connecting to the power button of the PC, in parallell.
I have a reverse proxy on my home network, sending outside traffic (via a subdomain I own) to the ESP8266, prompting a login. There are also functions that (supposedly) only work from inside the local network, which means I can let Home Assistant send signals.
It is possible to both send a "sleep/wake up"-signal, which closes the relay for 200 milliseconds, or send a longer signal, that will do a hard shutdown of the computer.
This small webserver makes it possible to start/wake/put my PC to sleep from wherever I am, would it be needed.
The same webserver checks for changes to my public IP, and updates my domains.

