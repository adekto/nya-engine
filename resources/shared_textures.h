//https://code.google.com/p/nya-engine/

#ifndef shared_textures_h
#define shared_textures_h

#include "resources/shared_resources.h"
#include "render/texture.h"

namespace nya_resources
{
    typedef shared_resources<nya_render::texture,8> shared_textures;
    typedef shared_textures::shared_resource_ref texture_ref;

    class shared_textures_manager: public shared_textures
    {
        bool fill_resource(const char *name,nya_render::texture &res);
        bool release_resource(nya_render::texture &res);
    };

    shared_textures_manager &get_shared_textures();
}
#endif

