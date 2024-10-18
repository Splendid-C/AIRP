import time as t

from pulp import *
import file_reader as reader
import allocate

# topo_name = 'dragonfly'
topo_name = 'fat_tree'
graph = reader.read_topo('../topo/{}.txt'.format(topo_name))
worker_list = reader.read_job()
alu_num = graph.switch_node_capacity_dict['0']
# eb = allocate.alu_to_bandwidth(alu_num,worker_num=4)

# 添加源节点
tasks = [i for i in range(len(worker_list))]
sources = [i for i in range(graph.switch_node, graph.total_node)]
sinks = list(map(int, graph.switch_node_id))
nodes = []
edges = [[i, j, k] for i in tasks for j in sources for k in sinks]
capacities = [[0 for i in range(len(graph.node_list))] for j in range(len(graph.node_list))]
# print(tasks)
# print(sources)
# print(sinks)
# print(edges)

for i in range(len(graph.node_list)):
    nodes.append(int(graph.node_list[i].name))

for i in range(len(graph.node_list)):
    for j in graph.node_list[i].out_dict.keys():
        capacities[int(graph.node_list[i].name)][int(j)] = graph.node_list[i].out_dict[j]
        capacities[int(j)][int(graph.node_list[i].name)] = graph.node_list[i].out_dict[j]
print(capacities)
for i in range(len(capacities)):
    for j in range(len(capacities[i])):
        if capacities[i][j] > 0:
            print('{}-{}: {}'.format(i,j,capacities[i][j]))

start_time = t.time()

# 初始化问题
prob = LpProblem("Max Flow Problem", LpMaximize)

# 定义决策变量
f = LpVariable.dicts("f", (tasks, sources, sinks), 0)

# 定义目标函数
prob += lpSum([f[i][j][k] for i in tasks for j in sources for k in sinks])

# 约束条件

# 每个源点的流量不包含不是运行在该源点上的任务的流量
for i in tasks:
    for j in sources:
        if str(j) not in worker_list[i]:
            print('j = {},worker_list[i]={}'.format(j,worker_list[i]))
            prob += lpSum(f[i][j][k] for k in sinks) == 0
            print('任务{}，从{}到所有汇点的流量为0'.format(i,j))

# done
# for node in nodes:
#     if node not in sources and node not in sinks:
#         # 除了源节点和汇节点，所有节点进出流量相等
#         prob += lpSum([f[i][j][node] for i in tasks for j in sources]) == lpSum(
#             [f[i][node][k] for i in tasks for k in sinks])

for i in tasks:
    for k in sinks:
        # 每个汇点的流量，满足同一个任务的每个源点到这个汇点的流量大小相等
        source_list = list(map(int,worker_list[i]))
        for j in range(len(source_list)-1):
            print('worker_list={},j={}'.format(worker_list[i], j))
            print('f[{}][{}][{}] == f[{}][{}][{}]'.format(i, source_list[j], k, i, source_list[j+1], k))
            prob += f[i][source_list[j]][k] == f[i][source_list[j+1]][k]



# # 每个汇点的聚合流量小于聚合容量
# for k in sinks:
#     prob += lpSum(allocate.bandwidth_to_alu(lpSum([f[i][j][k] for j in sources])/len(worker_list[i]), len(worker_list[i])) for i in tasks) <= alu_num


for edge in edges:
    # i:task;
    # j:source
    # k:sink
    i, j, k = edge
    # 每条边流量不超过边权重
    print('任务{}，从{}到{}，不多于{}'.format(i,j,k,capacities[j][k]))
    if capacities[j][k] > 0:
        prob += lpSum([f[i][j][k] for i in tasks]) <= capacities[j][k]



# for i in tasks:
#     for j in sources:
#         # 源节点所连边的流量不会来自其他源节点
#         key, value = list(graph.node_list[j].out_dict.items())[0]
#         prob += lpSum([f[i][j][k] for k in sinks]) <= graph.node_list[j].out_dict[key]



# for i in tasks:
#     for k in sinks:
#         # 每个汇点连接的边的流量，满足同一个任务的每个源点到这个汇点的流量大小相等
#         prob += lpSum([f[i][j][k] for j in sources]) == lpSum([f[i][j][k] for j in sources])




solve_begin_time = t.time()

# 求解问题
prob.solve()

end_time = t.time()

print('节点数：', len(graph.node_list))
print('Job数：', len(worker_list))
print('准备时间：', solve_begin_time - start_time)
print('计算时间：', end_time - solve_begin_time)
print('总时间时间：', end_time - start_time)
