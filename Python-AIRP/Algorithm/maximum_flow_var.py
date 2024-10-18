# 两种算法求解约束最大流问题
import copy
from collections import deque
from queue import PriorityQueue, Queue
from Node import Node,Label,Graph
import file_reader as reader


def Edmons_Karp_Solve(s, e, node_list, name_index_dict):
    '''
    Edmons Karp 算法核心函数
    :param s: 起始节点名称
    :param e: 终止节点名称
    :param node_list: 节点列表
    :param name_index_dict: 节点名字和索引字典
    :return: 返回搜索到的所有增广路径及其流值
    '''
    routes = []
    graph = copy.deepcopy(node_list)
    while True:
        res = bfs1(s, e, graph, name_index_dict)
        if res is None:
            return routes
        # 追加增广路径到routes
        routes.append([res.route, res.last_flow])
        # 更新node_list
        route, flow = res.route, res.last_flow

        for i in range(len(route) - 1):
            n1 = graph[name_index_dict[route[i]]]
            n2 = graph[name_index_dict[route[i + 1]]]
            # 正向更新 n1 -> n2 剩余流量减少
            if n2.name in n1.out_dict.keys() and n1.out_dict[n2.name] is not None:
                n1.out_dict[n2.name] = n1.out_dict[n2.name] - flow
                # print('{}到{}的路径更新为{}'.format(n1.name,n2.name,n1.out_dict[n2.name]))
            # 反向更新 n2 -> n1 剩余流量增加
            if n1.name in n2.out_dict.keys() and n2.out_dict[n1.name] is not None:
                n2.out_dict[n1.name] = n2.out_dict[n1.name] + flow


def bfs1(s, e, node_list, name_index_dict):  # 传统广度优先算法
    queue = Queue()
    queue.put(Label([s], None))
    queue_dict = dict()
    while queue.empty() is False:
        res = queue.get()
        index = name_index_dict[res.route[-1]]
        for next_node_name in node_list[index].out_dict.keys():
            if next_node_name not in res.route and queue_dict.get(next_node_name) is None:
                flow = node_list[index].out_dict[next_node_name]
                if flow is None or flow > 0.01:
                    route = res.route.copy()
                    route.append(next_node_name)
                    if next_node_name == e:
                        return Label(route, min_flow(res.last_flow, flow))
                    queue.put(Label(route, min_flow(res.last_flow, flow)))
                    queue_dict[next_node_name] = 1
    return None


def Binary_Source_Bandwidth_Solve(s_list, e, node_list, name_index_dict):  # 二分源带宽算法
    graph = copy.deepcopy(node_list)
    temp_name_index_dict = copy.deepcopy(name_index_dict)
    super_source = Node('S')
    temp_name_index_dict['S'] = len(node_list)
    graph.append(super_source)
    routes = []
    is_start = True
    high = next(iter(graph[temp_name_index_dict['E']].in_dict.values())) / len(s_list)
    low = 0.01
    while high - low > 0.01:
        if is_start:
            mid = high
            is_start = False
        else:
            mid = low + (high-low)/2
        for i in range(len(s_list)):  # 添加超级源节点，构成一个新的图
            super_source.out_dict[s_list[i]] = mid
            # graph[temp_name_index_dict[s_list[i]]].in_dict['S'] = mid
        routes = Edmons_Karp_Solve('S', e, graph, temp_name_index_dict)
        if not routes:
            return None
        flow = 0
        for i in range(len(routes)):
            # print(routes[i])
            path, temp = routes[i]
            routes[i] = path[1::], temp
            flow += temp
        if mid * len(s_list) - flow < 0.01:
            low = mid
        else:
            high = mid
    return routes


