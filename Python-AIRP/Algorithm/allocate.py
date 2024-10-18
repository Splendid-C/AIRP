from collections import deque
import maximum_flow_var as mf
from Node import Node,Graph


class AggAllocation:
    def __init__(self, worker_list, graph, algorithm):
        self.worker_list = worker_list
        self.graph = graph
        self.algorithm = algorithm
        self.maxflow = 0
        self.aggre_node_dict = {}  # 表示使用的聚合节点的capacity

    def add_super_dst(self, node_id, capacity, worker_num):
        bandwidth = alu_to_bandwidth(capacity, worker_num)
        # print('[{}]:capacity = {}, 等效为{}Gbps'.format(self.algorithm, capacity, bandwidth))
        end_node = Node("E")
        self.graph.name_index_dict['E'] = len(self.graph.node_list)
        self.graph.node_list.append(end_node)
        self.graph.node_list[self.graph.name_index_dict[node_id]].out_dict['E'] = bandwidth * worker_num
        end_node.in_dict[node_id] = bandwidth * worker_num

    def remove_super_dst(self, node_id):
        del self.graph.name_index_dict['E']
        del self.graph.node_list[self.graph.name_index_dict[node_id]].out_dict['E']
        self.graph.node_list.pop()

    def add_node(self, node_id, capacity, worker_num):
        delta_max_flow = 0
        self.add_super_dst(node_id, capacity, worker_num)
        all_route = []
        # self.graph.print_graph()
        if self.algorithm == 'Binary Source Bandwidth':
            # self.graph.node_list[self.graph.name_index_dict[node_id]].out_dict['E'] *= worker_num
            # print(worker_num,self.graph.node_list[self.graph.name_index_dict[node_id]].out_dict['E'])
            routes = mf.Binary_Source_Bandwidth_Solve(self.worker_list, 'E', self.graph.node_list, self.graph.name_index_dict)
        else:
            routes = mf.Augmented_Fat_Flow_Solve(self.worker_list, 'E', self.graph.node_list, self.graph.name_index_dict)

        if routes is None:
            self.remove_super_dst(node_id)
            return 0
        all_route.extend(routes)

        # 更新node_list图
        # print(routes)
        for j in range(len(routes)):
            route = routes[j][0]
            flow = routes[j][1]
            # print(route,flow)
            for i in range(len(route) - 1):
                n1 = self.graph.node_list[self.graph.name_index_dict[route[i]]]
                n2 = self.graph.node_list[self.graph.name_index_dict[route[i + 1]]]
                # 正向更新 n1 -> n2 剩余流量减少
                if n2.name in n1.out_dict.keys() and n1.out_dict[n2.name] is not None:
                    n1.out_dict[n2.name] = n1.out_dict[n2.name] - flow
                    n2.in_dict[n1.name] = n2.in_dict[n1.name] - flow
            delta_max_flow += flow

        alu_num = int(bandwidth_to_alu(delta_max_flow/worker_num, worker_num))
        # print('[{}]:bandwidth = {}Gbps, 等效为{}个alu'.format(self.algorithm, delta_max_flow/worker_num, alu_num))
        # if delta_max_flow == 0 or self.graph.node_list[self.graph.name_index_dict['0']].out_dict['8'] < 0:
        #     self.graph.print_graph()
        self.remove_super_dst(node_id)
        if alu_num >= 1:
            self.maxflow += delta_max_flow/worker_num
            if self.aggre_node_dict.get(node_id) is None:
                self.aggre_node_dict[node_id] = 0
            return alu_num
        else:
            return 0


def get_distance(node_id1, node_id2, graph):
    n1 = graph.node_list[graph.name_index_dict[node_id1]]
    n2 = graph.node_list[graph.name_index_dict[node_id2]]
    visited = set()
    queue = deque([(n1, 0)])
    while queue:
        current_node, distance = queue.popleft()
        # 如果找到目标节点，返回当前距离
        if current_node.name == n2.name:
            return distance

        # 如果当前节点未被访问过，将其标记为已访问，并将其邻居加入队列
        if current_node not in visited:
            visited.add(current_node)
            for neighbor in current_node.out_dict.keys():
                queue.append((graph.node_list[graph.name_index_dict[neighbor]], distance + 1))

    # 如果没有找到路径，返回None
    return None


