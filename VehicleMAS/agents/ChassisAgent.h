#pragma once
#include "BaseAgent.h"
#include "./../chassis/ChassisModel.h"

namespace mas::agent
{
    class ChassisAgent : public BaseAgent
    {
    private:
        model::ChassisModel m_model;
        model::ChassisState m_state;
        model::ChassisPrediction m_lastPrediction;
        double m_loadPercent = 50.0;
        RoadType m_road = RoadType::Highway;
        std::string m_replyTo;

    protected:
        void handleMessage(const comm::Message& msg) override;
        void runIteration() override;

    public:
        ChassisAgent(std::shared_ptr<comm::MessageBroker> broker, std::shared_ptr<comm::Blackboard> blackboard);
    };
}