#pragma once

#include "math/matrix.hpp"
#include "libold/base/view.hpp"
#include "math/box.hpp"
#include "utility/flag_set.hpp"
#include "libold/base/utility/randomizer.hpp"
#include "libold/content/flags/key_flag.hpp"
#include "libold/content/flags/geometry_mode.hpp"
#include "libold/content/flags/light_mode.hpp"
#include "level_shader.hpp"
#include "client/client_renderer_object_factory.hpp"
#include <stack>
#include <unordered_set>

namespace gorc {

class content_manager;
class material;

namespace content {
namespace assets {
class thing_template;
class level_sector;
class shader;
class model;
class animation;
class sprite;
class puppet;
}
}

namespace game {
class level_state;

namespace world {
class level_presenter;
class level_model;

namespace components {
class thing;
}

}

}

namespace client {

namespace world {

class level_view : public view {
private:
    client_renderer_object_factory &renderer_object_factory;
    randomizer rand;
    surface_shader surfaceShader;
    horizon_shader horizonShader;
    ceiling_shader ceilingShader;
    light_shader lightShader;

    level_shader* currentLevelShader = nullptr;

    matrix<4> projection_matrix = make_identity_matrix<4>();
    matrix<4> view_matrix = make_identity_matrix<4>();
    std::stack<matrix<4>> model_matrix_stack;

    template <typename T, typename... U> void set_current_shader(T& shader, U... args) {
        currentLevelShader = &shader;
        shader.activate(args...);
        shader.set_projection_matrix(projection_matrix);
        shader.set_view_matrix(view_matrix);
        shader.set_model_matrix(model_matrix_stack.top());
    }

    game::world::level_presenter* currentPresenter = nullptr;
    game::world::level_model* currentModel = nullptr;
    std::unordered_set<sector_id> sector_vis_scratch;
    std::unordered_set<sector_id> sector_visited_scratch;
    std::vector<std::tuple<sector_id, surface_id>> horizon_sky_surfaces_scratch;
    std::vector<std::tuple<sector_id, surface_id>> ceiling_sky_surfaces_scratch;
    std::vector<std::tuple<sector_id, surface_id, float>> translucent_surfaces_scratch;
    std::vector<std::tuple<thing_id, float>> visible_thing_scratch;

    void compute_visible_sectors(const box<2, int>& view_size);
    void record_visible_special_surfaces();
    void record_visible_things();
    void do_sector_vis(sector_id sec_num, const std::array<double, 16>& proj_mat, const std::array<double, 16>& view_mat,
            const std::array<int, 4>& viewport, const box<2, double>& adj_bbox, const vector<3>& cam_pos, const vector<3>& cam_look);
    void draw_visible_diffuse_surfaces();
    void draw_visible_sky_surfaces(const box<2, int>& screen_size, const color_rgb& sector_tint);
    void draw_visible_translucent_surfaces_and_things();
    void draw_pov_model();

    void draw_surface(surface_id surf_num, const content::assets::level_sector& sector, float alpha);
    void draw_sprite(const game::world::components::thing& thing, asset_ref<content::assets::sprite> sprite, float sector_light);
    void draw_thing(const game::world::components::thing& thing, thing_id);

public:
    level_view(content_manager& shadercontentmanager,
               client_renderer_object_factory &renderer_object_factory);

    inline void set_presenter(game::world::level_presenter* presenter) {
        currentPresenter = presenter;
    }

    inline game::world::level_presenter& get_presenter() const {
        return *currentPresenter;
    }

    inline void set_level_model(game::world::level_model* levelModel) {
        currentModel = levelModel;
    }

    inline void update_shader_model_matrix() {
        currentLevelShader->set_model_matrix(model_matrix_stack.top());
    }

    inline void concatenate_matrix(const matrix<4>& mat) {
        model_matrix_stack.top() = model_matrix_stack.top() * mat;
    }

    inline void push_matrix() {
        model_matrix_stack.push(model_matrix_stack.top());
    }

    inline void pop_matrix() {
        model_matrix_stack.pop();
    }

    void draw_saber(asset_ref<material> saber_tip, asset_ref<material> saber_blade,
            float saber_length, float saber_base_radius, float saber_tip_radius);

    void draw_sprite(const vector<3>& pos, asset_ref<material> mat, int frame, float width, float height,
            flags::geometry_mode geo, flags::light_mode light, float extra_light, const vector<3>& offset, float sector_light);

    virtual void draw(const gorc::time& time, const box<2, int>& view_size, graphics::render_target& target) override;
};

}
}
}
