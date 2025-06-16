### Overview
This project implements a networked block-level storage client that communicates with a remote JBOD server using a TCP-based protocol. 
The client imitates part of a simplified RAID management tool and can issue read/write commands over the network to a server-side JBOD system. The system is designed to
support flexible, scalable, and fault-tolerant data storage.


### Project Features
Overall, the project simulates a distributed storage environment that include the following features:

- Client-side logic implemented in C using TCP sockets.
- JBOD operations sent over the network using a structured binary protocol.
- Remote JBOD server that simulates disks and handles commands (provided as a testing binary)
- Caching mechanism used to reduce disk access latency and improve performance.

