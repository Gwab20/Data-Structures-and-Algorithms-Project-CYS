// HudPanel.cpp
#include "HudPanel.hpp"
#include "../include/radar/Gun.hpp"
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <map>
#include <random>
#include <string>
using namespace std;
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//  Shared sim state 
extern bool   g_simPaused;
extern double g_engageTimer;
extern int    g_killCount;

//  Helpers 
static mt19937& RNG(){ static mt19937 r(random_device{}()); return r; }
static double   rnd(double a,double b){
    return a+uniform_real_distribution<double>(0,1)(RNG())*(b-a);
}
static int      NID(){ static int n=2000; return n++; }

static ImVec4 spdCol(double s){
    if(s>200) return{1.f,.15f,.15f,1};
    if(s>100) return{1.f,.55f,.10f,1};
    if(s> 40) return{1.f, 1.f,.10f,1};
    return{.55f,1.f,.55f,1};
}
static ImVec4 thrCol(double d,double R){
    double f=d/R;
    if(f<.25) return{1.f,.08f,.08f,1};
    if(f<.50) return{1.f,.45f,.08f,1};
    if(f<.75) return{1.f, 1.f,.10f,1};
    return{.22f,.88f,.22f,1};
}
static const char* thrLbl(double d,double R){
    double f=d/R;
    if(f<.25)return"CRITICAL";
    if(f<.50)return"HIGH";
    if(f<.75)return"MEDIUM";
    return"LOW";
}

// ── Section header: Dummy FIRST, then draw into DrawList ─────
// This ensures the header background never covers the text.
static void SH(const char* label, ImVec4 accent={.18f,.80f,.18f,1.f}){
    const float H=22.f;
    ImVec2      pos=ImGui::GetCursorScreenPos();
    float       w  =ImGui::GetContentRegionAvail().x;
    ImGui::Dummy({w,H});                         // advance cursor first
    ImDrawList* dl=ImGui::GetWindowDrawList();
    dl->AddRectFilled(pos,{pos.x+w,pos.y+H}, IM_COL32(5,16,5,255));
    dl->AddRectFilled(pos,{pos.x+4,pos.y+H}, ImGui::ColorConvertFloat4ToU32(accent));
    dl->AddLine({pos.x,pos.y+H},{pos.x+w,pos.y+H}, IM_COL32(12,58,12,200),1.f);
    float ty=pos.y+(H-ImGui::GetTextLineHeight())*.5f;
    dl->AddText({pos.x+10,ty}, ImGui::ColorConvertFloat4ToU32(accent), label);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY()+2);
}

//  Simple kv table rows 
static void KV(const char*k,const char*fmt,...){
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextColored({.42f,.62f,.42f,1},"%s",k);
    ImGui::TableSetColumnIndex(1);
    char b[80]; va_list a; va_start(a,fmt); vsnprintf(b,80,fmt,a); va_end(a);
    ImGui::Text("%s",b);
}
static void KVC(const char*k,ImVec4 c,const char*fmt,...){
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextColored({.42f,.62f,.42f,1},"%s",k);
    ImGui::TableSetColumnIndex(1);
    char b[80]; va_list a; va_start(a,fmt); vsnprintf(b,80,fmt,a); va_end(a);
    ImGui::TextColored(c,"%s",b);
}

