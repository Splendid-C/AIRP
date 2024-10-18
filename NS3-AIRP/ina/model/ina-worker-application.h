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


#ifndef INA_WORKER_APPLICATION_H
#define INA_WORKER_APPLICATION_H

#include "ns3/application.h"
#include "ns3/socket.h"
#include "ns3/nstime.h"
 
#include <vector>


using namespace std;
namespace ns3 {



struct JobList
    {
        uint32_t jobId;
        uint32_t aggNodeId;
        uint32_t workerNum;
        // uint32_t totalAluAssign;
        uint32_t head;
        uint32_t tail;
        uint32_t subJobNum;
        uint32_t subCountNum;
        uint32_t aluAssign;
        uint32_t bigJobId;
        map<uint32_t, ns3::Address> workerAddr;
        vector<uint32_t> workerIdList;
    };




class InaWorker : public Application
{

 public:
    




     
    static uint32_t m_workerNum;
    static TypeId GetTypeId ();
    InaWorker ();
    ~InaWorker () override;

    void SetRemote (Address ip, uint16_t port);
    void SetRemote (Address addr);

    uint32_t GetWorkerId() const;
    uint32_t GetEpochId() const;
    uint32_t GetJobId() const;

  protected:
    void DoDispose () override;

  private:
    void StartApplication () override;
    void StopApplication () override;

    void StartNewEpoch();
    void DoTrainning();
    void FinishTraining();


    void SetupSocket();
    void Send();
    void FinishSend();
    Ptr<Packet> GeneratePacket(uint32_t sequence, uint32_t param);
    void HandleRead(Ptr<Socket> socket);
    void CheckAndUpdate(uint32_t sequence);
    bool CheckAllWorkerReady();
    
    
    // for communication
    uint32_t m_port;
    Time m_interval;
    uint32_t m_count;
    uint32_t m_size;
    uint32_t m_subJobNum;
    uint32_t m_totalJobCount;
    uint32_t m_seq;
    uint32_t m_unackSeq;
    uint32_t m_totalRx;
    uint32_t m_totalTx;
    uint32_t m_packetReceived;
    Ptr<Socket> m_socket;
    Address m_peerAddress;
    uint16_t m_peerPort;
    EventId m_sendEvent;
    bool m_initializeSocket;


    vector<JobList> m_pendingJobList;



    // for worker information 
    uint32_t m_workerId;
    uint32_t m_jobId;
    uint32_t m_epochId;

    EventId m_trainEvent;
    Time m_trainTime;
    uint32_t m_maxEpoch;

    uint32_t m_trainParam; 


};

} // namespace ns3

#endif /* INA_WORKER_APPLICATION_H */






