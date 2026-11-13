#include "demo_walkthrough.hpp"
#include <iostream>
using namespace elemental_coop;
using namespace prototype::verification;
int main() {
    try {
        for(std::size_t i=0;i<2;++i) for(auto h:{Hazard::lava,Hazard::water,Hazard::toxic}) {
            prototype::DemoLevel l; l.touch_hazard(i,h);
            const bool safe=(i==0 && h==Hazard::lava)||(i==1 && h==Hazard::water);
            check((l.state()==RoundState::playing)==safe,"Elemental immunity rules");
        }
        prototype::DemoLevel level;
        level.interact(); check(!level.lever(),"Lever cannot activate remotely");
        solve(level); level.restart();
        check(!level.open() && !level.lever() && !level.plate(),"Restart resets puzzle");
        check(level.position(0).x==150 && level.position(1).x==1130,"Restart restores spawns");
        std::cout<<"PASS: physical jump routes, cooperative gate, elemental hazards, both exits, restart\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
