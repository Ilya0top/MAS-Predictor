#include "FuelSystemModel.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <sstream>

namespace mas::model
{
	FuelSystemModel::FuelSystemModel(const FuelSystemConfig& config, bool verbose): m_config(config), m_verbose(verbose)
	{
	}

	void FuelSystemModel::log(const std::string& message) const
	{
		if (m_verbose)
			std::cout << "  [FUEL_VERBOSE] " << message << std::endl;
	}

	double FuelSystemModel::calculateInjectorWear(double loadPercent) const
	{
		double baseRate = m_config.maxInjectorWear / m_config.maxInjectorLife;
		double loadF = std::pow(loadPercent / 100.0, 1.2);

		double wear = baseRate * loadF;

		log("InjectorWear: load=" + std::to_string(loadPercent) +
			" baseRate=" + std::to_string(baseRate) +
			" loadF=" + std::to_string(loadF) +
			" -> " + std::to_string(wear) + " %/ч");

		return wear;
	}

	double FuelSystemModel::calculateFilterClogRate(double loadPercent) const
	{
		double baseRate = m_config.maxFilterClog / m_config.filterChangeInterval;
		double loadF = loadPercent / 100.0;

		double rate = baseRate * loadF;

		log("FilterClog: load=" + std::to_string(loadPercent) +
			" baseRate=" + std::to_string(baseRate) +
			" loadF=" + std::to_string(loadF) +
			" -> " + std::to_string(rate) + " %/ч");

		return rate;
	}

	double FuelSystemModel::calculateEfficiency(double injectorWear, double filterClog) const
	{
		double injectorEff = 1.0 - (injectorWear / m_config.maxInjectorWear) * 0.12;
		double filterEff = 1.0 - (filterClog / m_config.maxFilterClog) * 0.08;
		double efficiency = injectorEff * filterEff * 100.0;

		efficiency = std::clamp(efficiency, 50.0, 100.0);

		log("Efficiency: injWear=" + std::to_string(injectorWear) +
			" filtClog=" + std::to_string(filterClog) +
			" injEff=" + std::to_string(injectorEff) +
			" filtEff=" + std::to_string(filterEff) +
			" -> " + std::to_string(efficiency) + " %");

		return efficiency;
	}

	FuelSystemPrediction FuelSystemModel::predict(const FuelSystemState& state, double loadPercent) const
	{
		FuelSystemPrediction result{};

		double injectorRate = calculateInjectorWear(loadPercent);
		double filterRate = calculateFilterClogRate(loadPercent);

		result.efficiency = calculateEfficiency(state.injectorWear, state.filterClog);

		double injectorRemaining = m_config.maxInjectorWear - state.injectorWear;
		double filterRemaining = m_config.maxFilterClog - state.filterClog;

		result.injectorLife_hours = (injectorRate > 0.0) ? injectorRemaining / injectorRate : 1e9;
		result.filterLife_hours = (filterRate > 0.0) ? filterRemaining / filterRate : 1e9;

		result.injectorLife_hours = std::max(0.0, result.injectorLife_hours);
		result.filterLife_hours = std::max(0.0, result.filterLife_hours);

		double injRatio = state.injectorWear / m_config.maxInjectorWear;
		double filtRatio = state.filterClog / m_config.maxFilterClog;
		result.isCritical = (injRatio >= 0.8) || (filtRatio >= 0.85);

		std::ostringstream rec;
		if (result.isCritical)
			rec << "КРИТИЧЕСКОЕ СОСТОЯНИЕ! ";

		if (result.filterLife_hours < 100.0)
			rec << "Замена топливного фильтра через " << static_cast<int>(result.filterLife_hours) << " ч. ";

		if (result.injectorLife_hours < 400.0)
			rec << "Чистка/замена форсунок через " << static_cast<int>(result.injectorLife_hours) << " ч. ";

		if (rec.str().empty())
		{
			double minLife = std::min(result.injectorLife_hours, result.filterLife_hours);
			rec << "Топливная система в норме. КПД=" << static_cast<int>(result.efficiency) << "%. Ближайшее ТО через " << static_cast<int>(minLife) << " ч.";
		}
		result.recommendation = rec.str();

		log("PREDICT: efficiency=" + std::to_string(result.efficiency) +
			" injectorLife=" + std::to_string(result.injectorLife_hours) +
			" filterLife=" + std::to_string(result.filterLife_hours) +
			" critical=" + std::string(result.isCritical ? "YES" : "NO"));

		return result;
	}
}