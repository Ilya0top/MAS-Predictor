#pragma once
#include "BaseAgent.h"
#include "./../transmission/TransmissionModel.h"

namespace mas::agent
{
    class TransmissionAgent : public BaseAgent
    {
    private:
        model::TransmissionModel m_model;
        model::TransmissionState m_state;
        model::TransmissionPrediction m_lastPrediction;
        double m_loadPercent = 50.0;
        int m_gearShifts = 5;
        double m_heatFromEngine = 0.0;
        double m_coolantTemp = 80.0;
        bool m_hasCoolantTemp = false;
        bool m_hasHeatFromEngine = false;
        std::string m_replyTo;

    protected:
        void handleMessage(const comm::Message& msg) override;
        void runIteration() override;

    public:
        TransmissionAgent(std::shared_ptr<comm::MessageBroker> broker, std::shared_ptr<comm::Blackboard> blackboard);
    };
}