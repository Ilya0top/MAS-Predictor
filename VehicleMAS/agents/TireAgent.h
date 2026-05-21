#pragma once
#include "BaseAgent.h"
#include "./../tires/TireModel.h"

namespace mas::agent
{
    class TireAgent : public BaseAgent
    {
    private:
        model::TireModel m_model;
        model::TireState m_state;
        model::TirePrediction m_lastPrediction;
        double m_loadPercent = 50.0;
        RoadType m_road = RoadType::Highway;
        std::string m_replyTo;

    protected:
        void handleMessage(const comm::Message& msg) override;
        void runIteration() override;

    public:
        TireAgent(std::shared_ptr<comm::MessageBroker> broker, std::shared_ptr<comm::Blackboard> blackboard);
    };
}