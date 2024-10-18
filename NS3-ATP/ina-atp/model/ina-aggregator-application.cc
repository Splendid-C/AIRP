/*
 * Copyright (c) 2023 FNIL laboratory
 *
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
 *
 * Author: Minglin Li <liml21@m.fudan.edu.cn>
 *
 */

#include "ina-aggregator-application.h"
 
#include "ns3/uinteger.h"
#include <map>

#include "ns3/application.h"
 
#include "ns3/uinteger.h"
#include "ns3/vector.h"
#include "ns3/type-id.h"
#include "ns3/seq-ts-header.h"
#include "ns3/ina-header.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/core-module.h"

// map<uint32_t, bool>workerBitmap;
uint32_t index2 = 0;
map<uint32_t, map<uint32_t, map<uint32_t, bool>>> workerBitmap;//jobid, workerid, seqid, isReceived 
map<uint32_t, map<uint32_t, vector<uint32_t>>>switch_workerList; //<switchid, jobid, workerlist>
map<uint32_t, ns3::Address> workerAddress;
map<uint32_t, vector<ns3::Address>> switchAddress2;
map<uint32_t, map<uint32_t, uint32_t>> JobSeqCount2;// job, seq ,  count 


using namespace std;


namespace ns3
{

NS_LOG_COMPONENT_DEFINE("InaAggregator");

NS_OBJECT_ENSURE_REGISTERED(InaAggregator);

TypeId
InaAggregator::GetTypeId()
{
    static TypeId tid = TypeId("ns3::InaAggregtor")
                            .SetParent<Application>()
                            .SetGroupName("Ina")
                            .AddConstructor<InaAggregator>()
                            .AddAttribute("Port",
                                          "Port on which we listen for incoming packets.",
                                          UintegerValue(10010),
                                          MakeUintegerAccessor(&InaAggregator::m_port),
                                          MakeUintegerChecker<uint16_t>())
                            .AddAttribute("AggAluNum",
                                          "The number of aggregation ALUs",
                                          UintegerValue(2560000/32),
                                          MakeUintegerAccessor(&InaAggregator::m_aggAluNum),
                                          MakeUintegerChecker<uint32_t>())
                            .AddAttribute("AggDelay",
                                          "The delay of aggregation",
                                          TimeValue(Seconds(0.001)),
                                          MakeTimeAccessor(&InaAggregator::m_aggDelay),
                                          MakeTimeChecker())
                            .AddAttribute("Size",
                                          "Size of each packet",
                                          UintegerValue(256),
                                          MakeUintegerAccessor(&InaAggregator::m_size),
                                          MakeUintegerChecker<uint32_t>())
                            .AddAttribute("TotalWorkerNum",
                                          "The total number of workers",
                                          UintegerValue(100),
                                          MakeUintegerAccessor(&InaAggregator::m_totalWorkerNum),
                                          MakeUintegerChecker<uint32_t>())
                            .AddAttribute("PSAddress", "The PS address of the application",
						                AddressValue (),
						                MakeAddressAccessor (&InaAggregator::m_psAddress),
						                MakeAddressChecker ())
    ;
    return tid;
}

InaAggregator::InaAggregator()
{
    NS_LOG_FUNCTION(this);
    m_aggAluNum = 2560000/32;
}

InaAggregator::~InaAggregator()
{
    NS_LOG_FUNCTION(this);
}

void
InaAggregator::Initialize()
{
    NS_LOG_FUNCTION(this);

    // initial ALUs
    for (uint32_t i = 0; i < m_aggAluNum; ++i)
    {
        InaAggALU newAlu;
        m_aggAlu.push_back(newAlu);
        m_aggAlu[i].m_state = InaAggALU::IDLE;
        m_aggAlu[i].m_counter = 0;
        m_aggAlu[i].m_ECN = 0;
        m_aggAlu[i].m_serveJobId = -1; // current job id
        m_aggAlu[i].m_serveSeqId = -1; // current seq id
        m_aggAlu[i].m_timestamp = 0;
        m_aggAlu[i].m_value = 0;
    }
}

void
InaAggregator::AssignTotalWorkerNum(uint32_t workerNum)
{
    NS_LOG_FUNCTION(this);
    m_totalWorkerNum = workerNum;
}

void
InaAggregator::AddFatherList(map<uint32_t, uint32_t>fatherList)
{
    NS_LOG_FUNCTION(this);
    m_fatherList = fatherList;

}

void
InaAggregator::AddPendingJobList(vector<JobList> pendingJobList)
{
    NS_LOG_FUNCTION(this);
    m_pendingJobList = pendingJobList;

}


void 
InaAggregator::AddNodeId2Addr(map<uint32_t, Address> hostNode2Addr)
{
    NS_LOG_FUNCTION(this);
    m_NodeId2Addr = hostNode2Addr;
}

void
InaAggregator::AddJob(uint32_t jobId, uint32_t workerNum)
{
    NS_LOG_FUNCTION(this);

    // here we just know each job should serve how many workers
    InaJobStatus newJobStatus;
    newJobStatus.m_jobId = jobId;
    newJobStatus.m_workerNum = workerNum;

    m_jobStatusMap[jobId] = newJobStatus;
    return;
}

void
InaAggregator::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_socket = 0;
    m_socket2 = 0;
    Application::DoDispose();
}

