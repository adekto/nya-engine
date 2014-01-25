//https://code.google.com/p/nya-engine/

#include "matrix.h"
#include "vector.h"
#include "quaternion.h"
#include "constants.h"
#include <math.h>

namespace nya_math
{

mat4 &mat4::identity()
{
    for(int i=0;i<4;++i)
        for(int j=0;j<4;++j)
            m[i][j]=0.0f;

    m[0][0]=m[1][1]=m[2][2]=m[3][3]=1.0f;

    return *this;
}

mat4 &mat4::translate(float x,float y,float z)
{
    for(int i=0;i<4;++i)
        m[3][i]+=m[0][i]*x+m[1][i]*y+m[2][i]*z;

    return *this;
}

mat4 &mat4::translate(const nya_math::vec3 &v)
{
    for(int i=0;i<4;++i)
        m[3][i]+=m[0][i]*v.x+m[1][i]*v.y+m[2][i]*v.z;
        
        return *this;
}

mat4 &mat4::rotate(float angle,float x,float y,float z)
{
    const float mag=sqrtf(x*x+y*y+z*z);
    const float ang_rad=-angle*constants::pi/180.0f;
    const float sin_a=sinf(ang_rad);
    const float cos_a=cosf(ang_rad);

    if(mag < 0.001f)
        return *this;

    const float one_minus_cos=1.0f-cos_a;

    float xx, yy, zz, xy, yz, zx, xs, ys, zs;

    x/=mag; y/=mag; z/=mag;
    xx=x*x; yy=y*y; zz=z*z;
    xy=x*y; yz=y*z; zx=z*x;
    xs=x*sin_a; ys=y*sin_a; zs=z*sin_a;

    mat4 rot;

    rot[0][0]=(one_minus_cos*xx)+cos_a;
    rot[0][1]=(one_minus_cos*xy)-zs;
    rot[0][2]=(one_minus_cos*zx)+ys;
    rot[0][3]=0;

    rot[1][0]=(one_minus_cos*xy)+zs;
    rot[1][1]=(one_minus_cos*yy)+cos_a;
    rot[1][2]=(one_minus_cos*yz)-xs;
    rot[1][3]=0;

    rot[2][0]=(one_minus_cos*zx)-ys;
    rot[2][1]=(one_minus_cos*yz)+xs;
    rot[2][2]=(one_minus_cos*zz)+cos_a;
    rot[2][3]=0;

    rot[3][0]=0;
    rot[3][1]=0;
    rot[3][2]=0;
    rot[3][3]=1.0f;

    return *this=rot*(*this);
}

mat4 &mat4::rotate( const nya_math::quat &q )
{
    return *this=nya_math::mat4(q)*(*this);
}

mat4 &mat4::perspective(float fov,float aspect,float near,float far)
{
	const float h=tanf(fov*constants::pi/360.0f)*near;
    const float w=h*aspect;

    return frustrum(-w,w,-h,h,near,far);
}

mat4 &mat4::frustrum(float left,float right,float bottom,float top,float near,float far)
{
    if(near<=0 || far<=0)
        return identity();

    const float dx=right-left;
    const float dy=top-bottom;
    const float dz=far-near;

    if(dx<=0 || dy<=0 || dz<=0)
        return identity();

    mat4 frust;

    frust[0][0]=2.0f*near/dx;
    frust[0][1]=frust[0][2]=frust[0][3]=0;

    frust[1][1]=2.0f*near/dy;
    frust[1][0]=frust[1][2]=frust[1][3]=0;

    frust[2][0]=(right+left)/dx;
    frust[2][1]=(top+bottom)/dy;
    frust[2][2]= -(near+far)/dz;
    frust[2][3]= -1.0f;

    frust[3][2]= -2.0f*near*far/dz;
    frust[3][0]=frust[3][1]=frust[3][3]=0;

    return *this=frust*(*this);
}

inline float get_cofactor(float m0,float m1,float m2,float m3,float m4,float m5,float m6,float m7,float m8)
{
    return m0*(m4*m8 - m5*m7) - m1*(m3*m8 - m5*m6) + m2*(m3*m7 - m4*m6);
}

mat4 &mat4::invert()
{
    const float c00=get_cofactor(m[1][1],m[1][2],m[1][3],m[2][1],m[2][2],m[2][3],m[3][1],m[3][2],m[3][3]);
    const float c10=get_cofactor(m[1][0],m[1][2],m[1][3],m[2][0],m[2][2],m[2][3],m[3][0],m[3][2],m[3][3]);
    const float c20=get_cofactor(m[1][0],m[1][1],m[1][3],m[2][0],m[2][1],m[2][3],m[3][0],m[3][1],m[3][3]);
    const float c30=get_cofactor(m[1][0],m[1][1],m[1][2],m[2][0],m[2][1],m[2][2],m[3][0],m[3][1],m[3][2]);

    const float d=m[0][0]*c00 - m[0][1]*c10 + m[0][2]*c20 - m[0][3]*c30;
    if(fabs(d)<0.00001f)
        return identity();

    const float c01=get_cofactor(m[0][1],m[0][2],m[0][3],m[2][1],m[2][2],m[2][3],m[3][1],m[3][2],m[3][3]);
    const float c11=get_cofactor(m[0][0],m[0][2],m[0][3],m[2][0],m[2][2],m[2][3],m[3][0],m[3][2],m[3][3]);
    const float c21=get_cofactor(m[0][0],m[0][1],m[0][3],m[2][0],m[2][1],m[2][3],m[3][0],m[3][1],m[3][3]);
    const float c31=get_cofactor(m[0][0],m[0][1],m[0][2],m[2][0],m[2][1],m[2][2],m[3][0],m[3][1],m[3][2]);

    const float c02=get_cofactor(m[0][1],m[0][2],m[0][3],m[1][1],m[1][2],m[1][3],m[3][1],m[3][2],m[3][3]);
    const float c12=get_cofactor(m[0][0],m[0][2],m[0][3],m[1][0],m[1][2],m[1][3],m[3][0],m[3][2],m[3][3]);
    const float c22=get_cofactor(m[0][0],m[0][1],m[0][3],m[1][0],m[1][1],m[1][3],m[3][0],m[3][1],m[3][3]);
    const float c32=get_cofactor(m[0][0],m[0][1],m[0][2],m[1][0],m[1][1],m[1][2],m[3][0],m[3][1],m[3][2]);

    const float c03=get_cofactor(m[0][1],m[0][2],m[0][3],m[1][1],m[1][2],m[1][3],m[2][1],m[2][2],m[2][3]);
    const float c13=get_cofactor(m[0][0],m[0][2],m[0][3],m[1][0],m[1][2],m[1][3],m[2][0],m[2][2],m[2][3]);
    const float c23=get_cofactor(m[0][0],m[0][1],m[0][3],m[1][0],m[1][1],m[1][3],m[2][0],m[2][1],m[2][3]);
    const float c33=get_cofactor(m[0][0],m[0][1],m[0][2],m[1][0],m[1][1],m[1][2],m[2][0],m[2][1],m[2][2]);

    const float id=1.0f/d;
    m[0][0]=id*c00; m[0][1]= -id*c01; m[0][2]=id*c02; m[0][3]= -id*c03;
    m[1][0]= -id*c10; m[1][1]=id*c11; m[1][2]= -id*c12; m[1][3]=id*c13;
    m[2][0]=id*c20; m[2][1]= -id*c21; m[2][2]=id*c22; m[2][3]= -id*c23;
    m[3][0]= -id*c30; m[3][1]=id*c31; m[3][2]= -id*c32; m[3][3]=id*c33;

    return *this;
}

mat4 mat4::operator*(const mat4 &mat) const
{
    mat4 out;

    for(int i=0;i<4;++i)
    {
        for(int j=0;j<4;++j)
        {
            out[i][j]=(m[i][0]*mat[0][j])+
                        (m[i][1]*mat[1][j])+
                        (m[i][2]*mat[2][j])+
                        (m[i][3]*mat[3][j]);
        }
    }

    return out;
}

mat4 &mat4::scale(float sx,float sy,float sz)
{
    for(int i=0;i<4;++i)
    {
        m[0][i]*=sx;
        m[1][i]*=sy;
        m[2][i]*=sz;
    }

    return *this;
}

vec3 operator * (const vec3 &v,const mat4 &m)
{
	return vec3(m[0][0]*v.x+m[0][1]*v.y+m[0][2]*v.z+m[0][3],
				m[1][0]*v.x+m[1][1]*v.y+m[1][2]*v.z+m[1][3],
				m[2][0]*v.x+m[2][1]*v.y+m[2][2]*v.z+m[2][3]);
}

vec3 operator * (const mat4 &m,const vec3 &v)
{
	return vec3(m[0][0]*v.x+m[1][0]*v.y+m[2][0]*v.z+m[3][0],
				m[0][1]*v.x+m[1][1]*v.y+m[2][1]*v.z+m[3][1],
				m[0][2]*v.x+m[1][2]*v.y+m[2][2]*v.z+m[3][2]);
}

vec4 operator * (const vec4 &v,const mat4 &m)
{
    return vec4(m[0][0]*v.x+m[0][1]*v.y+m[0][2]*v.z+m[0][3]*v.w,
                m[1][0]*v.x+m[1][1]*v.y+m[1][2]*v.z+m[1][3]*v.w,
                m[2][0]*v.x+m[2][1]*v.y+m[2][2]*v.z+m[2][3]*v.w,
                m[3][0]*v.x+m[3][1]*v.y+m[3][2]*v.z+m[3][3]*v.w);
}

vec4 operator * (const mat4 &m,const vec4 &v)
{
    return vec4(m[0][0]*v.x+m[1][0]*v.y+m[2][0]*v.z+m[3][0]*v.w,
				m[0][1]*v.x+m[1][1]*v.y+m[2][1]*v.z+m[3][1]*v.w,
				m[0][2]*v.x+m[1][2]*v.y+m[2][2]*v.z+m[3][2]*v.w,
                m[0][3]*v.x+m[1][3]*v.y+m[2][3]*v.z+m[3][3]*v.w);
}

mat4::mat4(const quat &q)
{
    const float xx=q.v.x*q.v.x;
    const float yy=q.v.y*q.v.y;
    const float zz=q.v.z*q.v.z;
    const float xy=q.v.x*q.v.y;
    const float yz=q.v.y*q.v.z;
    const float xz=q.v.x*q.v.z;
    const float xw=q.v.x*q.w;
    const float yw=q.v.y*q.w;
    const float zw=q.v.z*q.w;

    m[0][0]=1.0f-2.0f*(yy+zz); m[1][0]=2.0f*(xy-zw); m[2][0]=2.0f*(xz+yw);
    m[0][1]=2.0f*(xy+zw); m[1][1]=1.0f-2.0f*(xx+zz); m[2][1]=2.0f*(yz-xw);
    m[0][2]=2.0f*(xz-yw); m[1][2]=2.0f*(yz+xw); m[2][2]=1.0f-2.0f*(xx+yy);

    for(int i=0;i<3;++i)
        m[3][i]=m[i][3]=0.0f;

    m[3][3]=1.0f;
}

}

