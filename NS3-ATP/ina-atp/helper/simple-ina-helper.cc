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

#include "simple-ina-helper.h"
#include "ns3/uinteger.h"

namespace ns3 {

SimpleAggregatorHelper::SimpleAggregatorHelper(){
    m_factory.SetTypeId(SimpleAggregator::GetTypeId());
}

SimpleAggregatorHelper::SimpleAggregatorHelper(uint32_t port){
    m_factory.SetTypeId(SimpleAggregator::GetTypeId());
    SetAttribute("Port", UintegerValue(port));
}

void
SimpleAggregatorHelper::SetAttribute(std::string name, const AttributeValue &value){
    m_factory.Set(name, value);
}

ApplicationContainer
SimpleAggregatorHelper::Install(NodeContainer c){
    ApplicationContainer apps;
    for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i){
        Ptr<Node> node = *i;

        m_application = m_factory.Create<SimpleAggregator>();
        node->AddApplication(m_application);
        apps.Add(m_application);
    }
    return apps;
}

Ptr<SimpleAggregator>
SimpleAggregatorHelper::GetApplication(){
    return m_application;
}



SimpleWorkerHelper::SimpleWorkerHelper(){
    m_factory.SetTypeId(SimpleWorker::GetTypeId());
}

SimpleWorkerHelper::SimpleWorkerHelper(Address address, uint16_t port){
    m_factory.SetTypeId(SimpleWorker::GetTypeId());
    SetAttribute("RemoteAddress", AddressValue(address));
    SetAttribute("RemotePort", UintegerValue(port));
}

SimpleWorkerHelper::SimpleWorkerHelper(Address address){
    m_factory.SetTypeId(SimpleWorker::GetTypeId());
    SetAttribute("RemoteAddress", AddressValue(address));
}

void
SimpleWorkerHelper::SetAttribute(std::string name, const AttributeValue &value){
    m_factory.Set(name, value);
}

ApplicationContainer
SimpleWorkerHelper::Install(NodeContainer c){
    ApplicationContainer apps;
    for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i){
        Ptr<Node> node = *i;

        m_worker = m_factory.Create<SimpleWorker>();
        node->AddApplication(m_worker);
        apps.Add(m_worker);
    }
    return apps;
}

} // namespace ns3