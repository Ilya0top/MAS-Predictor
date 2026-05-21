#pragma once
#include <string>
#include "./../common/Types.h"

namespace mas::model
{
	struct ChassisConfig
	{
		double maxShockerWear = 100.0;		// предельный износ амортизаторов, %
		double maxBushingWear = 5.0;		// предельный износ сайлентблоков, мм
		double maxSpringWear = 100.0;		// предельная усталость пружин, %
		double maxShockerLife = 8000.0;		// ресурс амортизаторов, часы
		double maxBushingLife = 10000.0;	// ресурс сайлентблоков, часы
		double maxSpringLife = 20000.0;		// ресурс пружин, часы
	};

	struct ChassisState
	{
		double shockerWear = 0.0;			// износ амортизаторов, %
		double bushingWear = 0.0;			// износ сайлентблоков, мм
		double springWear = 0.0;			// усталость пружин, %
		double loadPercent = 50.0;			// загрузка машины, %
	};

	struct ChassisPrediction
	{
		double shockerLife_hours;			// остаточный ресурс амортизаторов, часы
		double bushingLife_hours;			// остаточный ресурс сайлентблоков, часы
		double springLife_hours;			// остаточный ресурс пружин, часы
		bool   isCritical;					// критическое состояние? (износ >= 80% любого элемента)
		std::string recommendation;			// текстовая рекомендация по ТО
	};

	class ChassisModel
	{
	private:
		ChassisConfig m_config;
		bool m_verbose;

		// Коэффициент влияния типа дороги на износ
		double roadFactor(RoadType road) const;

		void log(const std::string& message) const;
	public:

		explicit ChassisModel(const ChassisConfig& config = ChassisConfig{}, bool verbose = false);

		// Износ амортизаторов за 1 час, %
		double calculateShockerWear(double loadPercent, RoadType road) const;

		// Износ сайлентблоков за 1 час, мм
		double calculateBushingWear(double loadPercent, RoadType road) const;

		// Усталость пружин за 1 час, %
		double calculateSpringWear(double loadPercent, RoadType road) const;

		// Комплексный прогноз: ресурс амортизаторов, сайлентблоков, пружин, рекомендация
		ChassisPrediction predict(const ChassisState& state, double loadPercent, RoadType road) const;

		const ChassisConfig& config() const { return m_config; }
	};
}