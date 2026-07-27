# C++ Port Scanner

Unix/Linux compatible port scanner that displays open ports


## Features:
Flag based port range selection<br>
Multithreaded implementation<br>
Dynamic thread optimization<br>
TCP connect scanning with POSIX sockets<br>


## Running the Program
g++ portscanner.cpp -o ps

./ps -FLAG

replace FLAG with any available flag:

    -s scans system level ports (1 - 1023)
    
    -r scans registered ports (1024 - 49151)
    
    -p scans private ports (49152 - 65535)
    
    -a scanns all ports (1 - 65535)

Example Terminal Input:

g++ portscanner.cpp -o ps

./ps -a

Output:

Open ports on 127.0.0.1:<br>
53<br>
5000<br>
7000<br>
7768<br>
8021<br>
29754<br>
49152<br>
51500<br>
51967<br>
57621<br>





## How It's Made:
### main function
  - Uses getopt to parse terminal flags
  - Flag determine port range
  - calls ```thread_manager```
### thread_manager function
  - stores open ports in a vector
  - determines max threads with ```optimize_max_threads```
  - splits ports across threads
  - synchronizes vector additions with a mutex
### optimize_max_threads function
  - uses ```hardware_concurrency()``` to determine number of available threads
  - defines a minimum and maximum number of threads that can be used
### scan_port_range function
  - each thread scans subset of ports
  - uses POSIX socket API
  - adds port to the vector on a successful connection

