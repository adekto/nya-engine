//https://code.google.com/p/nya-engine/

#include "frustum.h"

namespace nya_math
{

bool frustum::test_intersect(const aabb &box) const
{
	for(int i=0;i<6;++i)
	{
        const plane &p=m_planes[i];
		if(box.origin*p.n+box.delta*p.abs_n+p.d<0.0f)
			return false;
	}

	return true;
}

bool frustum::test_intersect(const vec3 &v) const
{
    const float eps = 0.001f;
	for(int i=0;i<6;++i)
    {
        const plane &p=m_planes[i];
        if((p.n*(p.n*p.d+v) )< -eps)
            return false;
    }

    return true;
}

frustum::frustum(const mat4 &m)
{
    for(int i=0;i<3;++i)
    {
        const int idx=i+i;
        plane &p=m_planes[idx];
        p.n.x=m.m[0][3]-m.m[0][i];
        p.n.y=m.m[1][3]-m.m[1][i];
        p.n.z=m.m[2][3]-m.m[2][i];
        p.d=m.m[3][3]-m.m[3][i];

        plane &p2=m_planes[idx+1];
        p2.n.x=m.m[0][3]+m.m[0][i];
        p2.n.y=m.m[1][3]+m.m[1][i];
        p2.n.z=m.m[2][3]+m.m[2][i];
        p2.d=m.m[3][3]+m.m[3][i];
    }

	for(int i=0;i<6;++i)
	{
        plane &p=m_planes[i];
        float len=p.n.length();
        if(len>0.0001f)
        {
            p.n/=len;
            p.d/=len;
        }

        p.abs_n.x=fabsf(p.n.x);
        p.abs_n.y=fabsf(p.n.y);
        p.abs_n.z=fabsf(p.n.z);
    }
}

}