//  TRACK PANEL  (left column, upper tab)
void renderTrackPanel(const Radar&radar,
                      const vector<Target>&targets,
                      Target*mt,
                      string&selectedId)
{
    //  Counts 
    int total=0,inR=0,htl=0,frn=0;
    for(const auto&t:targets){
        total++;
        if(radar.isInRange(t)){
            inR++;
            if(radar.identifyTarget(t))frn++; else htl++;
        }
    }

    //  Summary strip 
    SH("ACTIVE TRACKS");
    ImGui::Spacing();

    // Three labelled counters in one row
    if(ImGui::BeginTable("sc3",3,0)){
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextColored({.75f,.92f,.75f,1},"IN RANGE");
        ImGui::TextColored({.90f,1.f,.90f,1},"%d / %d",inR,total);
        ImGui::TableSetColumnIndex(1);
        ImGui::TextColored({1.f,.80f,.10f,1},"HOSTILE");
        ImGui::TextColored({1.f,.90f,.10f,1},"%d",htl);
        ImGui::TableSetColumnIndex(2);
        ImGui::TextColored({.10f,.90f,.95f,1},"FRIENDLY");
        ImGui::TextColored({.10f,1.f,1.f,1}, "%d",frn);
        ImGui::EndTable();
    }
    ImGui::Spacing();

    //  Target table 
    if(ImGui::BeginTable("tt",7,
        ImGuiTableFlags_Borders     |
        ImGuiTableFlags_RowBg       |
        ImGuiTableFlags_ScrollY     |
        ImGuiTableFlags_SizingFixedFit |
        ImGuiTableFlags_Reorderable,
        {0, 210}))
    {
        ImGui::TableSetupScrollFreeze(0,1);
        ImGui::TableSetupColumn("ID",    0, 92);
        ImGui::TableSetupColumn("IFF",   0, 46);
        ImGui::TableSetupColumn("Dist",  0, 56);
        ImGui::TableSetupColumn("Alt",   0, 52);
        ImGui::TableSetupColumn("Spd",   0, 54);
        ImGui::TableSetupColumn("Brg",   0, 68);
        ImGui::TableSetupColumn("Threat",0, 56);
        ImGui::TableHeadersRow();

        // Sort by distance
        vector<const Target*> sv;
        for(const auto&t:targets)
            if(radar.isInRange(t)) sv.push_back(&t);
        sort(sv.begin(),sv.end(),[&](const Target*a,const Target*b){
            return a->calculateHorizontalDistance(radar.getPosition())<
                   b->calculateHorizontalDistance(radar.getPosition());
        });

        for(const auto*tp:sv){
            const Target&t=*tp;
            bool iF=radar.identifyTarget(t);
            bool iM=mt&&t.getId()==mt->getId();
            bool iE=radar.isEngaged()&&radar.getEngagedTarget()==t.getId();
            bool iSel=t.getId()==selectedId;
            double d=t.calculateHorizontalDistance(radar.getPosition());

            ImGui::TableNextRow();

            // Highlight selected row
            if(iSel){
                ImU32 rowCol=IM_COL32(25,60,25,200);
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,rowCol);
            }

            // Selectable ID cell — clicking selects the row
            ImGui::TableSetColumnIndex(0);
            ImVec4 ic=iE?ImVec4{1,.08f,.08f,1}:iM?ImVec4{1,.5f,.25f,1}:
                       iF?ImVec4{.1f,.92f,.92f,1}:ImVec4{1,.88f,.1f,1};
            ImGui::PushStyleColor(ImGuiCol_Text,ic);
            char selLbl[64]; snprintf(selLbl,sizeof(selLbl),"##sel%s",t.getId().c_str());
            bool clicked=ImGui::Selectable(t.getId().c_str(),iSel,
                ImGuiSelectableFlags_SpanAllColumns,{0,0});
            ImGui::PopStyleColor();
            if(clicked) selectedId=(iSel?"":t.getId());

            ImGui::TableSetColumnIndex(1);
            if(iF) ImGui::TextColored({.1f,.92f,.92f,1},"FRN");
            else   ImGui::TextColored({1.f,.88f,.1f,1}, "UNK");

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.0f",d);

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.0f",t.getHeight());

            ImGui::TableSetColumnIndex(4);
            ImGui::TextColored(spdCol(t.getSpeed()),"%.0f",t.getSpeed());

            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%.0f\xc2\xb0%s",
                t.calculateBearingFrom(radar.getPosition()),
                t.getCompassDirectionFrom(radar.getPosition()).c_str());

            ImGui::TableSetColumnIndex(6);
            if(!iF) ImGui::TextColored(thrCol(d,radar.getRange()),
                                       "%s",thrLbl(d,radar.getRange()));
            else    ImGui::TextColored({.22f,.88f,.22f,1},"SAFE");
        }
        ImGui::EndTable();
    }

    //  Selected target detail card 
    if(!selectedId.empty()){
        // Find selected target
        const Target*sel=nullptr;
        for(const auto&t:targets)
            if(t.getId()==selectedId){ sel=&t; break; }

        if(sel && radar.isInRange(*sel)){
            ImGui::Spacing();
            SH(("SELECTED: "+selectedId).c_str(), {.6f,.6f,1.f,1.f});
            ImGui::Spacing();
            if(ImGui::BeginTable("seldet",2,0)){
                ImGui::TableSetupColumn("K",ImGuiTableColumnFlags_WidthFixed,95);
                ImGui::TableSetupColumn("V");
                Vector2D p=sel->getPosition();
                Vector2D v=sel->getVelocity();
                Vector2D ac=sel->getAcceleration();
                double d=sel->calculateHorizontalDistance(radar.getPosition());
                KV ("Position","(%.0f, %.0f) m",p.x,p.y);
                KVC("Distance",thrCol(d,radar.getRange()),"%.0f m",d);
                KV ("Altitude","%.0f m",sel->getHeight());
                KVC("Speed",   spdCol(sel->getSpeed()),"%.1f m/s",sel->getSpeed());
                KV ("Velocity","(%.1f, %.1f)",v.x,v.y);
                KV ("Accel",   "(%.1f, %.1f)",ac.x,ac.y);
                KV ("Bearing", "%.1f\xc2\xb0 %s",
                    sel->calculateBearingFrom(radar.getPosition()),
                    sel->getCompassDirectionFrom(radar.getPosition()).c_str());
                KV ("3D Disp", "%.0f m",
                    sel->calculateDisplacement(radar.getPosition()));
                ImGui::EndTable();
            }
            // Quick engage/abort button
            bool isEng=radar.isEngaged()&&radar.getEngagedTarget()==selectedId;
            ImGui::Spacing();
            if(isEng && g_engageTimer > 0.0){
                char cfmt[48];
                snprintf(cfmt,sizeof(cfmt),
                    "  \xe2\x9c\x88  MISSILE INBOUND  %.1fs  ",(float)g_engageTimer);
                float bright=0.5f+0.3f*sinf((float)(g_engageTimer*6.0));
                ImGui::PushStyleColor(ImGuiCol_Button,      {bright,.05f,.05f,1.f});
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered,{bright,.05f,.05f,1.f});
                ImGui::Button(cfmt,{-1,26});
                ImGui::PopStyleColor(2);
            } else if(isEng){
                ImGui::PushStyleColor(ImGuiCol_Button,{.45f,0,0,1});
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered,{.70f,0,0,1});
                if(ImGui::Button("ABORT ENGAGEMENT",{-1,26}))
                    const_cast<Radar&>(radar).clearEngagement();
                ImGui::PopStyleColor(2);
            } else if(!radar.identifyTarget(*sel)){
                ImGui::PushStyleColor(ImGuiCol_Button,{.60f,0,0,1});
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered,{.85f,0,0,1});
                if(ImGui::Button("ENGAGE THIS TARGET",{-1,26}))
                    const_cast<Radar&>(radar).setEngagement(selectedId);
                ImGui::PopStyleColor(2);
            }
        }
    }

    //  Mouse target live card 
    if(mt && radar.isInRange(*mt)){
        ImGui::Spacing();
        SH("LIVE MOUSE TRACK",{.90f,.28f,.28f,1.f});
        ImGui::Spacing();
        if(ImGui::BeginTable("mtt",2,0)){
            ImGui::TableSetupColumn("K",ImGuiTableColumnFlags_WidthFixed,90);
            ImGui::TableSetupColumn("V");
            Vector2D p=mt->getPosition(),v=mt->getVelocity(),a=mt->getAcceleration();
            double d=mt->calculateHorizontalDistance(radar.getPosition());
            KV ("Pos","(%.0f, %.0f) m",p.x,p.y);
            KVC("Spd",spdCol(mt->getSpeed()),"%.1f m/s",mt->getSpeed());
            KVC("Dst",thrCol(d,radar.getRange()),"%.0f m",d);
            KV ("Brg","%.1f\xc2\xb0 %s",
                mt->calculateBearingFrom(radar.getPosition()),
                mt->getCompassDirectionFrom(radar.getPosition()).c_str());
            KV ("Vel","(%.1f, %.1f) m/s",v.x,v.y);
            KV ("Acc","(%.1f, %.1f) m/s\xc2\xb2",a.x,a.y);
            KV ("Alt","%.0f m",mt->getHeight());
            ImGui::EndTable();
        }
    }
}

