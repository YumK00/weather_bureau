all:libftp.a libftp.so

PUBINCL=-I/A/public/
PUBCPP=/A/public/mpublic.cpp


libftp.a:ftplib.h ftplib.c
	gcc -c -o libftp.a ftplib.c 

libftp.so:ftplib.h ftplib.c
	gcc -fPIC -shared -o libftp.so ftplib.c


clean:
	rm -rf libftp.a libftp.so