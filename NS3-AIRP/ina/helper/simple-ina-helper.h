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

#ifndef SIMPLE_INA_HELPER_H
#define SIMPLE_INA_HELPER_H

#include "ns3/application-container.h"
#include "ns3/ipv4-address.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/simple-aggregator-application.h"
#include "ns3/simple-worker-application.h"

namespace ns3 {

class SimpleAggregatorHelper
{
	public:
		SimpleAggregatorHelper();
		SimpleAggregatorHelper(uint32_t port);

		void SetAttribute(std::string name, const AttributeValue &value);

		ApplicationContainer Install(NodeContainer c);

		Ptr<SimpleAggregator> GetApplication();

	private:
		Ptr<SimpleAggregator> m_application;
		ObjectFactory m_factory;

};


class SimpleWorkerHelper
{
	public:
		SimpleWorkerHelper();
		SimpleWorkerHelper(Address ip, uint16_t port);
		SimpleWorkerHelper(Address addr);

		void SetAttribute(std::string name, const AttributeValue &value);
		ApplicationContainer Install(NodeContainer c);

	private:
		Ptr<SimpleWorker> m_worker;
		ObjectFactory m_factory;
};

} // namespace ns3

#endif /* SIMPLE_INA_HELPER_H */