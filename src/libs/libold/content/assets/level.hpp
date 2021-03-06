#pragma once

#include "content/asset.hpp"
#include "libold/content/loaders/level_loader.hpp"

#include "math/vector.hpp"
#include "level_header.hpp"
#include "level_adjoin.hpp"
#include "level_surface.hpp"
#include "level_sector.hpp"
#include "jk/cog/script/script.hpp"
#include "jk/content/material.hpp"
#include "template.hpp"

#include <unordered_map>

namespace gorc {
namespace content {
namespace assets {

class level : public asset {
public:
    static fourcc const type;

    level_header header;
    std::vector<std::tuple<maybe<asset_ref<material>>, float, float, std::string>> materials;

    maybe<asset_ref<colormap>> master_colormap;
    std::vector<asset_ref<colormap>> colormaps;

    std::vector<asset_ref<cog::script>> scripts;

    std::vector<std::unique_ptr<std::string>> cog_strings;
    std::vector<std::tuple<maybe<asset_ref<cog::script>>, std::vector<cog::value>>> cogs;

    std::vector<vector<3>> vertices;
    std::vector<vector<2>> texture_vertices;
    std::vector<level_adjoin> adjoins;
    std::vector<level_surface> surfaces;
    std::vector<level_sector> sectors;

    std::vector<thing_template> templates;
    std::unordered_map<std::string, thing_template_id> template_map;
    std::vector<thing_template> things;
};

}
}
}
