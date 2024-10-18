/*
 * Copyright (c) 2007, 2008 University of Washington
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
 */

#include "ina-switch-net-device.h"
#include "ina-switch-channel.h"
#include "ina-switch.h"
#include "ns3/ppp-header.h"
#include "ns3/simulator.h"
#include "ns3/pointer.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("InaSwitchNetDevice");

NS_OBJECT_ENSURE_REGISTERED (InaSwitchNetDevice);

TypeId
InaSwitchNetDevice::GetTypeId(){
  static TypeId tid = 
      TypeId("ns3::InaSwitchNetDevice")
          .SetParent<NetDevice>() 
					.SetGroupName("Ina")
					.AddConstructor<InaSwitchNetDevice>()
					.AddAttribute("DataRate",
								  "The default data rate for point to point links",
								  DataRateValue(DataRate("10Gbps")),
								  MakeDataRateAccessor(&InaSwitchNetDevice::m_bps),
								  MakeDataRateChecker())
					.AddAttribute("Mtu",
								  "The MAC-level Maximum Transmission Unit",
								  UintegerValue(1500),
								  MakeUintegerAccessor(&InaSwitchNetDevice::SetMtu,
								  					   &InaSwitchNetDevice::GetMtu),
								  MakeUintegerChecker<uint16_t>())
					.AddAttribute("InterframeGap",
								  "The time to wait between packets",
								  TimeValue(NanoSeconds(0.0)),
								  MakeTimeAccessor(&InaSwitchNetDevice::m_tInterframeGap),
								  MakeTimeChecker())
					.AddAttribute("Address",
								  "The MAC address of this device",
								  Mac48AddressValue(),
								  MakeMac48AddressAccessor(&InaSwitchNetDevice::m_address),
								  MakeMac48AddressChecker())
					.AddAttribute("ReceiveErrorModel",
								  "The receiver error model used to simulate packet loss",
								  PointerValue(),
								  MakePointerAccessor(&InaSwitchNetDevice::m_receiveErrorModel),
								  MakePointerChecker<ErrorModel>())
					.AddAttribute("TxQueue",
								  "A queue to use as the transmit queue in the device.",
								  PointerValue(),
								  MakePointerAccessor(&InaSwitchNetDevice::m_queue),
								  MakePointerChecker<Queue<Packet>>())
			;
	return tid;
}

InaSwitchNetDevice::InaSwitchNetDevice()
	: m_inaSwitch(nullptr),
	  m_channel(nullptr),
	  m_linkUp(false),
	  m_txMachineState(READY),
	  m_currentPkt(nullptr)
{
	NS_LOG_FUNCTION (this);
}

InaSwitchNetDevice::~InaSwitchNetDevice(){
	NS_LOG_FUNCTION (this);
}

void
InaSwitchNetDevice::AddHeader(Ptr<Packet> p, uint16_t protocolNumber){
	NS_LOG_FUNCTION(this << p << protocolNumber);
	PppHeader ppph;
	ppph.SetProtocol(protocolNumber);
	p->AddHeader(ppph);
}

bool
InaSwitchNetDevice::ProcessHeader(Ptr<Packet> p, uint16_t& param){
	NS_LOG_FUNCTION(this << p << param);
	PppHeader ppph;
	p->RemoveHeader(ppph);
	param = ppph.GetProtocol();
	return true;
}

void
InaSwitchNetDevice::DoDispose(){
	NS_LOG_FUNCTION (this);
	m_channel = nullptr;
	m_queue = nullptr;
	m_receiveErrorModel = nullptr;
	m_node = nullptr;
	m_currentPkt = nullptr;
	m_inaSwitch = nullptr;
	NetDevice::DoDispose();
}