void
InaAggregator::StartApplication()
{
    NS_LOG_FUNCTION(this);

    uint32_t switchId = GetNode()->GetId();

    //find which worker and job bind to this aggregator/switch

    for(uint32_t k = 0; k < m_pendingJobList.size(); ++k){
        for(uint32_t i = 0 ; i < m_pendingJobList[k].workerIdList.size(); ++i){
             
            if (m_fatherList[m_pendingJobList[k].workerIdList[i]] == switchId){
                switch_workerList[switchId][k].push_back(m_pendingJobList[k].workerIdList[i]);
        }
        
        }
    }


    if (!m_socket)
    {
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_socket = Socket::CreateSocket(GetNode(), tid);
        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_port);
        if (m_socket->Bind(local) == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }
    }

    m_socket->SetRecvCallback(MakeCallback(&InaAggregator::HandleRead, this));

    if (!m_socket6)
    {
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_socket6 = Socket::CreateSocket(GetNode(), tid);
        Inet6SocketAddress local6 = Inet6SocketAddress(Ipv6Address::GetAny(), m_port);
        if (m_socket6->Bind(local6) == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }
    }

    m_socket6->SetRecvCallback(MakeCallback(&InaAggregator::HandleRead, this));
}


void
InaAggregator::StopApplication()
{
    NS_LOG_FUNCTION(this);
    // Cancel
    if (m_socket)
    {
        m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }

    if (m_socket6)
    {
        m_socket6->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }
}


void
InaAggregator::Send(Address m_workerAddr, Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this);

    m_socket->SendTo(packet, 0, m_workerAddr);
}

void 
InaAggregator::SetupSocket(Address address){
	NS_LOG_FUNCTION (this);

	// set up socket
	if(!m_socket2){
        
		TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
		m_socket2 = Socket::CreateSocket(GetNode(), tid);
		if(Ipv4Address::IsMatchingType(address) == true){
			if(m_socket2->Bind() == -1){
				NS_FATAL_ERROR("Failed to bind socket");
			}
			m_socket2->Connect(InetSocketAddress(Ipv4Address::ConvertFrom(address), 8888));
             
		}

	} else if(Ipv6Address::IsMatchingType(address) == true){
		if(m_socket2->Bind6() == -1){
			NS_FATAL_ERROR("Failed to bind socket");
		}
		m_socket2->Connect(Inet6SocketAddress(Ipv6Address::ConvertFrom(address), 8888));

	} else if(InetSocketAddress::IsMatchingType(address) == true){
		if(m_socket2->Bind() == -1){
			NS_FATAL_ERROR("Failed to bind socket");
		}
		m_socket2->Connect(address);

	} else if(Inet6SocketAddress::IsMatchingType(address) == true){
		if(m_socket2->Bind6() == -1){
			NS_FATAL_ERROR("Failed to bind socket");
		}
		m_socket2->Connect(address);
    }
	

	m_socket2->SetRecvCallback(MakeCallback(&InaAggregator::HandleRead, this));
	m_socket2->SetAllowBroadcast(false);

}



