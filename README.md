# napvban

Stream lossless low-latency audio over the network using the VBAN protocol.

The demo demonstrates sending and receiving audio over localhost. You can use this as a starting point to implement your own use case.

The main purpose is to have the lowest possible latency. To allow more latency you can increase the allowed latency in samples on the VBANStreamPlayerComponent.

Audio is converted into 16 bit PCM Wave format internally. SampleRate and channels can vary depending on settings.

The VBAN protocol specification can be found [here](VBANProtocol_Specifications.pdf)

## Installation
Compatible with NAP 0.6 and higher - [package release](https://github.com/napframework/nap/releases) and [source](https://github.com/napframework/nap) context.

### From ZIP

[Download](https://github.com/TimGroeneboom/napvban/archive/refs/heads/main.zip) the module as .zip archive and install it into the nap `modules` directory:

```
cd tools
./install_module.sh ~/Downloads/napvban-main.zip
```

### From Repository

Clone the repository and setup the module in the nap `modules` directory.

```
cd modules
clone https://github.com/TimGroeneboom/napvban.git
./../tools/setup_module.sh napvban
```

