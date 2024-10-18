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

#include "ina-helper.h"
#include "ns3/uinteger.h"

namespace ns3
{

InaAggregatorHelper::InaAggregatorHelper(){
    m_factory.SetTypeId(InaAggregator::GetTypeId());
}

InaAggregatorHelper::InaAggregatorHelper(uint32_t port){
    m_factory.SetTypeId(InaAggregator::GetTypeId());
    SetAttribute("Port", UintegerValue(port));
}

void
InaAggregatorHelper::SetAttribute(std::string name, const AttributeValue &value){
    m_factory.Set(name, value);
}

ApplicationContainer
InaAggregatorHelper::Install(NodeContainer c){
    ApplicationContainer apps;
    for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i){
        Ptr<Node> node = *i;

        m_application = m_factory.Create<InaAggregator>();
        node->AddApplication(m_application);
        apps.Add(m_application);
    }
    return apps;
}

Ptr<InaAggregator>
InaAggregatorHelper::GetApplication(){
    return m_application;
}

void
InaAggregatorHelper::Initialize(){
    m_application->Initialize();
}

void
InaAggregatorHelper::AssignTotalWorkerNum(uint32_t workerNum){
    m_application->AssignTotalWorkerNum(workerNum);
}

void
InaAggregatorHelper::AddJob(uint32_t jobId, uint32_t subWorkerNum, uint32_t head, uint32_t tail, map<uint32_t, ns3::Address> workerAddr,vector<uint32_t> workerIdList){
    m_application->AddJob(jobId, subWorkerNum, head, tail, workerAddr,workerIdList);
}



InaWorkerHelper::InaWorkerHelper(){
    m_factory.SetTypeId(InaWorker::GetTypeId());
}


// void
// InaWorkerHelper::AssignPendingJobList(vector<JobList> pendingJobList){
//     m_application->AssignPendingJobList(pendingJobList);
// }

InaWorkerHelper::InaWorkerHelper(Address ip, uint16_t port){
    m_factory.SetTypeId(InaWorker::GetTypeId());
    SetAttribute("RemoteAddress", AddressValue(ip));
    SetAttribute("RemotePort", UintegerValue(port));
}

InaWorkerHelper::InaWorkerHelper(Address addr){
    m_factory.SetTypeId(InaWorker::GetTypeId());
    SetAttribute("RemoteAddress", AddressValue(addr));
}

void
InaWorkerHelper::SetAttribute(std::string name, const AttributeValue &value){
    m_factory.Set(name, value);
}

ApplicationContainer
InaWorkerHelper::Install(NodeContainer c){
    ApplicationContainer apps;
    for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i){
        Ptr<Node> node = *i;

        m_application = m_factory.Create<InaWorker>();
        node->AddApplication(m_application);
        apps.Add(m_application);
    }
    return apps;
}
    
} // namespace ns3
