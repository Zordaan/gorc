#pragma once

#include "utility/flag_set.hpp"
#include "math/vector.hpp"
#include "game/world/components/thing.hpp"
#include "libold/base/utility/time.hpp"
#include "shape.hpp"
#include "contact.hpp"
#include <unordered_map>
#include <map>
#include <set>
#include <unordered_set>
#include <vector>
#include <tuple>
#include "game/world/level_model.hpp"
#include "game/world/level_presenter.hpp"
#include "game/world/keys/key_presenter.hpp"
#include "query.hpp"
#include "shape.hpp"
#include "jk/cog/script/verb_table.hpp"

namespace gorc {
namespace game {

class level_state;

namespace world {

class level_presenter;

namespace physics {

class physics_presenter {
private:
    level_presenter& presenter;
    level_model* model;
    event_bus* eventbus;

    std::unordered_multimap<thing_id, sector_id> physics_broadphase_thing_influence;
    std::unordered_multimap<sector_id, thing_id> physics_broadphase_sector_things;
    std::unordered_map<sector_id, int> physics_broadphase_sector_group;
    std::multimap<int, thing_id> physics_broadphase_thing_groups;
    std::unordered_set<int> physics_broadphase_groups_to_merge;
    std::set<thing_id> physics_overlapping_things;
    std::vector<physics::contact> physics_thing_resting_manifolds;
    std::set<sector_id> physics_thing_closed_set;
    std::vector<sector_id> physics_thing_open_set;
    std::set<std::tuple<thing_id, thing_id>> physics_touched_thing_pairs;
    std::set<std::tuple<thing_id, surface_id>> physics_touched_surface_pairs;
    std::set<sector_id> segment_query_closed_sectors;
    std::vector<sector_id> segment_query_open_sectors;

    void physics_calculate_broadphase(double dt);
    void physics_find_sector_resting_manifolds(const physics::sphere& sphere, sector_id, const vector<3>& vel_dir, thing_id current_thing_id);
    void physics_find_thing_resting_manifolds(const physics::sphere& sphere, const vector<3>& vel_dir, thing_id current_thing_id);
    void compute_current_velocity(components::thing &thing, double dt);
    void compute_thing_attachment_velocity(components::thing &thing, double dt);
    void physics_thing_step(thing_id, components::thing& thing, double dt);

    void update_thing_path_moving(thing_id, components::thing& thing, double dt);
    vector<3> get_thing_path_moving_point_velocity(thing_id, const vector<3>& rel_point);

    class physics_node_visitor {
    private:
        physics_presenter& presenter;
        std::vector<physics::contact>& resting_manifolds;
        std::set<std::tuple<thing_id, thing_id>>& physics_touched_thing_pairs;
        std::stack<matrix<4>> matrices;
        matrix<4> current_matrix = make_identity_matrix<4>();

    public:
        physics_node_visitor(physics_presenter& presenter, std::vector<physics::contact>& resting_manifolds,
                std::set<std::tuple<thing_id, thing_id>>& physics_touched_thing_pairs);

        inline void push_matrix() {
            matrices.push(current_matrix);
        }

        inline void pop_matrix() {
            current_matrix = matrices.top();
            matrices.pop();
        }

        inline void concatenate_matrix(const matrix<4>& mat) {
            current_matrix = current_matrix * mat;
        }

        void visit_mesh(asset_ref<content::assets::model> model, int mesh_id, int node_id);

        bool needs_response;
        thing_id moving_thing_id;
        thing_id visited_thing_id;
        physics::sphere sphere;
    } physics_anim_node_visitor;

    class segment_query_node_visitor {
    private:
        physics_presenter& presenter;
        std::stack<matrix<4>> matrices;
        matrix<4> current_matrix = make_identity_matrix<4>();

    public:
        segment_query_node_visitor(physics_presenter& presenter);

        inline void push_matrix() {
            matrices.push(current_matrix);
        }

        inline void pop_matrix() {
            current_matrix = matrices.top();
            matrices.pop();
        }

        inline void concatenate_matrix(const matrix<4>& mat) {
            current_matrix = current_matrix * mat;
        }

        void visit_mesh(asset_ref<content::assets::model> model, int mesh_id, int node_id);
        segment cam_segment;
        vector<3> closest_contact;
        vector<3> closest_contact_normal;
        bool has_closest_contact;
        float closest_contact_distance;
    } segment_query_anim_node_visitor;

public:
    physics_presenter(level_presenter& presenter);

    void start(level_model& model, event_bus& eventbus);
    void update(const gorc::time& time);

