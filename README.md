# napvban

Stream lossless low-latency audio over the network using the VBAN protocol.

The demo demonstrates sending and receiving audio over localhost. You can use this as a starting point to implement your own use case.

The main purpose is to have the lowest possible latency. To allow more latency you can increase the allow latency in samples on the VBANStreamPlayerComponent.

The VBAN protocol specification can be found [here](https://vb-audio.com/Voicemeeter/VBANProtocol_Specifications.pdf)
