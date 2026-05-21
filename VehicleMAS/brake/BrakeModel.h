#pragma once
#include <string>

namespace mas::model
{
	struct BrakeConfig
	{
		double maxPadWear = 10.0;			// предельный износ колодок, мм
		double maxDiscWear = 2.0;			// предельный износ дисков, мм
		double maxPadLife = 3000.0;			// ресурс колодок при норме, часы
		double maxDiscLife = 9000.0;		// ресурс дисков при норме, часы
		double maxBrakeTemp = 400.0;		// критическая температура, °C
		double brakeTempBase = 150.0;		// нормальная температура тормозов, °C
	};

	struct BrakeState
	{
		double padWear = 0.0;				// износ колодок, мм
		double discWear = 0.0;				// износ дисков, мм
		double brakeTemp = 150.0;			// температура тормозов, °C
		int    brakesPerHour = 5;			// среднее количество торможений в час
	};

	struct BrakePrediction
	{
		double padLife_hours;				// остаточный ресурс колодок
		double discLife_hours;				// остаточный ресурс дисков
		bool   isOverheat;					// перегрев?
		bool   isCritical;					// критическое состояние?
		std::string recommendation;
	};

	class BrakeModel
	{
	private:
		BrakeConfig m_config;
		bool m_verbose;

		void log(const std::string& message) const;
	public:
		BrakeModel(const BrakeConfig& config = BrakeConfig{}, bool verbose = false);

		// Износ тормозных колодок за 1 час
		double calculatePadWear(int brakesPerHour, double brakeTemp, double loadPercent) const;

		// Износ тормозных дисков за 1 час
		double calculateDiscWear(int brakesPerHour, double brakeTemp, double loadPercent) const;

		// Расчёт температуры тормозов после часа работы
		double calculateBrakeTemp(double loadPercent, int brakesPerHour, double currentTemp) const;

		// Комплексный прогноз: ресурс колодок, дисков, перегрев, рекомендация
		BrakePrediction predict(const BrakeState& state, double loadPercent) const;

		const BrakeConfig& config() const { return m_config; }
	};
}