#include "mas.h"
#include "imgui_app.h"
#include <memory>
#include <windows.h>
#include <algorithm>

using namespace mas;
using namespace mas::agent;

// Прототипы функций для окон
void ShowScenarioWindow(Scenario& s, CoordinatorAgent& coordinator, CombinedPrediction& result, bool& hasResult);
void ShowResultsWindow(const CombinedPrediction& result);
void ShowChartsWindow(const CombinedPrediction& result);
void ShowRoadComparisonWindow(const Scenario& s, CoordinatorAgent& coordinator);
void ShowAgentGraphWindow();

// Глобальные данные для графа
namespace GraphData 
{
    const int N = 8;
    float relX[N] = { 0.50f, 0.50f, 0.72f, 0.28f, 0.50f, 0.72f, 0.10f, 0.02f };
    float relY[N] = { 0.85f, 0.25f, 0.25f, 0.25f, 0.05f, 0.05f, 0.25f, 0.55f };
    const char* names[N] = { "Coordinator", "Engine", "Transmission", "Cooling", "Fuel", "Chassis", "Brake", "Tire" };
    ImVec4 colors[N] = 
    {
        ImVec4(1,0.8f,0.2f,1), ImVec4(0.2f,0.8f,0.2f,1), ImVec4(0.8f,0.6f,0.2f,1),
        ImVec4(0.3f,0.6f,1,1), ImVec4(0.8f,0.8f,0.2f,1), ImVec4(0.2f,0.4f,0.8f,1),
        ImVec4(0.8f,0.2f,0.2f,1), ImVec4(0.5f,0.5f,0.5f,1)
    };
    float animStart = -1.0f;
    int dragIdx = -1;
    bool dragStarted = false;
}

int main()
{
    setlocale(LC_ALL, "RUSSIAN");

    auto blackboard = std::make_shared<comm::Blackboard>();
    auto broker = std::make_shared<comm::MessageBroker>();
    broker->registerAgent("CoordinatorAgent");

    EngineAgent       engineAgent(broker, blackboard);
    TransmissionAgent transAgent(broker, blackboard);
    BrakeAgent        brakeAgent(broker, blackboard);
    ChassisAgent      chassisAgent(broker, blackboard);
    CoolingAgent      coolingAgent(broker, blackboard);
    FuelAgent         fuelAgent(broker, blackboard);
    TireAgent         tireAgent(broker, blackboard);
    CoordinatorAgent  coordinator(broker, blackboard);

    engineAgent.start(); transAgent.start(); brakeAgent.start();
    chassisAgent.start(); coolingAgent.start(); fuelAgent.start();
    tireAgent.start(); coordinator.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    ImGuiApp app("MAS-Predictor", 1280, 900);

    Scenario s;
    s.loadPercent = 70.0f; s.externalTemp = 25.0f; s.road = RoadType::Highway;
    s.gearShiftsPerHour = 10; s.brakesPerHour = 10;

    CombinedPrediction result;
    bool hasResult = false;

    while (!app.shouldClose())
    {
        app.beginFrame();

        ShowScenarioWindow(s, coordinator, result, hasResult);

        if (hasResult) 
            ShowResultsWindow(result);

        if (hasResult) 
            ShowChartsWindow(result);

        ShowRoadComparisonWindow(s, coordinator);
        ShowAgentGraphWindow();

        app.endFrame();
    }

    engineAgent.stop(); transAgent.stop(); brakeAgent.stop();
    chassisAgent.stop(); coolingAgent.stop(); fuelAgent.stop();
    tireAgent.stop(); coordinator.stop();
    return 0;
}

