/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ina-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/point-to-point-module.h"

#include <bitset>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("InaSimulator");

struct Interface
{
    uint32_t index;
    bool up;
    uint64_t delay;
    uint64_t bandwidth;
    Address srcAddress;
    Address dstAddress;

    Interface()
        : index(0),
          up(false),
          delay(0),
          bandwidth(0)
    {
    }
};

// struct JobList
// {
//     uint32_t psId;
//     uint32_t jobId;
//     uint32_t aggNodeId;
//     uint32_t fatherId;
//     uint32_t workerNum;
//     vector<uint32_t> workerIdList;
// };

/************************************************
 * Config Variables
 ***********************************************/
string topoFileName, flowFileName, jobFileName;

/************************************************
 * Runtime Variables
 ***********************************************/
ifstream topoFile, flowFile, jobFile;
uint32_t nodeNum = 0, switchNum = 0, linkNum = 0;
uint32_t maxCount = 100500;
uint32_t maxEpoch = 1;
uint32_t aluNum = 2560000;
uint32_t multi = 32;
// double multi2 = 0.001/8;
vector<uint32_t> switchNodeId;
vector<uint32_t> workerNodeId;
vector<uint32_t> aggregatorNodeId;
vector<JobList> pendingJobList;
map<uint32_t, uint32_t> aggregatorAbility;
map<uint32_t, uint32_t> fatherList;

// Topology Matrix
map<Ptr<Node>, map<Ptr<Node>, Interface>> node2if;
map<Ptr<Node>, Address> hostNode2Addr;
map<uint32_t, Address> NodeId2Addr;

void
readConfigFile(string configFileName)
{
    ifstream config;
    config.open(configFileName);

    // Read the configuration file
    cout << "*************************************" << endl;
    cout << "* Begin to load config file" << endl;
    cout << "*************************************" << endl;
    while (!config.eof())
    {
        string key;
        config >> key;
        if (key.compare("TOPOLOGY_FILE") == 0)
        {
            string tmp;
            config >> tmp;
            topoFileName = tmp;
            std::cout << "TOPOLOGY_FILE\t\t" << topoFileName << endl;
        }
        else if (key.compare("FLOW_FILE") == 0)
        {
            string tmp;
            config >> tmp;
            flowFileName = tmp;
            std::cout << "FLOW_FILE\t\t" << flowFileName << endl;
        }
        else if (key.compare("JOB_ASSIGN_FILE") == 0)
        {
            string tmp;
            config >> tmp;
            jobFileName = tmp;
            std::cout << "JOB_ASSIGN_FILE\t\t" << jobFileName << endl;
        }
    }
    cout << "*************************************" << endl;
    cout << "* Loading config finish!" << endl;
    cout << "*************************************" << endl;
}

/************************************************
 * Main
 ***********************************************/
