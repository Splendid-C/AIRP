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





/************************************************
* Config Variables
***********************************************/
string topoFileName, flowFileName, jobFileName;



/************************************************
* Runtime Variables
***********************************************/
ifstream topoFile, flowFile, jobFile;
uint32_t nodeNum = 0, switchNum = 0, linkNum = 0;
uint32_t maxCount = 800000; //the max count of bigjob
uint32_t maxEpoch = 1; //the training epoch
uint32_t multi = 32;
vector<uint32_t> switchNodeId;
vector<uint32_t> workerNodeId;
vector<uint32_t> aggregatorNodeId;
map<uint32_t, uint32_t> aggregatorAbility;
map<uint32_t, uint32_t> aggregatorAluIndex;
map<uint32_t, uint32_t> subJob; //subjobnum of each bigjob
map<uint32_t, uint32_t> bigJobCount; //the total count of bigjob
map<uint32_t, uint32_t> subJobTemp; //store temp of calculating subcount
map<uint32_t, uint32_t> totalCountNum;
vector<JobList> pendingJobList;
map<uint32_t, Address> NodeId2Addr;

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
    string configFile = "mix/config.txt";
    string topology = "fattree";

    CommandLine cmd(__FILE__);
    
   
    cmd.AddValue("maxCount", "the max count of bigjob", maxCount);
    cmd.AddValue("topology", "The topology to run the program.", topology);
    cmd.Parse(argc, argv); 

    Config::SetDefault("ns3::DropTailQueue<Packet>::MaxSize", StringValue("10000000000000000000p")); 

    if(topology == "fattree")
    {
        configFile = "mix/config.txt";
    }else if(topology == "dragonfly")
    {
        configFile = "mix2/config.txt";
    }

    
    
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

    // alu number
    for(uint32_t i = 0; i < switchNum; ++i){
        uint32_t aluNum;
        topoFile >> aluNum;

        if(aluNum == 0){
            aggregatorAbility[switchNodeId[i]] = 1;
        }else{
            aggregatorAbility[switchNodeId[i]] = aluNum/multi;
        }
        

        aggregatorAluIndex[switchNodeId[i]] = 0;
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
        sprintf(ipString, "10.%d.%d.0", (i/254) %254 + 1 , i%254 + 1);
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
    uint32_t totalSubJobNum, totalBigJobNum,totalWorkerNum, totalAggNum;
    jobFile >> totalSubJobNum >> totalBigJobNum >> totalWorkerNum >> totalAggNum;
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

    for(uint32_t i = 0; i < totalBigJobNum; ++i){
        uint32_t subJobNum;
        jobFile >> subJobNum;
        subJob[i] = subJobNum;
    }


    // Get assign job

    for (uint32_t j = 0; j < totalBigJobNum; ++j){

        bigJobCount[j] = maxCount;

        for(uint32_t i = 0; i < subJob[j]; ++i){
            uint32_t jobId, aggId, workerNum, aluAssign;
            jobFile >> jobId >> aggId >> workerNum >> aluAssign;
            JobList job;
            vector<uint32_t> jobWokerId;
            map<uint32_t, ns3::Address> newWorkerAddr;
            subJobTemp[j] += aluAssign/multi;

            for(uint32_t k = 0; k < workerNum; ++k){
                uint32_t workerId;
                jobFile >> workerId;
                newWorkerAddr[workerId] = NodeId2Addr[workerId];
                jobWokerId.push_back(workerId);
            }

            job.jobId = jobId;
            job.bigJobId = j;
            job.aggNodeId = aggId;
            job.workerNum = workerNum;
            job.subJobNum = subJob[j];
            job.workerAddr = newWorkerAddr;
            job.aluAssign = aluAssign/multi;
             

            // job.subCountNum = bigJobCount[j]*(double(aluAssign)/aggregatorAbility[aggId]);

            // assign alu to each job
            job.head = aggregatorAluIndex[aggId];
            job.tail = job.head + aluAssign/multi;
            aggregatorAluIndex[aggId] = job.tail + 1;
            // cout<<"subjob "<<job.jobId<<": "<<"aggID "<<aggId<<"from "<<job.head<<" to "<<job.tail<<", total "<< aluAssign/multi<<endl;
        
            job.workerIdList = jobWokerId;
            pendingJobList.push_back(job);
        }
    }

     
    for(uint32_t i = 0; i < totalSubJobNum; ++i)
    {
        uint32_t bigJob = pendingJobList[i].bigJobId;
        pendingJobList[i].subCountNum = bigJobCount[bigJob]*(double(pendingJobList[i].aluAssign)/subJobTemp[bigJob]);
         
        // cout <<" 99999999999999 "<<pendingJobList[i].aluAssign<<" "<<subJobTemp[i]<<" " <<pendingJobList[i].subCountNum<<endl;
         
    }

    for(uint32_t i = 0; i < totalSubJobNum; ++i)
    {
        uint32_t bigJob = pendingJobList[i].bigJobId;
        totalCountNum[bigJob] += pendingJobList[i].subCountNum;
        
    }


    //std::cout << pendingJobList[0].jobId << " " << pendingJobList[0].aggNodeId << std::endl;
    // create aggregator
    for(uint32_t i = 0; i < totalAggNum; ++i){
        InaAggregatorHelper aggregatorHelper(6666);
        aggregatorHelper.SetAttribute("AggAluNum", UintegerValue(aggregatorAbility[aggregatorNodeId[i]]));
        ApplicationContainer aggregatorApp = aggregatorHelper.Install(nodes.Get(aggregatorNodeId[i]));

        aggregatorHelper.Initialize();
        uint32_t totalWorkerNum = 0;
        
        // Search job list, find which job is assigned to this aggregator
        for(uint32_t j = 0; j < totalSubJobNum; ++j){
            if(pendingJobList[j].aggNodeId == aggregatorNodeId[i]){
                aggregatorHelper.AddJob(pendingJobList[j].jobId, pendingJobList[j].workerNum, pendingJobList[j].head, pendingJobList[j].tail,pendingJobList[j].workerAddr, pendingJobList[j].workerIdList);
                totalWorkerNum += pendingJobList[j].workerNum;
            }
        }

        aggregatorHelper.AssignTotalWorkerNum(totalWorkerNum);



        aggregatorApp.Start(Seconds(0.0));
        aggregatorApp.Stop(Seconds(1000.0));
    }

    // create worker according job list
    uint32_t index = 0;
    uint32_t port = 6666;
    for(uint32_t k = 0; k< totalBigJobNum; ++k){
        for(uint32_t i = 0; i < subJob[k]; ++i){
            uint32_t workerNum = pendingJobList[index].workerNum;
            for(uint32_t j = 0; j < workerNum; ++j){
                InaWorkerHelper workerHelper;
                // uint32_t workerId = pendingJobList[i].workerIdList[j];

                

                uint32_t aggId = pendingJobList[index].aggNodeId;
                workerHelper.SetAttribute("RemoteAddress", AddressValue(hostNode2Addr[nodes.Get(aggId)])); // [worker][agg].dst is mean the address of agg
                workerHelper.SetAttribute("RemotePort", UintegerValue(port));
                workerHelper.SetAttribute("Count", UintegerValue(pendingJobList[index].subCountNum));
                workerHelper.SetAttribute("JobId", UintegerValue(pendingJobList[index].jobId));
                workerHelper.SetAttribute("TotalSubJobNum", UintegerValue(pendingJobList[index].subJobNum));
                workerHelper.SetAttribute("TotalJobCount", UintegerValue(totalCountNum[k]));
                workerHelper.SetAttribute("MaxEpoch", UintegerValue(maxEpoch));
                ApplicationContainer workerApp = workerHelper.Install(nodes.Get(pendingJobList[index].workerIdList[j]));

                // workerHelper.AssignPendingJobList(pendingJobList);
                workerApp.Start(Seconds(0.0));
                workerApp.Stop(Seconds(1000.0));

            }
            index++;
        }
    }

    
    Simulator::Run();
    Simulator::Destroy();
    
    endTime = clock();
    std::cout << "The program spent :" <<  double (endTime - beginTime) / CLOCKS_PER_SEC << " s" << std::endl;
    //std::cout << configFile << std::endl;
    return 0;
    
}