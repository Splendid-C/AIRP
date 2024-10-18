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
#include "ns3/seq-ts-header.h"
#include "ina-header.h"
#include <unordered_map>
#include <map>

 

#include "ns3/application.h"
 
 
#include "ns3/vector.h"
#include "ns3/type-id.h"
#include "ns3/seq-ts-header.h"
#include "ns3/ina-header.h"
 
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/core-module.h"

map<uint32_t, ns3::Address> workerAddress;
uint32_t throughput = 0;
uint32_t total = 0;
map<uint32_t, map<uint32_t, uint32_t>> workerBitmap; //job, seq, num

namespace ns3{

NS_LOG_COMPONENT_DEFINE("InaAggregator");

NS_OBJECT_ENSURE_REGISTERED(InaAggregator);

TypeId
InaAggregator::GetTypeId(){
    static TypeId tid = 
        TypeId("ns3::InaAggregtor")
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
            UintegerValue(60),
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
    ;
    return tid;
}

InaAggregator::InaAggregator()
{
    NS_LOG_FUNCTION(this);
    m_aggAluNum = 2;
}

InaAggregator::~InaAggregator()
{
    NS_LOG_FUNCTION(this);  
}

void
InaAggregator::Initialize(){
    NS_LOG_FUNCTION(this);

    // initial ALUs
    for(uint32_t i = 0; i < m_aggAluNum + 10; ++i){
        InaAggALU newAlu;
        m_aggAlu.push_back(newAlu);
        m_aggAlu[i].m_state = InaAggALU::IDLE;
        m_aggAlu[i].m_serverJobId = 0;
        m_aggAlu[i].m_addSum = 0;
    }
}

void
InaAggregator::AssignTotalWorkerNum(uint32_t workerNum){
    NS_LOG_FUNCTION(this);
    m_totalWorkerNum = workerNum;
}

void
InaAggregator::AddWorker(Ptr<Node> workerNode){
    NS_LOG_FUNCTION(this);
}

void
InaAggregator::AddJob(uint32_t jobId, uint32_t workerNum, uint32_t head, uint32_t tail, map<uint32_t, ns3::Address> workerAddr, vector<uint32_t>workerIdList){
    NS_LOG_FUNCTION(this);

    // here we just know each job should serve how many workers
    InaJobStatus newJobStatus;
    newJobStatus.m_jobId = jobId;
    newJobStatus.m_workerNum = workerNum;
    newJobStatus.m_head = head;
    newJobStatus.m_tail = tail;
    newJobStatus.m_index = head;
    newJobStatus.workerAddr = workerAddr;
    newJobStatus.m_workerIdList = workerIdList;

    m_jobStatusMap[jobId] = newJobStatus;




    return;
}



void
InaAggregator::GetThroughput(){
    NS_LOG_FUNCTION(this);
    //a function to record throughput

    cout << throughput <<" throughput per 1ms, Total: "<< total << endl;
    throughput = 0;
    if(total >= 15000000){
        return;
    }else{
        Simulator::Schedule(Seconds(0.001), &InaAggregator::GetThroughput, this);

    }

    


}



void
InaAggregator::DoDispose(){
    NS_LOG_FUNCTION(this);
    Application::DoDispose();
}

void
InaAggregator::StartApplication(){
    NS_LOG_FUNCTION(this);


    
    m_switchId = GetNode()->GetId();
    if(m_switchId == 1){
        GetThroughput();
    }
    
    // Create socket if not already
    if(!m_socket){
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_socket = Socket::CreateSocket(GetNode(), tid);
        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_port);
        if(m_socket->Bind(local) == -1){
            NS_FATAL_ERROR("Failed to bind socket");
        }
    }

    m_socket->SetRecvCallback(MakeCallback(&InaAggregator::HandleRead, this));

    if(!m_socket6){
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_socket6 = Socket::CreateSocket(GetNode(), tid);
        Inet6SocketAddress local6 = Inet6SocketAddress(Ipv6Address::GetAny(), m_port);
        if(m_socket6->Bind(local6) == -1){
            NS_FATAL_ERROR("Failed to bind socket");
        }
    }

    m_socket6->SetRecvCallback(MakeCallback(&InaAggregator::HandleRead, this));
}

void
InaAggregator::StopApplication(){
    NS_LOG_FUNCTION(this);  
    // Cancel 
    if(m_socket){
        m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }

    if(m_socket6){
        m_socket6->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }

}

