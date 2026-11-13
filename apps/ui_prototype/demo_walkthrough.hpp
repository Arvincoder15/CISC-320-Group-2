#pragma once
#include "demo_level.hpp"
#include <stdexcept>
#include <string>
namespace prototype::verification {
using namespace elemental_coop;
void check(bool condition,const char* message) { if(!condition) throw std::runtime_error(message); }
// These paths use input frames and normal physics only: no teleports or win shortcuts.
void travel(prototype::DemoLevel& level,std::size_t who,float target,bool jump) {
    bool fired=false;
    for(int tick=0;tick<240;++tick) {
        const auto pos=level.position(who);
        PlayerInputs input{};
        const float distance=target-pos.x;
        if(std::abs(distance)>3) input[who].move_x=distance>0?1.0F:-1.0F;
        if(jump && !fired && level.player(who).grounded) { input[who].primary_pressed=true; fired=true; }
        level.tick(input,1.0F/120.0F);
        if(level.state()==RoundState::won) return;
        if(level.state()==RoundState::lost) throw std::runtime_error("Path hit hazard for player "+std::to_string(who)+" to "+std::to_string(target));
        if(tick>3 && std::abs(level.position(who).x-target)<=3 && level.player(who).grounded) return;
    }
    throw std::runtime_error("Path stuck to "+std::to_string(target)+" at "+std::to_string(level.position(who).x)+","+std::to_string(level.position(who).y));
}
void solve(prototype::DemoLevel& level) {
    travel(level,0,230,false); travel(level,0,485,true);
    level.interact(); check(level.lever(),"Lever activated"); check(!level.open(),"Lever alone cannot open door");
    travel(level,1,1060,false); travel(level,1,805,true);
    check(level.plate() && level.open(),"Lever and pressure plate unlock door");
    travel(level,0,340,true); travel(level,0,480,true); travel(level,0,320,true); travel(level,0,155,false);
    check(level.player(0).at_exit && level.state()==RoundState::playing,"One exit is insufficient");
    travel(level,1,940,true); travel(level,1,800,true); travel(level,1,950,true); travel(level,1,1125,false);
    check(level.state()==RoundState::won,"Both matching exits win");
}
}
