#此脚本用于启动所有的后台服务程序

#启动守护模块
/A/tools/bin/procctl 10 /A/tools/bin/checkproc /A/log/tools/checkproc.log
#本程序用于生成气象站点观测的分钟数据，程序十分钟运行一次，由调度模块启动。
/A/tools/bin/procctl 600 /A/idc/bin/crtsurfdata /A/idc/ini/stcode.ini /A/idc/observedata /A/log/idc/crtsurfdata.log csv,xml,json
#本程序把pathname目录及子目录中timeout天之前的匹配matchstr文件全部删除，timeout可以是小数。
/A/tools/bin/procctl 600 /A/tools/bin/deletefiles /A/idc/observedata \"*.gz\" 1
#本程序把pathname目录及子目录中timeout天之前的匹配matchstr文件全部压缩，timeout可以是小数。
/A/tools/bin/procctl 600 /A/tools/bin/gzipfiles /A/idc/observedata \"*.xml,*.json,*.csv\" 0.01

