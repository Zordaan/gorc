#include "update_surface_material_aspect.hpp"

gorc::game::world::animations::aspects::update_surface_material_aspect::update_surface_material_aspect(entity_component_system<thing_id>& cs, level_model& model)
    : inner_join_aspect(cs), model(model) {
    return;
}

void gorc::game::world::animations::aspects::update_surface_material_aspect::update(time_delta t, thing_id, components::surface_material& anim) {
    anim.framerate_accumulator += t.count();

    int surface_material = at_id(model.level->surfaces, anim.surface).material;
    if(surface_material < 0) {
        // TODO: Surface has no material but has an animation? report error
        return;
    }

    size_t num_cels = std::get<0>(model.level->materials[surface_material]).get_value()->cels.size();

    while(anim.framerate_accumulator >= anim.framerate) {
        anim.framerate_accumulator -= anim.framerate;

        size_t next_cel = at_id(model.surfaces, anim.surface).cel_number + 1;
        if(next_cel >= num_cels) {
            if(anim.flag & flags::anim_flag::skip_first_two_frames) {
                next_cel = 2UL % num_cels;
            }
            else if(anim.flag & flags::anim_flag::skip_first_frame) {
                next_cel = 1UL % num_cels;
            }
            else {
                next_cel = 0;
            }
        }

        at_id(model.surfaces, anim.surface).cel_number = static_cast<int>(next_cel);
    }
}
