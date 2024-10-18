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


#include "simple-worker-application.h"
#include "ns3/seq-ts-header.h"
#include "ns3/ina-header.h"
#include "ns3/uinteger.h"


namespace ns3 
{

NS_LOG_COMPONENT_DEFINE("SimpleWorker");

NS_OBJECT_ENSURE_REGISTERED(SimpleWorker);

uint32_t SimpleWorker::m_workerNum = 0;

TypeId
SimpleWorker::GetTypeId(){
	static TypeId tid = 
		TypeId("ns3::SimpleWorker")
			.SetParent<Application>()
			.SetGroupName("Ina")
			.AddConstructor<SimpleWorker>()
			.AddAttribute("RemoteAddress",
				"The destination Address of the outbound packets",
				AddressValue(),
				MakeAddressAccessor(&SimpleWorker::m_peerAddress),
				MakeAddressChecker())
			.AddAttribute("RemotePort",
				"The destination port of the outbound packets",
				UintegerValue(0),
				MakeUintegerAccessor(&SimpleWorker::m_peerPort),
				MakeUintegerChecker<uint16_t>())
			.AddAttribute("Port",
				"Port on which we listen for incoming packets.",
				UintegerValue(10000),
				MakeUintegerAccessor(&SimpleWorker::m_port),
				MakeUintegerChecker<uint16_t>())
			.AddAttribute("MaxPackets",
				"The maximum number of packets the application will send",
				UintegerValue(100),
				MakeUintegerAccessor(&SimpleWorker::m_count),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("Interval",
				"The time to wait between packets",
				TimeValue(Seconds(0)),
				MakeTimeAccessor(&SimpleWorker::m_interval),
				MakeTimeChecker())
			.AddAttribute("PacketSize",
				"Size of each packet in outbound packets",
				UintegerValue(1460),
				MakeUintegerAccessor(&SimpleWorker::m_size),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("MaxEpoch",
				"Maximum epoch number",
				UintegerValue(10),
				MakeUintegerAccessor(&SimpleWorker::m_maxEpoch),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("JobId",
				"",
				UintegerValue(1),
				MakeUintegerAccessor(&SimpleWorker::m_jobId),
				MakeUintegerChecker<uint32_t>())
			;
	return tid;
}

SimpleWorker::SimpleWorker(){
	NS_LOG_FUNCTION(this);

	m_state = WorkerState::IDLE;
	m_selfParam = 0;
	m_recv = 0;
	m_sent = 0;
	m_epochId = 0;
	m_totalTx = 0;
	m_socket = nullptr;
	m_sendEvent = EventId();
	m_initializeSocket = false;

	m_workerId = m_workerNum;
	m_workerNum++;
}

SimpleWorker::~SimpleWorker(){
	NS_LOG_FUNCTION(this);
}

void
SimpleWorker::SetRemote(Address ip, uint16_t port){
	NS_LOG_FUNCTION(this << ip << port);
	m_peerAddress = ip;
	m_peerPort = port;
}

void
SimpleWorker::SetRemote(Address addr){
	NS_LOG_FUNCTION(this << addr);
	m_peerAddress = addr;
}

uint32_t
SimpleWorker::GetWorkerId() const{
	NS_LOG_FUNCTION(this);
    return m_workerId;
}

void
SimpleWorker::DoDispose(){
	NS_LOG_FUNCTION(this);
	Application::DoDispose();
}

void
SimpleWorker::StartApplication(){
	NS_LOG_FUNCTION(this);
	
	// start training a model
	DoTraining();
}

void
SimpleWorker::StopApplication(){
	NS_LOG_FUNCTION(this);
	Simulator::Cancel(m_sendEvent);
}

void
SimpleWorker::DoTraining(){
	NS_LOG_FUNCTION(this);
	//NS_ASSERT(m_state == WorkerState::IDLE);
	m_recv = 0;

	// Here can be replaced by the real training process in the future 


	// schedule the finish training event
	m_trainEvent = Simulator::Schedule(m_trainTime, &SimpleWorker::FinishTraining, this);
}

void
SimpleWorker::FinishTraining(){
	NS_LOG_FUNCTION(this);

	// finish training a model

	// schedule the send even
	if(!m_initializeSocket)
		StartSend();
	else{
		m_sendEvent = Simulator::Schedule(Seconds(0.0), &SimpleWorker::Send, this);
	}
}

void
SimpleWorker::StartSend(){
	NS_LOG_FUNCTION(this);

	// set up socket
	if(!m_socket){
		TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
		m_socket = Socket::CreateSocket(GetNode(), tid);
		if(Ipv4Address::IsMatchingType(m_peerAddress) == true){
			if(m_socket->Bind() == -1){
				NS_FATAL_ERROR("Failed to bind socket");
			}
			m_socket->Connect(InetSocketAddress(Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
		}

	} else if(Ipv6Address::IsMatchingType(m_peerAddress) == true){
		if(m_socket->Bind6() == -1){
			NS_FATAL_ERROR("Failed to bind socket");
		}
		m_socket->Connect(Inet6SocketAddress(Ipv6Address::ConvertFrom(m_peerAddress), m_peerPort));

	} else if(InetSocketAddress::IsMatchingType(m_peerAddress) == true){
		if(m_socket->Bind() == -1){
			NS_FATAL_ERROR("Failed to bind socket");
		}
		m_socket->Connect(m_peerAddress);

	} else if(Inet6SocketAddress::IsMatchingType(m_peerAddress) == true){
		if(m_socket->Bind6() == -1){
			NS_FATAL_ERROR("Failed to bind socket");
		}
		m_socket->Connect(m_peerAddress);

	} else {
		NS_ASSERT_MSG(false, "Incompatible address type: " << m_peerAddress);
	}

	m_socket->SetRecvCallback(MakeCallback(&SimpleWorker::HandleRead, this));
	m_socket->SetAllowBroadcast(true);
	m_sendEvent = Simulator::Schedule(Seconds(0.0), &SimpleWorker::Send, this);
	m_initializeSocket = true;
}

void
SimpleWorker::Send(){
	NS_LOG_FUNCTION(this);
	NS_ASSERT(m_sendEvent.IsExpired());
	
	SeqTsHeader seqTs;
	seqTs.SetSeq(m_sent);

	InaHeader inaH;
	inaH.SetWorkerId(m_workerId);
	inaH.SetParam(m_selfParam);
	inaH.SetEpochId(m_epochId);
	inaH.SetJobId(m_jobId);
	m_selfParam ++;
	Ptr<Packet> p = Create<Packet>(m_size - seqTs.GetSerializedSize() - inaH.GetSerializedSize());

	p->AddHeader(inaH);
	p->AddHeader(seqTs);

	if((m_socket->Send(p)) >= 0){
		m_totalTx += p->GetSize();
		m_sent++;
		NS_LOG_INFO("Worker : Send " << p->GetSize() << " bytes to " << m_peerAddress << " Total Tx " << m_totalTx << " bytes");
	}else{
		NS_LOG_INFO("Error while sending " << p->GetSize() << " bytes to " << m_peerAddress);
	}

	if(m_sent < m_count || m_count == 0){
		m_sendEvent = Simulator::Schedule(m_interval, &SimpleWorker::Send, this);
	}else{
		FinishSend();
	}
}

void 
SimpleWorker::FinishSend(){
	NS_LOG_FUNCTION(this);
	//NS_ASSERT(m_state == WorkerState::SENDING);

	// waiting for receive the aggregated result
	m_sent = 0;

	m_epochId++;
}

void 
SimpleWorker::HandleRead(Ptr<Socket> socket){
	NS_LOG_FUNCTION(this);
	//NS_ASSERT(m_state == WorkerState::RECV);
	Ptr<Packet> packet;
	Address from;
	Address localAddress;

	while ((packet = socket->RecvFrom(from))){
		socket->GetSockName(localAddress);
		if(packet->GetSize() > 0){
			uint32_t receivedSize = packet->GetSize();
            SeqTsHeader seqTs;
	        InaHeader inaH;
            packet->RemoveHeader(seqTs);
            packet->RemoveHeader(inaH);
            uint32_t workID = inaH.GetWorkerId();
            uint64_t value = inaH.GetParam();

			NS_ASSERT(workID == m_workerId);

            uint32_t currentSequenceNum = seqTs.GetSeq(); 

            NS_LOG_INFO("Worker : Received " << receivedSize << " bytes from " << InetSocketAddress::ConvertFrom(from).GetIpv4()
                << " Sequence Number: " << currentSequenceNum << " Time: " << (Simulator::Now()).GetSeconds() << " WorkID: " << workID << " Value: " << value);

			m_recv++;
		}

		if(m_recv == m_count){
			// finish receiving the aggregated result
			// the number of received packets is equal to the number of sent packets
			m_state = WorkerState::IDLE;
			DoTraining();
			return;
		}
	}
	
}

}