void 
InaSwitchNetDevice::Receive(Ptr<Packet> packet){
	NS_LOG_FUNCTION (this << packet);
	//std::cout << "receive packet " << this->GetNode()->GetId() << " " << this->GetIfIndex() << std::endl;
	uint16_t protocol = 0;

	if(m_receiveErrorModel && m_receiveErrorModel->IsCorrupt(packet)){
		NS_LOG_LOGIC("Dropping corrupted packet " << packet);
		return;
	}else{

		Ptr<Packet> originalPacket = packet->Copy();
		ProcessHeader(packet, protocol);

		if(m_inaSwitch != nullptr){
			// switch 
			// if the packet is for me, forward it.
			// if(this->GetAddress())

		}else{
			// host
			m_rxCallback(this, packet, protocol, GetRemote());
		}
	}
}

bool
InaSwitchNetDevice::TransmitStart(Ptr<Packet> p){
	NS_LOG_FUNCTION (this << p);
	NS_LOG_LOGIC ("UID is " << p->GetUid() << " and size is " << p->GetSize());
	NS_ASSERT_MSG(m_txMachineState == READY, "Must be READY to start transmission");
	m_currentPkt = p;
	m_txMachineState = BUSY;

	Time txTime = m_bps.CalculateBytesTxTime(p->GetSize());
	Time txCompleteTime = txTime + m_tInterframeGap;

	NS_LOG_LOGIC("Schedule TransimitCompleteEvent at time " << txCompleteTime.As(Time::S));
	Simulator::Schedule(txCompleteTime, &InaSwitchNetDevice::TransmitComplete, this);

	bool result = m_channel->TransmitStart(p, this, txTime);
	if(result == false){
		// phy link drop
	}
	return result;
}

void
InaSwitchNetDevice::TransmitComplete(){
	NS_LOG_FUNCTION (this);
	NS_ASSERT_MSG(m_txMachineState == BUSY, "Must be BUSY to complete transmission");
	m_txMachineState = READY;
	m_currentPkt = nullptr;

	Ptr<Packet> p = m_queue->Dequeue();
	if(!p){
		NS_LOG_LOGIC("No more packets in queue");
		return;
	}

	TransmitStart(p);
}

Address
InaSwitchNetDevice::GetRemote() const{
	NS_LOG_FUNCTION (this);
	NS_ASSERT(m_channel->GetNDevices() == 2);
	for(uint32_t i = 0; i < m_channel->GetNDevices(); i++){
		Ptr<NetDevice> tmp = m_channel->GetDevice(i);
		if(tmp != this){
			return tmp->GetAddress();
		}
	}

	NS_ASSERT(false);
	return Address();
}

void
InaSwitchNetDevice::SetDateRate(DataRate bps){
	NS_LOG_FUNCTION (this << bps);
	m_bps = bps;
}

void
InaSwitchNetDevice::SetInterframeGap(Time t){
	NS_LOG_FUNCTION (this << t.As(Time::S));
	m_tInterframeGap = t;
}

void
InaSwitchNetDevice::SetReceiveErrorModel(Ptr<ErrorModel> em){
	NS_LOG_FUNCTION (this << em);
	m_receiveErrorModel = em;
}

void
InaSwitchNetDevice::SetQueue(Ptr<Queue<Packet> > q){
	NS_LOG_FUNCTION (this << q);
	m_queue = q;
}

Ptr<Queue<Packet> >
InaSwitchNetDevice::GetQueue() const{
	NS_LOG_FUNCTION (this);
	return m_queue;
}

bool
InaSwitchNetDevice::Attach(Ptr<InaSwitchChannel> ch){
	NS_LOG_FUNCTION (this << ch);
	m_channel = ch;
	m_channel->Attach(this);

	NotifyLinkUp();
	return true;
}

void
InaSwitchNetDevice::NotifyLinkUp(){
	NS_LOG_FUNCTION (this);
	m_linkUp = true;
}

void 
InaSwitchNetDevice::SetIfIndex(const uint32_t index){
	NS_LOG_FUNCTION (this << index);
	m_ifIndex = index;
}

uint32_t InaSwitchNetDevice::GetIfIndex() const{
	NS_LOG_FUNCTION (this);
	return m_ifIndex;
}

Ptr<Channel> 
InaSwitchNetDevice::GetChannel() const {
	NS_LOG_FUNCTION (this);
	return m_channel;
}