def get_affinity(agg_node_id, worker_list, graph):
    affinity = 0
    for worker_id in worker_list:
        affinity -= get_distance(agg_node_id, worker_id, graph)
    return affinity


def alu_to_bandwidth(alu_num, worker_num):
    # t = 0.01
    # t = (worker_num - 1) * 0.002 # 一个worker10毫秒
    t = 0.005
    # 数据 / 包头 = 32 * 4 / 18 + 20 + 8 + 7 + 32 * 4
    # payload_rate = (18 + 20 + 8 + 7 + 32) / (32)
    payload_rate = 256 / 128  # 数据包大小256B，数据大小128B
    return alu_num * 32 * payload_rate / (t * 1000000000)


def bandwidth_to_alu(bandwidth, worker_num):
    # t = 0.01
    # t = (worker_num - 1) * 0.002 # 一个worker10毫秒
    t = 0.005
    # 数据 / 包头 = 32 * 4 / 18 + 20 + 8 + 7 + 32 * 4
    # payload_rate = (18 + 20 + 8 + 7 + 32) / (32)
    payload_rate = 256 / 128  # 数据包大小256B，数据大小128B
    return (bandwidth * t * 1000000000) / (32 * payload_rate)


# def task_add_agg(node_list, source_list, switch_node_id, switch_node_capacity_dict, name_index_dict, algorithm):
#
#
#     maximum_flow = 0
#     index = 0  # 交换机（聚合节点）指针
#     new_flow = 1
#     end_node = Node("E")
#     name_index_dict['E'] = len(node_list)
#     node_list.append(end_node)
#     all_route = []
#     while new_flow > 0.1:
#         print('============增加{}节点为聚合节点'.format(sorted_switches[index]))
#         # print_graph(node_list)
#         new_flow = 0
#         end_name = 'E'+str(sorted_switches[index])
#         end_node = Node(end_name)
#         name_index_dict[end_name] = len(node_list)
#         node_list.append(end_node)
#         node_list[name_index_dict[sorted_switches[index]]].out_dict[end_name] = switch_node_capacity_dict[sorted_switches[index]]
#         end_node.in_dict[sorted_switches[index]] = switch_node_capacity_dict[sorted_switches[index]]
#         if algorithm == 'Binary Source Bandwidth':
#             node_list[name_index_dict[sorted_switches[index]]].out_dict[end_name] = switch_node_capacity_dict[sorted_switches[index]] * len(source_list)
#             routes = mf.Binary_Source_Bandwidth_Solve(source_list, end_name, node_list, name_index_dict)
#         else:
#             routes = mf.Augmented_Fat_Flow_Solve(source_list, end_name, node_list, name_index_dict)
#
#         if routes is None:
#             break
#         all_route.extend(routes)
#
#         # 更新node_list图
#         # print(routes)
#         for j in range(len(routes)):
#             route = routes[j][0]
#             flow = routes[j][1]
#             for i in range(len(route) - 1):
#                 n1 = node_list[name_index_dict[route[i]]]
#                 n2 = node_list[name_index_dict[route[i + 1]]]
#                 # 正向更新 n1 -> n2 剩余流量减少
#                 if n2.name in n1.out_dict.keys() and n1.out_dict[n2.name] is not None:
#                     n1.out_dict[n2.name] = n1.out_dict[n2.name] - flow
#                     n2.in_dict[n1.name] = n2.in_dict[n1.name] - flow
#
#
#         for i, (route, flow) in enumerate(routes):
#             # print(f"Route-{i + 1}: {route} , flow: {flow}")
#             new_flow += flow
#         maximum_flow += new_flow
#         index += 1
#         # print('此时最大流 = ',maximum_flow)
#         # print('此时路径：', all_route)
#     print('============结果==============')
#     print('最大流：',maximum_flow)
#     print('所有路径：')
#
#     for i, (route, flow) in enumerate(all_route):
#         print(f"Route-{i + 1}: {route} , flow: {flow}")
#
#
#
