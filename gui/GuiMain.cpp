// GuiMain.cpp  —  AN/TPY-2 Air Defense Radar  (Phases 1-11)
//
// Build (Windows, from project root):
//   g++ -std=c++14 -I./include -I./gui -I./gui/imgui -I./gui/imgui/backends
//       src/radar/Radar.cpp src/radar/Target.cpp src/radar/Gun.cpp
//       src/utils/MathUtils.cpp gui/GuiMain.cpp gui/RadarWidget.cpp gui/HudPanel.cpp
//       gui/imgui/imgui.cpp gui/imgui/imgui_draw.cpp gui/imgui/imgui_tables.cpp
//       gui/imgui/imgui_widgets.cpp gui/imgui/imgui_demo.cpp
//       gui/imgui/backends/imgui_impl_glfw.cpp
//       gui/imgui/backends/imgui_impl_opengl3.cpp
//       -o build/radar_gui.exe
//       -L./gui/glfw/lib-mingw-w64 -lglfw3 -lopengl32 -lgdi32 -luser32 -mwindows

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "../include/radar/Radar.hpp"
#include "../include/radar/Target.hpp"
#include "../include/radar/Gun.hpp"
#include "RadarWidget.hpp"
#include "HudPanel.hpp"

using namespace std;
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ── Shared state ─────────────────────────────────────────────
bool g_simPaused = false;    // written by HudPanel Controls tab

// Engagement / kill tracking
double g_engageTimer      = -1.0;          // countdown in seconds; -1 = idle
static const double ENGAGE_FLIGHT = 2.0;   // seconds to intercept
int    g_killCount        = 0;

struct Explosion {
    Vector2D worldPos;
    float    age;
    float    maxAge;
};
vector<Explosion> g_explosions;

static Radar*          g_radar    = nullptr;
static vector<Target>* g_targets  = nullptr;
static Target*         g_mouseTgt = nullptr;
static bool            g_mouseOn  = true;
static int             g_nid      = 3000;
static double          g_time     = 0.0;
static double          g_sweepT   = 0.0;
static const double    SWEEP_P    = 0.50;  // 16 × 0.5s = ~8s per rotation
static map<string,Vector2D> g_movePat;
static mt19937 g_rng(random_device{}());
static double rnd(double a,double b){
    return a+uniform_real_distribution<double>(0,1)(g_rng)*(b-a);
}

// ── Spawn ─────────────────────────────────────────────────────
static void spawnN(int n){
    double R=g_radar->getRange();
    for(int i=0;i<n;i++){
        double x=rnd(-R*1.1,R*1.1), y=rnd(-R*1.1,R*1.1), h=rnd(250,3500);
        bool fr=rnd(0,1)<.35;
        TargetType tp=fr?TargetType::FRIENDLY:TargetType::UNKNOWN;
        string id=(fr?"FRD-":"UNK-")+to_string(g_nid++);
        g_targets->push_back(Target(Vector2D(x,y),h,tp,id));
        if(fr) g_radar->registerFriendly(id);
        g_movePat[id]=Vector2D(rnd(-3,3),rnd(-3,3));
    }
}

// ── Per-frame movement ────────────────────────────────────────
static void moveTgts(double /*dt*/){
    if(g_simPaused) return;
    const double B=1700.;
    for(auto&t:*g_targets){
        if(g_mouseTgt&&t.getId()==g_mouseTgt->getId()) continue;
        const string&id=t.getId();
        if(g_movePat.find(id)==g_movePat.end())
            g_movePat[id]=Vector2D(rnd(-2,2),rnd(-2,2));
        Vector2D np=t.getPosition();
        np.x+=g_movePat[id].x; np.y+=g_movePat[id].y;
        if(fabs(np.x)>B){np.x=B*(np.x>0?.9:-.9); g_movePat[id].x*=-1;}
        if(fabs(np.y)>B){np.y=B*(np.y>0?.9:-.9); g_movePat[id].y*=-1;}
        t.updatePosition(np,g_time);
    }
    g_time+=0.016;   // fixed 60fps-equivalent time step
}

