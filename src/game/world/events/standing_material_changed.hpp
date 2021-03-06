#pragma once

#include "content/id.hpp"
#include "game/flags/standing_material_type.hpp"

namespace gorc {
namespace game {
namespace world {
namespace events {

class standing_material_changed {
public:
    #include "standing_material_changed.uid"

    thing_id thing;
    flags::standing_material_type type;

    standing_material_changed(thing_id thing, flags::standing_material_type type);
};

}
}
}
}
