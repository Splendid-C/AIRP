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

#ifndef SIMPLE_AGGREGATION_H
#define SIMPLE_AGGREGATION_H

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

class SimpleAggregator : public Application
{
  public:
    static TypeId GetTypeId();
    SimpleAggregator();
    ~SimpleAggregator() override;

  protected:
    void DoDispose() override;

  private:
    class WorkerStatus;

    void StartApplication() override;
    void StopApplication() override;

    void HandleRead(Ptr<Socket> socket);

		void CheckIfRecvAll();
    int FindAggALU();
    void DoAggregation(uint32_t aggALUId);
    void FinishAggregation(uint32_t aggregateSum, uint32_t aggALUId);

		void SendAggregationResult(uint32_t aggregateSum);
		void Send(WorkerStatus workerStatus, uint32_t aggregateSum);

    

    enum AggALUState
    {
        IDLE,
        BUSY
    };

    class AggALU
    {
      public:
        AggALU()
            : m_state(IDLE),
              m_value(0),
              m_recvCount(0),
							m_sent(0){};

        AggALUState m_state;
        uint64_t m_value;
        uint32_t m_recvCount;
				EventId m_sendEvent;
				uint32_t m_sent;
    };


		uint32_t m_size;
    uint16_t m_port;
    Ptr<Socket> m_socket;
    Ptr<Socket> m_socket6;
    uint64_t m_received;
		uint32_t m_count;
    deque<Ptr<Packet>> m_sendingQueue;
    Time m_interval;              // interval between two sending action.

    Time m_aggDelay;
    uint32_t m_aggALUNum;
    AggALU m_aggALU[10240];       // assume infinite ALU for aggregation, but actually it is limited by m_aggALUnum.


    // initial the worker and job
    void AddWorker();
    void AddJob();
   
    // use for recording worker infomation
    class WorkerStatus{
      public:
        WorkerStatus(){};
        WorkerStatus(uint32_t jobId, uint32_t epochId, uint32_t workerId, Address addr): m_jobId(0), m_epochId(epochId), m_workerId(workerId), m_workerAddr(addr), m_recvCnt(0), m_seq(0){};

      uint32_t m_jobId;
      uint32_t m_epochId;
      uint32_t m_workerId;
      Address m_workerAddr;
      uint32_t m_recvCnt;
      uint32_t m_seq;
    };

    uint32_t m_recvDataFlag[10240] = {false};    // assume the max number of workers is 1024
    uint32_t m_workerNum = 0;                   // this aggregator ser
    unordered_map<uint32_t, WorkerStatus> m_workerInfoMap;
    unordered_map<uint32_t, deque<Ptr<Packet>>> m_pendingQueue; 
    vector<uint32_t> m_jobList;       // this vector record the job id of the workers which have sent data to this aggregator.
};

} // namespace ns3

#endif /* SIMPLE_AGGREGATION_H */