// ── Mouse → world ─────────────────────────────────────────────
// Must match RadarWidget geometry exactly.
// RadarWidget uses: header=26, footer=22, radius=min(cW,cH)*0.42
static ImVec2 g_scopeMin={0,0}, g_scopeSz={500,500};
static void updateMouseTgt(){
    if(!g_mouseOn){ g_mouseTgt=nullptr; return; }
    const float HH=26.f, FH=22.f;
    float cW=g_scopeSz.x, cH=g_scopeSz.y-HH-FH;
    if(cH<50.f)return;
    ImVec2 orig={g_scopeMin.x, g_scopeMin.y+HH};
    ImVec2 ctr={orig.x+cW*.5f, orig.y+cH*.5f};
    float  R=min(cW,cH)*.42f;
    float  sc=R/(float)g_radar->getRange();
    ImVec2 mp=ImGui::GetMousePos();
    double wx=(mp.x-ctr.x)/sc, wy=-(mp.y-ctr.y)/sc;
    if(!g_mouseTgt){
        string id="MOUSE-"+to_string(g_nid++);
        g_mouseTgt=new Target(Vector2D(wx,wy),500.,TargetType::UNKNOWN,id);
        g_targets->push_back(*g_mouseTgt);
    } else {
        g_mouseTgt->updatePosition(Vector2D(wx,wy),g_time);
        for(auto&t:*g_targets)
            if(t.getId()==g_mouseTgt->getId()){t=*g_mouseTgt;break;}
    }
}

static void glfwErr(int e,const char*d){fprintf(stderr,"GLFW %d: %s\n",e,d);}

// ── Apply theme ───────────────────────────────────────────────
static void applyTheme(){
    ImGui::StyleColorsDark();
    ImGuiStyle&S=ImGui::GetStyle();
    // geometry
    S.WindowRounding=0; S.ChildRounding=2; S.FrameRounding=2;
    S.PopupRounding=2;  S.ScrollbarRounding=2; S.GrabRounding=2;
    S.TabRounding=3;
    S.WindowBorderSize=0; S.ChildBorderSize=1; S.FrameBorderSize=0;
    S.WindowPadding={8,6}; S.FramePadding={5,3};
    S.ItemSpacing={8,4};   S.ItemInnerSpacing={4,4};
    S.ScrollbarSize=10; S.GrabMinSize=7; S.TabBarBorderSize=1;
    // colours — deep phosphor-green CRT theme
    auto&C=S.Colors;
    C[ImGuiCol_Text]                 ={0.87f,0.93f,0.87f,1.f};
    C[ImGuiCol_TextDisabled]         ={0.36f,0.50f,0.36f,1.f};
    C[ImGuiCol_WindowBg]             ={0.03f,0.05f,0.03f,1.f};
    C[ImGuiCol_ChildBg]              ={0.04f,0.07f,0.04f,1.f};
    C[ImGuiCol_PopupBg]              ={0.05f,0.09f,0.05f,0.97f};
    C[ImGuiCol_Border]               ={0.12f,0.30f,0.12f,0.72f};
    C[ImGuiCol_FrameBg]              ={0.06f,0.11f,0.06f,1.f};
    C[ImGuiCol_FrameBgHovered]       ={0.09f,0.18f,0.09f,1.f};
    C[ImGuiCol_FrameBgActive]        ={0.13f,0.26f,0.13f,1.f};
    C[ImGuiCol_TitleBg]              ={0.03f,0.06f,0.03f,1.f};
    C[ImGuiCol_TitleBgActive]        ={0.04f,0.10f,0.04f,1.f};
    C[ImGuiCol_ScrollbarBg]          ={0.02f,0.04f,0.02f,1.f};
    C[ImGuiCol_ScrollbarGrab]        ={0.13f,0.33f,0.13f,1.f};
    C[ImGuiCol_ScrollbarGrabHovered] ={0.18f,0.46f,0.18f,1.f};
    C[ImGuiCol_ScrollbarGrabActive]  ={0.24f,0.60f,0.24f,1.f};
    C[ImGuiCol_CheckMark]            ={0.26f,0.82f,0.26f,1.f};
    C[ImGuiCol_SliderGrab]           ={0.18f,0.58f,0.18f,1.f};
    C[ImGuiCol_SliderGrabActive]     ={0.26f,0.78f,0.26f,1.f};
    C[ImGuiCol_Button]               ={0.07f,0.19f,0.07f,1.f};
    C[ImGuiCol_ButtonHovered]        ={0.12f,0.35f,0.12f,1.f};
    C[ImGuiCol_ButtonActive]         ={0.17f,0.52f,0.17f,1.f};
    C[ImGuiCol_Header]               ={0.07f,0.18f,0.07f,1.f};
    C[ImGuiCol_HeaderHovered]        ={0.11f,0.30f,0.11f,1.f};
    C[ImGuiCol_HeaderActive]         ={0.15f,0.44f,0.15f,1.f};
    C[ImGuiCol_Separator]            ={0.12f,0.30f,0.12f,0.60f};
    C[ImGuiCol_Tab]                  ={0.05f,0.11f,0.05f,1.f};
    C[ImGuiCol_TabHovered]           ={0.11f,0.32f,0.11f,1.f};
    C[ImGuiCol_TabActive]            ={0.09f,0.24f,0.09f,1.f};
    C[ImGuiCol_TabUnfocused]         ={0.03f,0.08f,0.03f,1.f};
    C[ImGuiCol_TabUnfocusedActive]   ={0.06f,0.16f,0.06f,1.f};
    C[ImGuiCol_TableHeaderBg]        ={0.05f,0.11f,0.05f,1.f};
    C[ImGuiCol_TableBorderStrong]    ={0.14f,0.34f,0.14f,1.f};
    C[ImGuiCol_TableBorderLight]     ={0.07f,0.18f,0.07f,0.70f};
    C[ImGuiCol_TableRowBg]           ={0,0,0,0};
    C[ImGuiCol_TableRowBgAlt]        ={0.04f,0.07f,0.04f,0.55f};
    C[ImGuiCol_TextSelectedBg]       ={0.11f,0.34f,0.11f,0.50f};
    C[ImGuiCol_NavHighlight]         ={0.24f,0.64f,0.24f,1.f};
    C[ImGuiCol_PlotLines]            ={0.26f,0.75f,0.26f,1.f};
    C[ImGuiCol_PlotHistogram]        ={0.16f,0.65f,0.16f,1.f};
}

