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


#include "ina-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("InaHeader");

NS_OBJECT_ENSURE_REGISTERED(InaHeader);

InaHeader::InaHeader(){}

InaHeader::~InaHeader(){}

TypeId
InaHeader::GetTypeId(){
    static TypeId tid = 
        TypeId("ns3::InaHeader")
            .SetParent<Header>()
            .SetGroupName("Ina")
            .AddConstructor<InaHeader>()
    ;
    return tid;
}

TypeId
InaHeader::GetInstanceTypeId() const{
    return GetTypeId();
}

void
InaHeader::Print(std::ostream &os) const{
    NS_LOG_FUNCTION(this << &os);
    return;
}

uint32_t
InaHeader::GetSerializedSize() const{
    NS_LOG_FUNCTION(this);
    return sizeof(m_param) + sizeof(m_workerId) + sizeof(m_epochId) + sizeof(m_jobId) + sizeof(m_workerNum) + sizeof(m_packetType);
}

void
InaHeader::Serialize(Buffer::Iterator start) const{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    i.WriteHtonU32(m_jobId);
    i.WriteHtonU32(m_epochId);
    i.WriteHtonU32(m_workerId);
    i.WriteHtonU32(m_workerNum);
    i.WriteHtonU16(m_packetType);
    i.WriteHtonU64(m_param);
}

uint32_t
InaHeader::Deserialize(Buffer::Iterator start){
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    m_jobId = i.ReadNtohU32();
    m_epochId = i.ReadNtohU32();
    m_workerId = i.ReadNtohU32();
    m_workerNum = i.ReadNtohU32();
    m_packetType = i.ReadNtohU16();
    m_param = i.ReadNtohU64();
    return GetSerializedSize();
}

void
InaHeader::SetWorkerId(uint32_t workerId){
    NS_LOG_FUNCTION(this << workerId);
    m_workerId = workerId;
}

uint32_t
InaHeader::GetWorkerId() const{
    NS_LOG_FUNCTION(this);
    return m_workerId;
}

void
InaHeader::SetParam(uint64_t param){
    NS_LOG_FUNCTION(this << param);
    m_param = param;
}

uint64_t
InaHeader::GetParam() const{
    NS_LOG_FUNCTION(this);
    return m_param;
}

void
InaHeader::SetJobId(uint32_t jobId){
    NS_LOG_FUNCTION(this << jobId);
    m_jobId = jobId;
}

uint32_t
InaHeader::GetJobId() const{
    NS_LOG_FUNCTION(this);
    return m_jobId;
}

void
InaHeader::SetEpochId(uint32_t epochId){
    NS_LOG_FUNCTION(this << epochId);
    m_epochId = epochId;
}

uint32_t
InaHeader::GetEpochId() const{
    NS_LOG_FUNCTION(this);
    return m_epochId;
}

void 
InaHeader::SetWorkerNum(uint32_t workerNum){
    NS_LOG_FUNCTION(this << workerNum);
    m_workerNum = workerNum;
}

uint32_t 
InaHeader::GetWorkerNum() const{
    NS_LOG_FUNCTION(this);
    return m_workerNum;
}

void 
InaHeader::SetPacketType(uint16_t packetType){
    NS_LOG_FUNCTION(this << packetType);
    m_packetType = packetType;
}

uint16_t   
InaHeader::GetPacketType() const{
    NS_LOG_FUNCTION(this);
    return m_packetType;
}

} // namespace ns3
