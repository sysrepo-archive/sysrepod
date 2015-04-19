CXX = g++
CC = gcc
AR = ar
LIBXML2_INCLUDE_PATH = /usr/include/libxml2

all: sysrepod client_SRD/clientsrd client_SRD/opDStoreClient client_SRD/opDStoreSubTree1 client_SRD/opDStoreSubTree2 client_SRD/opDStoreGetSubTree client_SRD/hugeTest server/genHuge install

clean:
	rm *.o sysrepod libsrd.a server/sysrepod server/genHuge client_SRD/hugeTest client_SRD/clientsrd client_SRD/opDStoreClient client_SRD/opDStoreSubTree1 client_SRD/opDStoreSubTree2 client_SRD/opDStoreGetSubTree client_SRD/*.o

sysrepod: common.o mainSysRepoD.o ClientSet.o DataStore.o DataStoreSet.o Client.o ClientSRD.o global.h application.h OpDataStore.o OpDataStoreSet.o
	$(CXX) -g -o sysrepod mainSysRepoD.o common.o ClientSet.o DataStore.o DataStoreSet.o OpDataStore.o OpDataStoreSet.o Client.o ClientSRD.o -pthread -lxml2
	
mainSysRepoD.o: mainSysRepoD.cpp global.h application.h
	$(CXX) -c -I$(LIBXML2_INCLUDE_PATH) -g mainSysRepoD.cpp
	
common.o: common.cpp common.h
	$(CXX) -c -g common.cpp

ClientSet.o: ClientSet.cpp ClientSet.h
	$(CXX) -I$(LIBXML2_INCLUDE_PATH) -c -g ClientSet.cpp
	
Client.o: Client.cpp Client.h
	$(CXX) -c -g Client.cpp
	
ClientSRD.o: ClientSRD.cpp ClientSRD.h
	$(CXX) -I$(LIBXML2_INCLUDE_PATH) -c -g ClientSRD.cpp
	
DataStore.o: DataStore.cpp DataStore.h
	$(CXX) -I$(LIBXML2_INCLUDE_PATH) -c -g DataStore.cpp

DataStoreSet.o: DataStoreSet.cpp DataStoreSet.h
	$(CXX) -I$(LIBXML2_INCLUDE_PATH) -c -g DataStoreSet.cpp
	
OpDataStore.o: OpDataStore.cpp OpDataStore.h
	$(CXX) -c -g OpDataStore.cpp

OpDataStoreSet.o: OpDataStoreSet.cpp OpDataStoreSet.h
	$(CXX) -I$(LIBXML2_INCLUDE_PATH) -c -g OpDataStoreSet.cpp

client_SRD/clientsrd: client_SRD/mainSRDClient.o srd.o libsrd.a
	$(CC) -g -o client_SRD/clientsrd client_SRD/mainSRDClient.o libsrd.a -lxml2

client_SRD/mainSRDClient.o: client_SRD/mainSRDClient.c
	$(CC) -I$(LIBXML2_INCLUDE_PATH) -I. -g -o client_SRD/mainSRDClient.o -c client_SRD/mainSRDClient.c
	
client_SRD/opDStoreClient: client_SRD/mainOpDStoreMgmtClient.o srd.o libsrd.a
	$(CC) -g -o client_SRD/opDStoreClient client_SRD/mainOpDStoreMgmtClient.o libsrd.a -lxml2
	
client_SRD/mainOpDStoreMgmtClient.o: client_SRD/mainOpDStoreMgmtClient.c
	$(CC) -I$(LIBXML2_INCLUDE_PATH) -I. -g -o client_SRD/mainOpDStoreMgmtClient.o -c client_SRD/mainOpDStoreMgmtClient.c

client_SRD/opDStoreSubTree1: client_SRD/mainOpDStoreSubTree1.o srd.o libsrd.a
	$(CC) -g -o client_SRD/opDStoreSubTree1 client_SRD/mainOpDStoreSubTree1.o libsrd.a -lxml2
	
client_SRD/mainOpDStoreSubTree1.o: client_SRD/mainOpDStoreSubTree1.c
	$(CC) -I$(LIBXML2_INCLUDE_PATH) -I. -g -o client_SRD/mainOpDStoreSubTree1.o -c client_SRD/mainOpDStoreSubTree1.c
	
client_SRD/opDStoreSubTree2: client_SRD/mainOpDStoreSubTree2.o srd.o libsrd.a
	$(CC) -g -o client_SRD/opDStoreSubTree2 client_SRD/mainOpDStoreSubTree2.o libsrd.a -lxml2
	
client_SRD/mainOpDStoreSubTree2.o: client_SRD/mainOpDStoreSubTree2.c
	$(CC) -I$(LIBXML2_INCLUDE_PATH) -I. -g -o client_SRD/mainOpDStoreSubTree2.o -c client_SRD/mainOpDStoreSubTree2.c
	
client_SRD/opDStoreGetSubTree: client_SRD/mainOpDStoreGetSubTree.o srd.o libsrd.a
	$(CC) -g -o client_SRD/opDStoreGetSubTree client_SRD/mainOpDStoreGetSubTree.o libsrd.a -lxml2
	
client_SRD/mainOpDStoreGetSubTree.o: client_SRD/mainOpDStoreGetSubTree.c
	$(CC) -I$(LIBXML2_INCLUDE_PATH) -I. -g -o client_SRD/mainOpDStoreGetSubTree.o -c client_SRD/mainOpDStoreGetSubTree.c
	
client_SRD/hugeTest: client_SRD/mainHugeTest.o
	$(CC) -g -o client_SRD/hugeTest client_SRD/mainHugeTest.o libsrd.a -lxml2
	
client_SRD/mainHugeTest.o: client_SRD/mainHugeTest.c
	$(CC) -I$(LIBXML2_INCLUDE_PATH) -I. -g -o client_SRD/mainHugeTest.o -c client_SRD/mainHugeTest.c
	
server/genHuge: server/genHugeXML.c
	$(CC) -g -o server/genHuge server/genHugeXML.c
	
srd.o: srd.c srd.h
	$(CC) -I$(LIBXML2_INCLUDE_PATH) -g -c srd.c

libsrd.a: srd.o
	$(AR) rcs libsrd.a srd.o
	
install: sysrepod
	cp sysrepod server
	
	
