/*
 * Copyright (c) 2007 University of Washington
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

#ifndef INA_SWITCH_CHANNEL_H
#define INA_SWITCH_CHANNEL_H

#include "ns3/channel.h"
#include "ns3/ina-switch-net-device.h"
#include "ns3/net-device.h"
#include "ns3/nstime.h"
#include "ns3/ptr.h"

namespace ns3
{

class InaSwitchNetDevice;
class Packet;

class InaSwitchChannel : public Channel
{
  public:
    static TypeId GetTypeId(void);
    InaSwitchChannel();
    ~InaSwitchChannel();

    void Attach(Ptr<InaSwitchNetDevice> device);
    virtual bool TransmitStart(Ptr<const Packet> p, Ptr<InaSwitchNetDevice> src, Time txTime);

    Ptr<InaSwitchNetDevice> GetInaSwitchNetDevice(std::size_t i) const;

    // inherited from ns3::Channel
    std::size_t GetNDevices() const override;
    Ptr<NetDevice> GetDevice(std::size_t i) const override;

  protected:
    Time GetDelay() const;
    bool IsInitialized() const;
    Ptr<InaSwitchNetDevice> GetSource(uint32_t i) const;
    Ptr<InaSwitchNetDevice> GetDestination(uint32_t i) const;

  private:
    Time m_delay;
    std::size_t m_nDevices;
    static const std::size_t N_DEVICES = 2;

    /** \brief Wire states
     *
     */
    enum WireState
    {
        /** Initializing state */
        INITIALIZING,
        /** Idle state (no transmission from NetDevice) */
        IDLE,
        /** Transmitting state (data being transmitted from NetDevice. */
        TRANSMITTING,
        /** Propagating state (data is being propagated in the channel. */
        PROPAGATING
    };

    /**
     * \brief Wire model for the PointToPointChannel
     */
    class Link
    {
      public:
        /** \brief Create the link, it will be in INITIALIZING state
         *
         */
        Link()
            : m_state(INITIALIZING),
              m_src(nullptr),
              m_dst(nullptr)
        {
        }

        WireState m_state;             //!< State of the link
        Ptr<InaSwitchNetDevice> m_src; //!< First NetDevice
        Ptr<InaSwitchNetDevice> m_dst; //!< Second NetDevice
    };

    Link m_link[N_DEVICES]; //!< Link model
};

} // namespace ns3

#endif /* INA_SWITCH_CHANNEL_H */