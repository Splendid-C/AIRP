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

#ifndef INA_PS_H
#define INA_PS_H

#include "ina-header.h"
#include "ina-aggregator-application.h"

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/seq-ts-header.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/traced-callback.h"

#include <algorithm>
#include <deque>
#include <unordered_map>
#include <vector>

 
#include <assert.h>
#include <string.h>
 
using namespace std;


namespace ns3
{


class InaPS : public Application
{
  public:
    static TypeId GetTypeId();
    InaPS();
    ~InaPS() override;

    // for initialize
    void Initialize();  //reset the state of ALU
    void AssignTotalSwitchNum(uint32_t switchNum);
    void AddSwitch(Ptr<Node> switchNode);
    void AddJob(uint32_t jobId, uint32_t switchNum);
	void AddPendingJobList(vector<JobList> pendingJobList);
     

protected:
		void DoDispose() override;

	private:


		// function for communicate with workers
		void StartApplication() override;
		void StopApplication() override;
		void HandleRead(Ptr<Socket> socket); //handle every packet received
		void Send(Address m_workerAddr, Ptr<Packet> packet); //send to address appointed 


        // function for aggregation
		void PacketArrivalStage(Ptr<Packet> packet);

		void CheckIfCanAgg();
        void SetupSocket();

		bool CheckJobDataReady(uint32_t jobId);

		int FindAggALU();
		 
		void DoAggregation(uint32_t aggAluId, uint32_t jobId);
		void FinishAggregation(uint32_t aggAluId, uint32_t jobId, uint32_t aggregateSum, uint32_t ackSeq);

        
		void SendToSwitch(uint32_t jobId, uint32_t aggregateSum, uint32_t ackSeq);
		void SendToWorker(uint32_t jobId, uint32_t aggregateSum, uint32_t ackSeq);

		












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
		vector<JobList> m_pendingJobList;

        Address m_psAddress;

        vector<InaAggALU> m_aggAlu; //ALU queue

 

};


}


#endif