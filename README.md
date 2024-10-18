# README

AIRP实验分为两部分：

1、使用Python生成job-assign文件

2、使用ns3运行仿真程序



## 1 Python实现AIRP算法

1. 生成拓扑：修改dragonfly.py或者fattree.py的参数，dragonfly有三个参数，fattree有一个，运行生成相应拓扑txt。alu数量设置为10w-100w左右
2. 生成任务：运行placement.py，可以设置job数量以及放置方式，均匀连续放置（uniform）或者随机（random）放置，注意选择相应拓扑名称，生成文件job_placement.txt
3. 运行AIRP.py，注意选定相应拓扑。运行后会将按照sub_job划分的聚合节点分配打印在job_assign.txt文件中

---

---

## 2 NS3实现仿真

本次仿真使用的软件版本为ns3.38，操作系统为Ubuntu20.04，在ns3.38的框架上增加了新的ina模块，分别实现了AIRP、ATP、PS的方案。

由于文件大小限制，这里仅附上核心代码，以及本次实验中测试的数据。

-----

####ina-header

该文件中定义在网计算的报头格式，可根据需求进行修改。

------------

#### ina-aggregator-application

该文件中定义在网计算交换机或聚合结点的处理逻辑，在使用时安装到进行聚合的结点上。

--------------

####ina-worker-application

该文件中定义worker端的发送和接收数据包的逻辑，可以调整发包的大小、间隔、轮数。

----

#### ina-ps

该文件中定义ATP方案中PS的处理逻辑。
