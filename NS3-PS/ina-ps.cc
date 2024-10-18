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


#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include <string>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include "ns3/ina-module.h"
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

    Interface() : index(0), up(false), delay(0), bandwidth(0) {}
};

struct JobList
{
    uint32_t jobId;
    uint32_t aggNodeId;
    uint32_t workerNum;
    vector<uint32_t> workerIdList;
};



/************************************************
* Config Variables
***********************************************/
string topoFileName, flowFileName, jobFileName;



/************************************************
* Runtime Variables
***********************************************/
ifstream topoFile, flowFile, jobFile;
uint32_t nodeNum = 0, switchNum = 0, linkNum = 0;
uint32_t maxCount = 800000;
vector<uint32_t> switchNodeId;
vector<uint32_t> workerNodeId;
vector<uint32_t> aggregatorNodeId;
vector<JobList> pendingJobList;

// Topology Matrix
map<Ptr<Node>, map<Ptr<Node>, Interface> > node2if;
map<Ptr<Node>, Address> hostNode2Addr;

void 
readConfigFile(string configFileName){
    ifstream config;
    config.open(configFileName);

    // Read the configuration file
    cout << "*************************************" << endl;
    cout << "* Begin to load config file" << endl;
    cout << "*************************************" << endl;
    while(!config.eof()){
        string key;
        config >> key;
        if (key.compare("TOPOLOGY_FILE") == 0){
            string tmp;
            config >> tmp;
            topoFileName = tmp;
            std::cout << "TOPOLOGY_FILE\t\t" << topoFileName << endl;
        }else if (key.compare("FLOW_FILE") == 0){
            string tmp;
            config >> tmp;
            flowFileName = tmp;
            std::cout << "FLOW_FILE\t\t" << flowFileName << endl;
        }else if (key.compare("JOB_ASSIGN_FILE") == 0){
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
main(int argc, char* argv[]) {

    LogComponentEnable("InaWorker", LOG_LEVEL_INFO);
    // LogComponentEnable("InaAggregator", LOG_LEVEL_INFO);
    
    clock_t beginTime, endTime;
    beginTime = clock();

    CommandLine cmd(__FILE__);
    string configFile = "mix/config.txt";
    cmd.AddValue("maxCount", "The count to run the program.", maxCount);
    cmd.AddValue("config", "The configuration file required to run the program.", configFile);

    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::DropTailQueue<Packet>::MaxSize", StringValue("1000000000000p"));
    
    readConfigFile(configFile);

    topoFile.open(topoFileName);
    topoFile >> nodeNum >> switchNum >> linkNum;
    
    // create node
    NS_LOG_INFO("Create nodes.");
    NodeContainer nodes;
    nodes.Create(nodeNum);
    for(uint32_t i = 0; i < switchNum; ++i){
        uint32_t switchId;
        topoFile >> switchId;
        switchNodeId.push_back(switchId);
    }

    for(uint32_t i = 0; i < switchNum; ++i){
        uint32_t aluNum;
        topoFile >> aluNum;
    }
    

    InternetStackHelper internet;
	internet.Install(nodes);


    // create link
    NS_LOG_INFO("Create channels.");
    Ptr<RateErrorModel> rem = CreateObject<RateErrorModel>();
	Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
    Ipv4AddressHelper ipv4;
    for(uint32_t i = 0; i < linkNum; ++i){
        uint32_t src, dst;
        string rate, delay;
        double errorRate;
        topoFile >> src >> dst >> rate >> delay >> errorRate;
        
        PointToPointHelper p2p;
        p2p.SetDeviceAttribute("DataRate", StringValue(rate));
        p2p.SetChannelAttribute("Delay", StringValue(delay));

        rem->SetRandomVariable(uv);
	    uv->SetStream(50);
        rem->SetAttribute("ErrorRate", DoubleValue(errorRate));
		rem->SetAttribute("ErrorUnit", StringValue("ERROR_UNIT_PACKET"));
        p2p.SetDeviceAttribute("ReceiveErrorModel", PointerValue(rem));

        // Setup topology matrix
        NetDeviceContainer devices = p2p.Install(nodes.Get(src), nodes.Get(dst));
        node2if[nodes.Get(src)][nodes.Get(dst)].index = devices.Get(0)->GetIfIndex();       // src NIC
        node2if[nodes.Get(src)][nodes.Get(dst)].up = true;
        node2if[nodes.Get(src)][nodes.Get(dst)].delay = DynamicCast<PointToPointChannel>(devices.Get(0)->GetChannel())->GetDelay().GetNanoSeconds();
        DataRateValue bw;
        devices.Get(0)->GetAttribute("DataRate", bw);
        node2if[nodes.Get(src)][nodes.Get(dst)].bandwidth = bw.Get().GetBitRate();

        node2if[nodes.Get(dst)][nodes.Get(src)].index = devices.Get(1)->GetIfIndex();       // dst NIC
        node2if[nodes.Get(dst)][nodes.Get(src)].up = true;
        node2if[nodes.Get(dst)][nodes.Get(src)].delay = DynamicCast<PointToPointChannel>(devices.Get(1)->GetChannel())->GetDelay().GetNanoSeconds();
        devices.Get(1)->GetAttribute("DataRate", bw);
        node2if[nodes.Get(dst)][nodes.Get(src)].bandwidth = bw.Get().GetBitRate();

        char ipString[16];
        sprintf(ipString, "10.%d.%d.0", i + 1, i%254 + 1);
        ipv4.SetBase(ipString, "255.255.255.0");
        Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);
    
        node2if[nodes.Get(src)][nodes.Get(dst)].srcAddress = interfaces.GetAddress(0);
        node2if[nodes.Get(src)][nodes.Get(dst)].dstAddress = interfaces.GetAddress(1);
        node2if[nodes.Get(dst)][nodes.Get(src)].srcAddress = interfaces.GetAddress(1);
        node2if[nodes.Get(dst)][nodes.Get(src)].dstAddress = interfaces.GetAddress(0);

         
        hostNode2Addr[nodes.Get(dst)] = interfaces.GetAddress(1);
        hostNode2Addr[nodes.Get(src)] = interfaces.GetAddress(0);
    }
    
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // create application
    NS_LOG_INFO("Create applications.");
    jobFile.open(jobFileName);
    uint32_t totalJobNum, totalWorkerNum, totalAggNum;
    jobFile >> totalJobNum >> totalWorkerNum >> totalAggNum;
    for(uint32_t i = 0; i < totalWorkerNum; ++i){
        uint32_t workerId;
        jobFile >> workerId;
        workerNodeId.push_back(workerId);
    }

    for(uint32_t i = 0; i < totalAggNum; ++i){
        uint32_t aggId;
        jobFile >> aggId;
        aggregatorNodeId.push_back(aggId);
    }

    // Get assign job
    for(uint32_t i = 0; i < totalJobNum; ++i){
        uint32_t jobId, aggId, workerNum;
        jobFile >> jobId >> workerNum >>  aggId ;
        JobList job;
        vector<uint32_t> jobWokerId;
        for(uint32_t j = 0; j < workerNum; ++j){
            uint32_t workerId;
            jobFile >> workerId;
            jobWokerId.push_back(workerId);
        }
        job.jobId = jobId;
        job.aggNodeId = aggId;
        job.workerNum = workerNum;
        job.workerIdList = jobWokerId;
        pendingJobList.push_back(job);
    }

    //std::cout << pendingJobList[0].jobId << " " << pendingJobList[0].aggNodeId << std::endl;
    // create aggregator
    for(uint32_t i = 0; i < totalAggNum; ++i){
        InaAggregatorHelper aggregatorHelper(6666);
        
        ApplicationContainer aggregatorApp = aggregatorHelper.Install(nodes.Get(aggregatorNodeId[i]));

        aggregatorHelper.Initialize();
        uint32_t totalWorkerNum = 0;
        
        // Search job list, find which job is assigned to this aggregator
        for(uint32_t j = 0; j < totalJobNum; ++j){
            if(pendingJobList[j].aggNodeId == aggregatorNodeId[i]){
                aggregatorHelper.AddJob(pendingJobList[j].jobId, pendingJobList[j].workerNum);
                totalWorkerNum += pendingJobList[j].workerNum;
            }
        }
        aggregatorHelper.AssignTotalWorkerNum(totalWorkerNum);


        aggregatorApp.Start(Seconds(0.0));
        aggregatorApp.Stop(Seconds(100.0));
    }

    // create worker according job list
    uint32_t port = 6666;
    for(uint32_t i = 0; i < totalJobNum; ++i){
        uint32_t workerNum = pendingJobList[i].workerNum;
        for(uint32_t j = 0; j < workerNum; ++j){
            InaWorkerHelper workerHelper;
            // uint32_t workerId = pendingJobList[i].workerIdList[j];
            uint32_t aggId = pendingJobList[i].aggNodeId;
            workerHelper.SetAttribute("RemoteAddress", AddressValue(hostNode2Addr[nodes.Get(aggId)])); // [worker][agg].dst is mean the address of agg
            workerHelper.SetAttribute("RemotePort", UintegerValue(port));
            workerHelper.SetAttribute("Count", UintegerValue(maxCount));
            workerHelper.SetAttribute("JobId", UintegerValue(pendingJobList[i].jobId));
            workerHelper.SetAttribute("PSId", UintegerValue(aggId));
            
            ApplicationContainer workerApp = workerHelper.Install(nodes.Get(pendingJobList[i].workerIdList[j]));
            workerApp.Start(Seconds(0.0));
            workerApp.Stop(Seconds(100.0));

        }
    }

    
    Simulator::Run();
    Simulator::Destroy();
    
    endTime = clock();
    std::cout << "The program spent :" <<  double (endTime - beginTime) / CLOCKS_PER_SEC << " s" << std::endl;
    //std::cout << configFile << std::endl;
    return 0;
    
}