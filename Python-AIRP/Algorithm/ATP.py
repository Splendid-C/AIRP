# 2 5  //total_job_number, total_worker_number
# 48 49 52 53 54 //worker_node_id
# 0 2 50  //job0, sub_worker_number, parameter_server_id
# 48 49   //job0_worker_node_id
# 1 3 55  //job1, sub_worker_number, parameter_server_id
# 52 53 54 //job1_worker_node_id

import file_reader as reader

# 读取拓扑和任务放置
topo_name = 'fat_tree'
# topo_name = 'dragonfly'
graph = reader.read_topo('../topo/{}.txt'.format(topo_name))
host_list = map(str, range(graph.switch_node,graph.total_node))
worker_list = reader.read_job()
total_job_num = len(worker_list)
total_worker_list = []
for i in range(len(worker_list)):
    total_worker_list.extend(worker_list[i])
total_worker_num = len(total_worker_list)

# 得到PS的备选列表（没有被放置任务的host）
ps_list = list(set(host_list)-set(total_worker_list))
if len(ps_list) < len(worker_list):
    print('任务放置太满，PS无法放置')
    exit(0)


# 写入文件
f = open('atp-job-assign.txt','w')
f.write(f'{total_job_num} {total_worker_num}\n')
f.write(' '.join(map(str, total_worker_list)) + '\n')
for i in range(len(worker_list)):
    f.write(f'{i} {len(worker_list[i])} {ps_list[i]}\n')
    f.write(' '.join(map(str, worker_list[i])) + '\n')
