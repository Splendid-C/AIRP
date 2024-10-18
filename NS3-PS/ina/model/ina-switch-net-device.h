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


#ifndef INA_SWITCH_NET_DEVICE_H
#define INA_SWITCH_NET_DEVICE_H

#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/mac48-address.h"
#include "ns3/ppp-header.h"
#include "ns3/data-rate.h"
#include "ns3/queue.h"
#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/error-model.h"
#include "ns3/nstime.h"
#include "ns3/ina-switch.h"
#include "ns3/address.h"

namespace ns3 {

class InaSwitchChannel;
class ErrorModel;

class InaSwitchNetDevice : public NetDevice
{	
  public:

    /**
     * \brief Get the TypeId
     *
     * \return The TypeId for this class
     */
    static TypeId GetTypeId();

    InaSwitchNetDevice();
    ~InaSwitchNetDevice() override;

    // Delete copy constructor and assignment operator to avoid misuse
    InaSwitchNetDevice& operator=(const InaSwitchNetDevice&) = delete;
    InaSwitchNetDevice(const InaSwitchNetDevice&) = delete;


    void SetDateRate(DataRate bps);
    void SetInterframeGap(Time tInterframeGap);
    void SetReceiveErrorModel(Ptr<ErrorModel> em);
    void SetQueue(Ptr<Queue<Packet> > q);
    Ptr<Queue<Packet> > GetQueue() const;
    bool Attach(Ptr<InaSwitchChannel> ch);

    void SendToSwitch(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);
    void Receive(Ptr<Packet> p);
    

    // inherited from NetDevice base class.
    void SetIfIndex(const uint32_t index) override;
    uint32_t GetIfIndex() const override;
    Ptr<Channel> GetChannel() const override;
    void SetAddress(Address address) override;
    Address GetAddress() const override;
    bool SetMtu(const uint16_t mtu) override;
    uint16_t GetMtu() const override;
    bool IsLinkUp() const override;
    void AddLinkChangeCallback(Callback<void> callback) override;
    bool IsBroadcast() const override;
    Address GetBroadcast() const override;
    bool IsMulticast() const override;
    Address GetMulticast(Ipv4Address multicastGroup) const override;
    bool IsPointToPoint() const override;
    bool IsBridge() const override;
    bool Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber) override;
    bool SendFrom(Ptr<Packet> packet,
                  const Address& source,
                  const Address& dest,
                  uint16_t protocolNumber) override;
    Ptr<Node> GetNode() const override;
    void SetNode(Ptr<Node> node) override;
    bool NeedsArp() const override;
    void SetReceiveCallback(NetDevice::ReceiveCallback cb) override;

    Address GetMulticast(Ipv6Address addr) const override;

    void SetPromiscReceiveCallback(PromiscReceiveCallback cb) override;
    bool SupportsSendFrom() const override;

  protected:


  private:
    void DoDispose() override;
    Address GetRemote() const;
    void AddHeader(Ptr<Packet> p, uint16_t protocolNumber);
    bool ProcessHeader(Ptr<Packet> p, uint16_t& param);

    bool TransmitStart(Ptr<Packet> p);
    void TransmitComplete(void);
    void NotifyLinkUp(void);



    enum TxMachineState
    {
      READY, /**< The transmitter is ready to begin transmission of a packet */
      BUSY   /**< The transmitter is busy transmitting a packet */
    };

    Time                  m_tInterframeGap;
    Ptr<ErrorModel>       m_receiveErrorModel;
    Ptr<Queue<Packet> >   m_queue;
    Ptr<Node>             m_node;
    Ptr<InaSwitch>        m_inaSwitch;
    Ptr<InaSwitchChannel> m_channel;
    Mac48Address          m_address;
    uint32_t              m_ifIndex;
    bool                  m_linkUp;
    DataRate              m_bps;
    TxMachineState        m_txMachineState;
    uint32_t              m_mtu;
    Ptr<Packet>           m_currentPkt;

    NetDevice::ReceiveCallback m_rxCallback;

    static const uint16_t DEFAULT_MTU = 1500;

    static uint16_t PppToEther(uint16_t protocol);
    static uint16_t EtherToPpp(uint16_t protocol);
};

}

#endif /* INA_SWITCH_NET_DEVICE_H */