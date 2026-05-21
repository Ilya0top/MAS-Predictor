#pragma once
#include "BaseAgent.h"
#include "./../engine/EngineModel.h"

namespace mas::agent
{
    class EngineAgent : public BaseAgent
    {
    private:
        model::EngineModel m_model;
        model::EngineState m_state;
        model::EnginePrediction m_lastPrediction;

        double m_loadPercent = 50.0;
        double m_fuelEfficiency = 100.0;
        double m_coolantTemp = 90.0;
        double m_vibrationFactor = 1.0;
        double m_transOilTemp = 80.0;
        bool m_hasFuelEff = false;
        bool m_hasCoolant = false;
        bool m_hasVibration = false;
        std::string m_replyTo;

    protected:
        void handleMessage(const comm::Message& msg) override;
        void runIteration() override;

    public:
        EngineAgent(std::shared_ptr<comm::MessageBroker> broker, std::shared_ptr<comm::Blackboard> blackboard);
    };
}