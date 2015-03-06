GCC = g++
Gcc = gcc

all: sysrepod clientsrd opDStoreClient install

clean:
	rm *.o sysrepod clientsrd libsrd.a server/sysrepod client_SRD/clientsrd

sysrepod: common.o mainSysRepoD.o ClientSet.o DataStore.o DataStoreSet.o Client.o ClientSRD.o global.h application.h OpDataStore.o OpDataStoreSet.o
	$(GCC) -g -o sysrepod mainSysRepoD.o common.o ClientSet.o DataStore.o DataStoreSet.o OpDataStore.o OpDataStoreSet.o Client.o ClientSRD.o -pthread -lxml2
	
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
	
OpDataStore.o: OpDataStore.cpp OpDataStore.h
	$(GCC) -c -g OpDataStore.cpp

OpDataStoreSet.o: OpDataStoreSet.cpp OpDataStoreSet.h
	$(GCC) -I/usr/include/libxml2 -c -g OpDataStoreSet.cpp

clientsrd: mainSRDClient.o srd.o libsrd.a
	$(Gcc) -g -o clientsrd mainSRDClient.o libsrd.a -lxml2

mainSRDClient.o: mainSRDClient.cpp
	$(Gcc) -I/usr/include/libxml2 -g -c mainSRDClient.cpp
	
opDStoreClient: mainOpDStoreMgmtClient.o srd.o libsrd.a
	$(Gcc) -g -o opDStoreClient mainOpDStoreMgmtClient.o libsrd.a -lxml2
	
mainOpDStoreMgmtClient.o: mainOpDStoreMgmtClient.cpp
	$(Gcc) -I/usr/include/libxml2 -g -c mainOpDStoreMgmtClient.cpp
	
srd.o: srd.cpp srd.h
	$(Gcc) -I/usr/include/libxml2 -g -c srd.cpp

libsrd.a: srd.o
	ar rcs libsrd.a srd.o
	
install: sysrepod
	cp sysrepod server
	cp clientsrd client_SRD
	cp opDStoreClient client_SRD
	
	
