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
 *	this is a simple worker application, just for test
 */

#ifndef SIMPLE_WORKER_H
#define SIMPLE_WORKER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/ptr.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"

namespace ns3
{

class Socket;
class Packet;

class SimpleWorker : public Application
{
  public:
    static uint32_t m_workerNum;
    static TypeId GetTypeId();
    SimpleWorker();
    ~SimpleWorker() override;

    void SetRemote(Address ip, uint16_t port);
    void SetRemote(Address addr);

    uint32_t GetWorkerId() const;
		uint32_t GetEpochId() const;
		uint32_t GetJobId() const;

  protected:
    void DoDispose() override;

  private:
    void StartApplication() override;
    void StopApplication() override;

    void Send();
		void DoTraining();
		void FinishTraining();
		void StartSend();
		void FinishSend();

		void HandleRead(Ptr<Socket> socket);

		enum WorkerState{
			IDLE,
			TRAINING,
      FINISH_TRAINING,
    	SENDING,
			RECV,
		};

		WorkerState m_state;
    uint32_t m_count;
    uint32_t m_size;
    uint32_t m_workerId;
    Time m_interval;
		uint32_t m_port;
		bool m_initializeSocket;

		EventId m_trainEvent;
		Time m_trainTime;
    uint32_t m_epochId;
		uint32_t m_jobId;
		uint32_t m_maxEpoch;
		
		uint32_t m_recv;
    uint32_t m_sent;
    uint64_t m_totalTx;
    Ptr<Socket> m_socket;
    Address m_peerAddress;
    uint16_t m_peerPort;
    EventId m_sendEvent;

    uint32_t m_selfParam;
};

} // namespace ns3

#endif