void
InaAggregator::HandleRead(Ptr<Socket> socket){
    NS_LOG_FUNCTION(this << socket);
    Ptr<Packet> packet;

    Address from;
    Address localAddress;

    while((packet = socket->RecvFrom(from))){
        socket->GetSockName(localAddress);
        if(packet->GetSize() > 0){
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
            //uint32_t workerNum = inaHeader.GetWorkerNum();
            // uint16_t packetType = inaHeader.GetPacketType();
            uint64_t param = inaHeader.GetParam();

            
            // use for recovery in the futrue
            // now, we assume in a lossless network
            uint32_t seqId = seqTsHeader.GetSeq();
            workerAddress[workerId] = from;
            

            NS_LOG_INFO("Aggregator " << m_switchId << " Received " << receivedSize << " bytes "
                << " Sequence Number: " << seqId << " Time: " << (Simulator::Now()).GetSeconds() << " WorkID: " << workerId 
                << " epochID: " <<  epochId  << " jobID: " << jobId << " Value: " << param  );
            


            workerBitmap[jobId][seqId] += 1;
             

            CheckIfCanAgg(jobId, seqId);


             
            
        }
    }
}

void
InaAggregator::Send(Address m_workerAddr, Ptr<Packet> packet){
    NS_LOG_FUNCTION(this);

    m_socket->SendTo(packet, 0, m_workerAddr);
}

void
InaAggregator::CheckIfCanAgg(uint32_t jobId, uint32_t seqId){
    NS_LOG_FUNCTION(this);

    // Traverse the worker under each job
    // Check weather their data is ready

    if(m_jobStatusMap[jobId].m_workerNum == workerBitmap[jobId][seqId]){
        int aluId = FindAggALU(jobId);

        if(aluId != -1){
                DoAggregation(aluId, jobId, seqId);
                // cout <<"99999999999"<<endl;
               
            }else{
                // if there is no idle ALU
                // then we need to wait for a while
                cout << "wait for an alu!"<<endl;
                if(m_waitForAggAluEvent.IsExpired())
                    m_waitForAggAluEvent = Simulator::Schedule(m_aggDelay, &InaAggregator::CheckIfCanAgg, this, jobId, seqId);
                return;
            }
        
         
    }else{

        return;
    }

    
}

    
int
InaAggregator::FindAggALU(uint32_t jobId){
    NS_LOG_FUNCTION(this);
    // A PS have multiple ALUs for aggregation
    // Find a idle ALU

    for(uint32_t i = m_jobStatusMap[jobId].m_head; i < m_jobStatusMap[jobId].m_tail; ++i){

        if(m_aggAlu[i].m_state == InaAggALU::IDLE){
            // cout << i <<m_jobStatusMap[jobId].m_tail<<endl;
            return i;
        }
    }
 
    return -1;
}






void 
InaAggregator::DoAggregation(uint32_t aggAluId, uint32_t jobId, uint32_t seqId){
    NS_LOG_FUNCTION(this);


    m_aggAlu[aggAluId].m_state == InaAggALU::BUSY;
   
    

    Simulator::Schedule(Seconds(0.0001)*m_jobStatusMap[jobId].m_workerNum, &InaAggregator::FinishAggregation, this, aggAluId, jobId, 0, seqId);
}

void
InaAggregator::FinishAggregation(uint32_t aggAluId, uint32_t jobId, uint32_t aggregateSum, uint32_t ackSeq){
    NS_LOG_FUNCTION(this);
    // reset states
    m_aggAlu[aggAluId].m_state = InaAggALU::IDLE;
     
 
     
    // send back aggregation result to workers
    for(uint32_t i = 0; i < m_jobStatusMap[jobId].m_workerIdList.size(); ++i){
        uint32_t workerId = m_jobStatusMap[jobId].m_workerIdList[i];
        SeqTsHeader seqTs;
        InaHeader inaH;
        Ptr<Packet> packet = Create<Packet> (m_size - seqTs.GetSerializedSize() - inaH.GetSerializedSize());
        seqTs.SetSeq(ackSeq);
        inaH.SetWorkerId(workerId);
        inaH.SetParam(aggregateSum);
        inaH.SetJobId(jobId);
        inaH.SetEpochId(0);
        inaH.SetPacketType(1);

        
        packet->AddHeader(inaH);
        packet->AddHeader(seqTs);

        throughput ++;
        total ++;
 
        Send(workerAddress[workerId], packet);

        
    }

 
}


}