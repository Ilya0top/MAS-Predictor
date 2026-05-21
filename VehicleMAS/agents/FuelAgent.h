#pragma once
#include "BaseAgent.h"
#include "./../fuel/FuelSystemModel.h"

namespace mas::agent
{
    class FuelAgent : public BaseAgent
    {
    private:
        model::FuelSystemModel m_model;
        model::FuelSystemState m_state;
        model::FuelSystemPrediction m_lastPrediction;
        double m_loadPercent = 50.0;
        std::string m_replyTo;

    protected:
        void handleMessage(const comm::Message& msg) override;
        void runIteration() override;

    public:
        FuelAgent(std::shared_ptr<comm::MessageBroker> broker, std::shared_ptr<comm::Blackboard> blackboard);
    };
}