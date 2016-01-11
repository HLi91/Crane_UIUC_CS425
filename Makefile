CC=g++
CFLAGS=-c -std=c++11 -pthread



all: workerOrigin workerRangeDis workerMaxRange TM


workerOrigin: inode.o workerPipe.o workerSys.o workerTopologyBuilder.o workerTest.o topology.o tuple.o
	g++ -o workerOriginExe inode.o workerPipe.o workerSys.o workerTopologyBuilder.o workerTest.o topology.o tuple.o -pthread

workerRangeDis: inode.o workerPipe.o workerSys.o workerTopologyBuilder.o workerPacketSizeRangeDis.o topology.o tuple.o
	g++ -o workerRangeDisExe inode.o workerPipe.o workerSys.o workerTopologyBuilder.o workerPacketSizeRangeDis.o topology.o tuple.o -pthread

workerMaxRange: inode.o workerPipe.o workerSys.o workerTopologyBuilder.o workerPacketSizeMaxRange.o topology.o tuple.o
	g++ -o workerMaxRangeExe inode.o workerPipe.o workerSys.o workerTopologyBuilder.o workerPacketSizeMaxRange.o topology.o tuple.o -pthread
	
TM: heartBeat.o UdpWrapper.o client.o server.o tcpWrapper.o SDFS.o taskManager.o TMCommunication.o TMMaster.o TMPipe.o topology.o tuple.o
	g++ -o tm heartBeat.o UdpWrapper.o client.o server.o tcpWrapper.o SDFS.o taskManager.o TMCommunication.o TMMaster.o TMPipe.o topology.o tuple.o -pthread


taskManager.o: taskManager.cpp
	$(CC) $(CFLAGS) taskManager.cpp
TMCommunication.o: TMCommunication.cpp
	$(CC) $(CFLAGS) TMCommunication.cpp
TMMaster.o: TMMaster.cpp
	$(CC) $(CFLAGS) TMMaster.cpp
TMPipe.o: TMPipe.cpp
	$(CC) $(CFLAGS) TMPipe.cpp




inode.o: inode.cpp
	$(CC) $(CFLAGS) inode.cpp
workerPipe.o: workerPipe.cpp
	$(CC) $(CFLAGS) workerPipe.cpp
workerSys.o: workerSys.cpp
	$(CC) $(CFLAGS) workerSys.cpp
workerTopologyBuilder.o: workerTopologyBuilder.cpp
	$(CC) $(CFLAGS) workerTopologyBuilder.cpp
workerTest.o: workerTest.cpp
	$(CC) $(CFLAGS) workerTest.cpp
workerPacketSizeRangeDis.o: workerPacketSizeRangeDis.cpp
	$(CC) $(CFLAGS) workerPacketSizeRangeDis.cpp
workerPacketSizeMaxRange.o: workerPacketSizeMaxRange.cpp
	$(CC) $(CFLAGS) workerPacketSizeMaxRange.cpp

topology.o: topology.cpp
	$(CC) $(CFLAGS) topology.cpp
tuple.o: tuple.cpp
	$(CC) $(CFLAGS) tuple.cpp

heartBeat.o: ./Res/heartBeat/heartBeat.cpp
	$(CC) $(CFLAGS) ./Res/heartBeat/heartBeat.cpp

UdpWrapper.o: ./Res/heartBeat/UdpWrapper.cpp
	$(CC) $(CFLAGS) ./Res/heartBeat/UdpWrapper.cpp
client.o: ./Res/TCPserver/client.cpp
	$(CC) $(CFLAGS)  ./Res/TCPserver/client.cpp
	
server.o: ./Res/TCPserver/server.cpp
	$(CC) $(CFLAGS)  ./Res/TCPserver/server.cpp
tcpWrapper.o: ./Res/TCPserver/tcpWrapper.cpp
	$(CC) $(CFLAGS)  ./Res/TCPserver/tcpWrapper.cpp
SDFS.o: ./Res/SDFS/SDFS.cpp
	$(CC) $(CFLAGS) ./Res/SDFS/SDFS.cpp
	
clean:
	rm -f *o output hb output.txt workerOriginExe workerRangeDisExe workerMaxRangeExe tm
	rm -f ./cloud/*
	
cl:
	rm -f ./cloud/*







