#pragma once
#include "BaseAgent.h"
#include "./../brake/BrakeModel.h"

namespace mas::agent
{
    class BrakeAgent : public BaseAgent
    {
    private:
        model::BrakeModel m_model;
        model::BrakeState m_state;
        model::BrakePrediction m_lastPrediction;
        double m_loadPercent = 50.0;
        double m_tireWear = 0.0;
        int m_brakesPerHour = 5;
        bool m_hasTireWear = false;
        std::string m_replyTo;

    protected:
        void handleMessage(const comm::Message& msg) override;
        void runIteration() override;

    public:
        BrakeAgent(std::shared_ptr<comm::MessageBroker> broker, std::shared_ptr<comm::Blackboard> blackboard);
    };
}