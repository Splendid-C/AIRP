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

#include "ina-ps.h"
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


#include "ina-ps.h"

map<int, bool> workerBitmap2;
map<uint32_t, vector<ns3::Address>> switchAddress;
map<uint32_t, map<uint32_t, uint32_t>> JobSeqValue;// job, seq , value
map<uint32_t, map<uint32_t, map<uint32_t, uint32_t>>> JobSeqCount;// job, seq , epoch, count 
namespace ns3
{

NS_LOG_COMPONENT_DEFINE("InaPS");

NS_OBJECT_ENSURE_REGISTERED(InaPS);


TypeId
InaPS::GetTypeId()
{
    static TypeId tid = TypeId("ns3::InaPS")
                            .SetParent<Application>()
                            // .SetGroupName("Ina")
                            .AddConstructor<InaPS>()
                            .AddAttribute("Port",
                                          "Port on which we listen for incoming packets.",
                                          UintegerValue(10010),
                                          MakeUintegerAccessor(&InaPS::m_port),
                                          MakeUintegerChecker<uint16_t>())
                            .AddAttribute("AggAluNum",
                                          "The number of aggregation ALUs",
                                          UintegerValue(2560000),
                                          MakeUintegerAccessor(&InaPS::m_aggAluNum),
                                          MakeUintegerChecker<uint32_t>())
                            .AddAttribute("AggDelay",
                                          "The delay of aggregation",
                                          TimeValue(Seconds(0.001)),
                                          MakeTimeAccessor(&InaPS::m_aggDelay),
                                          MakeTimeChecker())
                            .AddAttribute("Size",
                                          "Size of each packet",
                                          UintegerValue(256),
                                          MakeUintegerAccessor(&InaPS::m_size),
                                          MakeUintegerChecker<uint32_t>())
                            .AddAttribute("TotalWorkerNum",
                                          "The total number of workers",
                                          UintegerValue(10),
                                          MakeUintegerAccessor(&InaPS::m_totalWorkerNum),
                                          MakeUintegerChecker<uint32_t>())
                             
    ;
    return tid;
}


InaPS::InaPS()
{
    NS_LOG_FUNCTION(this);
    m_aggAluNum = 10;
    m_totalWorkerNum = 10;
}

InaPS::~InaPS()
{
    NS_LOG_FUNCTION(this);
}

void
InaPS::Initialize()
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
InaPS::AssignTotalSwitchNum(uint32_t switchNum)
{
    NS_LOG_FUNCTION(this);
    m_totalWorkerNum = switchNum;
}

void
InaPS::AddSwitch(Ptr<Node> switchNode)
{
    NS_LOG_FUNCTION(this);
}

void
InaPS::AddJob(uint32_t jobId, uint32_t switchNum)
{
    NS_LOG_FUNCTION(this);

    // here we just know each job should serve how many workers
    InaJobStatus newJobStatus;
    newJobStatus.m_jobId = jobId;
    newJobStatus.m_workerNum = switchNum;

    m_jobStatusMap[jobId] = newJobStatus;
    return;
}


void
InaPS::AddPendingJobList(vector<JobList> pendingJobList)
{
    NS_LOG_FUNCTION(this);
    m_pendingJobList = pendingJobList;

}


void
InaPS::DoDispose()
{
    NS_LOG_FUNCTION(this);
    Application::DoDispose();
}


void
InaPS::StartApplication()
{
    NS_LOG_FUNCTION(this);
    // Create socket if not already

    uint32_t psId = GetNode()->GetId();
    cout <<"PS "<< psId<< " start app"<<endl;
    if (!m_socket)
    {
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_socket = Socket::CreateSocket(GetNode(), tid);
        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), 8888);
        if (m_socket->Bind(local) == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }
    }

    m_socket->SetRecvCallback(MakeCallback(&InaPS::HandleRead, this));

    if (!m_socket6)
    {
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_socket6 = Socket::CreateSocket(GetNode(), tid);
        Inet6SocketAddress local6 = Inet6SocketAddress(Ipv6Address::GetAny(), 8888);
        if (m_socket6->Bind(local6) == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }
    }

    m_socket6->SetRecvCallback(MakeCallback(&InaPS::HandleRead, this));
}


void
InaPS::StopApplication()
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




//send back to switch
void
InaPS::Send(Address m_switchAddr, Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this);


    m_socket->SendTo(packet, 0, m_switchAddr);
}




void 
InaPS::SetupSocket(){
	NS_LOG_FUNCTION (this);

	// set up socket
	if(!m_socket){
		TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
		m_socket = Socket::CreateSocket(GetNode(), tid);
		if(Ipv4Address::IsMatchingType(m_psAddress) == true){
			if(m_socket->Bind() == -1){
				NS_FATAL_ERROR("Failed to bind socket");
			}
			m_socket->Connect(InetSocketAddress(Ipv4Address::ConvertFrom(m_psAddress), 8888));
		}

	} else if(Ipv6Address::IsMatchingType(m_psAddress) == true){
		if(m_socket->Bind6() == -1){
			NS_FATAL_ERROR("Failed to bind socket");
		}
		m_socket->Connect(Inet6SocketAddress(Ipv6Address::ConvertFrom(m_psAddress), 8888));

	} else if(InetSocketAddress::IsMatchingType(m_psAddress) == true){
		if(m_socket->Bind() == -1){
			NS_FATAL_ERROR("Failed to bind socket");
		}
		m_socket->Connect(m_psAddress);

	} else if(Inet6SocketAddress::IsMatchingType(m_psAddress) == true){
		if(m_socket->Bind6() == -1){
			NS_FATAL_ERROR("Failed to bind socket");
		}
		m_socket->Connect(m_psAddress);

	} else {
		NS_ASSERT_MSG(false, "Incompatible address type: " << m_psAddress);
	}

	m_socket->SetRecvCallback(MakeCallback(&InaPS::HandleRead, this));
	m_socket->SetAllowBroadcast(false);
	// m_sendEvent = Simulator::Schedule(Seconds(0.0), &InaPS::Send, this);
	// m_initializeSocket = true;
}


