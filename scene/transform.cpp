//https://code.google.com/p/nya-engine/

#include "transform.h"
#include "render/render.h"
#include "camera.h"

namespace
{
    const nya_scene_internal::transform *active_transform=0;
}

namespace nya_scene_internal
{

void transform::set(const transform &tr)
{
    active_transform=&tr;
    tr.apply();
}

const transform &transform::get()
{
    if(!active_transform)
    {
        static transform invalid;
        return invalid;
    }

    return *active_transform;
}

nya_math::vec3 transform::inverse_transform(const nya_math::vec3 &vec) const
{
    const float eps=0.0001f;
    if(m_scale*m_scale<eps)
        return nya_math::vec3();

    nya_math::vec3 out=m_rot.rotate_inv(vec-m_pos);
    out.x/=m_scale.x;
    out.y/=m_scale.y;
    out.z/=m_scale.z;

    return out;
}

nya_math::vec3 transform::inverse_rot(const nya_math::vec3 &vec) const
{
    return m_rot.rotate_inv(vec);
}

nya_math::aabb transform::transform_aabb(const nya_math::aabb &box) const
{
    return nya_math::aabb(box,m_pos,m_rot,m_scale);
}

void transform::apply() const
{
    nya_scene::camera_proxy cam=nya_scene::get_camera();
    nya_math::mat4 mat;
    if(cam.is_valid())
        mat=cam->get_view_matrix();

    mat.translate(m_pos).rotate(m_rot).scale(m_scale.x,m_scale.y,m_scale.z);

    nya_render::set_modelview_matrix(mat);
}

}
