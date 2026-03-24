PUBINCL= -I/A/public/
PUBCPP= /A/public/mpublic.cpp

# 明确设置ORACLE_HOME
ORACLE_HOME := /oracle/home
ORAINCL = -I$(ORACLE_HOME)/rdbms/public -I/A/public/db/oracle
ORALIB = -L$(ORACLE_HOME)/lib -L.

# 直接链接带版本的库
ORALIBS = -lclntsh

ORACPP = /A/public/db/oracle/_ooci.cpp
CFLAGS = -g

# 所有Oracle相关程序使用相同的链接选项
ORACLE_FLAGS = $(ORAINCL) $(ORALIB) -Wl,-rpath,$(ORACLE_HOME)/lib $(ORALIBS) $(ORACPP)

all:crtsurfdata obtcodetodb obtmindtodb  

crtsurfdata:crtsurfdata.cpp
	g++ $(CFLAGS) -o crtsurfdata crtsurfdata.cpp $(PUBINCL) $(PUBCPP) 
	cp -f crtsurfdata ../bin/.

obtcodetodb:obtcodetodb.cpp
	g++ $(CFLAGS) -o obtcodetodb obtcodetodb.cpp $(PUBINCL) $(PUBCPP) $(ORACLE_FLAGS)
	cp obtcodetodb ../bin/.

obtmindtodb: obtmindtodb.cpp idcapp.h idcapp.cpp
	g++ $(CFLAGS) -o obtmindtodb obtmindtodb.cpp idcapp.cpp $(PUBINCL) $(PUBCPP) $(ORACLE_FLAGS)
	cp obtmindtodb ../bin/.

clean:
	rm -f crtsurfdata obtcodetodb obtmindtodb