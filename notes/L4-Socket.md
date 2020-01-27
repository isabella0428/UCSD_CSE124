## L04	Sockets

#### Definition

* The point where a local application process attaches to the network

* Interface Between network and application
* Created by application



#### Socket Working Process

* Being created
* Being connected to the network
* Changing messages
* Being closed



#### Ports

Used to identify different applications(processes) in the same host



#### Socket Implementation

##### 1. Socket Type

* PF_INET

  * **INET** denotes the internet family. If u want to communicate with a remote host, u should use this.

  * An **INET** socket is bound to a specific IP address.

  * INET sockets sit at the top of a full TCP/IP stack, with traffic congestion algorithms, backoffs and the like to handle

* PF_UNIX

  * An **UNIX** socket is bound to a special file on our file system.

  * We use it as an a lightweight alternative to an `INET` socket via loopback, when u need communication between processes on the same host.
  * everything is designed to be local to the machine, so its code is much simpler and the communication is faster

* PF_PACKET

  * Used if u want to play with packets at the protocol level, if u are implementing ur own protocol
  * denotes direct access to the network interface (i.e., it bypasses the TCP/IP protocol stack)

**Remark**  AF_ and PF_ have the same values. And u can just use them alternatively, but AF_ seem to be more prevalent.