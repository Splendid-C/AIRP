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

#include "simple-aggregator-application.h"
#include "ns3/uinteger.h"
#include "ns3/seq-ts-header.h"
#include "ina-header.h"


namespace ns3{

NS_LOG_COMPONENT_DEFINE("SimpleAggregator");

NS_OBJECT_ENSURE_REGISTERED(SimpleAggregator);

TypeId
SimpleAggregator::GetTypeId(){
    static TypeId tid = 
        TypeId("ns3::SimpleAggregator")
            .SetParent<Application>()
            .SetGroupName("Ina")
            .AddConstructor<SimpleAggregator>()
        .AddAttribute("Port",
            "Port on which we listen for incoming packets.",
            UintegerValue(9),
            MakeUintegerAccessor(&SimpleAggregator::m_port),
            MakeUintegerChecker<uint16_t>())
        .AddAttribute("AggDelay", "Time for aggregation.",
            TimeValue(NanoSeconds(2.0)),
            MakeTimeAccessor(&SimpleAggregator::m_aggDelay),
            MakeTimeChecker())
        .AddAttribute("MaxPackets",
            "The maximum number of packets the application will send or recv",
            UintegerValue(100),
            MakeUintegerAccessor(&SimpleAggregator::m_count),
            MakeUintegerChecker<uint32_t>())
        .AddAttribute("PacketSize",
            "Size of packets generated",
            UintegerValue(1460),
            MakeUintegerAccessor(&SimpleAggregator::m_size),
            MakeUintegerChecker<uint32_t>())
        .AddAttribute("AggregateALU", 
            "", 
            UintegerValue(1), 
            MakeUintegerAccessor(&SimpleAggregator::m_aggALUNum), 
            MakeUintegerChecker<uint32_t>())
        .AddAttribute("WorkerNumber", 
            "" , 
            UintegerValue(4), 
            MakeUintegerAccessor(&SimpleAggregator::m_workerNum), 
            MakeUintegerChecker<uint32_t>())
    ;
    return tid;
}

SimpleAggregator::SimpleAggregator(){
    NS_LOG_FUNCTION(this);
}

SimpleAggregator::~SimpleAggregator(){
    NS_LOG_FUNCTION(this);
}

void
SimpleAggregator::DoDispose(){
    NS_LOG_FUNCTION(this);
    Application::DoDispose();
}

void
SimpleAggregator::StartApplication(){
    NS_LOG_FUNCTION(this);
    if(!m_socket){
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_socket = Socket::CreateSocket(GetNode(), tid);
        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_port);
        if(m_socket->Bind(local) == -1){
            NS_FATAL_ERROR("Failed to bind socket");
        }
    }

    m_socket->SetRecvCallback(MakeCallback(&SimpleAggregator::HandleRead, this));

    if(!m_socket6){
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_socket6 = Socket::CreateSocket(GetNode(), tid);
        Inet6SocketAddress local6 = Inet6SocketAddress(Ipv6Address::GetAny(), m_port);
        if(m_socket6->Bind(local6) == -1){
            NS_FATAL_ERROR("Failed to bind socket");
        }
    }

    m_socket6->SetRecvCallback(MakeCallback(&SimpleAggregator::HandleRead, this));
}

void 
SimpleAggregator::StopApplication(){
    NS_LOG_FUNCTION(this);
    if(m_socket){
        m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }

    if(m_socket6){
        m_socket6->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }
}

void
SimpleAggregator::HandleRead(Ptr<Socket> socket){
    NS_LOG_FUNCTION(this << socket);
    Ptr<Packet> packet;

    Address from;
    Address localAddress;
    while((packet = socket->RecvFrom(from))){
        socket->GetSockName(localAddress);
        if(packet->GetSize() > 0){
            Ptr<Packet> originalPacket = packet->Copy();

            // Get packet header
            uint32_t receivedSize = packet->GetSize();
            SeqTsHeader seqTs;
	        InaHeader inaH;
            packet->RemoveHeader(seqTs);
            packet->RemoveHeader(inaH);
            uint32_t workID = inaH.GetWorkerId();
            uint64_t value = inaH.GetParam();
            uint32_t epochID = inaH.GetEpochId();
            uint32_t jobID = inaH.GetJobId();

            // if(m_workerAddressMap.find(workID) == m_workerAddressMap.end()){
            //     m_workerAddressMap[workID] = from;
            // }

            uint32_t currentSequenceNum = seqTs.GetSeq(); 

            NS_LOG_INFO("PS : Received " << receivedSize << " bytes from " << InetSocketAddress::ConvertFrom(from).GetIpv4()
                << " Sequence Number: " << currentSequenceNum << " Time: " << (Simulator::Now()).GetSeconds() << " WorkID: " << workID << " epochID: " <<  epochID  << " jobID: " << jobID << " Value: " << value);
            

            if(m_workerInfoMap.find(workID) == m_workerInfoMap.end()){
                // a new worker
                WorkerStatus newWorkStatus(jobID, epochID, workID, from);
                deque<Ptr<Packet>> newPendingQueue;
                m_workerInfoMap[workID] = newWorkStatus;
                m_pendingQueue[workID] = newPendingQueue;
                m_pendingQueue[workID].push_back(originalPacket);
                m_recvDataFlag[workID] = true;

                // a new job?
                vector<uint32_t>::iterator it;
                it = find(m_jobList.begin(), m_jobList.end(), jobID);
                if(it == m_jobList.end()){
                    // a new job, record it
                    m_jobList.push_back(jobID);
                }

            }else{
                m_pendingQueue[workID].push_back(originalPacket);
                m_recvDataFlag[workID] = true;
            }
            //m_aggALU[workID].m_recvCount++;
            CheckIfRecvAll();
            
            // Ptr<Packet> p = Create<Packet>(100);
            // socket->SendTo(p, 0, from);
            m_received++;
        }   
    }

}

