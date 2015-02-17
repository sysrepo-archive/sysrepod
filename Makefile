GCC = g++

all: sysrepod clientsrd install

clean:
	rm *.o sysrepod clientsrd libsrd.a server/sysrepod client_SRD/clientsrd

sysrepod: common.o mainSysRepoD.o ClientSet.o DataStore.o DataStoreSet.o Client.o ClientSRD.o global.h application.h
	$(GCC) -g -o sysrepod mainSysRepoD.o common.o ClientSet.o DataStore.o DataStoreSet.o Client.o ClientSRD.o -pthread -lxml2
	
mainSysRepoD.o: mainSysRepoD.cpp global.h application.h
	$(GCC) -c -I/usr/include/libxml2 -g mainSysRepoD.cpp
	
common.o: common.cpp common.h
	$(GCC) -c -g common.cpp

ClientSet.o: ClientSet.cpp ClientSet.h
	$(GCC) -I/usr/include/libxml2 -c -g ClientSet.cpp
	
Client.o: Client.cpp Client.h
	$(GCC) -c -g Client.cpp
	
ClientSRD.o: ClientSRD.cpp ClientSRD.h
	$(GCC) -I/usr/include/libxml2 -c -g ClientSRD.cpp
	
DataStore.o: DataStore.cpp DataStore.h
	$(GCC) -I/usr/include/libxml2 -c -g DataStore.cpp

DataStoreSet.o: DataStoreSet.cpp DataStoreSet.h
	$(GCC) -I/usr/include/libxml2 -c -g DataStoreSet.cpp

clientsrd: mainSRDClient.o srd.o libsrd.a
	$(GCC) -g -o clientsrd mainSRDClient.o libsrd.a -lxml2

mainSRDClient.o: mainSRDClient.cpp
	$(GCC) -g -c mainSRDClient.cpp
	
srd.o: srd.cpp srd.h
	$(GCC) -I/usr/include/libxml2 -g -c srd.cpp

libsrd.a: srd.o
	ar rcs libsrd.a srd.o
	
install: sysrepod
	cp sysrepod server
	cp clientsrd client_SRD
	