/// @brief
// handle the Packet received, and store in
// m_workerStatusMap[workerId].m_pendingQueueMap[currentSequenceNum], next step is to decide whether
// the agg is ready
/// @param socket
void
InaAggregator::HandleRead(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    Ptr<Packet> packet;

    Address from;
    Address localAddress;

    while ((packet = socket->RecvFrom(from)))
    {
        socket->GetSockName(localAddress);
        if (packet->GetSize() > 0)
        {
            // copy a packet
            Ptr<Packet> originalPacket = packet->Copy();

            // Get packet header
            uint32_t receivedSize = packet->GetSize();
            SeqTsHeader seqTsHeader;
            InaHeader inaHeader;

            packet->RemoveHeader(seqTsHeader);
            packet->RemoveHeader(inaHeader);

            // Get packet information
            uint32_t workerId = inaHeader.GetWorkerId();
            uint32_t epochId = inaHeader.GetEpochId();
            uint32_t jobId = inaHeader.GetJobId();
            uint32_t workerNum = inaHeader.GetWorkerNum();
            uint16_t packetType = inaHeader.GetPacketType();
            uint32_t param = inaHeader.GetParam();
            uint32_t psId = inaHeader.GetPSId();

             

            // use for recovery in the futrue
            // now, we assume in a lossless network
            uint32_t currentSequenceNum = seqTsHeader.GetSeq();

        
            if (m_workerStatusMap.find(workerId) == m_workerStatusMap.end())
            {
                // new worker
                InaWorkerStatus newWorkerStatus;
                newWorkerStatus.m_workerId = workerId;
                newWorkerStatus.m_epochId = epochId;
                newWorkerStatus.m_jobId = jobId;
                newWorkerStatus.m_workerAddress = from;
                
                workerAddress[workerId] = from;
                newWorkerStatus.m_recvDataFlag = true;
                newWorkerStatus.m_recvCnt = 1;
                newWorkerStatus.m_expectedSeq = 0;
                 

                newWorkerStatus.m_pendingQueueMap[currentSequenceNum] = originalPacket;

                m_workerStatusMap.insert(pair<uint32_t, InaWorkerStatus>(workerId, newWorkerStatus));

                // joblist add worker id
                m_jobStatusMap[jobId].m_workerIdList.push_back(workerId);
            }
            else
            {
                if (m_workerStatusMap[workerId].m_epochId != epochId)
                {
                    // if the epoch id is not the same, then this is a new epoch
                    // reset the worker status
                    m_workerStatusMap[workerId].m_expectedSeq = 0;
                    m_workerStatusMap[workerId].m_epochId = epochId;
                    m_workerStatusMap[workerId].m_recvDataFlag = true;
                    m_workerStatusMap[workerId].m_pendingQueueMap.clear();
                    m_workerStatusMap[workerId].m_pendingQueueMap[currentSequenceNum] =
                        originalPacket;

                    m_jobStatusMap[jobId].m_epochId = epochId;
                    m_jobStatusMap[jobId].m_curSeq = 0;
                    m_jobStatusMap[jobId].m_aggSeq = 0;
                    m_jobStatusMap[jobId].m_readyPacketList.clear();
                    m_jobStatusMap[jobId].m_state = InaJobStatus::IDLE;
                    
                }
                else
                {
                    // worker has been recorded. update the status.

                    if (currentSequenceNum >= m_workerStatusMap[workerId].m_expectedSeq)
                    {
                        // this is a new packet
                        m_workerStatusMap[workerId].m_recvCnt++;

                        if (m_workerStatusMap[workerId].m_pendingQueueMap.find(
                                currentSequenceNum) ==
                            m_workerStatusMap[workerId].m_pendingQueueMap.end())
                        {
                            // make sure not to insert a packet twice
                            m_workerStatusMap[workerId].m_recvDataFlag = true;
                            m_workerStatusMap[workerId].m_pendingQueueMap[currentSequenceNum] =
                                originalPacket;
                        }
                    }
                }
            }

             
            if(packetType == 4){
                //ps-tor receive packets from ps

                 
                NS_LOG_INFO("Aggregator "
                        << GetNode()->GetId() << " Received " << receivedSize << " bytes from "
                        << "Worker: "<<workerId
                        << " Sequence Number: " << currentSequenceNum
                        << " Time: " << (Simulator::Now()).GetSeconds() << " WorkerID: " << workerId
                        << " epochID: " << epochId << " jobID: " << jobId << " Param: " << param
                        << " packetType: " << packetType);
                 
                uint32_t ackseq = m_jobStatusMap[jobId].m_aggSeq;
                SendToSwitch(jobId, param, currentSequenceNum);

            }else if(packetType == 0){
                //this switch is normal tor, receive packet from workers
                
                NS_LOG_INFO("Aggregator "
                        << GetNode()->GetId() << " Received " << receivedSize << " bytes from "
                        << "Worker: "<<workerId
                        << " Sequence Number: " << currentSequenceNum
                        << " Time: " << (Simulator::Now()).GetSeconds() << " WorkerID: " << workerId
                        << " epochID: " << epochId << " jobID: " << jobId << " Param: " << param
                        << " packetType: " << packetType);
                 
                PacketArrivalStage(originalPacket);


            }else if (packetType == 1){
                //this switch is ps-tor, do aggregation
                if(currentSequenceNum % 100000 == 0){
                NS_LOG_INFO("Aggregator "
                        << GetNode()->GetId() << " Received " << receivedSize << " bytes from "
                        // << InetSocketAddress::ConvertFrom(from).GetIpv4()
                        << "SwitchID: "<<workerId
                        << " Sequence Number: " << currentSequenceNum
                        << " Time: " << (Simulator::Now()).GetSeconds() << " WorkerID: " << workerId
                        << " epochID: " << epochId << " jobID: " << jobId << " Param: " << param
                        << " packetType: " << packetType);
                }
                switchAddress2[jobId].push_back(from);

                PSTorExecute(originalPacket);

                    
            }else if (packetType == 5){
                //tor receive packet from ps-tor
                NS_LOG_INFO("Aggregator "
                        << GetNode()->GetId() << " Received " << receivedSize << " bytes from "
                        // << InetSocketAddress::ConvertFrom(from).GetIpv4()
                        << "Worker: "<<workerId
                        << " Sequence Number: " << currentSequenceNum
                        << " Time: " << (Simulator::Now()).GetSeconds() << " WorkerID: " << workerId
                        << " epochID: " << epochId << " jobID: " << jobId << " Param: " << param
                        << " packetType: " << packetType);
              
                SendToWorker(jobId, currentSequenceNum, 0);

            
            }else if (packetType == 3){
                // ps-tor send to ps, will not be received by switch
                 
                NS_LOG_INFO("error");
            }
        }
    }
}



