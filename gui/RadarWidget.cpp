// RadarWidget.cpp — radar scope, renders inside an existing child window
#include "RadarWidget.hpp"
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <string>
#include <vector>
using namespace std;

// Engagement state from GuiMain
struct Explosion { Vector2D worldPos; float age; float maxAge; };
extern vector<Explosion> g_explosions;
extern double g_engageTimer;
static const double ENGAGE_FLIGHT_EXT = 2.0;  // must match GuiMain

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// colour helpers
static ImU32 rgb(int r,int g,int b,int a=255){return IM_COL32(r,g,b,a);}
static ImVec2 polar(ImVec2 c,float r,float deg){
    float rad=deg*(float)M_PI/180.f;
    return {c.x+r*cosf(rad), c.y-r*sinf(rad)};
}
// draw text with dark background so it's always readable
static void lblDraw(ImDrawList*dl,ImVec2 p,ImU32 col,const char*s){
    ImVec2 sz=ImGui::CalcTextSize(s);
    dl->AddRectFilled({p.x-2,p.y-1},{p.x+sz.x+2,p.y+sz.y+1},
                      IM_COL32(0,0,0,180),2.f);
    dl->AddText(p,col,s);
}

// ── Internal draw functions ───────────────────────────────────
static void drawRings(ImDrawList*dl,ImVec2 c,float R){
    // Spokes
    for(int a=0;a<360;a+=30)
        dl->AddLine(c,polar(c,R,(float)a),rgb(0,42,0,85),0.8f);
    // 3 range rings
    ImU32 rc[]={rgb(0,110,0,130), rgb(80,160,0,120), rgb(180,75,0,120)};
    float rw[]={1.f,1.2f,1.6f};
    for(int i=0;i<3;i++) dl->AddCircle(c,R*(i+1)/3.f,rc[i],120,rw[i]);
    // Ring distance labels at right side of each ring
    const char* rlbl[]={"33%","66%","100%"};
    for(int i=0;i<3;i++){
        float rx=c.x+R*(i+1)/3.f+3;
        dl->AddText({rx,c.y-7},rc[i],rlbl[i]);
    }
}

static void drawCompassRose(ImDrawList*dl,ImVec2 c,float R){
    // Cardinal
    struct{const char*l;float a;} card[]={{"N",90},{"E",0},{"S",270},{"W",180}};
    ImU32 cardCol=rgb(160,215,160,210);
    for(auto&d:card){
        ImVec2 lp=polar(c,R+20,d.a);
        ImVec2 ts=ImGui::CalcTextSize(d.l);
        dl->AddText({lp.x-ts.x*.5f,lp.y-ts.y*.5f},cardCol,d.l);
        dl->AddLine(polar(c,R,d.a),polar(c,R+10,d.a),cardCol,2.f);
    }
    // Intercardinal
    struct{const char*l;float a;} inter[]={{"NE",45},{"SE",315},{"SW",225},{"NW",135}};
    ImU32 subCol=rgb(100,160,100,160);
    for(auto&d:inter){
        ImVec2 lp=polar(c,R+14,d.a);
        ImVec2 ts=ImGui::CalcTextSize(d.l);
        dl->AddText({lp.x-ts.x*.5f,lp.y-ts.y*.5f},subCol,d.l);
        dl->AddLine(polar(c,R,d.a),polar(c,R+6,d.a),subCol,1.2f);
    }
    // Minor ticks every 10°
    for(int a=0;a<360;a+=10){
        if(a%45==0) continue;
        float len=(a%30==0)?5.f:3.f;
        dl->AddLine(polar(c,R,(float)a),polar(c,R+len,(float)a),subCol,0.7f);
    }
}

static void drawSweep(ImDrawList*dl,ImVec2 c,float R,float deg){
    // Fading fan trail — 45 steps of 2° each
    for(int i=45;i>=1;i--){
        float a=deg-i*2.f;
        float f=1.f-(float)i/45.f;
        float w=1.f+f*2.5f;
        int   green=(int)(f*f*170);
        int   alpha=(int)(f*f*75);
        dl->AddLine(c,polar(c,R,a),IM_COL32(0,green,18,alpha),w);
    }
    // Main bright line
    dl->AddLine(c,polar(c,R,deg),rgb(0,255,85,255),3.f);
    // Glowing tip
    ImVec2 tip=polar(c,R,deg);
    dl->AddCircleFilled(tip,6.f,rgb(0,255,85,255));
    dl->AddCircleFilled(tip,12.f,rgb(0,255,85,35));
}