def Augmented_Fat_Flow_Solve(s_list, e, node_list, name_index_dict):
    '''
    Edmons Karp 算法核心函数
    :param s: 起始节点名称
    :param e: 终止节点名称
    :param node_list: 节点列表
    :param name_index_dict: 节点名字和索引字典
    :return: 返回搜索到的所有增广路径及其流值
    '''
    routes = []
    graph = copy.deepcopy(node_list)
    while True:
        source_dict, global_min_flow = bfs(s_list, e, graph, name_index_dict)
        if len(source_dict.keys()) < len(s_list) or global_min_flow <= 0.01:  # 没有找到全部节点或者最大流为零
            return routes
        # 追加增广路径到routes
        for source_name in s_list:
            routes.append([source_dict[source_name].route, global_min_flow])
            # 更新node_list
            route, flow = source_dict[source_name].route, global_min_flow
            # print('bfs:',route,flow)
            for i in range(len(route) - 1):
                n1 = graph[name_index_dict[route[i]]]
                n2 = graph[name_index_dict[route[i + 1]]]
                # 正向更新 n1 -> n2 剩余流量减少
                if n2.name in n1.out_dict.keys() and n1.out_dict[n2.name] is not None:
                    n1.out_dict[n2.name] = n1.out_dict[n2.name] - flow
                    n2.in_dict[n1.name] = n2.in_dict[n1.name] - flow
                    # print('--------[out]{}到{}从{}减小为{}'.format(n1.name,n2.name,n1.out_dict[n2.name]+flow,n1.out_dict[n2.name]))
                    # print('--------[in]{}到{}从{}减小为{}'.format(n2.name, n1.name, n2.in_dict[n1.name] + flow,
                    #                                      n2.in_dict[n1.name]))
                # 反向更新 n2 -> n1 剩余流量增加
                if n1.name in n2.out_dict.keys() and n2.out_dict[n1.name] is not None:
                    n2.out_dict[n1.name] = n2.out_dict[n1.name] + flow
                    n1.in_dict[n2.name] = n1.in_dict[n2.name] + flow


def bfs(s_list, e, node_list, name_index_dict):
    # queue = PriorityQueue()  # 如果另需要优先级
    queue = Queue()
    queue.put(Label([e], None))
    source_dict = dict()
    queue_dict = dict()
    queue_dict[e] = 0  # 有记录表示是否已经访问过。数值表示下挂的终点（起点worker）数量
    global_min_flow = 0
    cnt = 0
    for node in node_list[name_index_dict[s_list[0]]].out_dict.keys():  # 将全局最大流的最小值赋值为第一个源节点的出带宽
        global_min_flow += node_list[name_index_dict[s_list[0]]].out_dict[node]
    # for i in s_list:
    #     source_dict[i] = None
    while queue.empty() is False and cnt < len(s_list):
        res = queue.get()  # bfs 队头出队
        # print(name_index_dict,res.route)
        index = name_index_dict[res.route[-1]]
        for next_node_name in node_list[index].in_dict.keys():
            if next_node_name not in res.route and queue_dict.get(next_node_name) is None :
                flow = node_list[index].in_dict[next_node_name]
                if flow is None or flow > 0:
                    route = res.route.copy()
                    route.append(next_node_name)
                    queue.put(Label(route, min_flow(res.last_flow, flow)))
                    queue_dict[next_node_name] = 0
                    if next_node_name in s_list:  # 到达一个源节点
                        for j in route:  # 该节点上每个节点下挂终点+1
                            queue_dict[j] += 1
                        # print('+++++++++++++++++',route)
                        cnt += 1
                        feasible_flow = min_flow(res.last_flow, flow)
                        if min_flow(feasible_flow, global_min_flow) == feasible_flow:
                            global_min_flow = feasible_flow
                        source_dict[next_node_name] = Label(route[::-1], global_min_flow)
                        if cnt == len(s_list):
                            break
    # 根据可行的通路，更新流量
    for v in source_dict.values():
        for i in range(len(v.route) - 1):
            node1 = node_list[name_index_dict[v.route[i]]]
            node2 = node_list[name_index_dict[v.route[i+1]]]
            if node1.out_dict[node2.name] / queue_dict[node1.name] < global_min_flow:
                global_min_flow = node1.out_dict[node2.name] / queue_dict[node1.name]

    if global_min_flow < 0.01:
        global_min_flow = 0
        source_dict = {}
    return source_dict, global_min_flow


def min_flow(f1, f2):
    '''
    求两个流量的较小者
    '''
    if f1 is None:
        return f2
    elif f2 is None:
        return f1
    else:
        return min(f1, f2)


