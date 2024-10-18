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


uint32_t aluIndex = 0;

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
            UintegerValue(100000000),
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
    m_aggAluNum = 4;
}

InaAggregator::~InaAggregator()
{
    NS_LOG_FUNCTION(this);  
}

void
InaAggregator::Initialize(){
    NS_LOG_FUNCTION(this);

    // initial ALUs
    for(uint32_t i = 0; i < m_aggAluNum; ++i){
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
InaAggregator::AddJob(uint32_t jobId, uint32_t workerNum){
    NS_LOG_FUNCTION(this);

    // here we just know each job should serve how many workers
    InaJobStatus newJobStatus;
    newJobStatus.m_jobId = jobId;
    newJobStatus.m_workerNum = workerNum;

    m_jobStatusMap[jobId] = newJobStatus;
    return;
}

void
InaAggregator::DoDispose(){
    NS_LOG_FUNCTION(this);
    Application::DoDispose();
}

void
InaAggregator::StartApplication(){
    NS_LOG_FUNCTION(this);
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
            uint32_t workerNum = inaHeader.GetWorkerNum();
            uint16_t packetType = inaHeader.GetPacketType();
            uint64_t param = inaHeader.GetParam();

            
            // use for recovery in the futrue
            // now, we assume in a lossless network
            uint32_t currentSequenceNum = seqTsHeader.GetSeq();


            NS_LOG_INFO("Aggregator "<< GetNode()->GetId() << " Received " << receivedSize << " bytes from " << InetSocketAddress::ConvertFrom(from).GetIpv4()
                << " Sequence Number: " << currentSequenceNum << " Time: " << (Simulator::Now()).GetSeconds() << " WorkerID: " << workerId 
                << " epochID: " <<  epochId  << " jobID: " << jobId << " Value: " << param << " packetType: " << packetType );
            
            if(m_workerStatusMap.find(workerId) == m_workerStatusMap.end()){
                // new worker
                InaWorkerStatus newWorkerStatus;
                newWorkerStatus.m_workerId = workerId;
                newWorkerStatus.m_epochId = epochId;
                newWorkerStatus.m_jobId = jobId;
                newWorkerStatus.m_workerAddress = from;
                newWorkerStatus.m_recvDataFlag = true;
                newWorkerStatus.m_recvCnt = 1;
                newWorkerStatus.m_expectedSeq = 0;

                newWorkerStatus.m_pendingQueueMap[currentSequenceNum] = originalPacket;

                m_workerStatusMap.insert(pair<uint32_t, InaWorkerStatus>(workerId, newWorkerStatus));

                // joblist add worker id
                m_jobStatusMap[jobId].m_workerIdList.push_back(workerId);
            }else {

                if(m_workerStatusMap[workerId].m_epochId != epochId){
                    // if the epoch id is not the same, then this is a new epoch
                    // reset the worker status
                    m_workerStatusMap[workerId].m_expectedSeq = 0;
                    m_workerStatusMap[workerId].m_epochId = epochId;
                    m_workerStatusMap[workerId].m_recvDataFlag = true;
                    m_workerStatusMap[workerId].m_pendingQueueMap.clear();
                    m_workerStatusMap[workerId].m_pendingQueueMap[currentSequenceNum] = originalPacket;

                    m_jobStatusMap[jobId].m_epochId = epochId;
                    m_jobStatusMap[jobId].m_curSeq = 0;
                    m_jobStatusMap[jobId].m_aggSeq = 0;
                    // m_jobStatusMap[jobId].m_readyPacketList.clear();
                    m_jobStatusMap[jobId].m_readyPacketList.clear();
                    m_jobStatusMap[jobId].m_state = InaJobStatus::IDLE;
                }else{
                    // worker has been recorded. update the status.

                    if(currentSequenceNum >= m_workerStatusMap[workerId].m_expectedSeq){
                        // this is a new packet
                        m_workerStatusMap[workerId].m_recvCnt++;

                        if(m_workerStatusMap[workerId].m_pendingQueueMap.find(currentSequenceNum) == m_workerStatusMap[workerId].m_pendingQueueMap.end()){
                            // make sure not to insert a packet twice
                            m_workerStatusMap[workerId].m_recvDataFlag = true;
                            m_workerStatusMap[workerId].m_pendingQueueMap[currentSequenceNum] = originalPacket;
                        }
                    }
                }
            }


            // NS_LOG_INFO("PS debug : jobseq " << m_jobStatusMap[jobId].m_aggSeq << " workerseq " << m_workerStatusMap[workerId].m_expectedSeq << " " << m_workerStatusMap[workerId].m_pendingQueueMap.size());
            // check if can do aggregation
            CheckIfCanAgg();
        }
    }
}

void
InaAggregator::Send(Address m_workerAddr, Ptr<Packet> packet){
    NS_LOG_FUNCTION(this);

    m_socket->SendTo(packet, 0, m_workerAddr);
}