bool
InaAggregator::CheckIfRecvAll(uint32_t jobId, uint32_t switchId, uint32_t seqId)
{
    
    for (uint32_t i = 0 ; i < switch_workerList[switchId][jobId].size(); ++i ){
        if (workerBitmap[jobId][switch_workerList[switchId][jobId][i]][seqId] == 0){
            return 0;
        }
    }

    return 1;

}



void 
InaAggregator::SendToWorker(
                                 
                                 uint32_t jobId,
                                 uint32_t aggregateSum,
                                 uint32_t ackSeq
)
{

    uint32_t switchId = GetNode()->GetId();

    for (uint32_t i = 0; i < switch_workerList[switchId][jobId].size(); ++i)
    {
        
        uint32_t workerId = switch_workerList[switchId][jobId][i];

                // cout <<"0000000000000 "<< workerAddress[workerId]<<endl;

        SeqTsHeader seqTs;
        InaHeader inaH;
        Ptr<Packet> packet =
            Create<Packet>(m_size - seqTs.GetSerializedSize() - inaH.GetSerializedSize());
        seqTs.SetSeq(aggregateSum);
        inaH.SetWorkerId(workerId);
        inaH.SetParam(aggregateSum);
        inaH.SetJobId(jobId);
        inaH.SetWorkerId(workerId);
        inaH.SetEpochId(m_workerStatusMap[workerId].m_epochId);
        //6:Layer 1 result back to worker
        inaH.SetPacketType(6);

        packet->AddHeader(inaH);
        packet->AddHeader(seqTs);
         
         
        Send(workerAddress[workerId], packet);
    }

}

uint32_t
InaAggregator::hashFunction(uint32_t param1, uint32_t param2) {
    uint32_t hash = param1 ^ (param2 + 0x9e3779b9 + (param1 << 6) + (param1 >> 2));
    hash = (hash ^ (hash >> 16)) * 0x85ebca6b;
    hash = (hash ^ (hash >> 13)) * 0xc2b2ae35;
    hash = hash ^ (hash >> 16);
    return hash % (m_aggAluNum); 
}



