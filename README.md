# 气象局数据开放平台（weather_bureau）

> 企业级气象数据开放平台 / IDC 数据中心，基于 **Linux C++** 实现的完整后端项目。
> 覆盖数据采集、解析、入库、存储、转发、代理、HTTP 访问、进程守护等一整套链路，并支持 shell 脚本一键启停。


## 一、项目简介

本项目是一个**气象局级的数据开放平台（IDC 数据中心）**，以"气象站点分钟观测数据"为主线，实现了从**数据生成 → 文件传输 → 解析入库 → 数据挖掘导出 → 网络分发/访问**的完整数据链路。

主要能力包括：

- 冷热数据处理（模拟生成观测数据、历史数据入库/出库）
- 数据入库 / 出库（Oracle 数据库）
- 正向代理 / 反向代理（端口转发）
- HTTP / HTTPS 访问（自研 Web 服务器）
- 自定义 TCP、FTP 等协议的数据传输
- shell 脚本一键启动 / 停止
- 进程心跳检测 + `procctl` 进程调度守护模型

---

## 二、目录结构

```
weather_bureau/
├── README.md
└── A/
    ├── idc/                 # 气象数据中心核心程序
    │   ├── cpp/             # 源码 + makefile + 启停脚本
    │   │   ├── crtsurfdata.cpp   # 站点地面分钟观测数据（数据源）
    │   │   ├── obtcodetodb.cpp   # 站点参数（T_ZHOBTCODE）入库
    │   │   ├── obtmindtodb.cpp   # 分钟观测数据（T_ZHOBTMIND）入库
    │   │   ├── idcapp.h/.cpp     # 观测数据入库业务类 CZHOBTMIND
    │   │   ├── start.sh / stop.sh
    │   │   └── makefile
    │   ├── ini/             # 配置文件（stcode.ini 站点代码、xmltodb.xml）
    │   └── observedata/     # 观测数据样例（CSV / JSON / XML）
    ├── idcdata/             # 数据中心数据目录（分钟观测数据 XML 等）
    ├── public/              # 公共库
    │   ├── mpublic.h / cmpublic.h       # 公共头（日志、进程、字符串、时间等）
    │   ├── db/oracle/       # Oracle 数据库 OCI 封装
    │   │   ├── _ooci.h/.cpp           # connection / sqlstatement 封装
    │   │   └── clobtofile / filetoclob # CLOB 大字段读写工具
    │   ├── socket/          # Socket 通信库
    │   └── demo/            # 网络编程示例（TCP 服务器/客户端、epoll 等）
    ├── tools/               # 工具集（源码 + 已编译二进制）
    │   ├── cpp/             # 各工具源码与 makefile
    │   └── bin/             # 编译产物
    ├── log/                 # 日志目录
    └── tmp/                 # 临时文件目录
```

---

## 三、核心模块说明

### 1. 数据中心核心 `idc/`

| 程序          | 功能                                                         |
| ------------- | ------------------------------------------------------------ |
| `crtsurfdata` | 生成气象站点的地面分钟观测数据（模拟原始数据源，产出 CSV/JSON/XML） |
| `obtcodetodb` | 把站点参数（站点代码表 `T_ZHOBTCODE`）从 XML/配置文件写入数据库 |
| `obtmindtodb` | 把分钟观测数据（`T_ZHOBTMIND`）解析后写入数据库              |
| `idcapp`      | 观测数据入库的业务类 `CZHOBTMIND`，封装字段拆分与 `insert` 操作 |

`T_ZHOBTMIND`（全国分钟观测数据）核心字段：

| 字段      | 含义     | 精度/单位  |
| --------- | -------- | ---------- |
| obtid     | 站点代码 | -          |
| ddatetime | 数据时间 | 精确到分钟 |
| t         | 温度     | 0.1℃       |
| p         | 气压     | 0.1 百帕   |
| u         | 相对湿度 | 0–100      |
| wd        | 风向     | 0–360      |
| wf        | 风速     | 0.1 m/s    |
| r         | 降雨量   | 0.1 mm     |
| vis       | 能见度   | 0.1 米     |

### 2. 工具集 `tools/`