/// @brief
// handle the Packet received, and store in
// m_workerStatusMap[workerId].m_pendingQueueMap[currentSequenceNum], next step is to decide whether
// the agg is ready
/// @param socket
void
InaPS::HandleRead(Ptr<Socket> socket)
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
            uint16_t packetType = inaHeader.GetPacketType();
            uint64_t param = inaHeader.GetParam();
            uint32_t workernum = inaHeader.GetWorkerNum();

            
            // use for recovery in the futrue
            // now, we assume in a lossless network
            uint32_t currentSequenceNum = seqTsHeader.GetSeq();
             
            // if(currentSequenceNum % 100000 == 0){
            NS_LOG_INFO("PS Node "
                        << GetNode()->GetId() << " Received " << receivedSize << " bytes from "
                        << InetSocketAddress::ConvertFrom(from).GetIpv4()
                         
                        << " Sequence Number: " << currentSequenceNum
                        << " Time: " << (Simulator::Now()).GetSeconds() << " SwitchID: " << workerId
                        << " epochID: " << epochId << " jobID: " << jobId << " Param: " << param
                        << " packetType: " << packetType << " WorkerNum: " << workernum);
            // }
            if (m_workerStatusMap.find(workerId) == m_workerStatusMap.end())
            {
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

                m_workerStatusMap.insert(
                    pair<uint32_t, InaWorkerStatus>(workerId, newWorkerStatus));

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
            


            if(packetType == 3){
                //receive packets from ps-tor
                switchAddress[jobId].push_back(from);
            }

            


            PacketArrivalStage(originalPacket);
            
        }
    }
}


void
InaPS::PacketArrivalStage(Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this);

    SeqTsHeader seqTsHeader;
    InaHeader inaHeader;

    packet->RemoveHeader(seqTsHeader);
    packet->RemoveHeader(inaHeader);

    uint32_t seqId = seqTsHeader.GetSeq();
    uint32_t jobId = inaHeader.GetJobId();
    uint64_t param = inaHeader.GetParam();
    uint32_t epochId = inaHeader.GetEpochId();
    uint32_t workernum = inaHeader.GetWorkerNum();
    uint32_t workerid = inaHeader.GetWorkerId();

     
     
    JobSeqValue[jobId][seqId] += 1;
    JobSeqCount[jobId][seqId][epochId] += workernum;

    if (JobSeqCount[jobId][seqId][epochId] == m_totalWorkerNum)
        {
            
                
            Simulator::Schedule(Seconds(0.002)*JobSeqValue[jobId][seqId], &InaPS::SendToSwitch, this ,jobId, JobSeqValue[jobId][seqId], seqId);
 
            JobSeqValue[jobId][seqId] = 0;
             
           
        
        }


}



bool
InaPS::CheckJobDataReady(uint32_t JobId)
{
    NS_LOG_FUNCTION(this);
    // if all workers under this job have been received, return true.
    // else return false

    uint32_t expectedSeq = m_jobStatusMap[JobId].m_aggSeq;
    for (auto& kv : m_workerStatusMap)
    {
        if (kv.second.m_jobId == JobId && kv.second.m_recvDataFlag == true)
        {
            // if this worker is under this job and has data
            // check if the data is the correct sequence

            if (kv.second.m_pendingQueueMap.find(expectedSeq) == kv.second.m_pendingQueueMap.end())
            {
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
            if (m_jobStatusMap[JobId].m_readyPacketList[expectedSeq].find(workId) ==
                m_jobStatusMap[JobId].m_readyPacketList[expectedSeq].end())
            {
                m_jobStatusMap[JobId].m_readyPacketList[expectedSeq][workId] = p;
            }

            // update sequence number
            ++kv.second.m_expectedSeq;
        }

        if (kv.second.m_jobId == JobId && kv.second.m_recvDataFlag == false)
        {
            // if one worker is not ready, then the job is not ready
            break;
        }
    }

    if (m_jobStatusMap[JobId].m_readyPacketList[expectedSeq].size() ==
        m_jobStatusMap[JobId].m_workerNum)
    {
        // if all workers under this job have been received, return true.
        // else return false
        m_jobStatusMap[JobId].m_state = InaJobStatus::READY;
        return true;
    }

    return false;


}


 


void
InaPS::SendToSwitch(uint32_t jobId, uint32_t aggregateSum, uint32_t seq)
{

//send result to ps-tor
    
        uint32_t psId = GetNode()->GetId();
        SeqTsHeader seqTs;
        InaHeader inaH;
        Ptr<Packet> packet =
            Create<Packet>(m_size - seqTs.GetSerializedSize() - inaH.GetSerializedSize());
        seqTs.SetSeq(seq);
        inaH.SetWorkerId(psId);
        inaH.SetParam(aggregateSum);
        inaH.SetJobId(jobId);
        inaH.SetEpochId(0);
        inaH.SetPacketType(4);

        packet->AddHeader(inaH);
        packet->AddHeader(seqTs);

        
    InetSocketAddress remoteAddress = InetSocketAddress(Ipv4Address::ConvertFrom(m_pendingJobList[jobId].psFatherAddr), 6666);
     
    m_socket->SendTo(packet, 0, remoteAddress);

}



 

}

