from Node import Node,Graph
from allocate import AggAllocation


class SubJob:
    def __init__(self, sub_job_id, worker_list, agg_node_id, alu_number):
        self.sub_job_id = sub_job_id
        self.worker_list = worker_list
        self.agg_node_id = agg_node_id
        self.alu_number = alu_number


def add_in_dict(node_list, name_index_dict):
    for node in node_list:
        for next_node_name in node.out_dict.keys():
            index = name_index_dict[next_node_name]
            node_list[index].in_dict[node.name] = node.out_dict[next_node_name]

def convert_bandwidth_to_number(bandwidth):
    if 'Gbps' in bandwidth:
        return int(bandwidth.replace('Gbps', ''))
    else:
        return "Unknown bandwidth format"

def convert_memory_to_bandwidth(memory):   # 这个函数改成内存等效带宽，用论文中的公式
    if 'Gbps' in memory:
        return int(memory.replace('Gbps', ''))
    else:
        return "Unknown bandwidth format"


def read_topo(file_path):   # 用来读取拓扑文件，如果需要修改拓扑文件格式，则相应修改这个文件
    name_index_dict = dict()
    node_dict = {}
    node_list = []
    with open(file_path, 'r') as file:
        lines = file.readlines()

        # 第一行: total_node, switch_node, link_num
        total_node, switch_node, link_num = map(int, lines[0].split())

        # 第二行: switch_node_id
        switch_node_id = lines[1].split()

        # 第三行: switch_node_capacity
        switch_node_capacity = list(map(int, lines[2].split()))
        switch_node_capacity_dict = {}
        for i in range(len(switch_node_id)):
            switch_node_capacity_dict[switch_node_id[i]] = switch_node_capacity[i]

        # 第四行及以后: src, dst, link_rate, link_delay, error_rate
        link_info = []
        cnt = 0
        for line in lines[3:]:
            if line == '\n':
                break
            src, dst, link_rate, link_delay, error_rate = line.split()
            link_info.append({
                'src': int(src),
                'dst': int(dst),
                'link_rate': link_rate,
                'link_delay': link_delay,
                'error_rate': float(error_rate)
            })

            if node_dict.get(src) is None:
                node_dict[src] = Node(src)
                name_index_dict[src] = cnt
                node_list.append(node_dict[src])
                cnt += 1
            if node_dict.get(dst) is None:
                node_dict[dst] = Node(dst)
                name_index_dict[dst] = cnt
                node_list.append(node_dict[dst])
                cnt += 1

            node_dict[src].out_dict[dst] = convert_bandwidth_to_number(link_rate)
            node_dict[dst].out_dict[src] = convert_bandwidth_to_number(link_rate)

        add_in_dict(node_list, name_index_dict)
        return Graph(total_node, switch_node, link_num, switch_node_id, switch_node_capacity_dict, node_list, name_index_dict)


def read_job():
    f = open('../topo/job_placement.txt', 'r')
    num_job = int(f.readline())
    worker_list = []
    for i in range(num_job):
        worker_list.append(f.readline().split())
    return worker_list


def write_ns3_file(allocation_list, worker_list):
    # first  line : total_job_number total_worker_number total_aggregator_number
    # second line : worker_node_id
    # third  line : aggregator_node_id
    # add line: sub_job_num for each big_job

    # four   line : job0id aggregator_node sub_worker_number alu_num
    # five   line : job0id work_node_id
    # six    line : job1id aggregator_node sub_worker_number alu_num
    # seven  line : job1id work_node_id
    sub_job_list = []
    workers = set()
    agg_nodes = set()
    for i in range(len(allocation_list)):
        workers.update(worker_list[i])
        agg_nodes.update(allocation_list[i].aggre_node_dict.keys())
        for agg_node_id in allocation_list[i].aggre_node_dict.keys():
            sub_job_list.append(SubJob(len(sub_job_list), worker_list[i], agg_node_id, allocation_list[i].aggre_node_dict[agg_node_id]))
        print('job {}----maxflow:{}---{}---'.format(i,round(allocation_list[i].maxflow,2),worker_list[i]), allocation_list[i].aggre_node_dict)

    f = open('job_assign.txt', 'w')
    f.write(f'{len(sub_job_list)} {len(allocation_list)} {len(workers)} {len(agg_nodes)}\n')
    f.write(' '.join(map(str, workers)) + '\n')
    f.write(' '.join(map(str, agg_nodes)) + '\n')
    lengths = [len(obj.aggre_node_dict) for obj in allocation_list]
    f.write(' '.join(map(str, lengths)) + '\n')

    for i in range(len(sub_job_list)):
        f.write(f'{i} {sub_job_list[i].agg_node_id} {len(sub_job_list[i].worker_list)} {int(sub_job_list[i].alu_number)}\n')
        f.write(' '.join(map(str, sub_job_list[i].worker_list)) + '\n')