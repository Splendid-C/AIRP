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



#ifndef INA_SWITCH_H
#define INA_SWITCH_H

#include "ns3/net-device.h" 
#include "ns3/node.h"
#include "ns3/mac48-address.h"
#include <unordered_map>
#include	<vector>


namespace ns3 {

class InaSwitch : public Object
{
  public:
		void SetNode(Ptr<Node> node);
		void AddPort(Ptr<NetDevice> port);
		void ReceiveFromDevice();

  private:
    Ptr<Node> 			m_node;
		// std::vector<Ptr<NetDevice> > 	m_ports;
		
};

} // namespace ns3

#endif /* INA_SWITCH_H */