# RicartAgrawalaAlgorithm
Implementation for the Ricart-Agrawala algorithm for distributed mutual exclusion, with the optimization proposed by Roucairol and Carvalho, in a client-server model.

Compile:

```
make
```
This will create server and client binaries.

Run:
```
./server <IP1> <IP2> <serverId> <portno>
```

```
./client <clientId> <portno> <IP>
```

For executing on UTD dcxx machines please find commands in file run.txt
