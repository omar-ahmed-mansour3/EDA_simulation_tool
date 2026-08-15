#include "WaveformGUI.hpp"
#include "raylib.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <thread>
#include <mutex>
#include <atomic>

namespace {

// Palette definition for sleek modern dark mode
const Color COLOR_BG           = Color{ 20, 22, 26, 255 };  // Main background
const Color COLOR_PANEL        = Color{ 28, 30, 36, 255 };  // Left sidebar & top toolbar
const Color COLOR_GRID         = Color{ 40, 44, 52, 255 };  // Time grid lines
const Color COLOR_BORDER       = Color{ 50, 54, 64, 255 };  // Borders / Dividers
const Color COLOR_TEXT_MAIN    = Color{ 230, 235, 245, 255 };// Primary text
const Color COLOR_TEXT_MUTED   = Color{ 140, 145, 160, 255 };// Secondary labels

// 4-State Logic Colors according to IEEE Verilog standard
const Color COLOR_STATE_ONE    = Color{ 0, 230, 118, 255 }; // Bright Neon Green
const Color COLOR_STATE_ZERO   = Color{ 220, 225, 235, 255 };// Clean White / Light Gray
const Color COLOR_STATE_X      = Color{ 255, 61, 0, 255 };  // Crimson Red
const Color COLOR_STATE_X_FILL = Color{ 255, 61, 0, 60 };   // Translucent Red Box
const Color COLOR_STATE_Z      = Color{ 255, 193, 7, 255 };  // Amber Yellow

struct GuiLog {
    std::string text;
    Color color;
};

// Custom stdout/stderr interceptor for catching IOController output
class OutputLogger {
public:
    static std::vector<GuiLog>& getLogs() {
        static std::vector<GuiLog> logs;
        return logs;
    }

    static void log(const std::string& msg, bool is_error = false) {
        Color c = is_error ? Color{ 255, 100, 100, 255 } : Color{ 160, 230, 160, 255 };
        getLogs().push_back({ msg, c });
        if (getLogs().size() > 50) {
            getLogs().erase(getLogs().begin());
        }
    }
};

} // namespace