static void drawBlips(ImDrawList*dl,ImVec2 c,float sc,
                      const vector<Target>&tgts,const Radar&radar,
                      const string&selectedId)
{
    // pulse for engaged target ring
    static float pulse=0.f; pulse+=0.04f;

    for(const Target&t:tgts){
        if(!radar.isInRange(t)) continue;
        Vector2D wp=t.getPosition();
        ImVec2 sp={c.x+(float)wp.x*sc, c.y-(float)wp.y*sc};

        int  lvl  = radar.classifyThreat(t);
        bool isMou= t.getId().find("MOUSE")!=string::npos;
        bool isEng= radar.isEngaged()&&radar.getEngagedTarget()==t.getId();
        bool isSel= t.getId()==selectedId;
        float spd = (float)t.getSpeed();

        // selection highlight
        if(isSel && !isEng)
            dl->AddCircle(sp,22.f,rgb(255,255,255,120),32,1.5f);

        // engagement rings (animated)
        if(isEng){
            float pr=18.f+sinf(pulse)*4.f;
            dl->AddCircle(sp,pr,       rgb(255,0,0,200),48,2.2f);
            dl->AddCircle(sp,pr+7.f,   rgb(255,0,0, 55),48,1.f);
            float ch=22.f;
            dl->AddLine({sp.x-ch,sp.y},{sp.x+ch,sp.y},rgb(255,0,0,180),1.5f);
            dl->AddLine({sp.x,sp.y-ch},{sp.x,sp.y+ch},rgb(255,0,0,180),1.5f);
        }

        // blip shape by IFF class
        if(isMou){
            // Orange diamond
            float s=10.f;
            dl->AddQuadFilled({sp.x,sp.y-s},{sp.x+s,sp.y},
                              {sp.x,sp.y+s},{sp.x-s,sp.y}, rgb(255,115,0,255));
            dl->AddQuad({sp.x,sp.y-s},{sp.x+s,sp.y},
                        {sp.x,sp.y+s},{sp.x-s,sp.y},rgb(255,255,255,160),1.5f);
            dl->AddCircle(sp,16.f,rgb(255,115,0,50),32,1.f);
        } else if(lvl==0){
            // Friendly — cyan upward triangle
            float s=8.f;
            dl->AddTriangleFilled({sp.x,sp.y-s},{sp.x+s,sp.y+s},
                                  {sp.x-s,sp.y+s},rgb(0,200,255,255));
            dl->AddTriangle({sp.x,sp.y-s},{sp.x+s,sp.y+s},
                            {sp.x-s,sp.y+s},rgb(255,255,255,140),1.2f);
            dl->AddCircle(sp,14.f,rgb(0,200,255,28),32,1.f);
        } else if(lvl==2){
            // Close hostile — red filled circle + double warning ring
            dl->AddCircleFilled(sp,9.5f, rgb(255,35,35,255));
            dl->AddCircle(sp,14.f, rgb(255,55,55,150),32,1.8f);
            dl->AddCircle(sp,20.f, rgb(255,55,55, 50),32,1.f);
        } else {
            // Unknown — yellow square
            float s=7.f;
            dl->AddRectFilled({sp.x-s,sp.y-s},{sp.x+s,sp.y+s},rgb(255,210,0,255));
            dl->AddRect({sp.x-s,sp.y-s},{sp.x+s,sp.y+s},rgb(255,255,255,140),0,0,1.2f);
            dl->AddCircle(sp,14.f,rgb(255,210,0,28),32,1.f);
        }

        // velocity arrow
        if(spd>1.f){
            Vector2D vel=t.getVelocity();
            float mag=(float)vel.magnitude();
            if(mag>0.01f){
                float nx=(float)vel.x/mag, ny=(float)vel.y/mag;
                float len=10.f+min(spd*.3f,28.f);
                ImVec2 tip2={sp.x+nx*len, sp.y-ny*len};
                dl->AddLine(sp,tip2,rgb(255,155,0,210),1.8f);
                // arrowhead
                float ha=0.45f;
                dl->AddLine(tip2,{tip2.x-(nx*cosf( ha)+ny*sinf( ha))*5.f,
                                   tip2.y+(nx*(-sinf( ha))+ny*cosf( ha))*5.f},rgb(255,155,0,210),1.4f);
                dl->AddLine(tip2,{tip2.x-(nx*cosf(-ha)+ny*sinf(-ha))*5.f,
                                   tip2.y+(nx*(-sinf(-ha))+ny*cosf(-ha))*5.f},rgb(255,155,0,210),1.4f);
            }
        }

        // label: ID + speed
        char lbl[48];
        if(spd>1.f) snprintf(lbl,sizeof(lbl),"%s  %.0fm/s",t.getId().c_str(),spd);
        else        snprintf(lbl,sizeof(lbl),"%s",t.getId().c_str());
        ImVec2 ls=ImGui::CalcTextSize(lbl);
        lblDraw(dl,{sp.x-ls.x*.5f, sp.y+14.f},rgb(190,235,190,255),lbl);
    }
}

