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

#include "ns3/ina-switch-channel.h"

#include "ns3/ina-switch-net-device.h"

#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/trace-source-accessor.h"


namespace ns3{

NS_LOG_COMPONENT_DEFINE("InaSwitchChannel");

NS_OBJECT_ENSURE_REGISTERED(InaSwitchChannel);

TypeId
InaSwitchChannel::GetTypeId(){
	static TypeId tid = TypeId("ns3::InaSwitchChannel")
		.SetParent<Channel>()
		.SetGroupName("Ina")
		.AddConstructor<InaSwitchChannel>()
		.AddAttribute("Delay",
					  "Propagation delay through the channel",
					  TimeValue(Seconds(0)),
					  MakeTimeAccessor(&InaSwitchChannel::m_delay),
					  MakeTimeChecker())
		;
  return tid;
}

InaSwitchChannel::InaSwitchChannel()
	: 	Channel(),
        m_delay(0.0),
	  	m_nDevices(0)
{
  	NS_LOG_FUNCTION(this);
}

InaSwitchChannel::~InaSwitchChannel(){
  	NS_LOG_FUNCTION(this);
}

void
InaSwitchChannel::Attach(Ptr<InaSwitchNetDevice> device){
	NS_LOG_FUNCTION(this << device);
    NS_ASSERT_MSG(m_nDevices < N_DEVICES, "Only two devices permitted");
    NS_ASSERT(device);

	m_link[m_nDevices++].m_src = device;
	//
    // If we have both devices connected to the channel, then finish introducing
    // the two halves and set the links to IDLE.
    //
    if (m_nDevices == N_DEVICES)
    {
        m_link[0].m_dst = m_link[1].m_src;
        m_link[1].m_dst = m_link[0].m_src;
        m_link[0].m_state = IDLE;
        m_link[1].m_state = IDLE;
    }
}

bool
InaSwitchChannel::TransmitStart(Ptr<const Packet> p, Ptr<InaSwitchNetDevice> src, Time txTime){
	NS_LOG_FUNCTION(this << p << src);
    NS_LOG_LOGIC("UID is " << p->GetUid() << ")");

    NS_ASSERT(m_link[0].m_state != INITIALIZING);
    NS_ASSERT(m_link[1].m_state != INITIALIZING);

    uint32_t wire = src == m_link[0].m_src ? 0 : 1;

    Simulator::ScheduleWithContext(m_link[wire].m_dst->GetNode()->GetId(),
                                   txTime + m_delay,
                                   &InaSwitchNetDevice::Receive,
                                   m_link[wire].m_dst,
                                   p->Copy());

    // Call the tx anim callback on the net device
    // m_txrxPointToPoint(p, src, m_link[wire].m_dst, txTime, txTime + m_delay);
    return true;
}

Ptr<InaSwitchNetDevice>
InaSwitchChannel::GetInaSwitchNetDevice(std::size_t i) const{
	NS_LOG_FUNCTION_NOARGS();
  	NS_ASSERT(i < 2);
  	return m_link[i].m_src;
}

std::size_t
InaSwitchChannel::GetNDevices() const{
  	return m_nDevices;
}

Ptr<NetDevice>
InaSwitchChannel::GetDevice(std::size_t i) const{
	NS_LOG_FUNCTION_NOARGS();
  	return GetInaSwitchNetDevice(i);
}

Time
InaSwitchChannel::GetDelay() const{
    return m_delay;
}

Ptr<InaSwitchNetDevice>
InaSwitchChannel::GetSource(uint32_t i) const{
  	return m_link[i].m_src;
}

Ptr<InaSwitchNetDevice>
InaSwitchChannel::GetDestination(uint32_t i) const{
  	return m_link[i].m_dst;
}

bool
InaSwitchChannel::IsInitialized() const{
    NS_ASSERT(m_link[0].m_state != INITIALIZING);
    NS_ASSERT(m_link[1].m_state != INITIALIZING);
  	return true;
}

} // namespace ns3