| 工具                                         | 功能                                                    |
| -------------------------------------------- | ------------------------------------------------------- |
| `procctl`                                    | 进程调度控制器（定时/守护式拉起业务进程，进程自举模型） |
| `checkproc`                                  | 进程心跳检查（配合 `procctl` 检测进程存活）             |
| `ftpgetfiles` / `ftpputfiles`                | FTP 协议的文件下载 / 上传                               |
| `fileserver`                                 | 文件传输服务端                                          |
| `tcpputfiles`                                | 基于 TCP 的文件上传客户端                               |
| `xmltodb`                                    | 解析 XML 数据并写入数据库                               |
| `dminingoracle`                              | 数据挖掘：从库中抽取数据导出                            |
| `gzipfiles`                                  | 文件 gzip 压缩                                          |
| `deletefiles` / `deletetable`                | 清理过期文件 / 清理数据库表                             |
| `webserver` / `webserver_reactor`            | 自研 HTTP Web 服务器（多进程 / epoll Reactor 两种实现） |
| `server` / `server1` / `server2` / `client3` | Socket 服务器 / 客户端示例                              |
| `reactor.h`                                  | epoll 多路复用（Reactor 模式）封装                      |
| `tools.h/.cpp`                               | 通用工具（进程、信号、时间、字符串等）                  |

### 3. 公共库 `public/`

- **`mpublic.h` / `cmpublic.h`**：日志类 `clogfile`、进程/信号、字符串、时间等公共函数。
- **`db/oracle/`**：基于 OCI 的 Oracle 数据库封装（`connection` 连接、`sqlstatement` SQL 执行），并提供 CLOB 大字段的读写工具 `clobtofile` / `filetoclob`。
- **`socket/`**：Socket 通信基础库。
- **`demo/`**：网络编程教学示例（TCP 服务端/客户端、epoll、多进程并发等）。

---

## 四、技术栈与技术亮点

- **语言/环境**：Linux + C/C++（g++/make），shell 脚本
- **进程模型**：多进程（`fork`）、进程间通信（信号、共享内存、管道）
- **进程守护**：`procctl` + `checkproc` 心跳模型，实现进程自举与崩溃自动拉起
- **网络编程**：TCP/UDP Socket、epoll Reactor 多路复用、HTTP 服务器
- **文件传输**：自实现 FTP 协议、TCP 文件上传/下载
- **数据库**：Oracle OCI 编程，含 CLOB 大字段处理
- **数据格式**：CSV / JSON / XML 解析与转换
- **代理**：正向 / 反向代理（端口转发，如 `rinetin` / `rined`）
- **其他**：gzip 压缩、定时任务、日志系统

---

## 五、构建与运行

> 依赖：Linux 环境、`g++`、`make`、Oracle 客户端（`libclntsh`）、`zlib` 等。

各模块自带独立的 `makefile`，在对应目录下执行：

```bash
cd A/idc/cpp && make      # 编译数据中心核心程序
cd A/tools/cpp && make    # 编译工具集
```

一键启停（数据中心）：

```bash
./start.sh    # 启动
./stop.sh     # 停止
```

---

## 六、整体数据流

```
[数据源]  crtsurfdata 生成观测数据 (CSV/JSON/XML)
    │
    ▼
[传输]  ftpgetfiles / ftpputfiles / fileserver / tcpputfiles
    │
    ▼
[解析入库]  xmltodb / obtcodetodb / obtmindtodb  →  Oracle (T_ZHOBTCODE / T_ZHOBTMIND)
    │
    ▼
[挖掘导出]  dminingoracle  →  gzipfiles 压缩
    │
    ▼
[分发/访问]  webserver(HTTP) / tcp / ftp / 正反向代理
```

所有业务进程由 `procctl` + `checkproc` 统一守护，日志统一落在 `A/log/` 目录。

---

## 七、备注

- 本项目为企业级完整项目，结构清晰、模块化强，适合作为 **Linux C++ 后端 / 网络编程 / 数据库 / 进程管理** 的综合练手项目。
- 仓库中包含已编译的二进制产物（`bin/`、`cpp/` 下的无后缀可执行文件）与运行日志，clone 后可直接参考运行效果。
