all:demo50 demo51 demo52 fuwuepoll kehu fuwuepoll1 kehu1

demo50:demo50.cpp
	cd .. && gcc -c -o libftp.a ftplib.c  
	cd .. && gcc -fPIC -shared -o libftp.so ftplib.c  
	g++ -g -o demo50 demo50.cpp ../mpublic.cpp ../ftp.cpp ../libftp.a 

demo51:demo51.cpp
	g++ -g -o demo51 demo51.cpp ../mpublic.cpp ../ftp.cpp ../libftp.a 

demo52:demo52.cpp
	g++ -g -o demo52 demo52.cpp ../mpublic.cpp ../ftp.cpp ../libftp.a


fuwuepoll:fuwuepoll.cpp
	g++ -g -o fuwuepoll fuwuepoll.cpp ../mpublic.cpp -lm -lc
kehu:kehu.cpp
	g++ -g -o kehu kehu.cpp ../mpublic.cpp -lm -lc
fuwuepoll1:fuwuepoll1.cpp
	g++ -g -o fuwuepoll1 fuwuepoll1.cpp ../mpublic.cpp -lm -lc
kehu1:kehu1.cpp
	g++ -g -o kehu1 kehu1.cpp ../mpublic.cpp -lm -lc
