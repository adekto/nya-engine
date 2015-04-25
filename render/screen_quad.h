//https://code.google.com/p/nya-engine/

#pragma once

#include "vbo.h"
#include "platform_specific_gl.h"

namespace nya_render
{

class screen_quad
{
public:
    void init()
    {
        struct { float x,y,s,t; } verts[4];
        for(int i=0;i<4;++i)
        {
            verts[i].x=i>1?-1.0f:1.0f,verts[i].y=i%2?1.0f:-1.0f;
            verts[i].s=i>1? 0.0f:1.0f,verts[i].t=i%2?1.0f:0.0f;
            DIRECTX11_ONLY(verts[i].t=1.0f-verts[i].t);
        }

        m_mesh.set_vertex_data(verts,sizeof(verts[0]),4);
        m_mesh.set_vertices(0,2);
        m_mesh.set_tc(0,2*4,2);
    }

    void draw(unsigned int instances_count=1) const
    {
        m_mesh.bind();
        vbo::draw(0,m_mesh.get_verts_count(),vbo::triangle_strip,instances_count);
        m_mesh.unbind();
    }

    void release() { m_mesh.release(); }

private:
    vbo m_mesh;
};

}
