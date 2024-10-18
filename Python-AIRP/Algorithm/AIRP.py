import time

import file_reader as reader
import allocate as al
from allocate import AggAllocation



# topo_name = 'dragonfly'
topo_name = 'fat_tree'
graph = reader.read_topo('../topo/{}.txt'.format(topo_name))
worker_list = reader.read_job()

algorithm = 'Binary Source Bandwidth'
# algorithm = 'Augmented Fat-Flow'

affinity_list = []
allocation_list = []
# 对每个job分别生成友好度序列
for i in range(len(worker_list)):
    # 根据友好度对交换机进行排序
    affinity_dict = {}
    for switch_id in graph.switch_node_id:
        affinity_dict[switch_id] = al.get_affinity(switch_id, worker_list[i],graph)
    sorted_switches = sorted(affinity_dict, key=affinity_dict.get, reverse=True)
    affinity_list.append(sorted_switches)
    allocation_list.append(AggAllocation(worker_list[i], graph, algorithm))

# 轮询每个job，发放credit
finished_job = 0
maxflow_list = [0 for _ in range(len(worker_list))]
index_list = [0 for _ in range(len(worker_list))]
agg_node_list = [[] for _ in range(len(worker_list))]
is_finished_list = [False for _ in range(len(worker_list))]
has_resource = True
# init_capacity = graph.switch_node_capacity_dict['0']
init_capacity = 2560000
credit_list = [0 for _ in range(len(worker_list))]
i = 0



start_time = time.time()
while finished_job < len(worker_list) and has_resource:
    if not is_finished_list[i]:  # 如果该任务没有被分配完，且还有剩余资源，则发credit继续分配
        credit_list[i] += (init_capacity / len(worker_list))
        print('[AIRP]:准备对job{}进行分配，credit={}'.format(i,credit_list[i]))
        # print(is_finished_list, index_list, has_resource)
        # input()
    if len(affinity_list[i]) <= index_list[i]:
        has_resource = False
    while credit_list[i] > 0 and len(affinity_list[i]) > index_list[i] and not is_finished_list[i]:  # 既有credit，又有可分配资源，还没分配完，则进行分配
        capacity = min(graph.switch_node_capacity_dict[affinity_list[i][index_list[i]]], credit_list[i])
        # print('计划为job{}分配{}节点，该节点所剩资源{}，credit还有{}'.format(i, affinity_list[i][index_list[i]], graph.switch_node_capacity_dict[affinity_list[i][index_list[i]]], credit))
        if capacity == 0:  # 当前聚合节点已经没有资源
            # print('聚合节点{}已经被用完，继续寻找下一个'.format(affinity_list[i][index_list[i]]))
            index_list[i] += 1  # 指针向右移动一个
            if len(affinity_list[i]) == index_list[i]:  # 已经达到末尾，资源全部耗尽
                has_resource = False
                break
            else:
                continue
        print('[AIRP]:尝试将{}节点分配给{}任务，现在该节点还剩余资源{}'.format(affinity_list[i][index_list[i]],i,graph.switch_node_capacity_dict[affinity_list[i][index_list[i]]]))
        alu_num = allocation_list[i].add_node(affinity_list[i][index_list[i]], capacity, len(worker_list[i]))
        print('[AIRP]:剩余资源{}，credit数量{}，经最大流计算需要使用{}'.format(graph.switch_node_capacity_dict[affinity_list[i][index_list[i]]],
                                                credit_list[i], alu_num))
        if alu_num > 0:  # 最大流更新
            # 有3种可能：
            # 1. credit不够了，更新资源剩余，job也没分配结束，等待下一轮轮分配
            # 2. credit还有，当前节点资源基本全部用完，所以使用了这个点的全部资源，job分配没结束，增加下一个节点进行分配
            # 3. credit还有，当前节点资源也有，job最大流增加不了了，job分配结束
            if alu_num < credit_list[i] * 0.9:  # credit 还有剩余，看看节点资源还有多少
                resource_remain = graph.switch_node_capacity_dict[affinity_list[i][index_list[i]]] - alu_num
                if resource_remain < 0.1 * init_capacity:  # 节点资源不够，将节点资源全部分配给这个节点
                    allocation_list[i].aggre_node_dict[affinity_list[i][index_list[i]]] += graph.switch_node_capacity_dict[affinity_list[i][index_list[i]]]
                    allocation_list[i].aggre_node_dict[affinity_list[i][index_list[i]]] = int(allocation_list[i].aggre_node_dict[affinity_list[i][index_list[i]]])
                    graph.switch_node_capacity_dict[affinity_list[i][index_list[i]]] = 0
                    credit_list[i] -= allocation_list[i].aggre_node_dict[affinity_list[i][index_list[i]]]
                    index_list[i] += 1
                    print('[AIRP]:节点资源不够，将节点资源全部分配给这个节点，聚合排队指针更新为：', index_list[i])
                else:  # 节点资源剩余较多，任务不再占用，说明达到最大流了，分配结束
                    allocation_list[i].aggre_node_dict[affinity_list[i][index_list[i]]] += int(alu_num)
                    graph.switch_node_capacity_dict[affinity_list[i][index_list[i]]] -= alu_num
                    credit_list[i] -= alu_num
                    print('[AIRP]:节点资源剩余较多，任务不再占用，说明达到最大流了，job {} 分配结束，聚合节点为：{}'.format(i, allocation_list[i].aggre_node_dict))
                    finished_job += 1
                    is_finished_list[i] = True
                break
            else:  # credit 不够，就把所有的credit全部使用，都分配给该节点
                print('[AIRP]:credit 不够，就把所有的credit全部使用，都分配给该节点，给该节点分配{}'.format(credit_list[i]))
                allocation_list[i].aggre_node_dict[affinity_list[i][index_list[i]]] += int(credit_list[i])
                graph.switch_node_capacity_dict[affinity_list[i][index_list[i]]] -= credit_list[i]
                credit_list[i] = 0
                if graph.switch_node_capacity_dict[affinity_list[i][index_list[i]]] < 0.1 * init_capacity:  # 节点资源不够，剩余资源设为0，指针向后移动
                    allocation_list[i].aggre_node_dict[affinity_list[i][index_list[i]]] += graph.switch_node_capacity_dict[affinity_list[i][index_list[i]]]
                    allocation_list[i].aggre_node_dict[affinity_list[i][index_list[i]]] = int(allocation_list[i].aggre_node_dict[affinity_list[i][index_list[i]]])
                    graph.switch_node_capacity_dict[affinity_list[i][index_list[i]]] = 0
                    index_list[i] += 1

        else:
            print('==========job {} 分配结束，新增节点不再增加，聚合节点序列为：{}'.format(i, allocation_list[i].aggre_node_dict))
            finished_job += 1
            is_finished_list[i] = True
    i = (i + 1) % len(worker_list)

end_time = time.time()

reader.write_ns3_file(allocation_list, worker_list)
if not has_resource:
    print('在网聚合资源耗尽')


print('执行时间：', end_time - start_time,'s')
print('节点数：', len(graph.node_list))
print('任务数：', len(worker_list))
print('边数：', graph.link_num)