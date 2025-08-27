# ABOUT

This is a passion project running on a server of hopes and dreams. The server is 8 2015 Mac Minis and 1 old thinkpad edu edition connected via ethernet.
If you want to steal some of my code for your own projects please do! You will have to configure my settings and such so that it fits your machine specs, but by all means go for it.

# CREDITS

Gotta credit some resources and people for helping me learn what I needed to learn for this project:
- [Varta Learning Platform Youtube](https://www.youtube.com/@vartetalearningplatform2271)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/html/split-wide/index.html)
- [Learn to Build a Neural Network From Scratch — Yes, Really.](https://medium.com/@waadlingaadil/learn-to-build-a-neural-network-from-scratch-yes-really-cac4ca457efc)
- Thank you Scott and thank you Colin.

# Current Working Plan:

~Master computer has thread with a blocking socket listening for as many connections as computers I have (8 specifically)~
~Master computer will ping all of the servers and await responses, the responses should consist of IP, Ram, and Threads.~
Master computer will then write the config for the model; based on the resources of each computer, it will split the full model into layers where computer one gets layers 1-3 and computer two gets 4-7 and so on until all computers have a distributed workload. 
After responding to master originally the node computers will await configurations from the server.
Master computer will send each node configs that discloses the size of arrays they should store in the heap, the address of the next node in the connection, the number of nodes in the sequence, whether they should expect a computer to use them as host (for end behavior).
Each computer will then run their newly configured generalized program and when the have successfully opened a connection to the correct addresses ping the master computer and await a response. 
Master computer will then send the loaded weights and biases for each node to each node respectively. If there is no model being loaded and this is the first time the master computer will send an empty array singling to the nodes to just randomly assign numbers to their arrays.
After that the master computer will start sending data through to the first node, it will process then send it through and so on until the last computer has had a chance. And after the first node has received the correct amount of data it will start back propagation.
The way the program will be structured on each node is a worker master queue system where on each node there is a master thread that creates and manages a work queue that the worker nodes (all other nodes) will pull and work from. Once a set of tasks is done the master thread will communicate that to the master computer who will then instruct it when to send to the next computer. The master thread will have 2 to 3 sockets open connecting to the previous next and master computers

- Jaxon Durken
