#用于停止所有的后台服务程序

killall -9 procctl

#缓和的杀,等五秒强制杀死
killall crtsurfdata deletefiles gzipfiles
sleep 5
killall -9 crtsurfdata deletefiles gzipfiles