void 
InaSwitchNetDevice::SetAddress(Address address){
	NS_LOG_FUNCTION (this << address);
	m_address = Mac48Address::ConvertFrom(address);
}

Address 
InaSwitchNetDevice::GetAddress() const {
	NS_LOG_FUNCTION (this);
	return m_address;
}

bool 
InaSwitchNetDevice::SetMtu(const uint16_t mtu){
	NS_LOG_FUNCTION (this << mtu);
	m_mtu = mtu;
	return true;
}

uint16_t 
InaSwitchNetDevice::GetMtu() const{
	NS_LOG_FUNCTION (this);
	return m_mtu;
}

bool 
InaSwitchNetDevice::IsLinkUp() const{
	NS_LOG_FUNCTION (this);
	return m_linkUp;
}

void 
InaSwitchNetDevice::AddLinkChangeCallback(Callback<void> callback){
	NS_LOG_FUNCTION (this);
}

bool 
InaSwitchNetDevice::IsBroadcast() const{
	NS_LOG_FUNCTION (this);
	return true;
}

Address 
InaSwitchNetDevice::GetBroadcast() const{
	NS_LOG_FUNCTION (this);
	return Mac48Address("ff:ff:ff:ff:ff:ff");
}

bool 
InaSwitchNetDevice::IsMulticast() const{
	NS_LOG_FUNCTION (this);
	return true;
}

Address 
InaSwitchNetDevice::GetMulticast(Ipv4Address multicastGroup) const{
	NS_LOG_FUNCTION (this);
	return Mac48Address("01:00:5e:00:00:00");
}

bool 
InaSwitchNetDevice::IsPointToPoint() const{
	NS_LOG_FUNCTION (this);
	return false;
}

bool	
InaSwitchNetDevice::IsBridge() const{
	NS_LOG_FUNCTION (this);
	return false;
}

bool 
InaSwitchNetDevice::Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber){
	NS_LOG_FUNCTION (this << packet << dest << protocolNumber);
	return false;
}

bool 
InaSwitchNetDevice::SendFrom(Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber){
	NS_LOG_FUNCTION (this << packet << source << dest << protocolNumber);
	return false;
}

Ptr<Node> 
InaSwitchNetDevice::GetNode() const{
	NS_LOG_FUNCTION (this);
	return m_node;
}

void 
InaSwitchNetDevice::SetNode(Ptr<Node> node){
	NS_LOG_FUNCTION (this << node);
	m_node = node;
}

bool
InaSwitchNetDevice::NeedsArp() const{
	NS_LOG_FUNCTION (this);
	return false;
}

void 
InaSwitchNetDevice::SetReceiveCallback(NetDevice::ReceiveCallback cb){
	NS_LOG_FUNCTION (this);
	m_rxCallback = cb;
}

Address 
InaSwitchNetDevice::GetMulticast(Ipv6Address addr) const{
	NS_LOG_FUNCTION (this);
	return Mac48Address("33:33:00:00:00:00");
}

void 
InaSwitchNetDevice::SetPromiscReceiveCallback(PromiscReceiveCallback cb){
	NS_LOG_FUNCTION (this);
}


bool 
InaSwitchNetDevice::SupportsSendFrom() const{
	NS_LOG_FUNCTION (this);
	return false;
}

uint16_t
InaSwitchNetDevice::PppToEther(uint16_t protocol){
	NS_LOG_FUNCTION_NOARGS();
	switch(protocol){
		case 0x0021:
			return 0x0800;		//  IPv4
		case 0x0057:
			return 0x86DD;		//  IPv6
		default:
			NS_ASSERT_MSG(false, "PPP Protocol number not defined!");
	}
	return 0;
}

uint16_t
InaSwitchNetDevice::EtherToPpp(uint16_t protocol){
	NS_LOG_FUNCTION_NOARGS();
	switch(protocol){
		case 0x0800:
			return 0x0021;		//  IPv4
		case 0x86DD:
			return 0x0057;		//  IPv6
		default:
			NS_ASSERT_MSG(false, "PPP Protocol number not defined!");
	}
	return 0;
}

} // namespace ns3