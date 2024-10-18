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

#include "ns3/application.h"
#include "ns3/socket.h"
#include "ina-worker-application.h"
#include "ns3/uinteger.h"
#include "ns3/type-id.h"
#include "ns3/seq-ts-header.h"
#include "ns3/ina-header.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("InaWorker");

NS_OBJECT_ENSURE_REGISTERED (InaWorker);

uint32_t InaWorker::m_workerNum = 0;

TypeId
InaWorker::GetTypeId (){
  	static TypeId tid = 
		TypeId ("ns3::InaWorker")
			.SetParent<Application> ()
			.AddConstructor<InaWorker> ()
			.AddAttribute ("TrainTime", "The needed Time of training a model.",
						TimeValue (Seconds (0)),
						MakeTimeAccessor (&InaWorker::m_trainTime),
						MakeTimeChecker ())
			.AddAttribute ("Size", "Size of each packet",
						UintegerValue (256),
						MakeUintegerAccessor (&InaWorker::m_size),
						MakeUintegerChecker<uint32_t> ())
			.AddAttribute ("Count", "Number of packets need to send after training",
						UintegerValue (10),
						MakeUintegerAccessor (&InaWorker::m_count),
						MakeUintegerChecker<uint32_t> ())
			.AddAttribute ("RemoteAddress", "The Address of the destination",
						AddressValue (),
						MakeAddressAccessor (&InaWorker::m_peerAddress),
						MakeAddressChecker ())
			.AddAttribute ("RemotePort", "The port of the destination",
						UintegerValue (10000),
						MakeUintegerAccessor (&InaWorker::m_peerPort),
						MakeUintegerChecker<uint16_t> ())
			.AddAttribute ("Interval", "Interval between two packets",
						TimeValue (Seconds(0.000000000001)),
						MakeTimeAccessor (&InaWorker::m_interval),
						MakeTimeChecker ())
			.AddAttribute ("Port", "Port on which we listen for incoming packets.",
						UintegerValue (6666),
						MakeUintegerAccessor (&InaWorker::m_port),
						MakeUintegerChecker<uint16_t> ())
			.AddAttribute("MaxEpoch", "The maximum number of epochs the application will train",
						UintegerValue(1),
						MakeUintegerAccessor(&InaWorker::m_maxEpoch),
						MakeUintegerChecker<uint32_t>())
			.AddAttribute("JobId", "The job id of the application",
						UintegerValue(0),
						MakeUintegerAccessor(&InaWorker::m_jobId),
						MakeUintegerChecker<uint32_t>())
			.AddAttribute("PSId", "The ps id of the application",
						UintegerValue(0),
						MakeUintegerAccessor(&InaWorker::m_psId),
						MakeUintegerChecker<uint32_t>())
			;
  return tid;
}

InaWorker::InaWorker (){
    NS_LOG_FUNCTION (this);
	
	m_workerId = 0;
	// ++m_workerNum;

	m_trainParam = 0;
	m_seq = 0;
	m_unackSeq = 0;
	m_epochId = 0;
	m_jobId = 0;
	m_maxEpoch = 0;
	m_initializeSocket = false;

}

InaWorker::~InaWorker (){
	NS_LOG_FUNCTION (this);
}

void
InaWorker::DoDispose (){
	NS_LOG_FUNCTION (this);
	m_socket = 0;
	Application::DoDispose ();
}

void
InaWorker::StartApplication(){
	NS_LOG_FUNCTION (this);

	m_workerId = GetNode()->GetId();

	// Start trainning a model
	DoTrainning();
}

void 
InaWorker::SetRemote (Address ip, uint16_t port){
	NS_LOG_FUNCTION(this << ip << port);
	m_peerAddress = ip;
	m_peerPort = port;
}

void 
InaWorker::SetRemote (Address addr){
	NS_LOG_FUNCTION(this << addr);
	m_peerAddress = addr;
}

void
InaWorker::StopApplication(){
	NS_LOG_FUNCTION (this);
}

void
InaWorker::StartNewEpoch(){
	NS_LOG_FUNCTION (this);

	// Update related information
	m_unackSeq = 0;
	m_seq = 0;
	m_epochId++;

	// Start trainning a model
	if(m_epochId < m_maxEpoch)
		DoTrainning();
}

