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

#ifndef INA_HELPER_H
#define INA_HELPER_H

#include "ns3/application-container.h"
#include "ns3/ipv4-address.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ina-aggregator-application.h"
#include "ns3/ina-worker-application.h"

namespace ns3 {

class InaAggregatorHelper
{
    public:
        InaAggregatorHelper();
        InaAggregatorHelper(uint32_t port);

        void SetAttribute(std::string name, const AttributeValue &value);

        ApplicationContainer Install(NodeContainer c);

        Ptr<InaAggregator> GetApplication();

        void Initialize();
        void AssignTotalWorkerNum(uint32_t workerNum);
        void AddJob(uint32_t jobId, uint32_t subWorkerNum);

    private:
        Ptr<InaAggregator> m_application;
        ObjectFactory m_factory;

};


class InaWorkerHelper
{
    public:
        InaWorkerHelper();
        InaWorkerHelper(Address ip, uint16_t port);
        InaWorkerHelper(Address addr);

        void SetAttribute(std::string name, const AttributeValue &value);
        ApplicationContainer Install(NodeContainer c);

        Ptr<InaWorker> GetApplication();

    private:
        Ptr<InaWorker> m_application;
        ObjectFactory m_factory;
};

} // namespace ns3

#endif /* INA_HELPER_H */