void
InaAggregator::CheckIfCanAgg(){
    NS_LOG_FUNCTION(this);

    // Traverse the worker under each job
    // Check weather their data is ready
    for(auto &kv : m_jobStatusMap){
        uint32_t JobId = kv.first;
        bool ifJobDataReady = CheckJobDataReady(JobId);
        if(ifJobDataReady){
            // this means data that all workers under this job have been received
            int aggAluId = FindAggALU();
            if(aggAluId != -1){
                DoAggregation(aggAluId, JobId);
            }else{
                // if there is no idle ALU
                // then we need to wait for a while
                if(m_waitForAggAluEvent.IsExpired())
                    m_waitForAggAluEvent = Simulator::Schedule(m_aggDelay, &InaAggregator::CheckIfCanAgg, this);
                return;
            }
        }
    }
    return;
}

bool
InaAggregator::CheckJobDataReady(uint32_t JobId){
    NS_LOG_FUNCTION(this);
    // if all workers under this job have been received, return true.
    // else return false


    uint32_t expectedSeq = m_jobStatusMap[JobId].m_aggSeq;
    for(auto &kv : m_workerStatusMap){
        if(kv.second.m_jobId == JobId && kv.second.m_recvDataFlag == true){
            // if this worker is under this job and has data
            // check if the data is the correct sequence
            
            if(kv.second.m_pendingQueueMap.find(expectedSeq) == kv.second.m_pendingQueueMap.end()){
                // if the expected sequence is not found, then the data is not ready
                // here can use for retransmition in a loss network
                // TODO
                m_jobStatusMap[JobId].m_state = InaJobStatus::WAIT;
                break;
            }
            // if the expected sequence is found, then the data is ready
            Ptr<Packet> p = kv.second.m_pendingQueueMap[expectedSeq]->Copy();
            kv.second.m_pendingQueueMap.erase(expectedSeq);

            // update job status
            uint32_t workId = kv.first;
            if(m_jobStatusMap[JobId].m_readyPacketList[expectedSeq].find(workId) == m_jobStatusMap[JobId].m_readyPacketList[expectedSeq].end()){
                m_jobStatusMap[JobId].m_readyPacketList[expectedSeq][workId] = p;
            }

            // update sequence number
            ++ kv.second.m_expectedSeq;
        }

        if(kv.second.m_jobId == JobId && kv.second.m_recvDataFlag == false){
            // if one worker is not ready, then the job is not ready
            break;
        }
    }


    if(m_jobStatusMap[JobId].m_readyPacketList[expectedSeq].size() == m_jobStatusMap[JobId].m_workerNum){
        // if all workers under this job have been received, return true.
        // else return false
        m_jobStatusMap[JobId].m_state = InaJobStatus::READY;
        return true;
    }

    return false;
}

int
InaAggregator::FindAggALU(){
    NS_LOG_FUNCTION(this);
    // A PS have multiple ALUs for aggregation
    // Find a idle ALU
    for(uint32_t i = aluIndex; i < m_aggAluNum; ++i){
        if(m_aggAlu[i].m_state == InaAggALU::IDLE){
            aluIndex++;
            return i;
        }
    }

    // if all ALUs are busy, return -1
    return -1;
}

void 
InaAggregator::DoAggregation(uint32_t aggAluId, uint32_t jobId){
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(m_aggAlu[aggAluId].m_state == InaAggALU::IDLE, "In aggregation, AggALU must be idle");
    //NS_ASSERT_MSG(m_jobStatusMap[jobId].m_state == InaJobStatus::READY, "In aggregation, Job must be ready");

    // change ALU status
    m_aggAlu[aggAluId].m_state = InaAggALU::BUSY;

    // fetch data
    // uint32_t aggregateSum = 0;
    uint32_t ackseq = m_jobStatusMap[jobId].m_aggSeq;
    // for(auto &kv : m_jobStatusMap[jobId].m_readyPacketList[ackseq]){
    //     Ptr<Packet> p = kv.second->Copy();
        
    //     SeqTsHeader seqTsHeader;
    //     InaHeader inaHeader;
    //     p->RemoveHeader(seqTsHeader);
    //     p->RemoveHeader(inaHeader);
        
    //     // aggregate data
    //     aggregateSum += inaHeader.GetParam();
    // }
    m_jobStatusMap[jobId].m_readyPacketList.erase(ackseq);
    ++ m_jobStatusMap[jobId].m_aggSeq;

    // Simulator::Schedule(Seconds(0.002)*m_jobStatusMap[jobId].m_workerNum, &InaAggregator::FinishAggregation, this, aggAluId, jobId, 0, ackseq);

    Simulator::Schedule(Seconds(0.015), &InaAggregator::FinishAggregation, this, aggAluId, jobId, 0, ackseq);
}

void
InaAggregator::FinishAggregation(uint32_t aggAluId, uint32_t jobId, uint32_t aggregateSum, uint32_t ackSeq){
    NS_LOG_FUNCTION(this);
    // reset states
    m_aggAlu[aggAluId].m_state = InaAggALU::IDLE;
    m_jobStatusMap[jobId].m_state = InaJobStatus::IDLE;

    
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
        inaH.SetEpochId(m_workerStatusMap[workerId].m_epochId);
        inaH.SetPacketType(1);

        
        packet->AddHeader(inaH);
        packet->AddHeader(seqTs);

        Send(m_workerStatusMap[workerId].m_workerAddress, packet);
    }


    if(m_waitForAggAluEvent.IsExpired()){
        CheckIfCanAgg();
    } else{
        m_waitForAggAluEvent.Cancel();
        CheckIfCanAgg();
    }
}


}