void
SimpleAggregator::CheckIfRecvAll(){
    NS_LOG_FUNCTION(this);
    // when recv a worker's packet, trigger once
    // check if all worker send data here
    // We currently assume that all workers are the same task

    // std::cout << m_jobList.size() << " " << m_workerInfoMap.size() << std::endl;
    // for(uint32_t i = 0; i < m_jobList.size(); i++){
    //     // for each job
    //     bool jobIsReady = true;

    //     for(auto &it: m_workerInfoMap){
    //         // for each worker
    //         if(it.second.m_jobId == m_jobList[i]){
    //             // if this worker is in this job
    //             if(m_recvDataFlag[it.second.m_workerId]){
    //                 // if this worker's data is ready
    //                 continue;
    //             }else{
    //                 // there are worker in this job whose data is not ready
    //                 jobIsReady = false;
    //                 break;
    //             }
    //         }
    //     }

    //     if(jobIsReady){
    //         int aggALUId = FindAggALU();
    //         if(aggALUId != -1)
    //             DoAggregation(aggALUId);
    //     }

    // }


    for(uint32_t i = 0; i < m_workerNum; i++){
        if(!m_recvDataFlag[i]){
            // All data is not ready
            return;
        }
    }
    // If all data is ready, start to aggregate.
    // Also there must have a idle ALU
    int aggALUId = FindAggALU();
    if(aggALUId != -1)
        DoAggregation(aggALUId);
    return;
}

int
SimpleAggregator::FindAggALU(){
    NS_LOG_FUNCTION(this);
    // support a switch have multiple aggregation ALU
    // we should find a ALU which is not busy
    for(uint32_t i = 0; i < m_aggALUNum; i++){
        if(m_aggALU[i].m_state == AggALUState::IDLE){
            // find the first idle ALU
            return i;
        }
    }

    // if all ALU is busy, return -1
    return -1;
}

void
SimpleAggregator::DoAggregation(uint32_t aggALUId){
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(m_aggALU[aggALUId].m_state == AggALUState::IDLE, "AggALU is not idle");
    m_aggALU[aggALUId].m_state = AggALUState::BUSY;

    // fetch data
    uint32_t aggregateSum = 0;
    for(auto kv:m_workerInfoMap){
        
        // search for the correct worker data queue
        uint32_t workId = kv.second.m_workerId;
        Ptr<Packet> packet = m_pendingQueue[workId].front();
        m_pendingQueue[workId].pop_front();

        // Get packet header
        SeqTsHeader seqTs;
        InaHeader inaH;
        packet->RemoveHeader(seqTs);
        packet->RemoveHeader(inaH);
        
        uint64_t value = inaH.GetParam();
        // uint32_t epochID = inaH.GetEpochId();
        // uint32_t jobId = inaH.GetJobId();

        // aggregate
        aggregateSum += value;

        // reset data ready flag
        if(m_pendingQueue[workId].size() == 0){
            m_recvDataFlag[workId] = false;
        }
    }
    Simulator::Schedule(m_aggDelay, &SimpleAggregator::FinishAggregation, this, aggregateSum, aggALUId);
}

void
SimpleAggregator::FinishAggregation(uint32_t aggregateSum, uint32_t aggALUId){
    NS_LOG_FUNCTION(this);

    // reset Aggregator 
    NS_ASSERT_MSG(m_aggALU[aggALUId].m_state == AggALUState::BUSY, "AggALU must be busy");
    m_aggALU[aggALUId].m_state = AggALUState::IDLE;

    // send result
    SendAggregationResult(aggregateSum);
}

void
SimpleAggregator::SendAggregationResult(uint32_t aggregateSum){
    NS_LOG_FUNCTION(this);
    
    // send aggregated result to all workers
    for(auto &it: m_workerInfoMap){
        Send(it.second, aggregateSum);
        it.second.m_seq++;
    }
}

void
SimpleAggregator::Send(WorkerStatus workerStatus, uint32_t aggregateSum){
    NS_LOG_FUNCTION(this << aggregateSum);

    // prepare packet
    SeqTsHeader seqTs;
    InaHeader inaH;
    Ptr<Packet> packet = Create<Packet> (m_size - seqTs.GetSerializedSize() - inaH.GetSerializedSize());
    seqTs.SetSeq(workerStatus.m_seq);
    inaH.SetWorkerId(workerStatus.m_workerId);
    inaH.SetParam(aggregateSum);
    packet->AddHeader(inaH);
    packet->AddHeader(seqTs);

    m_socket->SendTo(packet, 0, workerStatus.m_workerAddr);

}


} // namespace ns3