int
main(int argc, char* argv[])
{
    LogComponentEnable("InaWorker", LOG_LEVEL_INFO);
    // LogComponentEnable("InaAggregator", LOG_LEVEL_INFO);
    // LogComponentEnable("InaPS", LOG_LEVEL_INFO);

    clock_t beginTime, endTime;
    beginTime = clock();

    CommandLine cmd(__FILE__);
    string configFile = "mix/config.txt";
    cmd.AddValue("maxCount", "the max count of bigjob", maxCount);
    cmd.AddValue("config", "The configuration file required to run the program.", configFile);
    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::DropTailQueue<Packet>::MaxSize", StringValue("1000000000000000p"));

    readConfigFile(configFile);

    topoFile.open(topoFileName);
    topoFile >> nodeNum >> switchNum >> linkNum;

    // create node
    NS_LOG_INFO("Create nodes.");
    NodeContainer nodes;
    nodes.Create(nodeNum);

    // Ptr<Node> node;
    // for(uint32_t i = 0; i < nodeNum; ++i){
    //     node = nodes.Get(i);
    //     cout << node->GetId()<<endl;
    // }

    for (uint32_t i = 0; i < switchNum; ++i)
    {
        uint32_t switchId;
        topoFile >> switchId;
        switchNodeId.push_back(switchId);
    }

     // alu number
    for(uint32_t i = 0; i < switchNum; ++i){
        uint32_t aluNum;
        topoFile >> aluNum;      
        if(aluNum == 0){
            aggregatorAbility[switchNodeId[i]] = 1;
        }else{
            aggregatorAbility[switchNodeId[i]] = aluNum/multi;
        }
    }

    InternetStackHelper internet;
    internet.Install(nodes);

    NS_LOG_INFO("Create channels.");
    Ptr<RateErrorModel> rem = CreateObject<RateErrorModel>();
    Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
    Ipv4AddressHelper ipv4;
    PointToPointHelper p2p;
    

    rem->SetRandomVariable(uv);
    uv->SetStream(50);
    rem->SetAttribute("ErrorRate", DoubleValue(0.0));
    rem->SetAttribute("ErrorUnit", StringValue("ERROR_UNIT_PACKET"));
    p2p.SetDeviceAttribute("ReceiveErrorModel", PointerValue(rem));

    // create link

    for (uint32_t i = 0; i < linkNum; ++i)
    {
        uint32_t src, dst;
        string rate, delay;
        double errorRate;
        topoFile >> src >> dst >> rate >> delay >> errorRate;

        p2p.SetDeviceAttribute("DataRate", StringValue(rate));
        p2p.SetChannelAttribute("Delay", StringValue(delay));
        fatherList[dst] = src;

        // Setup topology matrix
        NetDeviceContainer devices = p2p.Install(nodes.Get(src), nodes.Get(dst));
        node2if[nodes.Get(src)][nodes.Get(dst)].index = devices.Get(0)->GetIfIndex(); // src NIC
        node2if[nodes.Get(src)][nodes.Get(dst)].up = true;
        node2if[nodes.Get(src)][nodes.Get(dst)].delay =
            DynamicCast<PointToPointChannel>(devices.Get(0)->GetChannel())
                ->GetDelay()
                .GetNanoSeconds();
        DataRateValue bw;
        devices.Get(0)->GetAttribute("DataRate", bw);
        node2if[nodes.Get(src)][nodes.Get(dst)].bandwidth = bw.Get().GetBitRate();

        node2if[nodes.Get(dst)][nodes.Get(src)].index = devices.Get(1)->GetIfIndex(); // dst NIC
        node2if[nodes.Get(dst)][nodes.Get(src)].up = true;
        node2if[nodes.Get(dst)][nodes.Get(src)].delay =
            DynamicCast<PointToPointChannel>(devices.Get(1)->GetChannel())
                ->GetDelay()
                .GetNanoSeconds();
        devices.Get(1)->GetAttribute("DataRate", bw);
        node2if[nodes.Get(dst)][nodes.Get(src)].bandwidth = bw.Get().GetBitRate();

        char ipString[16];
        sprintf(ipString, "10.%d.%d.0", i + 1, i % 254 + 1);
        ipv4.SetBase(ipString, "255.255.255.0");
        Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

        node2if[nodes.Get(src)][nodes.Get(dst)].srcAddress = interfaces.GetAddress(0);
        node2if[nodes.Get(src)][nodes.Get(dst)].dstAddress = interfaces.GetAddress(1);
        node2if[nodes.Get(dst)][nodes.Get(src)].srcAddress = interfaces.GetAddress(1);
        node2if[nodes.Get(dst)][nodes.Get(src)].dstAddress = interfaces.GetAddress(0);

        hostNode2Addr[nodes.Get(src)] = interfaces.GetAddress(0);
        hostNode2Addr[nodes.Get(dst)] = interfaces.GetAddress(1);

        
        NodeId2Addr[src] = interfaces.GetAddress(0);
        NodeId2Addr[dst] = interfaces.GetAddress(1);


    }

   
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // create application
    NS_LOG_INFO("Create applications.");
    jobFile.open(jobFileName);
    uint32_t totalJobNum, totalWorkerNum;
    jobFile >> totalJobNum >> totalWorkerNum;
    for (uint32_t i = 0; i < totalWorkerNum; ++i)
    {
        uint32_t workerId;
        jobFile >> workerId;
        workerNodeId.push_back(workerId);
    }

 
    // Get assign job
    for (uint32_t i = 0; i < totalJobNum; ++i)
    {
        uint32_t jobId, workerNum, psId;
        jobFile >> jobId >> workerNum >> psId;
        JobList job;
        vector<uint32_t> jobWokerId;
        vector<uint32_t> torswitch;
        map<uint32_t, ns3::Address> newWorkerAddr;

        for (uint32_t j = 0; j < workerNum; ++j)
        {
            uint32_t workerId;
            jobFile >> workerId;
            newWorkerAddr[workerId] = NodeId2Addr[workerId];

            torswitch.push_back(fatherList[workerId]);
            jobWokerId.push_back(workerId);
        }

        job.psId = psId;
        job.jobId = jobId;
        // job.aggNodeId = aggId;

        sort(torswitch.begin(),torswitch.end());
        torswitch.erase(unique(torswitch.begin(), torswitch.end()), torswitch.end());
        
        job.torswitchIdList = torswitch;
        job.psFatherAddr = NodeId2Addr[fatherList[psId]];
        job.workerNum = workerNum;
        job.workerIdList = jobWokerId;
        job.workerAddr = newWorkerAddr;
        pendingJobList.push_back(job);
    }


    // create ps
    for (uint32_t i = 0; i < totalJobNum; ++i)
    {
        InaPSHelper psHelper(8888);

        psHelper.SetAttribute("TotalWorkerNum", UintegerValue(pendingJobList[i].workerNum));
         
        ApplicationContainer psApp = psHelper.Install(nodes.Get(pendingJobList[i].psId));

        psHelper.Initialize();
        cout << "PS "<< pendingJobList[i].psId << " bind success! WorkerNum: " << pendingJobList[i].workerNum << endl;


        psHelper.AddPendingJobList(pendingJobList);
        psApp.Start(Seconds(0.0));
        psApp.Stop(Seconds(1000.0));
    }




    // // create aggregator

    InaAggregatorHelper aggregatorHelper(6666);

    // uint32_t psId = pendingJobList[i].psId;
    // aggregatorHelper.SetAttribute("PSAddress", AddressValue(hostNode2Addr[nodes.Get(psId)]));

    for (uint32_t i = 0; i < totalJobNum; ++i)
    {
        

        for (uint32_t j = 0; j < pendingJobList[i].workerNum; j++)
        {
            uint32_t switchId = fatherList[pendingJobList[i].workerIdList[j]];
            aggregatorNodeId.push_back(switchId);
        }

        for (uint32_t k = 0; k < pendingJobList.size(); k++){
            uint32_t switchId = fatherList[pendingJobList[i].psId];
            aggregatorNodeId.push_back(switchId);
             
        }

        
    }
        sort(aggregatorNodeId.begin(), aggregatorNodeId.end());
        aggregatorNodeId.erase(unique(aggregatorNodeId.begin(), aggregatorNodeId.end()),
                               aggregatorNodeId.end());

        // cout << aggregatorNodeId.size() << endl;

        for (uint32_t k = 0; k < aggregatorNodeId.size(); k++)
        {
            aggregatorHelper.SetAttribute("AggAluNum", UintegerValue(aggregatorAbility[aggregatorNodeId[k]]));

            ApplicationContainer aggregatorApp = aggregatorHelper.Install(nodes.Get(aggregatorNodeId[k]));
            aggregatorHelper.Initialize();
            cout << "aggregator " << aggregatorNodeId[k] << " bind success!" << endl;
             

            // // Search job list, find which job is assigned to this aggregator
            // for(uint32_t j = 0; j < totalJobNum; ++j){
            //     if(pendingJobList[j].aggNodeId == aggregatorNodeId[i]){
            //         aggregatorHelper.AddJob(pendingJobList[j].jobId,
            //         pendingJobList[j].workerNum); totalWorkerNum += pendingJobList[j].workerNum;
            //     }
            // }

            aggregatorHelper.AddFatherList(fatherList);
            aggregatorHelper.AddPendingJobList(pendingJobList);
            aggregatorHelper.AddNodeId2Addr(NodeId2Addr);

            aggregatorApp.Start(Seconds(0.0));
            aggregatorApp.Stop(Seconds(1000.0));
        }
        // aggregatorNodeId.clear();
    





    // create worker according job list
    uint32_t port = 6666;
    for (uint32_t i = 0; i < totalJobNum; ++i)
    {
        uint32_t workerNum = pendingJobList[i].workerNum;

        uint32_t psId = pendingJobList[i].psId;
        Ptr<Node> node = nodes.Get(psId);
        Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
        Ipv4Address address = ipv4->GetAddress(1, 0).GetLocal();

        for (uint32_t j = 0; j < workerNum; ++j)
        {
            InaWorkerHelper workerHelper;
            uint32_t workerId = pendingJobList[i].workerIdList[j];
            // uint32_t aggId = pendingJobList[i].aggNodeId;
            // uint32_t psId = pendingJobList[i].psId;
            workerHelper.SetAttribute("RemoteAddress",AddressValue(hostNode2Addr[nodes.Get(fatherList[workerId])])); // [worker][agg].dst is mean the address of agg
            workerHelper.SetAttribute("RemotePort", UintegerValue(port));
            workerHelper.SetAttribute("JobId", UintegerValue(pendingJobList[i].jobId));
            workerHelper.SetAttribute("PSAddress", AddressValue(address)); 
            workerHelper.SetAttribute("Count", UintegerValue(maxCount));
            workerHelper.SetAttribute("MaxEpoch", UintegerValue(maxEpoch));
            // workerHelper.SetAttribute("WorkerBitmap", (pendingJobList[i].workerIdList));
            workerHelper.SetAttribute("PSId", UintegerValue(pendingJobList[i].psId));
            ApplicationContainer workerApp = workerHelper.Install(nodes.Get(pendingJobList[i].workerIdList[j]));

            workerApp.Start(Seconds(0.0));
            workerApp.Stop(Seconds(1000.0));
        }
    }

    

    Simulator::Run();
    Simulator::Destroy();

    endTime = clock();
    std::cout << "The program spent :" << double(endTime - beginTime) / CLOCKS_PER_SEC << " s"
              << std::endl;
    // std::cout << configFile << std::endl;
    return 0;
}