//  CONTROLS PANEL 
void renderControlsPanel(Radar&radar,
                         vector<Target>&targets,
                         bool&mouseAsTarget)
{
    static float rngVal=(float)radar.getRange();
    static int   spawnN=4;
    static float spdMul=1.0f;
    static map<string,Vector2D> mp;

    //  Radar parameters 
    SH("RADAR PARAMETERS");
    ImGui::Spacing();

    ImGui::SetNextItemWidth(-1);
    if(ImGui::SliderFloat("##rng",&rngVal,200.f,3000.f,"Detection Range: %.0f m"))
        radar.setRange((double)rngVal);
    ImGui::Spacing();

    // Sweep status
    float sw=ImGui::GetContentRegionAvail().x;
    ImGui::Text("Sweep angle:"); ImGui::SameLine();
    ImGui::TextColored({.2f,.9f,.3f,1},"%.1f\xc2\xb0",radar.getCurrentSweepAngle());
    ImGui::SameLine();
    if(ImGui::SmallButton("Step##s")) radar.advanceSweep();
    ImGui::SameLine();
    ImGui::TextColored({.4f,.6f,.4f,1},"(auto: ~8s/rot)");

    //  Simulation 
    ImGui::Spacing();
    SH("SIMULATION CONTROL",{.75f,.62f,.1f,1.f});
    ImGui::Spacing();

    if(g_simPaused){
        ImGui::PushStyleColor(ImGuiCol_Button,{.50f,.32f,.0f,1});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,{.72f,.45f,.0f,1});
        if(ImGui::Button("  \xe2\x96\xb6  RESUME SIMULATION  ",{-1,30}))
            g_simPaused=false;
        ImGui::PopStyleColor(2);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button,{.06f,.18f,.06f,1});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,{.10f,.30f,.10f,1});
        if(ImGui::Button("  \xe2\x96\xae\xe2\x96\xae  PAUSE SIMULATION  ",{-1,30}))
            g_simPaused=true;
        ImGui::PopStyleColor(2);
    }
    ImGui::Spacing();

    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##spm",&spdMul,0.1f,6.f,"Target Speed Multiplier: %.1fx");
    ImGui::TextColored({.38f,.58f,.38f,1},"  Applied to newly spawned targets");

    // Target management 
    ImGui::Spacing();
    SH("TARGET MANAGEMENT",{.85f,.72f,.10f,1.f});
    ImGui::Spacing();

    ImGui::Checkbox("Mouse cursor = unknown target",&mouseAsTarget);
    if(mouseAsTarget)
        ImGui::TextColored({.92f,.52f,.26f,1},"  Move cursor over radar scope");
    ImGui::Spacing();

    ImGui::SetNextItemWidth(90);
    ImGui::InputInt("##sn",&spawnN,1,5);
    spawnN=max(1,min(spawnN,30));
    ImGui::SameLine(0,8);
    ImGui::Text("targets (35%% friendly)");
    ImGui::Spacing();

    // Spawn button
    ImGui::PushStyleColor(ImGuiCol_Button,{.08f,.22f,.08f,1});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,{.14f,.38f,.14f,1});
    if(ImGui::Button("SPAWN TARGETS",{-1,28})){
        for(int i=0;i<spawnN;i++){
            double R=radar.getRange();
            double x=rnd(-R*1.15,R*1.15), y=rnd(-R*1.15,R*1.15), h=rnd(250,3500);
            bool fr=rnd(0,1)<.35;
            TargetType tp=fr?TargetType::FRIENDLY:TargetType::UNKNOWN;
            string id=(fr?"FRD-":"UNK-")+to_string(NID());
            targets.push_back(Target(Vector2D(x,y),h,tp,id));
            if(fr) radar.registerFriendly(id);
            double spd=spdMul;
            mp[id]=Vector2D(rnd(-3,3)*spd, rnd(-3,3)*spd);
        }
    }
    ImGui::PopStyleColor(2);

    ImGui::PushStyleColor(ImGuiCol_Button,{.22f,.05f,.05f,1});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,{.38f,.09f,.09f,1});
    if(ImGui::Button("CLEAR ALL TARGETS",{-1,28})){
        targets.clear();
        radar.clearEngagement();
    }
    ImGui::PopStyleColor(2);

    // IFF override
    ImGui::Spacing();
    SH("IFF TOOLS",{.55f,.55f,.9f,1.f});
    ImGui::Spacing();
    ImGui::TextColored({.4f,.6f,.4f,1},"Manually register a target as friendly:");
    static char iffBuf[32]="";
    ImGui::SetNextItemWidth(-60);
    ImGui::InputText("##iffin",iffBuf,sizeof(iffBuf));
    ImGui::SameLine();
    if(ImGui::Button("IFF")){
        if(iffBuf[0]) radar.registerFriendly(string(iffBuf));
        iffBuf[0]='\0';
    }
    ImGui::Spacing();

    // System status 
    SH("SYSTEM STATUS",{.32f,.60f,.90f,1.f});
    ImGui::Spacing();

    int inR2=0,frC=0,ukC=0; double avgS=0,maxS=0;
    for(const auto&t:targets){
        if(radar.identifyTarget(t))frC++; else ukC++;
        if(radar.isInRange(t)){
            inR2++;
            double s=t.getSpeed(); avgS+=s;
            if(s>maxS)maxS=s;
        }
    }
    if(inR2)avgS/=inR2;

    if(ImGui::BeginTable("ss2",2,0)){
        ImGui::TableSetupColumn("K",ImGuiTableColumnFlags_WidthFixed,105);
        ImGui::TableSetupColumn("V");
        KV("Total targets","%d",(int)targets.size());
        KV("In range",     "%d",inR2);
        KV("Range",        "%.0f m",radar.getRange());
        KV("Avg speed",    "%.1f m/s",avgS);
        KV("Max speed",    "%.1f m/s",maxS);
        KV("Sweep angle",  "%.1f\xc2\xb0",radar.getCurrentSweepAngle());
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::TextColored({.1f,.92f,.92f,1},"IFF Friendly");
        ImGui::TableSetColumnIndex(1); ImGui::TextColored({.1f,.92f,.92f,1},"%d",frC);
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::TextColored({1.f,.88f,.1f,1},"IFF Unknown");
        ImGui::TableSetColumnIndex(1); ImGui::TextColored({1.f,.88f,.1f,1},"%d",ukC);
        ImGui::EndTable();
    }

    if(radar.isEngaged()){
        ImGui::Spacing();
        ImGui::TextColored({1.f,.18f,.18f,1},"  ENGAGING: %s",
            radar.getEngagedTarget().c_str());
        ImGui::PushStyleColor(ImGuiCol_Button,{.45f,0,0,1});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,{.70f,0,0,1});
        if(ImGui::Button("ABORT ENGAGEMENT",{-1,0})) radar.clearEngagement();
        ImGui::PopStyleColor(2);
    }
}