// ── Public entry point ────────────────────────────────────────
void renderRadarWidget(const Radar&radar,
                       const vector<Target>&targets,
                       Target*mouseTarget,
                       const string&selectedId)
{
    ImDrawList*dl  = ImGui::GetWindowDrawList();
    ImVec2     av  = ImGui::GetContentRegionAvail();

    // fixed header and footer heights
    const float HH=26.f, FH=22.f;
    float cW=av.x;
    float cH=av.y-HH-FH;
    if(cH<80.f) cH=80.f;

    // ── Header bar ───────────────────────────────────────────
    {
        ImVec2 hp=ImGui::GetCursorScreenPos();
        // reserve space first so text renders on top of background
        ImGui::Dummy({cW,HH});
        dl->AddRectFilled(hp,{hp.x+cW,hp.y+HH},IM_COL32(2,10,2,255));
        dl->AddRectFilled(hp,{hp.x+4,hp.y+HH}, IM_COL32(0,195,60,255));
        dl->AddLine({hp.x,hp.y+HH},{hp.x+cW,hp.y+HH},IM_COL32(0,85,0,220),1.f);
        float ty=hp.y+(HH-ImGui::GetTextLineHeight())*.5f;
        char hbuf[100];
        snprintf(hbuf,sizeof(hbuf),
            "  RADAR SCOPE    SWEEP: %05.1f\xc2\xb0    RANGE: %.0f m",
            radar.getCurrentSweepAngle(), radar.getRange());
        dl->AddText({hp.x,ty}, IM_COL32(45,215,70,255), hbuf);
        // Paused indicator on right
        extern bool g_simPaused;
        if(g_simPaused){
            const char*ps="  \xe2\x96\xae\xe2\x96\xae PAUSED  ";
            float pw=ImGui::CalcTextSize(ps).x;
            dl->AddText({hp.x+cW-pw-4,ty},IM_COL32(255,200,0,255),ps);
        }
    }

    // ── Scope canvas ─────────────────────────────────────────
    ImVec2 orig=ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##scope",{cW,cH});

    // radius: use 42% of the smaller dimension, leaving ~16% margin for labels
    float R   = min(cW,cH)*0.42f;
    // Centre the scope circle in the canvas
    ImVec2 ctr = {orig.x + cW*0.5f, orig.y + cH*0.5f};
    float  scl = R / (float)radar.getRange();

    dl->PushClipRect(orig,{orig.x+cW,orig.y+cH},true);

    // Background
    dl->AddRectFilled(orig,{orig.x+cW,orig.y+cH},IM_COL32(2,7,2,255));
    // Scope inner glow
    dl->AddCircleFilled(ctr,R+2.f,  IM_COL32(0,22,0,255),120);
    dl->AddCircleFilled(ctr,R,      IM_COL32(2,8,2,255), 120);
    // Canvas border
    dl->AddRect(orig,{orig.x+cW,orig.y+cH},IM_COL32(0,85,0,210),3.f,0,1.5f);
    // Scope circle border
    dl->AddCircle(ctr,R,IM_COL32(0,100,0,200),120,1.5f);

    drawRings(dl,ctr,R);
    drawCompassRose(dl,ctr,R);
    drawSweep(dl,ctr,R,(float)radar.getCurrentSweepAngle());
    drawBlips(dl,ctr,scl,targets,radar,selectedId);

    // extra highlight on mouse target
    if(mouseTarget && radar.isInRange(*mouseTarget)){
        Vector2D wp=mouseTarget->getPosition();
        ImVec2 sp={ctr.x+(float)wp.x*scl, ctr.y-(float)wp.y*scl};
        dl->AddCircle(sp,18.f,IM_COL32(255,125,0,190),32,2.f);
    }

    // Radar/Gun station at centre
    dl->AddCircleFilled(ctr,6.f,IM_COL32(0,215,0,255));
    dl->AddCircle(ctr,10.f,IM_COL32(0,180,0,110),32,1.f);
    dl->AddLine({ctr.x-13,ctr.y},{ctr.x+13,ctr.y},IM_COL32(0,215,0,255),1.5f);
    dl->AddLine({ctr.x,ctr.y-13},{ctr.x,ctr.y+13},IM_COL32(0,215,0,255),1.5f);
    lblDraw(dl,{ctr.x+14,ctr.y+9},IM_COL32(0,215,0,255),"GUN");

    // ── Explosion effects ────────────────────────────────────
    for(const auto&ex : g_explosions){
        float t = ex.age / ex.maxAge;          // 0..1 progress
        float easing = 1.f - t*t;             // fast-out
        float R2 = R * 0.18f * t;             // expanding ring radius
        float R3 = R * 0.30f * t;
        float R4 = R * 0.08f * (1.f-t);       // shrinking core flash

        // World → screen
        float ex_sx = ctr.x + (float)ex.worldPos.x * scl;
        float ex_sy = ctr.y - (float)ex.worldPos.y * scl;
        ImVec2 ep = {ex_sx, ex_sy};

        int  alp1 = (int)(easing * 240);
        int  alp2 = (int)(easing * 140);
        int  alpC = (int)((1.f-t)*(1.f-t) * 255);

        // Core white flash
        dl->AddCircleFilled(ep, R4,        IM_COL32(255,255,220,alpC));
        // Inner orange ring
        dl->AddCircle(ep,      R2,         IM_COL32(255,140, 20,alp1), 48, 3.f);
        // Outer red ring
        dl->AddCircle(ep,      R3,         IM_COL32(255, 40, 20,alp2), 48, 2.f);
        // Debris sparks — 8 radial lines
        for(int sp=0;sp<8;sp++){
            float sa = sp * (float)M_PI * .25f;
            float slen = R * .12f * easing;
            ImVec2 from = {ep.x + cosf(sa)*R*0.05f,  ep.y + sinf(sa)*R*0.05f};
            ImVec2 to   = {ep.x + cosf(sa)*slen,     ep.y + sinf(sa)*slen};
            dl->AddLine(from, to, IM_COL32(255,200,80,alp1), 1.5f);
        }
        // "DESTROYED" label while still fresh
        if(t < 0.5f){
            const char* dlbl = "DESTROYED";
            ImVec2 dts = ImGui::CalcTextSize(dlbl);
            int da = (int)((0.5f-t)*2.f * 255);
            dl->AddRectFilled(
                {ep.x-dts.x*.5f-3, ep.y-22-dts.y},
                {ep.x+dts.x*.5f+3, ep.y-22},
                IM_COL32(0,0,0,da), 2.f);
            dl->AddText({ep.x-dts.x*.5f, ep.y-22-dts.y+1},
                        IM_COL32(255,80,80,da), dlbl);
        }
    }

    // ── Intercept countdown beam ──────────────────────────────
    // While a shot is in flight, draw a pulsing line from GUN to target
    if(g_engageTimer > 0.0 && radar.isEngaged()){
        // Find target world position
        // We don't have targets here, but we can draw from centre outward
        // using the engaged target's last known screen pos — drawn in drawBlips.
        // Instead draw a simple "missile in flight" pulsing dot moving outward.
        // We'll just animate a dot along the centre→engaged-target vector.
        // Since we don't have targets list here, we signal via a bright
        // pulsing ring around GUN centre.
        static float beamPulse = 0.f; beamPulse += 0.18f;
        float bR = 8.f + sinf(beamPulse)*4.f;
        dl->AddCircle(ctr, bR,      IM_COL32(255,200,0,200), 32, 2.f);
        dl->AddCircle(ctr, bR+6.f,  IM_COL32(255,150,0, 80), 32, 1.f);
        // countdown text next to GUN
        char cbuf[24];
        snprintf(cbuf, sizeof(cbuf), "%.1fs", (float)g_engageTimer);
        float tw2 = ImGui::CalcTextSize(cbuf).x;
        dl->AddRectFilled(
            {ctr.x-tw2*.5f-2, ctr.y+14},
            {ctr.x+tw2*.5f+2, ctr.y+28},
            IM_COL32(0,0,0,200), 2.f);
        dl->AddText({ctr.x-tw2*.5f, ctr.y+15},
                    IM_COL32(255,220,0,255), cbuf);
    }

    dl->PopClipRect();

    // ── Footer bar ───────────────────────────────────────────
    {
        ImVec2 fp=ImGui::GetCursorScreenPos();
        ImGui::Dummy({cW,FH});
        dl->AddRectFilled(fp,{fp.x+cW,fp.y+FH},IM_COL32(2,8,2,255));
        dl->AddLine(fp,{fp.x+cW,fp.y},IM_COL32(0,75,0,190),1.f);
        float ty=fp.y+(FH-ImGui::GetTextLineHeight())*.5f;

        int frn=0,unk=0,htl=0;
        for(const auto&t:targets){
            if(!radar.isInRange(t)) continue;
            int lv=radar.classifyThreat(t);
            if(lv==0)frn++; else if(lv==2)htl++; else unk++;
        }
        char fb[160];
        snprintf(fb,sizeof(fb),
            "   FRN: %d   UNK: %d   HTL: %d"
            "        \xe2\x96\xb2 Friendly   \xe2\x96\xa0 Unknown"
            "   \xe2\x97\x8f Hostile   \xe2\x97\x86 Mouse",
            frn,unk,htl);
        dl->AddText({fp.x,ty},IM_COL32(125,178,125,215),fb);
    }
}