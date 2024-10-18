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

#ifndef INA_HEADER_H
#define INA_HEADER_H

#include "ns3/buffer.h"
#include "ns3/header.h"
#include "ns3/nstime.h"
#include "ns3/log.h"
#include "ns3/vector.h"
namespace ns3
{

class InaHeader : public Header
{
  public:
    InaHeader();
    ~InaHeader() override;

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;

    void SetWorkerId(uint32_t workerId);
    uint32_t GetWorkerId() const;

    void SetJobId(uint32_t jobId);
    uint32_t GetJobId() const;

    void SetPSId(uint32_t psId);
    uint32_t GetPSId() const;

    void SetEpochId(uint32_t epochId);
    uint32_t GetEpochId() const;

    void SetWorkerNum(uint32_t workerNum);
    uint32_t GetWorkerNum() const;

    void SetPacketType(uint16_t packetType);
    uint16_t GetPacketType() const;

    void SetParam(uint32_t param);
    uint32_t GetParam() const;

    void SetEdgeSwitchIdentifier(uint32_t edgeSwitchIdentifier);
    uint32_t GetEdgeSwitchIdentifier() const;

    

  private:
    uint32_t m_psId;
    uint32_t m_jobId;
    uint32_t m_epochId;
    uint32_t m_workerId;
    uint32_t m_workerNum;
    uint16_t m_packetType;
    uint32_t m_param;
    uint32_t m_edgeSwitchIdentifier;
    
     

    
};

} // namespace ns3

#endif /* INA_HEADER_H */