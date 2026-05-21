#pragma once
#include <string>

namespace mas::model
{
	struct CoolingConfig
	{
		double maxCoolantTemp = 110.0;			// предельная температура ОЖ, °C
		double maxRadiatorClog = 100.0;			// предельное загрязнение радиатора, %
		double maxPumpWear = 100.0;				// предельный износ помпы, %
		double coolantChangeInterval = 2000.0;	// интервал замены ОЖ, часы
		double nominalCoolantTemp = 85.0;		// нормальная температура ОЖ, °C
	};

	struct CoolingState
	{
		double coolantTemp = 85.0;				// температура ОЖ, °C
		double coolantQuality = 100.0;			// качество ОЖ, % (100 = новое)
		double radiatorClog = 0.0;				// загрязнение радиатора, %
		double pumpWear = 0.0;					// износ помпы, %
	};

	struct CoolingPrediction
	{
		double predictedCoolantTemp;			// прогнозируемая температура ОЖ, °C
		double coolantLife_hours;				// через сколько замена ОЖ
		double radiatorClean_hours;				// через сколько чистка радиатора
		double pumpLife_hours;					// остаточный ресурс помпы
		bool   isOverheat;						// перегрев?
		bool   isCritical;						// критическое состояние?
		std::string recommendation;				// текстовая рекомендация
	};

	class CoolingModel
	{
	private:
		CoolingConfig m_config;
		bool m_verbose;

		void log(const std::string& message) const;
	public:
		CoolingModel(const CoolingConfig& config = CoolingConfig{}, bool verbose = false);

		// Расчёт температуры ОЖ
		double calculateCoolantTemp(double loadPercent, double externalTemp, double radiatorClog, double pumpWear) const;

		// Деградация ОЖ за 1 час, %
		double calculateCoolantDegradation(double coolantTemp) const;

		// Скорость загрязнения радиатора, %/ч
		double calculateRadiatorClogRate(double externalTemp) const;

		// Износ помпы за 1 час, %
		double calculatePumpWear(double coolantTemp) const;

		// Комплексный прогноз
		CoolingPrediction predict(const CoolingState& state, double loadPercent, double externalTemp) const;

		const CoolingConfig& config() const { return m_config; }
	};
}