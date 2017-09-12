# libquicknet
#### Simple and small reliable UDP networking library for games

###### :exclamation: libquicknet is under development and not suitable for production code :exclamation:


---
The main target for this library are game prototypes or jam games that require a plug&play, fast networking subsystem.<br>
Very easy to get up and running!

#### Features:
* Connection oriented UDP protocol with 3-step handshake
* Client<->Server and Peer-to-Peer support
* Low bandwidth usage
* Sequenced/Unsequenced Reliable/unreliable support
* Fast redundant acknowledgement system for reliable messages
* Server discovery (LAN only)
* Full checksum system to avoid message corruption
* Optional message merging on send
* Fixed selectable send rate
* Duplicated message detection
* Fake latency and packet loss support
* Ping and Round-Trip-Time estimation
* Cross-platform (Windows/Linux)

#### Future Features:
* Time synchronization
* RNG synchronization for deterministic systems
* Bitpacking & compression
* Symmetric encryption with key exchange on handshake
* NAT traversal and punch-through
* Endianness awareness
* Improve acknowledgement system

---
#### Installation
Drop all the source files in your project and compile!

#### Usage
* include quicknet_peer.h
* create a descendant of quicknet::Peer
* override OnConnection, OnDisconnection and OnGameMessage methods
* Define your custom game messages as instructed in quicknet_messagetypes.h
* Include your message IDs in quicknet_messagelookup.h
* Ready!

For a simple example please check test.cpp

---
#### Background
This library was written in one week, and even if its fully functional it still needs a lot of work to make it even better!<br>
There's still a lot of hardcoded stuff that needs care, but I'll try to turn this into an easily customizable library.
