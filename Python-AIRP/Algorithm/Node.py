
class Node:
    def __init__(self, name):
        self.name = name
        self.out_dict = {}  # 到各个点的边权dict
        self.in_dict = {}  # 各个点到该点的边权dict


class Label:
    def __init__(self, route, last_flow):
        self.route = route
        self.last_flow = last_flow

    def __lt__(self, o):
        if len(self.route) == len(o.route):
            return 0
        else:
            return 1 if len(self.route) > len(o.route) else -1


class Graph:
    def __init__(self,total_node, switch_node, link_num, switch_node_id, switch_node_capacity_dict, node_list, name_index_dict):
        self.total_node = total_node
        self.switch_node = switch_node
        self.link_num =  link_num
        self.switch_node_id = switch_node_id
        self.switch_node_capacity_dict = switch_node_capacity_dict
        self.node_list = node_list
        self.name_index_dict = name_index_dict

    def print_graph(self):
        for i in range(len(self.node_list)):
            print('{} : {}'.format(self.node_list[i].name, self.node_list[i].out_dict))
