import file_reader as reader
from allocate import AggAllocation

topo_name = 'fat_tree'
graph = reader.read_topo('../topo/{}.txt'.format(topo_name))
worker_list = reader.read_job()
host_job_dict = {}
tor_host_dict = {}
tor_job_dict = {}
job_alloc_dict = {}
job_isalloc = []

# 建立host到job的映射
for i in range(len(worker_list)):
    job_isalloc.append(False)
    for j in worker_list[i]:
        host_job_dict[j] = i

# 建立tor到host进而到job的映射
for i in range(graph.switch_node, graph.total_node):
    key, value = list(graph.node_list[graph.name_index_dict[str(i)]].out_dict.items())[0]
    host_name = graph.node_list[graph.name_index_dict[str(i)]].name
    if tor_host_dict.get(key) is None:
        tor_host_dict[key] = [host_name]
        if host_job_dict.get(host_name) is not None:
            tor_job_dict[key] = [host_job_dict[host_name]]
    else:
        tor_host_dict[key].append(host_name)
        if host_job_dict.get(host_name) is not None:
            tor_job_dict[key] = [host_job_dict[host_name]]


print(host_job_dict)
print(tor_host_dict)
print(tor_job_dict)

if len(job_isalloc) > len(tor_host_dict):
    print('无法分配！job数量多于tor。')
    exit(0)

# 遍历每个tor，依次分配给各个job
for key in tor_job_dict.keys():
    index = 0
    while index < len(tor_job_dict[key]) and job_isalloc[tor_job_dict[key][index]]:
        index += 1
    if index < len(tor_job_dict[key]):
        job_alloc_dict[tor_job_dict[key][index]] = key
        job_isalloc[tor_job_dict[key][index]] = True

print(job_alloc_dict)

# 生成allocation对象
allocation_list = []
for key in job_alloc_dict.keys():
    alloc = AggAllocation(worker_list, graph, 'SwitchML')
    agg_node = job_alloc_dict[key]
    alloc.aggre_node_dict = {agg_node: graph.switch_node_capacity_dict[agg_node]}
    allocation_list.append(alloc)


# 将分配结果打印到文件中
reader.write_ns3_file(allocation_list, worker_list)


