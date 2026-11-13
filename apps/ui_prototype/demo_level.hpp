#pragma once
// App-local reference room. Reuses the team's ECS, input frames and collision system.
// It intentionally does not replace the shared elemental_coop::Game implementation.
#include "elemental_coop/game.hpp"
#include <vector>
#include <cmath>

namespace prototype {
struct Rect { float x,y,w,h; };
struct Pool { Rect box; elemental_coop::Hazard kind; };
class DemoLevel {
public:
    const std::vector<Rect> platforms{
        {0,580,260,70},{410,580,155,70},{705,580,190,70},{1030,580,250,70},
        {260,630,150,20},{565,630,140,20},{895,630,135,20},
        {290,465,100,28},{890,465,100,28},
        {400,350,165,32},{565,380,145,32},{710,350,170,32},
        {50,255,300,38},{930,255,300,38}
    };
    const std::array<Pool,3> pools{{{{260,596,150,34},elemental_coop::Hazard::lava},
        {{565,596,140,34},elemental_coop::Hazard::water},{{895,596,135,34},elemental_coop::Hazard::toxic}}};
    static constexpr Rect door{605,270,70,110};
    DemoLevel() { restart(); }
    void restart() {
        world_=engine::World{};
        engine::PhysicsSettings settings; settings.gravity={0,1000}; settings.max_speed={260,900};
        physics_=engine::PhysicsSystem{settings};
        for(const auto& r:platforms) solid(r);
        solid({-30,0,30,720}); solid({1280,0,30,720});
        door_=solid(door);
        for(std::size_t i=0;i<2;++i) {
            engine::Transform t; t.position={i==0?150.0F:1130.0F,580};
            const auto id=world_.create(t);
            world_.add<engine::Body>(id);
            world_.add<engine::Collider>(id,{{-14,-46},{28,46}});
            physics_.add(world_,id);
            players_[i]={id,i==0?elemental_coop::Element::fire:elemental_coop::Element::water,{},true,true,false};
        }
        state_=elemental_coop::RoundState::playing; lever_=plate_=open_=false;
    }
    void interact() {
        if(state_!=elemental_coop::RoundState::playing) return;
        for(std::size_t i=0;i<2;++i) {
            const auto p=position(i);
            if(std::abs(p.x-485)<60 && std::abs(p.y-580)<12) { lever_=!lever_; break; }
        }
    }
    void tick(const elemental_coop::PlayerInputs& frames,float dt) {
        if(state_!=elemental_coop::RoundState::playing || dt<=0) return;
        for(std::size_t i=0;i<2;++i) {
            auto* body=world_.get<engine::Body>(players_[i].entity);
            body->velocity.x=frames[i].move_x*240;
            if(frames[i].primary_pressed && players_[i].grounded) body->velocity.y=-630;
        }
        physics_.step(world_,dt);
        plate_=false;
        for(std::size_t i=0;i<2;++i) {
            auto& player=players_[i];
            player.body=*world_.get<engine::Body>(player.entity);
            const auto* contacts=world_.get<engine::ContactState>(player.entity);
            player.grounded=contacts && contacts->grounded;
            const auto p=position(i);
            if(p.x>752 && p.x<858 && std::abs(p.y-580)<4) plate_=true;
            const engine::Aabb body{{p.x-14,p.y-46},{28,46}};
            for(const auto& pool:pools)
                if(engine::overlaps(body,{{pool.box.x,pool.box.y},{pool.box.w,pool.box.h}})) touch_hazard(i,pool.kind);
            if(p.y>715) touch_hazard(i,elemental_coop::Hazard::toxic);
        }
        if(lever_ && plate_ && !open_) { open_=true; world_.destroy(door_); }
        bool both=true;
        for(std::size_t i=0;i<2;++i) {
            const auto p=position(i);
            players_[i].at_exit=open_ && std::abs(p.x-(i==0?155.0F:1125.0F))<35 && std::abs(p.y-255)<4;
            both=both && players_[i].at_exit;
        }
        if(both && state_==elemental_coop::RoundState::playing) state_=elemental_coop::RoundState::won;
    }
    void touch_hazard(std::size_t i,elemental_coop::Hazard h) {
        const bool safe=(i==0 && h==elemental_coop::Hazard::lava) || (i==1 && h==elemental_coop::Hazard::water);
        if(!safe) { players_.at(i).alive=false; state_=elemental_coop::RoundState::lost; }
    }
    engine::Vec2 position(std::size_t i) const { return world_.find(players_.at(i).entity)->position; }
    const engine::World& world() const { return world_; }
    const elemental_coop::PlayerState& player(std::size_t i) const { return players_.at(i); }
    elemental_coop::RoundState state() const { return state_; }
    bool lever() const { return lever_; }
    bool plate() const { return plate_; }
    bool open() const { return open_; }
private:
    engine::World world_;
    engine::PhysicsSystem physics_;
    std::array<elemental_coop::PlayerState,2> players_{};
    elemental_coop::RoundState state_{};
    engine::EntityId door_{};
    bool lever_{},plate_{},open_{};
    engine::EntityId solid(Rect r) {
        engine::Transform t; t.position={r.x,r.y};
        const auto id=world_.create(t);
        world_.add<engine::Collider>(id,{{0,0},{r.w,r.h}}); physics_.add(world_,id); return id;
    }
};
}
