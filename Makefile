GCC = g++
Gcc = gcc

all: sysrepod client_SRD/clientsrd client_SRD/opDStoreClient client_SRD/opDStoreSubTree1 client_SRD/opDStoreSubTree2 client_SRD/opDStoreGetSubTree install

clean:
	rm *.o sysrepod lib/libsrd.a server/sysrepod client_SRD/clientsrd client_SRD/opDStoreClient client_SRD/opDStoreSubTree1 client_SRD/opDStoreSubTree2 client_SRD/opDStoreGetSubTree client_SRD/*.o

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

client_SRD/clientsrd: client_SRD/mainSRDClient.o srd.o lib/libsrd.a
	$(Gcc) -g -o client_SRD/clientsrd client_SRD/mainSRDClient.o lib/libsrd.a -lxml2

client_SRD/mainSRDClient.o: client_SRD/mainSRDClient.c
	$(Gcc) -I/usr/include/libxml2 -I. -g -o client_SRD/mainSRDClient.o -c client_SRD/mainSRDClient.c
	
client_SRD/opDStoreClient: client_SRD/mainOpDStoreMgmtClient.o srd.o lib/libsrd.a
	$(Gcc) -g -o client_SRD/opDStoreClient client_SRD/mainOpDStoreMgmtClient.o lib/libsrd.a -lxml2
	
client_SRD/mainOpDStoreMgmtClient.o: client_SRD/mainOpDStoreMgmtClient.c
	$(Gcc) -I/usr/include/libxml2 -I. -g -o client_SRD/mainOpDStoreMgmtClient.o -c client_SRD/mainOpDStoreMgmtClient.c

client_SRD/opDStoreSubTree1: client_SRD/mainOpDStoreSubTree1.o srd.o lib/libsrd.a
	$(Gcc) -g -o client_SRD/opDStoreSubTree1 client_SRD/mainOpDStoreSubTree1.o lib/libsrd.a -lxml2
	
client_SRD/mainOpDStoreSubTree1.o: client_SRD/mainOpDStoreSubTree1.c
	$(Gcc) -I/usr/include/libxml2 -I. -g -o client_SRD/mainOpDStoreSubTree1.o -c client_SRD/mainOpDStoreSubTree1.c
	
client_SRD/opDStoreSubTree2: client_SRD/mainOpDStoreSubTree2.o srd.o lib/libsrd.a
	$(Gcc) -g -o client_SRD/opDStoreSubTree2 client_SRD/mainOpDStoreSubTree2.o lib/libsrd.a -lxml2
	
client_SRD/mainOpDStoreSubTree2.o: client_SRD/mainOpDStoreSubTree2.c
	$(Gcc) -I/usr/include/libxml2 -I. -g -o client_SRD/mainOpDStoreSubTree2.o -c client_SRD/mainOpDStoreSubTree2.c
	
client_SRD/opDStoreGetSubTree: client_SRD/mainOpDStoreGetSubTree.o srd.o lib/libsrd.a
	$(Gcc) -g -o client_SRD/opDStoreGetSubTree client_SRD/mainOpDStoreGetSubTree.o lib/libsrd.a -lxml2
	
client_SRD/mainOpDStoreGetSubTree.o: client_SRD/mainOpDStoreGetSubTree.c
	$(Gcc) -I/usr/include/libxml2 -I. -g -o client_SRD/mainOpDStoreGetSubTree.o -c client_SRD/mainOpDStoreGetSubTree.c
	
	
srd.o: srd.c srd.h
	$(Gcc) -I/usr/include/libxml2 -g -c srd.c

lib/libsrd.a: srd.o
	ar rcs lib/libsrd.a srd.o
	
install: sysrepod
	cp sysrepod server
	
	
