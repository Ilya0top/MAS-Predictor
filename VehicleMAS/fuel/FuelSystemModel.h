#pragma once
#include <string>

namespace mas::model
{
	struct FuelSystemConfig
	{
		double maxInjectorWear = 100.0;			// предельный износ форсунок, %
		double maxFilterClog = 100.0;			// предельное загрязнение фильтра, %
		double maxInjectorLife = 5000.0;		// ресурс форсунок, часы
		double filterChangeInterval = 500.0;	// интервал замены фильтра, часы
	};

	struct FuelSystemState
	{
		double injectorWear = 0.0;				// износ форсунок, %
		double filterClog = 0.0;				// загрязнение фильтра, %
	};

	struct FuelSystemPrediction
	{
		double efficiency;						// КПД топливной системы, % (100 = идеал)
		double injectorLife_hours;				// остаточный ресурс форсунок
		double filterLife_hours;				// через сколько замена фильтра
		bool   isCritical;
		std::string recommendation;
	};

	class FuelSystemModel
	{
	private:
		FuelSystemConfig m_config;
		bool m_verbose;

		void log(const std::string& message) const;
	public:
		FuelSystemModel(const FuelSystemConfig& config = FuelSystemConfig{}, bool verbose = false);

		// Износ форсунок за 1 час, %
		double calculateInjectorWear(double loadPercent) const;

		// Загрязнение фильтра за 1 час, %
		double calculateFilterClogRate(double loadPercent) const;

		// Текущий КПД топливной системы
		double calculateEfficiency(double injectorWear, double filterClog) const;

		// Комплексный прогноз
		FuelSystemPrediction predict(const FuelSystemState& state, double loadPercent) const;

		const FuelSystemConfig& config() const { return m_config; }
	};
}