// ── Main ──────────────────────────────────────────────────────
int main(){
    Radar radar(Vector2D(0,0),1000.);
    vector<Target> targets;
    g_radar=&radar; g_targets=&targets;
    spawnN(6);

    glfwSetErrorCallback(glfwErr);
    if(!glfwInit()){cerr<<"GLFW failed\n";return -1;}
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,0);

    GLFWwindow*win=glfwCreateWindow(1440,900,
        "AN/TPY-2  Air Defense Radar",nullptr,nullptr);
    if(!win){cerr<<"Window failed\n";glfwTerminate();return -1;}
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO&io=ImGui::GetIO();
    io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;

    applyTheme();

    ImGui_ImplGlfw_InitForOpenGL(win,true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Detection log ring buffer
    static char   logBuf[30][80]={};
    static int    logN=0;
    double        logTimer=0.;

    // Selected target for detail card + scope highlight
    string selectedId="";

    double lastT=glfwGetTime();

    while(!glfwWindowShouldClose(win)){
        glfwPollEvents();
        double now=glfwGetTime(), dt=now-lastT; lastT=now;

        // ── Simulation tick ───────────────────────────────────
        moveTgts(dt);
        if(!g_simPaused){
            g_sweepT+=dt;
            if(g_sweepT>=SWEEP_P){ g_sweepT=0.; radar.advanceSweep(); }
        }
        radar.updateDetections(targets.data(),(int)targets.size());

        // ── Engagement countdown & kill ───────────────────────
        if(radar.isEngaged() && g_engageTimer < 0.0){
            // Engagement just started — begin countdown
            g_engageTimer = ENGAGE_FLIGHT;
        }
        if(g_engageTimer >= 0.0 && !g_simPaused){
            g_engageTimer -= dt;
            if(g_engageTimer <= 0.0){
                // Time's up — find target, record position, remove it
                g_engageTimer = -1.0;
                string killId = radar.getEngagedTarget();
                for(int ki=0;ki<(int)targets.size();ki++){
                    if(targets[ki].getId()==killId){
                        // Spawn explosion at kill position
                        Explosion ex;
                        ex.worldPos = targets[ki].getPosition();
                        ex.age      = 0.f;
                        ex.maxAge   = 1.4f;   // 1.4s animation
                        g_explosions.push_back(ex);
                        // Remove target
                        targets.erase(targets.begin()+ki);
                        g_killCount++;
                        // If it was the selected target, deselect
                        if(selectedId==killId) selectedId="";
                        break;
                    }
                }
                radar.clearEngagement();
            }
        }
        // If engagement was aborted externally, reset timer
        if(!radar.isEngaged()) g_engageTimer = -1.0;

        // ── Age / remove expired explosions ───────────────────
        for(auto&ex:g_explosions) ex.age += (float)dt;
        g_explosions.erase(
            remove_if(g_explosions.begin(),g_explosions.end(),
                [](const Explosion&e){return e.age>=e.maxAge;}),
            g_explosions.end());

        // ── Detection log (refresh every 0.5s) ───────────────
        logTimer+=dt;
        if(logTimer>0.5){
            logTimer=0.; logN=0;
            for(const auto&t:targets){
                if(logN>=30) break;
                bool inR=radar.isInRange(t);
                snprintf(logBuf[logN++],80,"[%s] %-10s  %s   %.0fm",
                    inR?"IN ":"OUT", t.getId().c_str(),
                    radar.identifyTarget(t)?"FRN":"UNK",
                    t.calculateHorizontalDistance(radar.getPosition()));
            }
        }

        // ── ImGui frame ───────────────────────────────────────
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int wW,wH; glfwGetFramebufferSize(win,&wW,&wH);

        // ── Root fullscreen window ────────────────────────────
        ImGui::SetNextWindowPos({0,0});
        ImGui::SetNextWindowSize({(float)wW,(float)wH});
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
        ImGui::Begin("##ROOT",nullptr,
            ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|
            ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollbar|
            ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::PopStyleVar();

        ImDrawList*rdl=ImGui::GetWindowDrawList();
        const float TH=30.f;   // top bar height
        const float BH=18.f;   // bottom bar height
        // Body area is between top and bottom bars
        float bodyH=(float)wH-TH-BH;
        float bodyW=(float)wW;

        // ── TOP BAR ──────────────────────────────────────────
        rdl->AddRectFilled({0,0},{(float)wW,TH},IM_COL32(2,9,2,255));
        rdl->AddRectFilled({0,0},{4,TH},         IM_COL32(0,200,58,255));
        rdl->AddLine({0,TH},{(float)wW,TH},      IM_COL32(0,88,0,230),1.5f);

        // Title on left
        ImGui::SetCursorPos({14.f,(TH-ImGui::GetTextLineHeight())*.5f});
        ImGui::TextColored({0.17f,0.88f,0.22f,1.f},
            "AN/TPY-2   AIR DEFENSE RADAR SIMULATION");

        // Live counters on right
        {
            int inR=0,htl=0,frn=0;
            for(const auto&t:targets){
                if(!radar.isInRange(t))continue; inR++;
                if(radar.identifyTarget(t))frn++; else htl++;
            }
            char rb[200];
            snprintf(rb,sizeof(rb),
                "TRACKS: %2d    HTL: %2d    FRN: %2d    KILLS: %2d"
                "      SWEEP: %05.1f\xc2\xb0      T+ %.0fs      %.0f FPS   ",
                inR,htl,frn,g_killCount,
                radar.getCurrentSweepAngle(),g_time,io.Framerate);
            float tw=ImGui::CalcTextSize(rb).x;
            ImGui::SetCursorPos({(float)wW-tw-6,(TH-ImGui::GetTextLineHeight())*.5f});
            ImGui::TextColored({0.37f,0.61f,0.37f,1.f},"%s",rb);
        }
        // Paused banner centred
        if(g_simPaused){
            const char*ps=" \xe2\x96\xae\xe2\x96\xae  SIMULATION PAUSED  \xe2\x96\xae\xe2\x96\xae ";
            float pw=ImGui::CalcTextSize(ps).x;
            ImGui::SetCursorPos({((float)wW-pw)*.5f,(TH-ImGui::GetTextLineHeight())*.5f});
            ImGui::TextColored({1.f,.85f,.10f,1.f},"%s",ps);
        }

        // ── LAYOUT ───────────────────────────────────────────
        //
        //   +-------------------+------------------+
        //   |                   |   RADAR SCOPE    |
        //   |   LEFT PANEL      |   (square)       |
        //   |   (Track Info /   +------------------+
        //   |    Controls)      | FIRE CTRL/THREAT |
        //   |                   |   (tabs)         |
        //   +-------------------+------------------+
        //
        // Left column: 440px wide, full body height
        // Right column: fills remaining width
        //   Right-top: square radar scope
        //   Right-bottom: fire control / threat tabs

        const float LP   = 6.f;          // layout padding
        const float GAP  = 5.f;          // gap between columns / rows
        const float LW   = 440.f;        // left panel width
        float       RW   = bodyW - LW - GAP - LP*2;  // right panel width
        // Make the radar scope square — side = min(RW, bodyH/2)
        float       scopeW= RW;
        float       scopeH= min(scopeW, bodyH*.52f);  // ~52% of body height
        float       btmH  = bodyH - scopeH - GAP;     // bottom-right height

        // ── LEFT PANEL — Fire Control (full height, wide enough for all data)
        ImGui::SetCursorPos({LP, TH+LP});
        ImGui::BeginChild("LeftPanel",{LW, bodyH-LP*2}, true,
                          ImGuiWindowFlags_None);
        renderFireControl(radar,targets);
        ImGui::EndChild();

        // ── RIGHT TOP — RADAR SCOPE ───────────────────────────
        float rxStart = LP + LW + GAP;
        ImGui::SetCursorPos({rxStart, TH+LP});
        ImGui::BeginChild("ScopePanel",{RW, scopeH}, true,
                          ImGuiWindowFlags_NoScrollbar|
                          ImGuiWindowFlags_NoScrollWithMouse);

        // Record canvas position for mouse-to-world conversion
        g_scopeMin = ImGui::GetCursorScreenPos();
        g_scopeSz  = ImGui::GetContentRegionAvail();
        updateMouseTgt();
        renderRadarWidget(radar,targets,g_mouseTgt,selectedId);

        ImGui::EndChild();

        // ── RIGHT BOTTOM — Track Info / Controls / Threat ─────
        ImGui::SetCursorPos({rxStart, TH+LP+scopeH+GAP});
        ImGui::BeginChild("BtmPanel",{RW, btmH-LP}, true,
                          ImGuiWindowFlags_None);

        if(ImGui::BeginTabBar("BTabs")){
            if(ImGui::BeginTabItem("Track Info")){
                renderTrackPanel(radar,targets,g_mouseTgt,selectedId);
                ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("Controls")){
                renderControlsPanel(radar,targets,g_mouseOn);
                // Handle mouse target removal when toggled off
                if(!g_mouseOn && g_mouseTgt){
                    targets.erase(remove_if(targets.begin(),targets.end(),
                        [](const Target&t){
                            return t.getId().find("MOUSE")!=string::npos;
                        }), targets.end());
                    delete g_mouseTgt; g_mouseTgt=nullptr;
                }
                ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("Threat & Log")){
                renderThreatPanel(radar,targets,logBuf,logN);
                ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("Reference")){
                ImGui::Spacing();
                ImGui::TextColored({0,.88f,.88f,1}, "\xe2\x96\xb2 = Friendly (IFF confirmed)");
                ImGui::TextColored({1,.88f,.10f,1}, "\xe2\x96\xa0 = Unknown / Unclassified");
                ImGui::TextColored({1,.18f,.18f,1}, "\xe2\x97\x8f = Hostile  (dist < 33%% range)");
                ImGui::TextColored({1,.50f,.10f,1}, "\xe2\x97\x86 = Mouse-controlled target");
                ImGui::Spacing();
                ImGui::TextColored({0,.88f,.30f,1}, "Fan glow  = sweep trail");
                ImGui::TextColored({1,.60f,.10f,1}, "Orange arrow = velocity vector");
                ImGui::TextColored({1,.10f,.10f,1}, "Red ring/cross = engaged target");
                ImGui::Spacing();
                ImGui::TextColored({.55f,.75f,.55f,1},
                    "Sweep: 16 steps x 0.5s = ~8s/rotation");
                ImGui::Spacing();
                ImGui::TextColored({.45f,.65f,.45f,1},
                    "DSA structures in use:\n"
                    "  Queue          Detection event FIFO\n"
                    "  Circular LL    Sweep step rotation\n"
                    "  Hash Map       IFF O(1) lookup\n"
                    "  Array          Position history\n"
                    "  Expr Tree      Gun angle calc\n"
                    "  QuickSort      Threat priority");
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::EndChild();

        // ── BOTTOM STATUS BAR ─────────────────────────────────
        float by=(float)wH-BH;
        rdl->AddRectFilled({0,by},{(float)wW,(float)wH},IM_COL32(1,7,1,255));
        rdl->AddLine({0,by},{(float)wW,by},IM_COL32(0,65,0,200),1.f);
        ImGui::SetCursorPos({10.f,by+(BH-ImGui::GetTextLineHeight())*.5f});
        ImGui::TextColored({.26f,.44f,.26f,1.f},
            "DSA Project  |  Phases 1-11 Complete  |  "
            "Queue  CircularLL  HashMap  Array  ExprTree  QuickSort");

        ImGui::End();

        // ── Render ────────────────────────────────────────────
        ImGui::Render();
        glViewport(0,0,wW,wH);
        glClearColor(.03f,.05f,.03f,1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(win);
        this_thread::sleep_for(chrono::milliseconds(8));
    }

    delete g_mouseTgt;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}