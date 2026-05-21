#pragma once
#include "BaseAgent.h"
#include "./../engine/EngineModel.h"
#include "./../transmission/TransmissionModel.h"
#include "./../brake/BrakeModel.h"
#include "./../chassis/ChassisModel.h"
#include "./../cooling/CoolingModel.h"
#include "./../fuel/FuelSystemModel.h"
#include "./../tires/TireModel.h"
#include "./../common/Types.h"
#include <sstream>

namespace mas
{
    struct Scenario
    {
        float loadPercent = 50.0f;
        float externalTemp = 20.0f;
        RoadType road = RoadType::Highway;
        int gearShiftsPerHour = 5;
        int brakesPerHour = 5;
    };

    struct CombinedPrediction
    {
        model::EnginePrediction       engine;
        model::TransmissionPrediction transmission;
        model::BrakePrediction        brakes;
        model::ChassisPrediction      chassis;
        model::CoolingPrediction      cooling;
        model::FuelSystemPrediction   fuel;
        model::TirePrediction         tires;

        double minLifeHours = 0.0;
        std::string summary;
    };
}

namespace mas::agent
{
    class CoordinatorAgent : public BaseAgent
    {
    private:
        void parseResponse(const std::string& sender, const std::string& content);
        CombinedPrediction buildSummary();

        CombinedPrediction m_combined;
        bool m_collecting = false;
        int m_responsesReceived = 0;

    protected:
        void handleMessage(const comm::Message& msg) override;
        void runIteration() override;
        void run(std::stop_token stoken) override;

    public:
        CoordinatorAgent(std::shared_ptr<comm::MessageBroker> broker, std::shared_ptr<comm::Blackboard> blackboard);
        ~CoordinatorAgent() override = default;

        CombinedPrediction runPrediction(const Scenario& scenario);
    };
}