void
InaWorker::DoTrainning(){
	NS_LOG_FUNCTION (this);

	// Here can be replaced by the real training process in the future 

	// schedule the finish training event
	m_trainEvent = Simulator::Schedule(m_trainTime, &InaWorker::FinishTraining, this);
}

void 
InaWorker::FinishTraining(){
	NS_LOG_FUNCTION (this);

	// Update related information


	// Start to send packets to the PS
	if(!m_initializeSocket){
		SetupSocket();
	}else{
		m_sendEvent = Simulator::Schedule(Seconds(0.0), &InaWorker::Send, this);
	}
}

void 
InaWorker::SetupSocket(){
	NS_LOG_FUNCTION (this);

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

	m_socket->SetRecvCallback(MakeCallback(&InaWorker::HandleRead, this));
	m_socket->SetAllowBroadcast(false);
	m_sendEvent = Simulator::Schedule(Seconds(0.0), &InaWorker::Send, this);
	m_initializeSocket = true;
}

void 
InaWorker::Send(){
	NS_LOG_FUNCTION(this);
	NS_ASSERT(m_sendEvent.IsExpired());

	Ptr<Packet> p = GeneratePacket(m_seq, m_trainParam);
	if((m_socket->Send(p)) >= 0){
		m_totalTx += p->GetSize();
		++m_seq;
		// ++m_trainParam;
		if(m_seq%m_count == 0){
		NS_LOG_INFO("Worker " << m_workerId << " send packet " << m_seq << " to PS " << m_psId << " port " << m_peerPort);
		}
	}else{
		NS_LOG_INFO("Worker " << m_workerId << " send packet " << m_seq << " to address " << m_peerAddress << " port " << m_peerPort << " failed");
	}

	if(m_seq < m_count || m_count == 0){
		m_sendEvent = Simulator::Schedule(m_interval, &InaWorker::Send, this);
	}else{
		FinishSend();
	}

}

void
InaWorker::FinishSend(){
	NS_LOG_FUNCTION(this);
}

void 
InaWorker::HandleRead(Ptr<Socket> socket){
	NS_LOG_FUNCTION(this);

	Ptr<Packet> packet;
	Address from;
	Address localAddress;

	while((packet = socket->RecvFrom(from))){
		socket->GetSockName(localAddress);

		if(packet->GetSize() > 0){
			uint32_t recvSize = packet->GetSize();
			m_totalRx += recvSize;

			SeqTsHeader seqTs;
			InaHeader inaH;
			packet->RemoveHeader(seqTs);
			packet->RemoveHeader(inaH);

			uint32_t seq = seqTs.GetSeq();
			uint32_t param = inaH.GetParam();
			uint32_t epochId = inaH.GetEpochId();
			uint32_t jobId = inaH.GetJobId();
			uint32_t workerId = inaH.GetWorkerId();

			NS_ASSERT((jobId == m_jobId) && (epochId == m_epochId) && (workerId == m_workerId));
			if(seq == m_count - 1){
			NS_LOG_INFO("Worker " << m_workerId << " receive packet " << seq << " epoch " << epochId 
								  << " Time:" << Simulator::Now().GetSeconds() << " Sequence:" << seq << " Param:" << param );
			}
			CheckAndUpdate(seq);
		}

		if(m_unackSeq >= m_count){
			// Finish communication of this epoch
			StartNewEpoch();
			return;
		}
	} 
}

void 
InaWorker::CheckAndUpdate(uint32_t sequence){
	NS_LOG_FUNCTION(this);
	if(sequence == m_unackSeq){
		++m_unackSeq;
	}else{
		// packet loss 
		 

	}
}

Ptr<Packet> 
InaWorker::GeneratePacket(uint32_t sequence, uint32_t param){
	NS_LOG_FUNCTION(this);	
	// prepare header
	SeqTsHeader seqTs;
	seqTs.SetSeq(sequence);
	InaHeader inaH;
	inaH.SetWorkerId(m_workerId);
	inaH.SetParam(param);
	inaH.SetEpochId(m_epochId);
	inaH.SetJobId(m_jobId);
	// 0 : normal packet
	inaH.SetPacketType(0);

	Ptr<Packet> p = Create<Packet>(m_size - seqTs.GetSerializedSize() - inaH.GetSerializedSize());

	p->AddHeader(inaH);
	p->AddHeader(seqTs);
	return p;
}

} // namespace ns3
