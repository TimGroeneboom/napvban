# napvban

Stream lossless low-latency audio over the network using the VBAN protocol.

The demo demonstrates sending and receiving audio over localhost. You can use this as a starting point to implement your own use case.

The main purpose is to have the lowest possible latency. To allow more latency you can increase the allowed latency in samples on the VBANStreamPlayerComponent.

Audio is converted into 16 bit PCM Wave format internally. SampleRate and channels can vary depending on settings.

The VBAN protocol specification can be found [here](VBANProtocol_Specifications.pdf)
