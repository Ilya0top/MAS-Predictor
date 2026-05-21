#pragma once
#include <string>

namespace mas::model
{
	struct TransmissionConfig
	{
		double maxFrictionWear = 0.8;		// предельный износ фрикционов, мм
		double maxGearWear = 0.3;			// предельный износ шестерён, мм
		double maxLifeHours = 20000.0;		// ресурс при 100% нагрузке
		double maxOilTemp = 110.0;			// предельная температура масла КПП
	};

	struct TransmissionState
	{
		double frictionWear = 0.0;			// износ фрикционов, мм
		double gearWear = 0.0;				// износ шестерён, мм
		double oilTemperature = 80.0;		// температура масла КПП, °C
		double oilQuality = 100.0;			// качество масла КПП, %
	};

	struct TransmissionPrediction 
	{
		double frictionLife_hours;			// остаточный ресурс фрикционов
		double gearLife_hours;				// остаточный ресурс шестерён
		double oilLife_hours;				// через сколько замена масла
		bool   isCritical;					// критическое состояние?
		std::string recommendation;			// текстовая рекомендация
	};

	class TransmissionModel
	{
	private:
		TransmissionConfig m_config;
		bool m_verbose;

		void log(const std::string& message) const;
	public:

		TransmissionModel(const TransmissionConfig& config = TransmissionConfig{}, bool verbose = false);

		// Износ фрикционов за 1 час
		double calculateFrictionWear(double loadPercent, double oilTemp, double oilQuality, int gearShiftsPerHour) const;

		// Износ шестерён за 1 час
		double calculateGearWear(double loadPercent, double oilTemp, double oilQuality) const;

		// Деградация масла КПП за 1 час, %/ч
		double calculateOilDegradation(double oilTemp, double loadPercent) const;

		// Комплексный прогноз
		TransmissionPrediction predict(const TransmissionState& state, double loadPercent, int gearShiftsPerHour) const;

		const TransmissionConfig& config() const { return m_config; }
	};
}