void ShowScenarioWindow(Scenario& s, CoordinatorAgent& coordinator, CombinedPrediction& result, bool& hasResult)
{
    float mainW = ImGui::GetIO().DisplaySize.x;
    float mainH = ImGui::GetIO().DisplaySize.y;

    const char* roadNames[] = { "Highway", "City", "Gravel", "Off-Road" };
    static int currentRoad = 0;

    ImGui::SetNextWindowPos(ImVec2(10, mainH * 0.5f + 10));
    ImGui::SetNextWindowSize(ImVec2(mainW * 0.50f - 10, mainH * 0.25f - 20));
    ImGui::Begin("Scenario");
    ImGui::SliderFloat("Load, %", &s.loadPercent, 10.0f, 100.0f);
    ImGui::SliderFloat("Temperature, C", &s.externalTemp, -10.0f, 45.0f);
    currentRoad = static_cast<int>(s.road);
    if (ImGui::Combo("Road", &currentRoad, roadNames, 4))
        s.road = static_cast<RoadType>(currentRoad);
    ImGui::SliderInt("Shifts/h", &s.gearShiftsPerHour, 1, 30);
    ImGui::SliderInt("Brakes/h", &s.brakesPerHour, 1, 30);
    if (ImGui::Button("Run Prediction", ImVec2(200, 40)))
    {
        result = coordinator.runPrediction(s);
        hasResult = true;
    }
    ImGui::End();
}

void ShowResultsWindow(const CombinedPrediction& result)
{
    float mainW = ImGui::GetIO().DisplaySize.x;
    float mainH = ImGui::GetIO().DisplaySize.y;

    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowSize(ImVec2(mainW * 0.50f - 10, mainH * 0.5f - 10));
    ImGui::Begin("Results");
    auto bar = [](const char* label, double val, double maxVal, ImVec4 color)
    {
        ImGui::Text("%s: %.0f h", label, val);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
        float frac = (float)(val / maxVal); if (frac > 1) frac = 1;
        ImGui::ProgressBar(frac, ImVec2(-1, 15), "");
        ImGui::PopStyleColor(); ImGui::Spacing();
    };
    double M = 30000.0;
    bar("Piston Rings  ", result.engine.pistonLife_hours, M, ImVec4(0.2f, 0.8f, 0.2f, 1));
    bar("Cylinders     ", result.engine.cylinderLife_hours, M, ImVec4(0.2f, 0.6f, 0.2f, 1));
    ImGui::Text("Power: %.0f kW | Fuel: %.1f L/h", result.engine.currentPower_kW, result.engine.fuelConsumption);
    ImGui::Separator();
    bar("Frictions     ", result.transmission.frictionLife_hours, M, ImVec4(0.8f, 0.6f, 0.2f, 1));
    bar("Gears         ", result.transmission.gearLife_hours, M, ImVec4(0.6f, 0.6f, 0.2f, 1));
    ImGui::Text("Trans Oil: %.0f h", result.transmission.oilLife_hours);
    ImGui::Separator();
    bar("Brake Pads    ", result.brakes.padLife_hours, 10000.0, ImVec4(0.8f, 0.2f, 0.2f, 1));
    bar("Brake Discs   ", result.brakes.discLife_hours, 20000.0, ImVec4(0.6f, 0.2f, 0.2f, 1));
    ImGui::Separator();
    bar("Shock Absorbers", result.chassis.shockerLife_hours, M, ImVec4(0.2f, 0.4f, 0.8f, 1));
    bar("Bushings      ", result.chassis.bushingLife_hours, M, ImVec4(0.2f, 0.3f, 0.6f, 1));
    bar("Springs       ", result.chassis.springLife_hours, 60000.0, ImVec4(0.2f, 0.2f, 0.5f, 1));
    ImGui::Separator();
    ImGui::Text("Coolant: %.0f C | FuelEff: %.0f%%", result.cooling.predictedCoolantTemp, result.fuel.efficiency);
    bar("Tires         ", result.tires.tireLife_hours, 10000.0, ImVec4(0.5f, 0.5f, 0.5f, 1));
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), ">>> Service in %.0f h <<<", result.minLifeHours);
    ImGui::End();
}