void WaveformGUI::runApplication(SimEngine* engine) {
    if (!engine) {
        std::cerr << "[WaveformGUI] Error: SimEngine pointer is null.\n";
        return;
    }

    const int initialWidth = 1280;
    const int initialHeight = 720;
    
    // Set config flags for resizable window
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(initialWidth, initialHeight, "EDA Logic Simulation Engine - Waveform GUI");
    SetTargetFPS(60);

    // Viewport & Pan/Zoom State
    float zoom_factor = 20.0f; // Pixels per nanosecond
    float pan_offset  = 30.0f; // Horizontal pixel offset
    float scroll_y    = 0.0f;  // Vertical lane scroll offset

    // Simulation controls
    bool is_simulating = false;
    float sim_accumulator = 0.0f;
    uint64_t sim_step_speed = 5; // ns per step

    // UI Input box state
    char input_buffer[256] = "";
    int input_length = 0;
    bool input_active = true;

    // Background thread for terminal CMD inputs
    std::atomic<bool> thread_running{true};
    std::vector<std::string> pending_cli_commands;
    std::mutex cli_mutex;

    std::thread cin_thread([&]() {
        std::string line;
        while (thread_running) {
            if (std::getline(std::cin, line)) {
                if (!line.empty()) {
                    std::lock_guard<std::mutex> lock(cli_mutex);
                    pending_cli_commands.push_back(line);
                }
            } else {
                break;
            }
        }
    });

    OutputLogger::log("System Ready. Enter commands in CMD or GUI text box (e.g. 'set a 1 at 5').");

    while (!WindowShouldClose()) {
        // Process any commands entered from Windows CMD terminal
        {
            std::vector<std::string> cmds;
            {
                std::lock_guard<std::mutex> lock(cli_mutex);
                cmds = pending_cli_commands;
                pending_cli_commands.clear();
            }
            for (const auto& cmd : cmds) {
                OutputLogger::log("[CMD] > " + cmd, false);
                std::ostringstream buffer;
                std::streambuf* old_cout = std::cout.rdbuf(buffer.rdbuf());
                std::streambuf* old_cerr = std::cerr.rdbuf(buffer.rdbuf());

                IOController::executeCommand(cmd, *engine);

                std::cout.rdbuf(old_cout);
                std::cerr.rdbuf(old_cerr);

                std::string out = buffer.str();
                if (!out.empty()) {
                    while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
                    OutputLogger::log(out, out.find("Error") != std::string::npos || out.find("Warning") != std::string::npos);
                }
            }
        }
        int screenWidth  = GetScreenWidth();
        int screenHeight = GetScreenHeight();

        const float topToolbarHeight = 45.0f;
        const float timeRulerHeight  = 35.0f;
        const float leftSidebarWidth = 180.0f;
        const float bottomConsoleH  = 150.0f;

        float waveformAreaTop    = topToolbarHeight + timeRulerHeight;
        float waveformAreaBottom = screenHeight - bottomConsoleH;
        float waveformAreaHeight = waveformAreaBottom - waveformAreaTop;
        float waveformAreaWidth  = screenWidth - leftSidebarWidth;

        // -------------------------------------------------------------------
        // A. Process User Input & Controls
        // -------------------------------------------------------------------

        // Mouse Wheel Zooming
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            Vector2 mousePos = GetMousePosition();
            if (mousePos.x > leftSidebarWidth && mousePos.y > topToolbarHeight && mousePos.y < waveformAreaBottom) {
                float mouseTime = (mousePos.x - leftSidebarWidth - pan_offset) / zoom_factor;
                zoom_factor += wheel * 2.5f;
                if (zoom_factor < 2.0f)  zoom_factor = 2.0f;
                if (zoom_factor > 150.0f) zoom_factor = 150.0f;
                // Keep time under mouse fixed
                pan_offset = (mousePos.x - leftSidebarWidth) - (mouseTime * zoom_factor);
            } else if (mousePos.x <= leftSidebarWidth) {
                scroll_y += wheel * 20.0f;
                if (scroll_y > 0.0f) scroll_y = 0.0f;
            }
        }

        // Mouse Drag Panning
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 mousePos = GetMousePosition();
            if (mousePos.x > leftSidebarWidth && mousePos.y > topToolbarHeight && mousePos.y < waveformAreaBottom) {
                Vector2 delta = GetMouseDelta();
                pan_offset += delta.x;
                scroll_y   += delta.y;
                if (scroll_y > 0.0f) scroll_y = 0.0f;
            }
        }

        // Keyboard navigation
        if (IsKeyDown(KEY_RIGHT)) pan_offset -= 10.0f;
        if (IsKeyDown(KEY_LEFT))  pan_offset += 10.0f;

        // Keyboard Text Input for CLI Box
        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= 32) && (key <= 125) && (input_length < 255)) {
                input_buffer[input_length] = (char)key;
                input_buffer[input_length + 1] = '\0';
                input_length++;
            }
            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE)) {
            input_length--;
            if (input_length < 0) input_length = 0;
            input_buffer[input_length] = '\0';
        }

        if (IsKeyPressed(KEY_ENTER) && input_length > 0) {
            std::string cmd(input_buffer);
            OutputLogger::log("> " + cmd, false);

            // Redirect stdout to capture IOController messages
            std::ostringstream buffer;
            std::streambuf* old_cout = std::cout.rdbuf(buffer.rdbuf());
            std::streambuf* old_cerr = std::cerr.rdbuf(buffer.rdbuf());

            IOController::executeCommand(cmd, *engine);

            std::cout.rdbuf(old_cout);
            std::cerr.rdbuf(old_cerr);

            std::string out = buffer.str();
            if (!out.empty()) {
                // Strip trailing newline
                while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
                OutputLogger::log(out, out.find("Error") != std::string::npos || out.find("Warning") != std::string::npos);
            }

            input_buffer[0] = '\0';
            input_length = 0;
        }


        // -------------------------------------------------------------------
        // B. Simulation Step Execution
        // -------------------------------------------------------------------
        if (is_simulating) {
            sim_accumulator += GetFrameTime();
            if (sim_accumulator >= 0.05f) { // 20 steps per second
                uint64_t target = engine->getCurrentTime() + sim_step_speed;
                engine->stepTo(target);
                sim_accumulator = 0.0f;
            }
        }

        // Gather all Wires from Netlist & History
        std::vector<Wire*> display_wires;
        Netlist* netlist = engine->getNetlist();
        if (netlist) {
            for (const auto& w_ptr : netlist->wires) {
                display_wires.push_back(w_ptr.get());
            }
        } else {
            // Fallback to history wires
            for (const auto& ev : engine->getHistory()) {
                if (ev.wire && std::find(display_wires.begin(), display_wires.end(), ev.wire) == display_wires.end()) {
                    display_wires.push_back(ev.wire);
                }
            }
        }

        // -------------------------------------------------------------------
        // C. Render GUI
        // -------------------------------------------------------------------
        BeginDrawing();
        ClearBackground(COLOR_BG);

        uint64_t current_time = engine->getCurrentTime();

        // --- 1. Draw Waveform Grid & Time Ruler ---
        float gridStepNs = 10.0f;
        if (zoom_factor < 5.0f)   gridStepNs = 50.0f;
        if (zoom_factor > 40.0f)  gridStepNs = 2.0f;
        if (zoom_factor > 80.0f)  gridStepNs = 1.0f;

        float startNs = std::max(0.0f, (-pan_offset) / zoom_factor);
        float endNs   = (waveformAreaWidth - pan_offset) / zoom_factor;

        // Background for Time Ruler
        DrawRectangle(leftSidebarWidth, topToolbarHeight, waveformAreaWidth, timeRulerHeight, COLOR_PANEL);
        DrawLine(leftSidebarWidth, topToolbarHeight + timeRulerHeight, screenWidth, topToolbarHeight + timeRulerHeight, COLOR_BORDER);

        for (float t = std::floor(startNs / gridStepNs) * gridStepNs; t <= endNs + gridStepNs; t += gridStepNs) {
            float posX = leftSidebarWidth + (t * zoom_factor) + pan_offset;
            if (posX >= leftSidebarWidth && posX <= screenWidth) {
                // Vertical Grid line
                DrawLine(posX, waveformAreaTop, posX, waveformAreaBottom, COLOR_GRID);

                // Time Ruler Tick & Text Label
                DrawLine(posX, topToolbarHeight + timeRulerHeight - 8, posX, topToolbarHeight + timeRulerHeight, COLOR_TEXT_MUTED);
                std::string timeLabel = std::to_string((int)t) + "ns";
                DrawText(timeLabel.c_str(), posX + 4, topToolbarHeight + 10, 11, COLOR_TEXT_MUTED);
            }
        }

        // --- 2. Render Waveforms per Wire ---
        float laneHeight = 55.0f;
        float currentY = waveformAreaTop + 10.0f + scroll_y;

        // Scissor box for waveforms to prevent rendering outside viewport
        BeginScissorMode(leftSidebarWidth, waveformAreaTop, waveformAreaWidth, waveformAreaHeight);

        const auto& history = engine->getHistory();

        for (size_t i = 0; i < display_wires.size(); ++i) {
            Wire* w = display_wires[i];
            if (currentY + laneHeight >= waveformAreaTop && currentY <= waveformAreaBottom) {
                float yHigh = currentY + 12.0f;
                float yLow  = currentY + 38.0f;
                float yMid  = currentY + 25.0f;

                // Extract history for this specific wire
                std::vector<Event> wire_events;
                for (const auto& ev : history) {
                    if (ev.wire == w) {
                        wire_events.push_back(ev);
                    }
                }
                std::sort(wire_events.begin(), wire_events.end(), [](const Event& a, const Event& b) {
                    if (a.timestamp != b.timestamp) return a.timestamp < b.timestamp;
                    return a.sequence_id < b.sequence_id;
                });

                // Initial state before first event at t=0
                LogicState active_state = LogicState::X;
                uint64_t last_t = 0;

                auto renderSegment = [&](uint64_t t1, uint64_t t2, LogicState state) {
                    float x1 = leftSidebarWidth + (t1 * zoom_factor) + pan_offset;
                    float x2 = leftSidebarWidth + (t2 * zoom_factor) + pan_offset;

                    if (x2 < leftSidebarWidth || x1 > screenWidth) return;

                    switch (state) {
                        case LogicState::ONE:
                            DrawLineEx(Vector2{ x1, yHigh }, Vector2{ x2, yHigh }, 2.5f, COLOR_STATE_ONE);
                            break;
                        case LogicState::ZERO:
                            DrawLineEx(Vector2{ x1, yLow }, Vector2{ x2, yLow }, 2.5f, COLOR_STATE_ZERO);
                            break;
                        case LogicState::Z:
                            DrawLineEx(Vector2{ x1, yMid }, Vector2{ x2, yMid }, 1.5f, COLOR_STATE_Z);
                            break;
                        case LogicState::X:
                        default:
                            DrawRectangleRec(Rectangle{ x1, yHigh, x2 - x1, yLow - yHigh }, COLOR_STATE_X_FILL);
                            DrawLineEx(Vector2{ x1, yHigh }, Vector2{ x2, yHigh }, 1.5f, COLOR_STATE_X);
                            DrawLineEx(Vector2{ x1, yLow }, Vector2{ x2, yLow }, 1.5f, COLOR_STATE_X);
                            break;
                    }
                };

                auto renderTransition = [&](uint64_t t, LogicState oldState, LogicState newState) {
                    float x = leftSidebarWidth + (t * zoom_factor) + pan_offset;
                    if (x < leftSidebarWidth || x > screenWidth) return;

                    float y1 = (oldState == LogicState::ONE) ? yHigh : ((oldState == LogicState::ZERO) ? yLow : yMid);
                    float y2 = (newState == LogicState::ONE) ? yHigh : ((newState == LogicState::ZERO) ? yLow : yMid);

                    Color transColor = (newState == LogicState::ONE) ? COLOR_STATE_ONE :
                                       (newState == LogicState::ZERO) ? COLOR_STATE_ZERO :
                                       (newState == LogicState::Z) ? COLOR_STATE_Z : COLOR_STATE_X;

                    DrawLineEx(Vector2{ x, y1 }, Vector2{ x, y2 }, 2.0f, transColor);
                };

                for (const auto& ev : wire_events) {
                    renderSegment(last_t, ev.timestamp, active_state);
                    renderTransition(ev.timestamp, active_state, ev.new_state);
                    active_state = ev.new_state;
                    last_t = ev.timestamp;
                }

                // Draw up to current_time or view end
                uint64_t drawUntil = std::max(current_time, last_t);
                renderSegment(last_t, drawUntil, active_state);

                // Horizontal lane divider
                DrawLine(leftSidebarWidth, currentY + laneHeight - 2, screenWidth, currentY + laneHeight - 2, COLOR_GRID);
            }
            currentY += laneHeight;
        }

        // Render current clock marker line
        float clockX = leftSidebarWidth + (current_time * zoom_factor) + pan_offset;
        if (clockX >= leftSidebarWidth && clockX <= screenWidth) {
            DrawLineEx(Vector2{ clockX, waveformAreaTop }, Vector2{ clockX, waveformAreaBottom }, 2.0f, Color{ 255, 82, 82, 255 });
        }

        EndScissorMode();

        // --- 3. Left Sidebar Panel (Signal Names) ---
        DrawRectangle(0, topToolbarHeight, leftSidebarWidth, screenHeight - topToolbarHeight, COLOR_PANEL);
        DrawLine(leftSidebarWidth - 1, topToolbarHeight, leftSidebarWidth - 1, screenHeight, COLOR_BORDER);

        // Sidebar Header
        DrawRectangle(0, topToolbarHeight, leftSidebarWidth, timeRulerHeight, Color{ 34, 36, 44, 255 });
        DrawText("SIGNALS / WIRES", 15, topToolbarHeight + 10, 12, COLOR_TEXT_MUTED);
        DrawLine(0, topToolbarHeight + timeRulerHeight, leftSidebarWidth, topToolbarHeight + timeRulerHeight, COLOR_BORDER);

        currentY = waveformAreaTop + 10.0f + scroll_y;
        for (size_t i = 0; i < display_wires.size(); ++i) {
            Wire* w = display_wires[i];
            if (currentY + laneHeight >= waveformAreaTop && currentY <= waveformAreaBottom) {
                // Signal status badge
                Color badgeColor = (w->current_state == LogicState::ONE) ? COLOR_STATE_ONE :
                                   (w->current_state == LogicState::ZERO) ? COLOR_STATE_ZERO :
                                   (w->current_state == LogicState::Z) ? COLOR_STATE_Z : COLOR_STATE_X;
                DrawRectangle(12, currentY + 16, 8, 16, badgeColor);

                // Signal Name
                DrawText(w->name.c_str(), 28, currentY + 15, 14, COLOR_TEXT_MAIN);

                // Current State text
                const char* stateStr = (w->current_state == LogicState::ONE) ? "1" :
                                        (w->current_state == LogicState::ZERO) ? "0" :
                                        (w->current_state == LogicState::Z) ? "Z" : "X";
                DrawText(stateStr, leftSidebarWidth - 30, currentY + 15, 14, badgeColor);

                DrawLine(0, currentY + laneHeight - 2, leftSidebarWidth, currentY + laneHeight - 2, COLOR_BORDER);
            }
            currentY += laneHeight;
        }

        // --- 4. Top Toolbar (Controls & Time Display) ---
        DrawRectangle(0, 0, screenWidth, topToolbarHeight, COLOR_PANEL);
        DrawLine(0, topToolbarHeight - 1, screenWidth, topToolbarHeight - 1, COLOR_BORDER);

        // Title
        DrawText("EDA WAVEFORM SIMULATOR", 15, 13, 16, COLOR_TEXT_MAIN);

        // Simulation Clock Display
        std::string clockStr = "CLOCK: " + std::to_string(current_time) + " ns";
        DrawText(clockStr.c_str(), 270, 14, 15, COLOR_STATE_ONE);

        // Play / Pause Button
        Rectangle playBtnRect = { 420, 8, 100, 28 };
        Vector2 mouseP = GetMousePosition();
        bool playHover = CheckCollisionPointRec(mouseP, playBtnRect);
        DrawRectangleRec(playBtnRect, playHover ? Color{ 60, 65, 80, 255 } : Color{ 45, 50, 62, 255 });
        DrawRectangleLinesEx(playBtnRect, 1, COLOR_BORDER);
        DrawText(is_simulating ? "PAUSE [||]" : "PLAY [>]", playBtnRect.x + 18, playBtnRect.y + 7, 12, is_simulating ? COLOR_STATE_Z : COLOR_STATE_ONE);

        if (playHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            is_simulating = !is_simulating;
        }

        // Step Button (+10ns)
        Rectangle stepBtnRect = { 530, 8, 90, 28 };
        bool stepHover = CheckCollisionPointRec(mouseP, stepBtnRect);
        DrawRectangleRec(stepBtnRect, stepHover ? Color{ 60, 65, 80, 255 } : Color{ 45, 50, 62, 255 });
        DrawRectangleLinesEx(stepBtnRect, 1, COLOR_BORDER);
        DrawText("+10ns STEP", stepBtnRect.x + 10, stepBtnRect.y + 7, 12, COLOR_TEXT_MAIN);

        if (stepHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            engine->stepTo(engine->getCurrentTime() + 10);
            OutputLogger::log("Stepped +10ns to " + std::to_string(engine->getCurrentTime()) + "ns");
        }

        // Export VCD Button
        Rectangle exportBtnRect = { 630, 8, 110, 28 };
        bool exportHover = CheckCollisionPointRec(mouseP, exportBtnRect);
        DrawRectangleRec(exportBtnRect, exportHover ? Color{ 60, 65, 80, 255 } : Color{ 45, 50, 62, 255 });
        DrawRectangleLinesEx(exportBtnRect, 1, COLOR_BORDER);
        DrawText("EXPORT VCD", exportBtnRect.x + 14, exportBtnRect.y + 7, 12, COLOR_STATE_ZERO);

        if (exportHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (netlist) {
                IOController::exportVCD("output_gui.vcd", *netlist, engine->getHistory());
                OutputLogger::log("Exported waveform to output_gui.vcd");
            }
        }

        // Reset Button
        Rectangle resetBtnRect = { 750, 8, 80, 28 };
        bool resetHover = CheckCollisionPointRec(mouseP, resetBtnRect);
        DrawRectangleRec(resetBtnRect, resetHover ? Color{ 80, 45, 45, 255 } : Color{ 60, 35, 35, 255 });
        DrawRectangleLinesEx(resetBtnRect, 1, COLOR_BORDER);
        DrawText("RESET", resetBtnRect.x + 20, resetBtnRect.y + 7, 12, COLOR_STATE_X);

        if (resetHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            engine->reset();
            is_simulating = false;
            OutputLogger::log("Simulation state reset.");
        }

        // Zoom / Control Hints
        DrawText("Mouse Wheel: Zoom | Left Click Drag: Pan", screenWidth - 300, 15, 12, COLOR_TEXT_MUTED);

        // --- 5. Bottom Log Console & CLI Input Box ---
        DrawRectangle(0, waveformAreaBottom, screenWidth, bottomConsoleH, COLOR_PANEL);
        DrawLine(0, waveformAreaBottom, screenWidth, waveformAreaBottom, COLOR_BORDER);

        // Log Console Messages Area
        float logY = waveformAreaBottom + 8.0f;
        DrawText("CONSOLE OUTPUT & HISTORY", 15, logY, 11, COLOR_TEXT_MUTED);
        logY += 18.0f;

        const auto& logs = OutputLogger::getLogs();
        int maxVisibleLogs = 4;
        int startIndex = std::max(0, (int)logs.size() - maxVisibleLogs);
        for (size_t i = startIndex; i < logs.size(); ++i) {
            DrawText(logs[i].text.c_str(), 20, logY, 13, logs[i].color);
            logY += 18.0f;
        }

        // Interactive CLI Input Box
        Rectangle inputRect = { 15, screenHeight - 38.0f, screenWidth - 30.0f, 28.0f };
        DrawRectangleRec(inputRect, Color{ 18, 20, 24, 255 });
        DrawRectangleLinesEx(inputRect, 1, input_active ? COLOR_STATE_ONE : COLOR_BORDER);

        DrawText("CLI >", inputRect.x + 10, inputRect.y + 7, 13, COLOR_STATE_ONE);
        DrawText(input_buffer, inputRect.x + 55, inputRect.y + 7, 13, COLOR_TEXT_MAIN);

        // Blinking cursor
        if ((int)(GetTime() * 2.0) % 2 == 0) {
            int textW = MeasureText(input_buffer, 13);
            DrawText("_", inputRect.x + 55 + textW + 2, inputRect.y + 7, 13, COLOR_STATE_ONE);
        }

        EndDrawing();
    }

    thread_running = false;
    if (cin_thread.joinable()) {
        cin_thread.detach();
    }
    CloseWindow();
}
