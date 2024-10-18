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

#include "ina-header.h"

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



 

class BitMap
{

public:
    //构造函数
    BitMap(const size_t & range) {
        assert(range >= 0);
        if (bits != nullptr) {
            delete[] bits;
        }
        count = range;
        size = range / 32 + 1;
        bits = new unsigned int[size];
    }
    //析构函数
    ~BitMap() {
        delete[] bits;
    }
    //初始化数据，把所有数据置0
    void init() {
        for (int i = 0; i < size; i++)
            bits[i] = 0;
    }
    //增加数据到位图
    void add(const size_t & num) {
        assert(count > num);
        int index = num / 32;
        int bit_index = num % 32;
        bits[index] |= 1 << bit_index;
    }
    //删除数据到位图
    void remove(const size_t & num){
        assert(count > num );
        int index = num / 32;
        int bit_index = num % 32;
        bits[index] &= ~(1 << bit_index);
    }
    //查找数据到位图
    bool find(const size_t & num) {
        assert(count > num);
        int index = num / 32;
        int bit_index = num % 32;
        return (bits[index] >> bit_index) & 1;
    }
//位图相关数据

    unsigned int* bits=nullptr;
    int size;
    int count;


BitMap(int range = 0);

};

 

// each ALU is an InaAggALU object
class InaAggALU
{
  public:
    InaAggALU(){};

    enum AggAluStatus
    {
        IDLE,
        BUSY
    };

    
    // BitMap bitmap; //whether a packet aggregated
    AggAluStatus m_state;  // the state of this ALU
    uint32_t m_counter; //how many worker
    uint32_t m_ECN;
    uint32_t m_serveJobId; // current jobid
    uint32_t m_serveSeqId; // current seqid
    uint32_t m_timestamp;
    uint64_t m_value;
    Address m_PSAddress;
};

// each worker is an InaWorkerStatus object
class InaWorkerStatus
{
  public:
    InaWorkerStatus(){};

    uint32_t m_jobId; // current jobid
    uint32_t m_epochId;
    uint32_t m_workerId;    // set a workerid
    uint32_t m_recvCnt;     // the number of packet received
    uint32_t m_expectedSeq; // the next seq
    bool m_recvDataFlag;    // if the worker receive the result
    Address m_workerAddress;
    // deque<Ptr<Packet>> m_pendingQueue;

    // here store the packet, key is the seq number
    unordered_map<uint32_t, Ptr<Packet>> m_pendingQueueMap;
};

// each job is an InaJobStatus object
class InaJobStatus
{
  public:
    InaJobStatus()
    {
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
        WAIT, // wait for all packets
        READY,
    };

    uint32_t m_jobId;
    uint32_t m_epochId;
    uint32_t m_workerNum; // this job contain how many workers
    uint32_t m_curSeq;    // current seq
    uint32_t m_aggSeq;    // the seq to be aggregated
    JobStatus m_state;
    unordered_map<uint32_t, unordered_map<uint32_t, Ptr<Packet>>> m_readyPacketList; // for resent, store <sequence, workerId, packet>
    vector<uint32_t> m_workerIdList; // this job contain which workers
};


struct JobList
{   
    uint32_t psId;
    uint32_t jobId;
    uint32_t aggNodeId;
    ns3::Address psFatherAddr;
    uint32_t workerNum;
    vector<uint32_t> workerIdList;
    map<uint32_t, ns3::Address> workerAddr;
    vector<uint32_t> torswitchIdList;//store the tor of this job
    
};

class InaAggregator : public Application
{
  public:
    static TypeId GetTypeId();
    InaAggregator();
    ~InaAggregator() override;

    // for initialize
    void Initialize();  //reset the state of ALU
    void AssignTotalWorkerNum(uint32_t workerNum);
    void AddWorker(Ptr<Node> workerNode);
    void AddJob(uint32_t jobId, uint32_t workerNum);
    void AddFatherList(map<uint32_t, uint32_t>fatherList);
    void AddPendingJobList(vector<JobList> pendingJobList);
    void AddNodeId2Addr(map<uint32_t, Address> hostNode2Addr);
    vector<InaAggALU> m_aggAlu; //ALU queue
    




protected:
		void DoDispose() override;

	private:


		// function for communicate with workers
		void StartApplication() override;
		void StopApplication() override;
		void HandleRead(Ptr<Socket> socket); //handle every packet received
		void Send(Address m_workerAddr, Ptr<Packet> packet); //send to address appointed 


        // function for aggregation
		void PacketArrivalStage(Ptr<Packet> pkt);
        void SendToWorker(uint32_t jobId, uint32_t aggregateSum, uint32_t ackSeq);
        void PSTorExecute(Ptr<Packet> pkt);

		void CheckIfCanAgg();
        void SetupSocket(Address address);
        int FindAggALU(uint32_t jobId);

		bool CheckJobDataReady(uint32_t jobId);
        bool CheckIfRecvAll(uint32_t jobId, uint32_t switchId, uint32_t seqId);

		int FindAggALU();
		 
		void DoAggregation(uint32_t aggAluId, uint32_t jobId);
		void FinishAggregation(uint32_t aggAluId, uint32_t jobId, uint32_t aggregateSum, uint32_t ackSeq);

        void SendtoPS(uint32_t jobId, uint32_t seqId, uint32_t packetType, uint32_t param, uint32_t workernum, uint32_t psId);
        void SendtoPSTor(uint32_t jobId, uint32_t packetType, uint32_t seqId, uint32_t workernum, uint32_t psId, uint32_t port);
        void SendToSwitch(uint32_t jobId, uint32_t aggregateSum, uint32_t ackSeq);
        uint32_t hashFunction(uint32_t param1, uint32_t param2);



// value for communication 
		uint32_t m_size;
		uint16_t m_port;
		Ptr<Socket> m_socket;
        Ptr<Socket> m_socket2;
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
        EventId m_doAggregation;

		// value for worker information
        map<uint32_t, uint32_t>m_fatherList;
        vector<JobList> m_pendingJobList;
        map<uint32_t, Address> m_NodeId2Addr;
		uint32_t m_totalWorkerNum;
		unordered_map<uint32_t, InaWorkerStatus> m_workerStatusMap; 

        Address m_psAddress;









};

} // namespace ns3

#endif