void ShowChartsWindow(const CombinedPrediction& result)
{
    float mainW = ImGui::GetIO().DisplaySize.x;
    float mainH = ImGui::GetIO().DisplaySize.y;

    ImGui::SetNextWindowPos(ImVec2(mainW * 0.50f + 10, 10));
    ImGui::SetNextWindowSize(ImVec2(mainW * 0.50f - 20, mainH * 0.5f - 10));
    ImGui::Begin("Charts");
    int steps = 50;
    float pw = (ImGui::GetContentRegionAvail().x - 10) / 2;
    float ph = ImGui::GetContentRegionAvail().y - 10;
    auto plotOil = [&]() 
    {
        float oilLife = (float)result.engine.oilLife_hours;
        if (oilLife < 100) oilLife = 500;
        float tot = oilLife * 1.5f;
        std::vector<float> h(steps), v(steps);
        for (int i = 0; i < steps; i++) 
        {
            h[i] = (float)i / (steps - 1) * tot;
            float q = 100 - (h[i] / oilLife) * 80;
            v[i] = q > 0 ? q : 0;
        }
        if (ImPlot::BeginPlot("Oil Degradation", ImVec2(pw, ph))) 
        {
            ImPlot::SetupAxes("Hours", "Quality, %");
            ImPlot::SetupAxisLimits(0, tot, 0, 105);
            ImPlot::PlotLine("Oil", h.data(), v.data(), steps);
            float cx[] = { 0, tot }, cy[] = { 20, 20 };
            ImPlot::PlotLine("Critical", cx, cy, 2);
            ImPlot::EndPlot();
        }
    };
    auto plotWear = [&]() 
    {
        float pistonLife = (float)result.engine.pistonLife_hours;
        float tot = pistonLife * 1.5f;
        std::vector<float> h(steps), v(steps);
        for (int i = 0; i < steps; i++) 
        {
            h[i] = (float)i / (steps - 1) * tot;
            float w = (h[i] / pistonLife) * 0.5f;
            v[i] = w < 0.5f ? w : 0.5f;
        }
        if (ImPlot::BeginPlot("Ring Wear", ImVec2(pw, ph))) 
        {
            ImPlot::SetupAxes("Hours", "Wear, mm");
            ImPlot::SetupAxisLimits(0, tot, 0, 0.55f);
            ImPlot::PlotLine("Wear", h.data(), v.data(), steps);
            float cx[] = { 0, tot }, cy[] = { 0.5f, 0.5f };
            ImPlot::PlotLine("Max", cx, cy, 2);
            ImPlot::EndPlot();
        }
    };
    plotOil(); ImGui::SameLine(); plotWear();
    ImGui::End();
}