void
InaAggregator::PacketArrivalStage(Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this);
    

    SeqTsHeader seqTsHeader;
    InaHeader inaHeader;

    packet->RemoveHeader(seqTsHeader);
    packet->RemoveHeader(inaHeader);

    // Get packet information
    uint32_t workerId = inaHeader.GetWorkerId();
    uint32_t epochId = inaHeader.GetEpochId();
    uint32_t jobId = inaHeader.GetJobId();
    uint32_t workerNum = inaHeader.GetWorkerNum();
    uint16_t packetType = inaHeader.GetPacketType();
    uint32_t param = inaHeader.GetParam();
    uint32_t psId = inaHeader.GetPSId();
    uint32_t switchId = GetNode()->GetId();
    uint32_t seqId = seqTsHeader.GetSeq();

    

    uint32_t aggAluId =  hashFunction(jobId, seqId);

    
    if (m_aggAlu[aggAluId].m_state == InaAggALU::IDLE)
    {
        //occupy this ALU and set the state
        m_aggAlu[aggAluId].m_state = InaAggALU::BUSY;
        m_aggAlu[aggAluId].m_serveJobId = jobId;
        m_aggAlu[aggAluId].m_serveSeqId = seqId;
        m_aggAlu[aggAluId].m_value = param;
        m_aggAlu[aggAluId].m_counter++;

        workerBitmap[jobId][workerId][seqId] = 1;
         


        if (CheckIfRecvAll(jobId, switchId,seqId)){
            
            Simulator::Schedule(Seconds(0), &InaAggregator::SendtoPSTor, this, jobId, 1, seqId, switch_workerList[switchId][jobId].size(), psId, 6666);

            FinishAggregation(aggAluId, jobId,m_aggAlu[aggAluId].m_value, seqId);
            // workerBitmap[jobId].clear();
        }
         

    }else if ((m_aggAlu[aggAluId].m_state == InaAggALU::BUSY) && (m_aggAlu[aggAluId].m_serveJobId == jobId) && (m_aggAlu[aggAluId].m_serveSeqId == seqId)){
        //add at the same ALU
        
       
         
        // m_aggAlu[aggAluId].m_value += param;
        workerBitmap[jobId][workerId][seqId] = 1;

        
        if (CheckIfRecvAll(jobId, switchId, seqId)){
            uint32_t subWorkerNum = switch_workerList[switchId][jobId].size();

            // Simulator::Schedule(Seconds(0.01)*(subWorkerNum-1), &InaAggregator::SendtoPSTor, this, jobId, 1, seqId, subWorkerNum, psId, 6666);
            Simulator::Schedule(Seconds(0), &InaAggregator::SendtoPSTor, this, jobId, 1, seqId, subWorkerNum, psId, 6666);
            // Simulator::Schedule(Seconds(0.002)*(subWorkerNum-1), &InaAggregator:: FinishAggregation, this , aggAluId, jobId,m_aggAlu[aggAluId].m_value, seqId);
             
            Simulator::Schedule(Seconds(0.015), &InaAggregator:: FinishAggregation, this , aggAluId, jobId,m_aggAlu[aggAluId].m_value, seqId);
            // Simulator::Schedule(Seconds(0), &InaAggregator:: FinishAggregation, this , aggAluId, jobId,m_aggAlu[aggAluId].m_value, seqId);
           
        }


       
    }else {
        //send to PS
         
        index2++;
         
        SendtoPS(jobId, seqId, 2, param, 1, psId);
         
    }

}




void 
InaAggregator::PSTorExecute(Ptr<Packet> packet)
{
    Ptr<Packet> pkt = packet->Copy();

    SeqTsHeader seqTsHeader;
    InaHeader inaHeader;

    packet->RemoveHeader(seqTsHeader);
    packet->RemoveHeader(inaHeader);

    uint32_t seqId = seqTsHeader.GetSeq();
    uint32_t jobId = inaHeader.GetJobId();
    uint64_t param = inaHeader.GetParam();
    uint32_t workernum = inaHeader.GetWorkerNum();
    uint32_t workerid = inaHeader.GetWorkerId();
    uint32_t psId = inaHeader.GetPSId();
    


    JobSeqCount2[jobId][seqId] += workernum;
    if(JobSeqCount2[jobId][seqId] == m_pendingJobList[jobId].workerNum){
        // wait 0.03s 
        //Simulator::Schedule(Seconds(0.03), &InaAggregator::SendtoPS,this, jobId, seqId, 3, 0 , JobSeqCount2[jobId][seqId], m_pendingJobList[jobId].psId);
        Simulator::Schedule(Seconds(0), &InaAggregator::SendtoPS,this, jobId, seqId, 3, 0 , JobSeqCount2[jobId][seqId], m_pendingJobList[jobId].psId);
        
    }

 

}