# def read_graph():
#     # 格式: [节点名, 后继节点的名称, 当前节点到各个后继的流量] （None 代表流量无穷大）
#     graph = [
#         ["S", ["1", "2", "3"], [None, None, None]],
#         ["1", ["4"], [1]],
#         ["2", ["4", "6"], [1, 1]],
#         ["3", ["5"], [1]],
#         ["4", ["1", "2", "E"], [0, 0, 1]],
#         ["5", ["3", "E"], [0, 1]],
#         ["6", ["2", "E"], [0, 1]],
#         ["E", [], []]
#     ]
#     return graph

# def Ford_Fulkerson_Solve(s, e, node_list, name_index_dict):
#     '''
#     Ford_Fulkerson 算法核心函数
#     :param s: 起始节点名称
#     :param e: 终止节点名称
#     :param node_list: 节点列表
#     :param name_index_dict: 节点名字和索引字典
#     :return: 返回搜索到的所有增广路径及其流值
#     '''
#     routes = []
#     graph = copy.deepcopy(node_list)
#     max_flow = 0
#     while True:
#         # print(routes)
#
#         res = dfs(e, [s], None, graph, name_index_dict)
#         print('res = ', res)
#         # 追加增广路径到routes
#         routes.append(res)
#         # 更新node_list
#         route, flow = res
#         max_flow += flow
#         if res is None or flow < 0.1:
#             return routes, max_flow
#
#         for i in range(len(route) - 1):
#             n1 = graph[name_index_dict[route[i]]]
#             n2 = graph[name_index_dict[route[i + 1]]]
#             # 正向更新 n1 -> n2 剩余流量减少
#             if n2.name in n1.out_dict.keys() and n1.out_dict[n2.name] is not None:
#                 n1.out_dict[n2.name] = n1.out_dict[n2.name] - flow
#             # 反向更新 n2 -> n1 剩余流量增加
#             if n1.name in n2.out_dict.keys() and n2.out_dict[n1.name] is not None:
#                 n2.out_dict[n1.name] = n2.out_dict[n1.name] + flow
#
#
#
# def dfs(e, cur_route, last_flow, node_list, name_index_dict):
#     '''
#     DFS搜索增广路径
#     :param e: 终点节点名称
#     :param cur_route: 当前路径
#     :param last_flow: 上一个节点的流值，用来计算最小流
#     :param node_list: 节点列表
#     :param name_index_dict: 节点名字和索引字典
#     :return: 返回搜索到的增广路径及其流值，如果没找到就返回 None
#     '''
#     # print('dfs...')
#     if cur_route[-1] == e:  # 找到了终点，运行结束
#         return cur_route, last_flow
#     index = name_index_dict[cur_route[-1]]
#     for next_node_name in node_list[index].out_dict.keys():
#         if next_node_name not in cur_route:  # 该节点没被找过
#             flow = node_list[index].out_dict[next_node_name]
#             print(cur_route)
#             if flow is None or flow > 0.01:  # 该边能够连通
#                 cur_route.append(next_node_name)
#                 route, max_flow = dfs(e, cur_route, min_flow(last_flow, flow), node_list, name_index_dict)
#                 # if route is not None or max_flow > 0.01:
#                 #     return res
#                 if max_flow > 0.01:
#                     return route, max_flow
#                 cur_route.pop(-1)




if __name__ == '__main__':
    graph = reader.read_topo('topology.txt')
    source_list = ['20','21','22','23','24']  # worker list,可以另做一个文件
    aggregation_list = []

    # algorithm = 'Binary Source Bandwidth'
    algorithm = 'Augmented Fat-Flow'

    task_add_agg(node_list, source_list, switch_node_id, switch_node_capacity_dict, name_index_dict, algorithm=algorithm)

    # 调用 EK 算法求解最大流
    # routes = Augmented_Fat_Flow_Solve(source_list, "0", node_list, name_index_dict)
    # 调用 FF 算法求解最大流
    # routes = Ford_Fulkerson_Solve("S", "E", node_list, name_index_dict)
    # for i, (route, flow) in enumerate(routes):
    #     print(f"Route-{i + 1}: {route} , flow: {flow}")