#pragma once
#include "BaseAgent.h"
#include "./../cooling/CoolingModel.h"

namespace mas::agent
{
    class CoolingAgent : public BaseAgent
    {
    private:
        model::CoolingModel m_model;
        model::CoolingState m_state;
        model::CoolingPrediction m_lastPrediction;
        double m_loadPercent = 50.0;
        double m_externalTemp = 20.0;
        double m_brakeHeat = 0.0;
        bool m_hasBrakeHeat = false;
        std::string m_replyTo;

    protected:
        void handleMessage(const comm::Message& msg) override;
        void runIteration() override;

    public:
        CoolingAgent(std::shared_ptr<comm::MessageBroker> broker, std::shared_ptr<comm::Blackboard> blackboard);
    };
}