all:procctl  checkproc deletefiles gzipfiles ftpgetfiles tcpputfiles fileserver dminingoracle xmltodb deletetable inted server1 server2 webserver client3 rined rinetin webserver_reactor

PUBINCL= -I/A/public/#头文件目录
PUBCPP= /A/public/mpublic.cpp#cpp文件目录

##################################################
# oracle头文件路径
ORAINCL = -I$(ORACLE_HOME)/rdbms/public -I/A/public/db/oracle

# oracle库文件路
ORALIB = -L/oracle/home/lib -L. -Wl,-rpath,/oracle/home/lib

# oracle的oci库
ORALIBS = -lclntsh

# 开发框架oracle的cpp文件名，这里直接包含进来，没有采用链接库，是为了方便调试。
ORACPP = /A/public/db/oracle/_ooci.cpp
##################################################


procctl:procctl.cpp
	g++ -g -o procctl procctl.cpp $(PUBINCL) $(PUBCPP)
	cp -f procctl ../bin/.

# server:server.cpp
# 	g++ -g -o server server.cpp $(PUBINCL) $(PUBCPP)
# 	cp -f server ../bin/.

checkproc:checkproc.cpp
	g++ -g -o checkproc checkproc.cpp $(PUBINCL) $(PUBCPP)
	cp -f checkproc ../bin/.

deletefiles:deletefiles.cpp
	g++ -g -o deletefiles deletefiles.cpp $(PUBINCL) $(PUBCPP)
	cp -f deletefiles ../bin/.

gzipfiles:gzipfiles.cpp
	g++ -g -o gzipfiles gzipfiles.cpp $(PUBINCL) $(PUBCPP)
	cp -f gzipfiles ../bin/.


ftpgetfiles:ftpgetfiles.cpp   
	g++ -g -o ftpgetfiles ftpgetfiles.cpp $(PUBINCL) $(PUBCPP) /A/public/libftp.a /A/public/ftp.cpp
	cp -f ftpgetfiles ../bin/.

tcpputfiles:tcpputfiles.cpp
	g++ -g -o tcpputfiles tcpputfiles.cpp $(PUBINCL) $(PUBCPP)
	cp -f tcpputfiles ../bin/.

fileserver:fileserver.cpp
	g++ -g -o fileserver fileserver.cpp $(PUBCPP) $(PUBINCL)
	cp -f fileserver ../bin/.

dminingoracle:dminingoracle.cpp
	g++ -g -o dminingoracle dminingoracle.cpp  $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORACPP) $(ORALIB) $(ORALIBS) -lm -lc -ldl -lpthread
	cp -f dminingoracle ../bin/.
xmltodb:xmltodb.cpp
	g++ -g -o xmltodb xmltodb.cpp  $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORACPP) $(ORALIB) $(ORALIBS) /A/tools/cpp/tools.cpp  -lm -lc -ldl -lpthread
	cp -f xmltodb ../bin/.

#deletetable:deletetable.cpp
#	g++ -g -o deletetable deletetable.cpp  $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORACPP) $(ORALIB) $(ORALIBS) /A/tools/cpp/tools.cpp  -lm -lc -ldl -lpthread
#	cp -f deletetable ../bin/.

inted:inted.cpp
	g++ -g -o inted inted.cpp $(PUBCPP) $(PUBINCL)
	cp -f inted ../bin/.

rined:rined.cpp
	g++ -g -o rined rined.cpp $(PUBCPP) $(PUBINCL)
	cp -f rined ../bin/.

rinetin:rinetin.cpp
	g++ -g -o rinetin rinetin.cpp  $(PUBCPP) $(PUBINCL)
	cp -f rinetin ../bin/.

server1:server1.cpp
	g++ -g -o server1 server1.cpp $(PUBCPP) $(PUBINCL)

server2:server2.cpp
	g++ -g -o server2 server2.cpp $(PUBCPP) $(PUBINCL) $(ORAINCL) $(ORACPP) $(ORALIB) $(ORALIBS) -lm -lc -ldl -lpthread

webserver:webserver.cpp
	g++ -g -o webserver webserver.cpp $(PUBCPP) $(PUBINCL) $(ORAINCL) $(ORACPP) $(ORALIB) $(ORALIBS) -lm -lc -ldl -lpthread

client3:client3.cpp
	g++ -g -o client3 client3.cpp $(PUBCPP) $(PUBINCL)
	cp -f client3 ../bin/.

webserver_reactor:webserver_reactor.cpp
	g++ -g -o webserver_reactor webserver_reactor.cpp $(PUBCPP) $(PUBINCL) $(ORAINCL) $(ORACPP) $(ORALIB) $(ORALIBS) -lm -lc -ldl -lpthread
	cp -f webserver_reactor ../bin/.

clean:
	# rm -rf procctl checkproc deletefiles gzipfiles ftpgetfiles tcpputfiles fileserver dminingoracle xmltodb deletetable inted server1 server2 webserver client3  rined rinetin webserver_reactor