    template <typename ThingP, typename SurfaceP> maybe<contact> segment_query(const segment& cam_segment, sector_id initial_sector, thing_id ray_cast_thing,
            ThingP thing_p, SurfaceP surface_p, const maybe<contact>& prev_contact = maybe<contact>()) {
        // Search for closest thing-ray intersection.
        float closest_contact_distance = std::numeric_limits<float>::max();
        bool has_contact = false;
        thing_id closest_contact_thing_id = invalid_id;
        surface_id closest_contact_surface_id = invalid_id;
        vector<3> closest_contact_position;
        vector<3> closest_contact_normal;

        maybe_if(prev_contact, [&](contact const &prev_ct) {
            closest_contact_distance = length(prev_ct.position - std::get<0>(cam_segment));
            has_contact = true;
            maybe_if(prev_ct.contact_thing_id, [&](thing_id c) {
                closest_contact_thing_id = c;
            });

            maybe_if(prev_ct.contact_surface_id, [&](surface_id c) {
                closest_contact_surface_id = c;
            });

            closest_contact_position = prev_ct.position;
            closest_contact_normal = prev_ct.normal;
        });

        // Find contact among sectors.
        // Get list of sectors within thing influence.
        segment_query_closed_sectors.clear();
        segment_query_open_sectors.clear();
        segment_query_open_sectors.emplace_back(initial_sector);
        while(!segment_query_open_sectors.empty()) {
            sector_id current_sector = segment_query_open_sectors.back();
            segment_query_open_sectors.pop_back();

            if(segment_query_closed_sectors.find(current_sector) != segment_query_closed_sectors.end()) {
                continue;
            }

            segment_query_closed_sectors.emplace(current_sector);

            const auto& sector = at_id(model->sectors, current_sector);

            for(int i = sector.first_surface; i < sector.first_surface + sector.surface_count; ++i) {
                const auto& surface = model->surfaces[i];

                auto maybe_nearest_point = physics::segment_surface_intersection_point(cam_segment, *model->level, surface);
                maybe_if(maybe_nearest_point, [&](vector<3> const &nearest_point) {
                    if(surface_p(surface_id(i))) {
                        auto dist = length(nearest_point - std::get<0>(cam_segment));
                        if(dist < closest_contact_distance) {
                            closest_contact_distance = dist;
                            has_contact = true;
                            closest_contact_surface_id = surface_id(i);
                            closest_contact_position = nearest_point;
                            closest_contact_normal = surface.normal;
                        }
                    }

                    if(surface.adjoin >= 0) {
                        segment_query_open_sectors.push_back(surface.adjoined_sector);
                    }
                });
            }
        }

        // Find contact among things.
        // Get list of things within thing influence.
        physics_overlapping_things.clear();
        for(auto sector_id : segment_query_closed_sectors) {
            auto influenced_thing_range = physics_broadphase_sector_things.equal_range(sector_id);
            for(auto jt = std::get<0>(influenced_thing_range); jt != std::get<1>(influenced_thing_range); ++jt) {
                physics_overlapping_things.emplace(jt->second);
            }
        }

        for(auto col_thing_id : physics_overlapping_things) {
            auto& col_thing = model->get_thing(col_thing_id);

            if(col_thing_id == ray_cast_thing || !thing_p(col_thing_id)) {
                continue;
            }

            if(col_thing.collide == flags::collide_type::sphere) {
                auto maybe_int = segment_sphere_intersection(cam_segment, sphere(col_thing.position, col_thing.size));
                maybe_if(maybe_int, [&](vector<3> const &int_point) {
                    // Sphere intersected.
                    float col_dist = length(int_point - std::get<0>(cam_segment));
                    if(col_dist < closest_contact_distance) {
                        closest_contact_distance = col_dist;
                        has_contact = true;
                        closest_contact_surface_id = invalid_id;
                        closest_contact_thing_id = col_thing_id;
                        closest_contact_normal = normalize(int_point - col_thing.position);
                        closest_contact_position = int_point;
                    }
                });
            }
            else if(col_thing.collide == flags::collide_type::face) {
                if(!col_thing.model_3d.has_value()) {
                    continue;
                }

                segment_query_anim_node_visitor.cam_segment = cam_segment;
                segment_query_anim_node_visitor.closest_contact_distance = closest_contact_distance;
                segment_query_anim_node_visitor.has_closest_contact = false;
                presenter.key_presenter->visit_mesh_hierarchy(segment_query_anim_node_visitor, col_thing.model_3d.get_value(), col_thing.position,
                        col_thing.orient, col_thing_id, /* is pov mix */ false);

                if(segment_query_anim_node_visitor.has_closest_contact) {
                    closest_contact_distance = segment_query_anim_node_visitor.closest_contact_distance;
                    has_contact = true;
                    closest_contact_surface_id = invalid_id;
                    closest_contact_thing_id = col_thing_id;
                    closest_contact_normal = segment_query_anim_node_visitor.closest_contact_normal;
                    closest_contact_position = segment_query_anim_node_visitor.closest_contact;
                }
            }
        }

        if(has_contact) {
            contact new_contact(closest_contact_position, closest_contact_normal, make_zero_vector<3, float>());
            if(closest_contact_surface_id.is_valid()) {
                new_contact.contact_surface_id = closest_contact_surface_id;
            }

            if(closest_contact_thing_id.is_valid()) {
                new_contact.contact_thing_id = closest_contact_thing_id;
            }

            return new_contact;
        }

        return nothing;
    }

    template <typename ThingP, typename SurfaceP> maybe<contact> thing_segment_query(thing_id current_thing_id, const vector<3>& direction,
            ThingP thing_p, SurfaceP surface_p, const maybe<contact>& previous_contact = maybe<contact>()) {
        const auto& current_thing = model->get_thing(current_thing_id);
        return segment_query<ThingP, SurfaceP>(segment(current_thing.position, current_thing.position + direction), current_thing.sector, current_thing_id,
                thing_p, surface_p, previous_contact);
    }

    bool surface_needs_collision_response(thing_id moving_thing_id, surface_id);
    bool thing_needs_collision_response(thing_id moving_thing_id, thing_id collision_thing_id);

    static void register_verbs(cog::verb_table&, level_state&);
};

}
}
}
}