void 
InaAggregator::SendtoPS(uint32_t jobId, uint32_t seqId, uint32_t packetType, uint32_t param, uint32_t workernum, uint32_t psId){

    NS_LOG_FUNCTION(this);
    
    uint32_t switchId = GetNode()->GetId();

    SeqTsHeader seqTs;
    InaHeader inaH;
    Ptr<Packet> packet = Create<Packet>(m_size - seqTs.GetSerializedSize() - inaH.GetSerializedSize());

    seqTs.SetSeq(seqId);
    inaH.SetWorkerId(switchId);
    inaH.SetParam(param);
    inaH.SetJobId(jobId);
    inaH.SetEpochId(0);
    inaH.SetPacketType(packetType);

    inaH.SetWorkerNum(workernum);

    packet->AddHeader(inaH);
    packet->AddHeader(seqTs);

    
    InetSocketAddress remoteAddress = InetSocketAddress(Ipv4Address::ConvertFrom(m_NodeId2Addr[psId]), 8888);
    
    m_socket->SendTo(packet, 0, remoteAddress);
    
    
}



void
InaAggregator::SendToSwitch(uint32_t jobId, uint32_t aggregateSum, uint32_t ackSeq)
{

                
 // send back aggregation result to switches
     


    for(uint32_t i = 0; i< m_pendingJobList[jobId].torswitchIdList.size(); ++i)
    {
        uint32_t psId = GetNode()->GetId();
        uint32_t switchId = m_pendingJobList[jobId].torswitchIdList[i];
        uint32_t workerId = psId;
        SeqTsHeader seqTs;
        InaHeader inaH;
        Ptr<Packet> packet =
            Create<Packet>(m_size - seqTs.GetSerializedSize() - inaH.GetSerializedSize());
        seqTs.SetSeq(ackSeq);
        inaH.SetWorkerId(psId);
        inaH.SetParam(aggregateSum);
        inaH.SetJobId(jobId);
        inaH.SetEpochId(m_workerStatusMap[workerId].m_epochId);
        inaH.SetPacketType(5);

        packet->AddHeader(inaH);
        packet->AddHeader(seqTs);


        InetSocketAddress remoteAddress = InetSocketAddress(Ipv4Address::ConvertFrom(m_NodeId2Addr[switchId]), 6666);
        m_socket->SendTo(packet, 0, remoteAddress);
        
    }


}



void 
InaAggregator::SendtoPSTor(uint32_t jobId, uint32_t packetType, uint32_t seqId, uint32_t workernum, uint32_t psId, uint32_t port){

    NS_LOG_FUNCTION(this);
    

    uint32_t switchId = GetNode()->GetId();

    SeqTsHeader seqTs;
    InaHeader inaH;
    Ptr<Packet> packet = Create<Packet>(m_size - seqTs.GetSerializedSize() - inaH.GetSerializedSize());
    
    seqTs.SetSeq(seqId);
    inaH.SetWorkerId(switchId);
    inaH.SetParam(0);
    inaH.SetJobId(jobId);
    inaH.SetEpochId(0);
    inaH.SetPacketType(packetType);
    inaH.SetWorkerNum(workernum);

    packet->AddHeader(inaH);
    packet->AddHeader(seqTs);


    InetSocketAddress remoteAddress = InetSocketAddress(Ipv4Address::ConvertFrom(m_NodeId2Addr[m_fatherList[psId]]), port);

    

    m_socket->SendTo(packet, 0, remoteAddress);
    

    
}

void
InaAggregator::FinishAggregation(uint32_t aggAluId,
                                 uint32_t jobId,
                                 uint32_t aggregateSum,
                                 uint32_t ackSeq)
{
    NS_LOG_FUNCTION(this);

    // reset states
    m_aggAlu[aggAluId].m_state = InaAggALU::IDLE;
    m_aggAlu[aggAluId].m_value = 0;
    m_aggAlu[aggAluId].m_serveJobId = -1;
    m_aggAlu[aggAluId].m_serveSeqId = -1;
    m_aggAlu[aggAluId].m_counter = 0;
    m_aggAlu[aggAluId].m_ECN = 0;


    m_jobStatusMap[jobId].m_state = InaJobStatus::IDLE;
    

    // send back aggregation result to workers
    


}



} // namespace ns3