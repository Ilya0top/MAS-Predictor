#pragma once
#include <string>

namespace mas::model
{
	// дизельный двигатель 150 кВт (+- 200 л.с.)
	struct EngineConfig
	{
		double maxWear = 0.5;				// предельный износ, мм
		double maxLifeHours = 15000.0;		// ресурс при 100% нагрузке, часы
		double maxOilTemp = 150.0;			// предельная температура масла, °C
		double nominalPower_kW = 150.0;		// номинальная мощность, кВт
		double fuelConsumptionBase = 0.22;  // удельный расход топлива, л/кВт*ч
	};

	struct EngineState
	{
		double pistonRingWear = 0.0;		// износ колец, мм
		double cylinderWear = 0.0;			// износ цилиндров, мм
		double oilTemperature = 90.0;		// температура масла, °C
		double oilQuality = 100.0;			// качество масла, %
	};

	struct EnginePrediction
	{
		double pistonLife_hours;			// остаточный ресурс колец
		double cylinderLife_hours;			// остаточный ресурс цилиндров
		double oilLife_hours;				// через сколько замена масла
		double currentPower_kW;				// текущая мощность
		double powerPercent;				// мощность в % от номинала
		double fuelConsumption;				// расход топлива, л/ч
		double heatEmission_kW;				// тепловыделение, кВт
		bool   isCritical;					// критическое состояние?
		std::string recommendation;			// текстовая рекомендация
	};

	class EngineModel
	{
	private:
		EngineConfig m_config;
		bool m_verbose;

		double temperatureFactor(double oilTemp) const;
		double oilQualityFactor(double oilQuality) const;
		double loadFactor(double loadPercent) const;
		void log(const std::string& message) const;
	public:

		EngineModel(const EngineConfig& config = EngineConfig{}, bool verbose = false);

		// Износ поршневых колец за 1 час
		double calculatePistonWear(double loadPercent, double oilTemp, double oilQuality) const;

		// Износ цилиндров за 1 час
		double calculateCylinderWear(double loadPercent, double oilTemp, double oilQuality) const;

		// Деградация масла за 1 час работы
		double calculateOilDegradation(double oilTemp, double loadPercent) const;

		// Текущая мощность
		double calculateCurrentPower(double pistonWear, double cylinderWear, double fuelEfficiency = 100) const;

		// Расход топлива
		double calculateFuelConsumption(double loadPercent, double currentPower_kW) const;

		// Тепловыделение
		double calculateHeatEmission(double loadPercent, double currentPower_kW) const;

		// Комплексный прогноз 
		EnginePrediction predict(const EngineState& state, double loadPercent, double fuelEfficiency = 100) const;

		const EngineConfig& config() const { return m_config; }
	};
}