# Loadbalancer
This project is based on creating a load balancer that plays a role as a bridge for multi-threaded HTTP servers and clients. It will frequently request a healthcheck of all servers to see each server's performance. 

## Usage
to build an executable, type
```bash
make
```
to run the load balancer (**server/servers need to be up** or 500 error)
```bash
./loadbalancer clientport serverport -N n -R r 
```
#### you have specify *at least one port* number for a server/servers and *one port numer* for the client
optional:
n:   number of parallel connections that can be handled at the same time (default n = 4)
r: request a health check of all servers in every r requests
(default r = 5)

### PUT command: upload a file to the server
```bash
curl -v -T FILENAME http://localhost:8080
```
this will upload a file named FILENAME to the server

### GET command: to get the contents of a file 
```bash
curl -v http://localhost:8080/FILENAME
```
this will show the contents of a file named FILENAME to the client
```bash
curl -v http://localhost:8080/FILENAME > FILENAME
```
this will download the file to the client diectory

### HEAD command: to get the header of a file 
```bash
curl -v -I http://localhost:8080/FILENAME
```
this will show the exact same header as GET
### Healthcheck
```bash
curl -v http://localhost:8080/healthcheck
```
this will provides the record of how many errors and entries are there. 