//  FIRE CONTROL  
void renderFireControl(Radar&radar,const vector<Target>&targets){
    SH("FIRE CONTROL SYSTEM",{.92f,.18f,.18f,1.f});
    const Gun&gun=radar.getDefenseGun();

    // Collect and sort in-range unknowns closest-first
    vector<const Target*> unk;
    for(const auto&t:targets)
        if(radar.isInRange(t)&&!radar.identifyTarget(t))
            unk.push_back(&t);
    sort(unk.begin(),unk.end(),[&](const Target*a,const Target*b){
        return a->calculateHorizontalDistance(radar.getPosition())<
               b->calculateHorizontalDistance(radar.getPosition());
    });

    if(unk.empty()){
        ImGui::Spacing();
        ImGui::TextColored({.2f,.9f,.2f,1},"  \xe2\x9c\x93  ALL CLEAR");
        ImGui::TextColored({.35f,.55f,.35f,1},"  No hostile targets in range.");
        radar.clearEngagement();
        return;
    }

    double cd=unk[0]->calculateHorizontalDistance(radar.getPosition());
    ImGui::Spacing();
    ImGui::Text("Hostiles: %d    Closest: %.0f m",(int)unk.size(),cd);
    ImGui::SameLine(0,10);
    ImGui::TextColored(thrCol(cd,radar.getRange()),"[%s]",thrLbl(cd,radar.getRange()));
    ImGui::Spacing();

    const char* pri[]={"P1","P2","P3","P4","P5"};
    int show=min(5,(int)unk.size());

    for(int i=0;i<show;i++){
        const Target&t=*unk[i];
        FiringSolution s=gun.calculateFiringSolution(t);
        bool eng=radar.isEngaged()&&radar.getEngagedTarget()==t.getId();

        ImGui::PushID(i);

        // Priority + title bar
        {
            ImVec2 bp=ImGui::GetCursorScreenPos();
            float  bw=ImGui::GetContentRegionAvail().x;
            const float BH=24.f;
            ImGui::Dummy({bw,BH});
            ImDrawList*dl=ImGui::GetWindowDrawList();
            // Background: red tint if engaged, dark green otherwise
            ImU32 bg = eng ? IM_COL32(80,0,0,240) : IM_COL32(8,22,8,255);
            dl->AddRectFilled(bp,{bp.x+bw,bp.y+BH},bg,2.f);
            // Left priority stripe colour by rank
            ImU32 stripe[]={
                IM_COL32(255,  0,  0,255),  // P1 red
                IM_COL32(255,120,  0,255),  // P2 orange
                IM_COL32(255,220,  0,255),  // P3 yellow
                IM_COL32(180,220,  0,255),  // P4 yellow-green
                IM_COL32(100,200, 50,255),  // P5 green
            };
            dl->AddRectFilled(bp,{bp.x+4,bp.y+BH},stripe[i]);
            dl->AddLine({bp.x,bp.y+BH},{bp.x+bw,bp.y+BH},IM_COL32(14,50,14,200),1.f);
            // Title text
            char hdr[96];
            snprintf(hdr,sizeof(hdr),"  [%s]  %s     %s     %.0f m",
                pri[i],t.getId().c_str(),s.direction.c_str(),s.distance);
            float ty=bp.y+(BH-ImGui::GetTextLineHeight())*.5f;
            ImU32 tc=eng ? IM_COL32(255,80,80,255) : IM_COL32(200,240,200,255);
            dl->AddText({bp.x,ty},tc,hdr);
            // Engaged marker on right
            if(eng){
                const char*em=" \xe2\x97\x8f ENGAGING ";
                float ew=ImGui::CalcTextSize(em).x;
                dl->AddText({bp.x+bw-ew-4,ty},IM_COL32(255,60,60,255),em);
            }
        }

        //Solution table
        if(ImGui::BeginTable("fsol",2,
            ImGuiTableFlags_Borders|ImGuiTableFlags_RowBg,{0,0})){
            ImGui::TableSetupColumn("Parameter",
                ImGuiTableColumnFlags_WidthFixed,118);
            ImGui::TableSetupColumn("Value");

            KVC("Elevation",    {0,1,1,1},          "%.1f\xc2\xb0",  s.elevation);
            KVC("Azimuth",      {0,1,1,1},          "%.1f\xc2\xb0",  s.azimuth);
            KV ("H-Distance",                       "%.0f m",         s.distance);
            KV ("3D Displacement",                  "%.0f m",
                t.calculateDisplacement(radar.getPosition()));
            KV ("Direction",                        "%s",s.direction.c_str());
            KVC("Target Speed", spdCol(t.getSpeed()),"%.1f m/s",t.getSpeed());
            KV ("Target Accel",                     "%.1f m/s\xc2\xb2",
                t.getAcceleration().magnitude());
            double lead=s.distance>0
                ? atan2(t.getSpeed()*.5,s.distance)*180./M_PI : 0.;
            KVC("Lead Angle",   {1,.8f,.35f,1},     "%.2f\xc2\xb0",lead);
            KVC("Solution",     {0,1,.5f,1},         "%s",s.solutionText.c_str());

            const char*st; ImVec4 sc;
            double R=radar.getRange();
            if(s.distance<R*.30){st="CRITICAL \xe2\x80\x94 FIRE NOW";sc={1,0,0,1};}
            else if(s.distance<R*.60){st="IN RANGE";           sc={1,.8f,0,1};}
            else                    {st="TRACKING";             sc={.8f,.8f,0,1};}
            KVC("Status",sc,"%s",st);
            ImGui::EndTable();
        }

        // ── Engage / Abort / Countdown button ────────────────
        ImGui::Spacing();
        if(eng && g_engageTimer > 0.0){
            // Shot is in flight — show animated countdown, no abort allowed
            char cfmt[48];
            snprintf(cfmt, sizeof(cfmt),
                "  â  MISSILE INBOUND  %.1fs  ",
                (float)g_engageTimer);
            // Pulse the button colour using timer
            float pulse = (float)(g_engageTimer * 6.0);
            float bright = 0.5f + 0.3f * sinf(pulse);
            ImGui::PushStyleColor(ImGuiCol_Button,
                {bright, 0.05f, 0.05f, 1.f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                {bright, 0.05f, 0.05f, 1.f});
            ImGui::Button(cfmt, {-1, 26});
            ImGui::PopStyleColor(2);
        } else if(eng){
            // Engaged but timer not started yet (one-frame gap)
            ImGui::PushStyleColor(ImGuiCol_Button,      {.44f,0,0,1});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,{.70f,0,0,1});
            if(ImGui::Button("ABORT ENGAGEMENT",{-1,26}))
                radar.clearEngagement();
            ImGui::PopStyleColor(2);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button,      {.62f,0,0,1});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,{.88f,0,0,1});
            if(ImGui::Button("ENGAGE TARGET",{-1,26}))
                radar.setEngagement(t.getId());
            ImGui::PopStyleColor(2);
        }

        ImGui::PopID();
        if(i<show-1){
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
        }
    }
}

