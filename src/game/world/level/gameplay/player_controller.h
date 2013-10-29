#pragma once

#include "character_controller.h"

namespace gorc {
namespace game {
namespace world {
namespace level {

namespace gameplay {

class player_controller : public character_controller {
public:
	using character_controller::character_controller;

	virtual void update(int thing_id, double dt) override;
};

}
}
}
}
}
