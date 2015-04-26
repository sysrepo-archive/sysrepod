GCC = g++
Gcc = gcc

all: sysrepod client_SRD/clientsrd client_SRD/opDStoreClient client_SRD/opDStoreSubTree1 client_SRD/opDStoreSubTree2 client_SRD/opDStoreGetSubTree client_SRD/hugeTest client_SRD/signalTest server/genHuge install

clean:
	rm *.o sysrepod libsrd.a server/sysrepod server/genHuge server/huge.xml client_SRD/hugeTest client_SRD/clientsrd client_SRD/opDStoreClient client_SRD/opDStoreSubTree1 client_SRD/opDStoreSubTree2 client_SRD/signalTest client_SRD/opDStoreGetSubTree client_SRD/*.o

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

client_SRD/clientsrd: client_SRD/mainSRDClient.o srd.o libsrd.a
	$(Gcc) -g -o client_SRD/clientsrd client_SRD/mainSRDClient.o libsrd.a -lxml2

client_SRD/mainSRDClient.o: client_SRD/mainSRDClient.c
	$(Gcc) -I/usr/include/libxml2 -I. -g -o client_SRD/mainSRDClient.o -c client_SRD/mainSRDClient.c
	
client_SRD/opDStoreClient: client_SRD/mainOpDStoreMgmtClient.o srd.o libsrd.a
	$(Gcc) -g -o client_SRD/opDStoreClient client_SRD/mainOpDStoreMgmtClient.o libsrd.a -lxml2
	
client_SRD/mainOpDStoreMgmtClient.o: client_SRD/mainOpDStoreMgmtClient.c
	$(Gcc) -I/usr/include/libxml2 -I. -g -o client_SRD/mainOpDStoreMgmtClient.o -c client_SRD/mainOpDStoreMgmtClient.c

client_SRD/opDStoreSubTree1: client_SRD/mainOpDStoreSubTree1.o srd.o libsrd.a
	$(Gcc) -g -o client_SRD/opDStoreSubTree1 client_SRD/mainOpDStoreSubTree1.o libsrd.a -lxml2
	
client_SRD/mainOpDStoreSubTree1.o: client_SRD/mainOpDStoreSubTree1.c
	$(Gcc) -I/usr/include/libxml2 -I. -g -o client_SRD/mainOpDStoreSubTree1.o -c client_SRD/mainOpDStoreSubTree1.c
	
client_SRD/opDStoreSubTree2: client_SRD/mainOpDStoreSubTree2.o srd.o libsrd.a
	$(Gcc) -g -o client_SRD/opDStoreSubTree2 client_SRD/mainOpDStoreSubTree2.o libsrd.a -lxml2
	
client_SRD/mainOpDStoreSubTree2.o: client_SRD/mainOpDStoreSubTree2.c
	$(Gcc) -I/usr/include/libxml2 -I. -g -o client_SRD/mainOpDStoreSubTree2.o -c client_SRD/mainOpDStoreSubTree2.c
	
client_SRD/opDStoreGetSubTree: client_SRD/mainOpDStoreGetSubTree.o srd.o libsrd.a
	$(Gcc) -g -o client_SRD/opDStoreGetSubTree client_SRD/mainOpDStoreGetSubTree.o libsrd.a -lxml2
	
client_SRD/mainOpDStoreGetSubTree.o: client_SRD/mainOpDStoreGetSubTree.c
	$(Gcc) -I/usr/include/libxml2 -I. -g -o client_SRD/mainOpDStoreGetSubTree.o -c client_SRD/mainOpDStoreGetSubTree.c

client_SRD/signalTest: client_SRD/mainSignalExample.o
	$(Gcc) -g -o client_SRD/signalTest client_SRD/mainSignalExample.o libsrd.a -lxml2
	
client_SRD/mainSignalExample.o: client_SRD/mainSignalExample.c
	$(Gcc) -I/usr/include/libxml2 -I. -g -o client_SRD/mainSignalExample.o -c client_SRD/mainSignalExample.c
	
client_SRD/hugeTest: client_SRD/mainHugeTest.o
	$(Gcc) -g -o client_SRD/hugeTest client_SRD/mainHugeTest.o libsrd.a -lxml2
	
client_SRD/mainHugeTest.o: client_SRD/mainHugeTest.c
	$(Gcc) -I/usr/include/libxml2 -I. -g -o client_SRD/mainHugeTest.o -c client_SRD/mainHugeTest.c
	
server/genHuge: server/genHugeXML.c
	$(Gcc) -g -o server/genHuge server/genHugeXML.c
	
srd.o: srd.c srd.h
	$(Gcc) -I/usr/include/libxml2 -g -c srd.c

libsrd.a: srd.o
	ar rcs libsrd.a srd.o
	
install: sysrepod
	cp sysrepod server
	
	