//  THREAT PANEL  (bottom-right, tab 2)

void renderThreatPanel(const Radar&radar,
                       const vector<Target>&targets,
                       const char logBuf[][80], int logN)
{
    SH("THREAT OVERVIEW",{.90f,.64f,.10f,1.f});

    int total=0,crit=0,high=0,med=0,low=0;
    double R=radar.getRange();
    for(const auto&t:targets){
        if(!radar.isInRange(t)||radar.identifyTarget(t))continue;
        total++;
        double f=t.calculateHorizontalDistance(radar.getPosition())/R;
        if(f<.25)crit++; else if(f<.50)high++; else if(f<.75)med++; else low++;
    }

    ImGui::Spacing();
    if(!total){
        ImGui::TextColored({.2f,.9f,.2f,1},"  No hostile contacts in range.");
    } else {
        ImGui::Text("Hostile contacts: %d",total);
        ImGui::Spacing();
        // Segmented bar
        float bw=ImGui::GetContentRegionAvail().x, bh=24.f;
        ImVec2 bp=ImGui::GetCursorScreenPos();
        ImGui::Dummy({bw,bh});
        ImDrawList*dl=ImGui::GetWindowDrawList();
        float x=bp.x;
        auto seg=[&](int n,ImVec4 c){
            if(!n)return;
            float w=bw*n/(float)total;
            dl->AddRectFilled({x,bp.y},{x+w,bp.y+bh},
                              ImGui::ColorConvertFloat4ToU32(c));
            char lb[6]; snprintf(lb,sizeof(lb),"%d",n);
            ImVec2 ts=ImGui::CalcTextSize(lb);
            dl->AddText({x+(w-ts.x)*.5f,bp.y+(bh-ts.y)*.5f},
                        IM_COL32(0,0,0,230),lb);
            x+=w;
        };
        seg(crit,{1,.10f,.10f,1}); seg(high,{1,.50f,.10f,1});
        seg(med, {1, 1.f,.10f,1}); seg(low, {.22f,.88f,.22f,1});
        dl->AddRect(bp,{bp.x+bw,bp.y+bh},IM_COL32(50,50,50,200),0,0,1.f);
        ImGui::Spacing();
        ImGui::TextColored({1,.10f,.10f,1},"CRIT %d",crit); ImGui::SameLine(0,10);
        ImGui::TextColored({1,.50f,.10f,1},"HIGH %d",high); ImGui::SameLine(0,10);
        ImGui::TextColored({1, 1.f,.10f,1},"MED %d", med);  ImGui::SameLine(0,10);
        ImGui::TextColored({.22f,.88f,.22f,1},"LOW %d",low);
    }

    // Detection log
    ImGui::Spacing();
    SH("DETECTION LOG",{.38f,.65f,.92f,1.f});
    ImGui::Spacing();

    ImGui::BeginChild("DL",{0,150},true);
    for(int i=0;i<logN;i++){
        bool in=(logBuf[i][1]=='I');
        ImGui::TextColored(
            in?ImVec4(.22f,.88f,.22f,1):ImVec4(.40f,.40f,.40f,1),
            "%s",logBuf[i]);
    }
    if(ImGui::GetScrollY()>=ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.f);
    ImGui::EndChild();

    // Reference 
    ImGui::Spacing();
    SH("REFERENCE",{.55f,.55f,.55f,1.f});
    ImGui::Spacing();
    ImGui::TextColored({0,.88f,.88f,1},  "  \xe2\x96\xb2 Friendly (IFF confirmed)");
    ImGui::TextColored({1,.88f,.10f,1},  "  \xe2\x96\xa0 Unknown");
    ImGui::TextColored({1,.18f,.18f,1},  "  \xe2\x97\x8f Hostile (< 33%% range)");
    ImGui::TextColored({1,.50f,.10f,1},  "  \xe2\x97\x86 Mouse target");
    ImGui::Spacing();
    ImGui::TextColored({.50f,.72f,.50f,1},
        "  DSA: Queue CircularLL HashMap\n"
        "       Array ExprTree QuickSort");
}