void ShowRoadComparisonWindow(const Scenario& s, CoordinatorAgent& coordinator)
{
    float mainW = ImGui::GetIO().DisplaySize.x;
    float mainH = ImGui::GetIO().DisplaySize.y;

    //ImGui::SetNextWindowPos(ImVec2(10, 730), ImGuiCond_FirstUseEver);
    //ImGui::SetNextWindowSize(ImVec2(880, 180), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(10, mainH * 0.75f));
    ImGui::SetNextWindowSize(ImVec2(mainW * 0.50f - 10, mainH * 0.25f - 10));
    ImGui::Begin("Road Comparison");
    static bool done = false;
    static CombinedPrediction res[4];
    if (ImGui::Button("Compare All Roads", ImVec2(180, 30))) 
    {
        Scenario sc; sc.loadPercent = s.loadPercent; sc.externalTemp = s.externalTemp;
        sc.gearShiftsPerHour = s.gearShiftsPerHour; sc.brakesPerHour = s.brakesPerHour;
        RoadType roads[] = { RoadType::Highway, RoadType::City, RoadType::Gravel, RoadType::OffRoad };
        for (int i = 0; i < 4; i++) { sc.road = roads[i]; res[i] = coordinator.runPrediction(sc); }
        done = true;
    }
    if (done) 
    {
        ImGui::SameLine();
        ImGui::Text("Load: %.0f%%, Temp: %.0fC", s.loadPercent, s.externalTemp);
        if (ImGui::BeginTable("T", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) 
        {
            ImGui::TableSetupColumn("Component"); ImGui::TableSetupColumn("Highway");
            ImGui::TableSetupColumn("City"); ImGui::TableSetupColumn("Gravel"); ImGui::TableSetupColumn("Off-Road");
            ImGui::TableHeadersRow();
            auto row = [](const char* n, double v0, double v1, double v2, double v3) 
            {
                ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::Text("%s", n);
                ImGui::TableNextColumn(); ImGui::Text("%.0f", v0); ImGui::TableNextColumn(); ImGui::Text("%.0f", v1);
                ImGui::TableNextColumn(); ImGui::Text("%.0f", v2); ImGui::TableNextColumn(); ImGui::Text("%.0f", v3);
            };
            row("Engine Rings", res[0].engine.pistonLife_hours, res[1].engine.pistonLife_hours, res[2].engine.pistonLife_hours, res[3].engine.pistonLife_hours);
            row("Shocks", res[0].chassis.shockerLife_hours, res[1].chassis.shockerLife_hours, res[2].chassis.shockerLife_hours, res[3].chassis.shockerLife_hours);
            row("Tires", res[0].tires.tireLife_hours, res[1].tires.tireLife_hours, res[2].tires.tireLife_hours, res[3].tires.tireLife_hours);
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

void ShowAgentGraphWindow()
{
    float mainW = ImGui::GetIO().DisplaySize.x;
    float mainH = ImGui::GetIO().DisplaySize.y;

    using namespace GraphData;
    ImGui::SetNextWindowPos(ImVec2(mainW * 0.50f + 10, mainH * 0.5f + 10));
    ImGui::SetNextWindowSize(ImVec2(mainW * 0.50f - 20, mainH * 0.5f - 20));
    ImGui::Begin("Agent Graph", nullptr, ImGuiWindowFlags_NoMove);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 canvas = ImGui::GetCursorScreenPos();
    float winW = ImGui::GetContentRegionAvail().x, winH = ImGui::GetContentRegionAvail().y - 35;
    ImGui::Dummy(ImVec2(winW, winH));
    float radius = winW * 0.022f; if (radius < 10) radius = 10; if (radius > 20) radius = 20;
    ImVec2 pos[N];
    for (int i = 0; i < N; i++) { pos[i].x = canvas.x + relX[i] * winW; pos[i].y = canvas.y + relY[i] * winH; }

    // Drag
    ImVec2 mouse = ImGui::GetMousePos();
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
        for (int i = 0; i < N; i++) {
            float d = sqrtf((mouse.x - pos[i].x) * (mouse.x - pos[i].x) + (mouse.y - pos[i].y) * (mouse.y - pos[i].y));
            if (d < radius + 6) { dragIdx = i; dragStarted = true; break; }
        }
    }
    if (dragStarted && ImGui::IsMouseDragging(0) && dragIdx >= 0) {
        pos[dragIdx].x += ImGui::GetIO().MouseDelta.x; pos[dragIdx].y += ImGui::GetIO().MouseDelta.y;
        pos[dragIdx].x = max(canvas.x + radius, min(pos[dragIdx].x, canvas.x + winW - radius));
        pos[dragIdx].y = max(canvas.y + radius, min(pos[dragIdx].y, canvas.y + winH - radius));
        relX[dragIdx] = (pos[dragIdx].x - canvas.x) / winW; relY[dragIdx] = (pos[dragIdx].y - canvas.y) / winH;
    }
    if (ImGui::IsMouseReleased(0)) { dragIdx = -1; dragStarted = false; }

    // Animation
    if (ImGui::Button("Animation")) animStart = (float)ImGui::GetTime();
    ImGui::SameLine(); ImGui::Text("(agent communication phases)");

    float ph1 = 0, ph2a = 0, ph2b = 0, ph2c = 0, ph2d = 0, ph2e = 0;
    float resFuel = 0, resChassis = 0, resTire = 0, resBrake = 0, resCooling = 0, resEngine = 0, resTrans = 0;

    auto cl = [](float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); };

    if (animStart >= 0) {
        float el = (float)ImGui::GetTime() - animStart;
        float totalDur = 8.0f;
        if (el > totalDur) { el = totalDur; animStart = -1.0f; }

        // Фаза 1: 0.0 - 2.0 (Coordinator → все)
        if (el < 2.0f) ph1 = (el < 1.0f) ? el : (2.0f - el);

        // Фаза 2a: 2.0 - 2.7 (Tire→Brake, Chassis→Engine, Fuel→Engine)
        if (el > 2.0f && el < 2.7f) {
            float t = el - 2.0f;
            ph2a = (t < 0.35f) ? t / 0.35f : (0.7f - t) / 0.35f;
        }
        // Fuel, Chassis, Tire — получили всё → 0.5 сек расчёт → ответ Coordinator
        {
            float start = 2.0f, delay = 0.5f;
            if (el > start + delay) {
                float t = el - start - delay;
                float fade = (t < 0.3f) ? t / 0.3f : 1.0f;
                resFuel = cl(fade); resChassis = cl(fade); resTire = cl(fade);
            }
        }

        // Фаза 2b: 2.7 - 3.4 (Brake→Cooling)
        if (el > 2.7f && el < 3.4f) {
            float t = el - 2.7f;
            ph2b = (t < 0.35f) ? t / 0.35f : (0.7f - t) / 0.35f;
        }
        // Brake — получил → 0.5 сек расчёт → ответ Coordinator
        {
            float start = 2.7f, delay = 0.5f;
            if (el > start + delay) {
                float t = el - start - delay;
                float fade = (t < 0.3f) ? t / 0.3f : 1.0f;
                resBrake = cl(fade);
            }
        }

        // Фаза 2c: 3.4 - 4.1 (Cooling→Engine, Cooling→Transmission)
        if (el > 3.4f && el < 4.1f) {
            float t = el - 3.4f;
            ph2c = (t < 0.35f) ? t / 0.35f : (0.7f - t) / 0.35f;
        }
        // Cooling — получил → 0.5 сек расчёт → ответ Coordinator
        {
            float start = 3.4f, delay = 0.5f;
            if (el > start + delay) {
                float t = el - start - delay;
                float fade = (t < 0.3f) ? t / 0.3f : 1.0f;
                resCooling = cl(fade);
            }
        }

        // Фаза 2d: 4.1 - 4.8 (Engine→Transmission)
        if (el > 4.1f && el < 4.8f) {
            float t = el - 4.1f;
            ph2d = (t < 0.35f) ? t / 0.35f : (0.7f - t) / 0.35f;
        }

        // Фаза 2e: 4.8 - 5.5 (Transmission→Engine — tKPP)
        if (el > 4.8f && el < 5.5f) {
            float t = el - 4.8f;
            ph2e = (t < 0.35f) ? t / 0.35f : (0.7f - t) / 0.35f;
        }
        // Transmission → ответ Coordinator (после общения)
        {
            float start = 4.8f, delay = 0.5f;
            if (el > start + delay) {
                float t = el - start - delay;
                float fade = (t < 0.3f) ? t / 0.3f : 1.0f;
                resTrans = cl(fade);
            }
        }
        // Engine → ждёт tKPP от Transmission (5.3с) → 0.5 сек расчёт → ответ Coordinator
        {
            float start = 5.3f, delay = 0.5f;
            if (el > start + delay) {
                float t = el - start - delay;
                float fade = (t < 0.3f) ? t / 0.3f : 1.0f;
                resEngine = cl(fade);
            }
        }

        // Общее затухание всех res-стрелок после 6.5 сек
        if (el > 6.5f) {
            float fade = cl((7.5f - el) / 1.0f);
            resFuel *= fade; resChassis *= fade; resTire *= fade;
            resBrake *= fade; resCooling *= fade; resEngine *= fade; resTrans *= fade;
        }
    }

    struct Arrow { int from, to; const char* label; ImVec4 color; float phase; };
    Arrow arrows[] = {
        // Фаза 1
        {0,1,"req",ImVec4(1,0.8f,0.2f,1),ph1},{0,2,"req",ImVec4(1,0.8f,0.2f,1),ph1},{0,3,"req",ImVec4(1,0.8f,0.2f,1),ph1},
        {0,4,"req",ImVec4(1,0.8f,0.2f,1),ph1},{0,5,"req",ImVec4(1,0.8f,0.2f,1),ph1},{0,6,"req",ImVec4(1,0.8f,0.2f,1),ph1},
        {0,7,"req",ImVec4(1,0.8f,0.2f,1),ph1},
        // Фаза 2a
        {7,6,"wear",ImVec4(0.5f,0.5f,0.5f,1),ph2a},{5,1,"vibro",ImVec4(0.5f,0.3f,1,1),ph2a},{4,1,"KPD",ImVec4(1,0.8f,0.2f,1),ph2a},
        // Фаза 2b
        {6,3,"heat",ImVec4(1,0.4f,0.2f,1),ph2b},
        // Фаза 2c
        {3,1,"tC",ImVec4(0.3f,0.6f,1,1),ph2c},{3,2,"tC",ImVec4(0.3f,0.6f,1,1),ph2c},
        // Фаза 2d
        {1,2,"heat",ImVec4(1,0.4f,0.2f,1),ph2d},
        // Фаза 2e
        {2,1,"tKPP",ImVec4(1,0.5f,0,1),ph2e},
        // Ответы Coordinator (появляются когда агент готов)
        {4,0,"res",ImVec4(0.5f,1,0.5f,1),resFuel},
        {5,0,"res",ImVec4(0.5f,1,0.5f,1),resChassis},
        {7,0,"res",ImVec4(0.5f,1,0.5f,1),resTire},
        {6,0,"res",ImVec4(0.5f,1,0.5f,1),resBrake},
        {3,0,"res",ImVec4(0.5f,1,0.5f,1),resCooling},
        {1,0,"res",ImVec4(0.5f,1,0.5f,1),resEngine},
        {2,0,"res",ImVec4(0.5f,1,0.5f,1),resTrans},
    };

    // Отрисовка стрелок
    for (auto& a : arrows) {
        if (a.phase <= 0) continue;
        ImVec2 from = pos[a.from], to = pos[a.to];
        float dx = to.x - from.x, dy = to.y - from.y, len = sqrtf(dx * dx + dy * dy);
        if (len < 1) continue;
        float ux = dx / len, uy = dy / len, nx = -uy, ny = ux;
        float shift = ((a.from == 1 && a.to == 2) || (a.from == 2 && a.to == 1)) ? 15.0f : 0;
        ImVec2 fromS(from.x + ux * radius + nx * shift, from.y + uy * radius + ny * shift);
        ImVec2 toS(to.x - ux * radius + nx * shift, to.y - uy * radius + ny * shift);
        float alpha = a.phase;
        ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(a.color.x, a.color.y, a.color.z, 0.2f + 0.6f * alpha));
        draw->AddLine(fromS, toS, col, 2.0f + alpha * 2.0f);
        float al = radius * 0.8f;
        ImVec2 tip = toS, left(tip.x - ux * al + nx * al * 0.5f, tip.y - uy * al + ny * al * 0.5f), right(tip.x - ux * al - nx * al * 0.5f, tip.y - uy * al - ny * al * 0.5f);
        draw->AddTriangleFilled(tip, left, right, col);
        float ls = shift * 0.6f;
        ImVec2 mid((fromS.x + toS.x) / 2 + nx * (12 + ls), (fromS.y + toS.y) / 2 + ny * (12 + ls) - 4);
        draw->AddText(mid, ImGui::ColorConvertFloat4ToU32(ImVec4(a.color.x, a.color.y, a.color.z, 0.5f + 0.5f * alpha)), a.label);
    }

    // Круги
    for (int i = 0; i < N; i++) {
        ImU32 col = ImGui::ColorConvertFloat4ToU32(colors[i]);
        ImU32 border = (dragIdx == i) ? ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, 0.9f)) : ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, 0.3f));
        draw->AddCircleFilled(pos[i], radius, col); draw->AddCircle(pos[i], radius, border, 0, 2.0f);
        float tw = ImGui::CalcTextSize(names[i]).x;
        draw->AddText(ImVec2(pos[i].x - tw / 2, pos[i].y + radius + 4), ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, 1)), names[i]);
    }

    ImGui::SetCursorPos(ImVec2(10, winH + 8));
    if (ImGui::Button("Reset Positions")) {
        float dx[N] = { 0.5f,0.5f,0.72f,0.28f,0.5f,0.72f,0.1f,0.02f }, dy[N] = { 0.85f,0.25f,0.25f,0.25f,0.05f,0.05f,0.25f,0.55f };
        for (int i = 0; i < N; i++) { relX[i] = dx[i]; relY[i] = dy[i]; }
    }
    ImGui::End();
}