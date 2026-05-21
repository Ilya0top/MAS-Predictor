#pragma once
#include <string>
#include "./../common/Types.h"

namespace mas::model
{
    struct TireConfig
    {
        double maxTreadWear = 8.0;              // предельный износ протектора, мм
        double maxTireLife = 5000.0;            // ресурс шин при норме, часы
    };

    struct TireState
    {
        double treadWear = 0.0;                 // износ протектора, мм
    };

    struct TirePrediction
    {
        double tireLife_hours;                  // остаточный ресурс шин
        bool   isCritical;                      // критическое состояние?
        std::string recommendation;             // текстовая рекомендация
    };

    class TireModel
    {
    private:
        TireConfig m_config;
        bool m_verbose;

        void log(const std::string& message) const;
    public:
        TireModel(const TireConfig& config = TireConfig{}, bool verbose = false);

        // Износ протектора за 1 час, мм
        double calculateTreadWear(double loadPercent, RoadType road) const;

        // Комплексный прогноз
        TirePrediction predict(const TireState& state, double loadPercent, RoadType road) const;

        const TireConfig& config() const { return m_config; }
    };
}