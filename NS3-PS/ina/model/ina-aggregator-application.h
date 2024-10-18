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
 *  this is a simple worker application, just for test
 */

#ifndef INA_AGGREGATION_H
#define INA_AGGREGATION_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/traced-callback.h"
#include <unordered_map>
#include "ns3/seq-ts-header.h"
#include "ina-header.h"
#include <deque>
#include <algorithm>
#include <vector>

using namespace std;

namespace ns3
{

class InaAggALU{
	public:
		InaAggALU(){};
	
		enum AggAluStatus
		{
			IDLE,
			BUSY
		};

		AggAluStatus m_state;
		uint32_t m_serverJobId;
		uint64_t m_addSum;
};

class InaWorkerStatus{
	public:
		InaWorkerStatus(){};

		uint32_t m_jobId;
		uint32_t m_epochId;
		uint32_t m_workerId;
		uint32_t m_recvCnt;
		uint32_t m_expectedSeq;
		bool m_recvDataFlag;
		Address m_workerAddress;
		// deque<Ptr<Packet>> m_pendingQueue;

		// here store the packet, key is the seq number
		unordered_map<uint32_t, Ptr<Packet>> m_pendingQueueMap;
};

class InaJobStatus{
	public:
		InaJobStatus(){
			m_jobId = 0;
			m_epochId = 0;
			m_workerNum = 0;
			m_curSeq = 0;
			m_aggSeq = 0;
			m_state = IDLE;
		};
	
		enum JobStatus
		{
			IDLE,
			WAIT,
			READY,
		};

		uint32_t m_jobId;
		uint32_t m_epochId;
		uint32_t m_workerNum;			// this job contain how many workers
		uint32_t m_curSeq;
		uint32_t m_aggSeq;
		JobStatus m_state;
		unordered_map<uint32_t, unordered_map<uint32_t, Ptr<Packet> > > m_readyPacketList;	// store <sequence, workerId, packet>
		vector<uint32_t> m_workerIdList;		// this job contain which workers
};


class InaAggregator : public Application
{
	public:
		static TypeId GetTypeId();
		InaAggregator();
		~InaAggregator() override;


		// for initialize
		void Initialize();
		void AssignTotalWorkerNum(uint32_t workerNum);
		void AddWorker(Ptr<Node> workerNode);
		void AddJob(uint32_t jobId, uint32_t workerNum);
		vector<InaAggALU> m_aggAlu;


	protected:
		void DoDispose() override;

	private:


		// function for communicate with workers
		void StartApplication() override;
		void StopApplication() override;
		void HandleRead(Ptr<Socket> socket);
		void Send(Address m_workerAddr, Ptr<Packet> packet);

		
		// may be used in the future
		/*
		void Resume();
		void Pause();
		void UpdateDataRate();
		void Timeout();
		void Resend(Ptr<Socket> socket, Ptr<Packet> packet);
		*/


		// function for aggregation
		void CheckIfCanAgg();
		bool CheckJobDataReady(uint32_t jobId);
		int FindAggALU();
		void DoAggregation(uint32_t aggAluId, uint32_t jobId);
		void FinishAggregation(uint32_t aggAluId, uint32_t jobId, uint32_t aggregateSum, uint32_t ackSeq);
		

		// value for communication 
		uint32_t m_size;
		uint16_t m_port;
		Ptr<Socket> m_socket;
		Ptr<Socket> m_socket6;
		uint32_t m_count;
		Time m_interval;
		EventId m_sendEvent;

		// value for aggregation
		EventId m_waitForAggAluEvent;
		uint32_t m_totalJobNum;
		unordered_map<uint32_t, InaJobStatus> m_jobStatusMap;	// store <jobid, jobStatus>
		Time m_aggDelay;
		uint32_t m_aggAluNum;

		// value for worker information
		uint32_t m_totalWorkerNum;
		unordered_map<uint32_t, InaWorkerStatus> m_workerStatusMap;
};

} // namespace ns3


#endif