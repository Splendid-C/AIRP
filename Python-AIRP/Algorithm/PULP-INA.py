import time as t

from pulp import *
import file_reader as reader
import allocate

topo_name = 'dragonfly'
# topo_name = 'fat_tree'
graph = reader.read_topo('../topo/{}.txt'.format(topo_name))
worker_list = reader.read_job()
alu_num = graph.switch_node_capacity_dict['0']
sources = []
for i in range(len(worker_list)):
    sources.extend(worker_list[i])

# eb = allocate.alu_to_bandwidth(alu_num,worker_num=4)

# 添加源节点
tasks = [i for i in range(len(worker_list))]
sources = list(map(int, sources))
sinks = list(map(int, graph.switch_node_id))
nodes = []
edges = [[i, j, k] for i in tasks for j in sources for k in sinks]
capacities = [[0 for i in range(len(graph.node_list))] for j in range(len(graph.node_list))]

source_job_dict = {}
for i in range(len(worker_list)):
    for j in worker_list[i]:
        source_job_dict[int(j)] = worker_list[i]
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
# print(capacities)
# for i in range(len(capacities)):
#     for j in range(len(capacities[i])):
#         if capacities[i][j] > 0:
#             print('{}-{}: {}'.format(i,j,capacities[i][j]))

start_time = t.time()

# 初始化问题
prob = LpProblem("Max_Flow_Problem", LpMaximize)

# 定义决策变量
f = LpVariable.dicts("f", (sources, nodes, nodes, sinks), 0)

# 定义目标函数
prob += lpSum([f[s][s][j][k] for s in sources for j in sinks for k in sinks])

# 约束条件

# 每个源点的流量不包含不是运行在该源点上的任务的流量
for s in sources:
    for i in sources:
        if i != s:
            prob += lpSum(f[i][s][j][k] for j in sinks for k in sinks) == 0

# 除了源节点和汇节点，所有节点进出流量相等
for j in sinks:
    for k in sinks:
        if j != k:
            prob += lpSum([f[s][i][j][k] for s in sources for i in nodes]) == lpSum(
                [f[s][j][i][k] for s in sources for i in sinks])

# 每个汇点的流量，满足同一个任务的每个源点到这个汇点的流量大小相等
for s in sources:
    for k in sinks:
        source_list = list(map(int, source_job_dict[s]))
        for l in range(len(source_list) - 1):
            # prob += lpSum(f[source_list[l]][source_list[l]][j][k] for j in sinks) == lpSum(
            #     f[source_list[l + 1]][source_list[l]][j][k] for j in sinks)
            prob += f[source_list[l]][source_list[l]][k][k] == f[source_list[l + 1]][source_list[l + 1]][k][k]

# 每个汇点的聚合流量小于聚合容量
for k in sinks:
    prob += lpSum(
        allocate.bandwidth_to_alu(lpSum([f[int(worker_list[l][0])][int(worker_list[l][0])][j][k] for j in nodes]),
                                  len(worker_list[l])) for l in
        range(len(worker_list))) <= alu_num

# 每条边流量不超过边权重
for i in nodes:
    for j in sinks:
        prob += lpSum([f[s][i][j][k] for s in sources for k in sinks]) <= capacities[i][j]


solve_begin_time = t.time()

# 求解问题
prob.solve()

# 构造源点到汇点的流数组
source_sink_flow = [[0 for j in range(graph.switch_node)] for i in range(graph.total_node)]
for v in prob.variables():
    f, source, src, dst, sink = v.name.split('_')
    flow = v.varValue
    if flow > 0 and int(src) in sources and int(sink) in sinks:
        source_sink_flow[int(src)][int(sink)] += flow


# # 打印源点到汇点的流数组
# for i in sources:
#     for j in sinks:
#         if source_sink_flow[i][j] > 0:
#             print('{} to {}, flow = {}'.format(i, j, source_sink_flow[i][j]))


# 生成分配方案
alloc_list = []
for i in range(len(worker_list)):
    alloc = allocate.AggAllocation(worker_list,graph,'LP')
    src = int(worker_list[i][0])
    for j in range(graph.switch_node):
        if source_sink_flow[src][j] > 0:
            alloc.aggre_node_dict[j] = int(allocate.bandwidth_to_alu(source_sink_flow[src][j], len(worker_list[i])))
    alloc_list.append(alloc)

reader.write_ns3_file(alloc_list,worker_list)

end_time = t.time()

print('节点数：', len(graph.node_list))
print('Job数：', len(worker_list))
print('准备时间：', solve_begin_time - start_time)
print('计算时间：', end_time - solve_begin_time)
print('总时间时间：', end_time - start_time)
