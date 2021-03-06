//https://code.google.com/p/nya-engine/

#include "load_pmd.h"
#include "scene/mesh.h"
#include "memory/memory_reader.h"
#include "string_encoding.h"

namespace
{

struct add_data: public pmd_loader::additional_data, nya_scene::shared_mesh::additional_data
{
    const char *type() { return "pmd"; }
};

inline void read_vector(nya_math::vec3 &out,nya_memory::memory_reader &reader)
{
    out.x=reader.read<float>(),out.y=reader.read<float>(),out.z=-reader.read<float>();
}

inline void read_angle(nya_math::vec3 &out,nya_memory::memory_reader &reader)
{
    out.x=-reader.read<float>(),out.y=-reader.read<float>(),out.z=reader.read<float>();
}

}

bool pmd_loader::load(nya_scene::shared_mesh &res,nya_scene::resource_data &data,const char* name)
{
    nya_memory::memory_reader reader(data.get_data(),data.get_size());
    if(!reader.test("Pmd",3))
        return false;

    if(reader.read<float>()!=1.0f)
        return false;

    reader.skip(20+256); //name and comment

    typedef unsigned int uint;
    typedef unsigned short ushort;
    typedef unsigned char uchar;

    const uint vert_count=reader.read<uint>();
    const size_t pmd_vert_size=sizeof(float)*8+sizeof(ushort)*2+sizeof(uchar)*2;
    if(!reader.check_remained(pmd_vert_size*vert_count))
        return false;

    std::vector<vert> vertices(vert_count);
    for(size_t i=0;i<vertices.size();++i)
    {
        read_vector(vertices[i].pos,reader);
        read_vector(vertices[i].normal,reader);

        vertices[i].tc.x=reader.read<float>();
        vertices[i].tc.y=1.0f-reader.read<float>();

        vertices[i].bone_idx[0]=reader.read<ushort>();
        vertices[i].bone_idx[1]=reader.read<ushort>();
        vertices[i].bone_weight=reader.read<uchar>()/100.0f;

        reader.skip(1);
    }
    
    const uint ind_count=reader.read<uint>();
    const size_t inds_size=sizeof(ushort)*ind_count;
    if(!reader.check_remained(inds_size))
        return false;

    res.vbo.set_index_data(data.get_data(reader.get_offset()),nya_render::vbo::index2b,ind_count);
    reader.skip(inds_size);

    const uint mat_count=reader.read<uint>();
    if(!reader.check_remained(mat_count*(sizeof(pmd_material_params)+2+sizeof(uint)+20)))
        return false;

    res.groups.resize(mat_count);
    res.materials.resize(mat_count);

    std::string path(name);
    size_t p=path.rfind("/");
    if(p==std::string::npos)
        p=path.rfind("\\");
    if(p==std::string::npos)
        path.clear();
    else
        path.resize(p+1);

    const nya_math::vec3 mmd_light_dir=-nya_math::vec3(-0.5,-1.0,-0.5).normalize();

    std::vector<char> toon_idxs(mat_count);

    for(uint i=0,ind_offset=0;i<mat_count;++i)
    {
        nya_scene::shared_mesh::group &g=res.groups[i];

        const pmd_material_params params=reader.read<pmd_material_params>();

        toon_idxs[i]=reader.read<char>();
        const char edge_flag=reader.read<char>();

        g.name="mesh";
        g.offset=ind_offset;
        g.count=reader.read<uint>();
        g.material_idx=i;
        ind_offset+=g.count;

        const std::string tex_name((const char*)data.get_data(reader.get_offset()));
        reader.skip(20);

        std::string base_tex=tex_name;
        size_t pos=base_tex.find('*');
        if(pos!=std::string::npos)
            base_tex.resize(pos);

        nya_scene::material &m = res.materials[i];

        enum
        {
            no_env=0,
            env_mult=1,
            env_add=2
        } sph_mode=no_env;

        if(base_tex.size()>2)
        {
            const char check_p=base_tex[base_tex.length()-2];
            if(check_p=='p' || check_p=='P')
            {
                const char last=base_tex[base_tex.length()-1];
                if(last=='a' || last=='A')
                    sph_mode=env_add;
                else if(last=='h' || last=='H')
                    sph_mode=env_mult;
            }
        }

        {
            nya_scene::texture tex;
            if(sph_mode!=no_env || base_tex.empty() || !base_tex[0] || !tex.load((path+base_tex).c_str()))
            {
                typedef unsigned char uchar;
                unsigned char data[4]={uchar(params.diffuse[2]*255),uchar(params.diffuse[1]*255),
                                       uchar(params.diffuse[0]*255),uchar(params.diffuse[3]*255)};
                tex.build(&data,1,1,nya_render::texture::color_bgra);
            }

            m.set_texture("diffuse",tex);
        }

        {
            bool add=false,mult=false;
            nya_scene::texture tex;

            if(sph_mode!=no_env && !base_tex.empty() && base_tex[0] && tex.load((path+base_tex).c_str()))
            {
                if(sph_mode==env_add)
                    m.set_texture("env add",tex),add=true;
                else if(sph_mode==env_mult)
                    m.set_texture("env mult",tex),mult=true;
            }

            if(!add)
            {
                nya_scene::texture tex;
                tex.build("\x00\x00\x00\x00",1,1,nya_render::texture::color_bgra);
                m.set_texture("env add",tex);
            }

            if(!mult)
            {
                nya_scene::texture tex;
                tex.build("\xff\xff\xff\xff",1,1,nya_render::texture::color_bgra);
                m.set_texture("env mult",tex);
            }
        }

        nya_scene::material::pass &p=m.get_pass(m.add_pass(nya_scene::material::default_pass));

        nya_scene::shader sh;
        sh.load("pmd.nsh");
        p.set_shader(sh);
        p.get_state().set_blend(true,nya_render::blend::src_alpha,nya_render::blend::inv_src_alpha);
        p.get_state().set_cull_face(true,nya_render::cull_face::cw);

        m.set_param(m.get_param_idx("light dir"),mmd_light_dir);
        m.set_param(m.get_param_idx("amb k"),params.ambient[0],params.ambient[1],params.ambient[2],1.0f);
        m.set_param(m.get_param_idx("diff k"),params.diffuse[0],params.diffuse[1],params.diffuse[2],params.diffuse[3]);
        m.set_param(m.get_param_idx("spec k"),params.specular[0],params.specular[1],params.specular[2],params.shininess);

        if(!edge_flag)
            continue;

        res.groups.resize(res.groups.size()+1);
        nya_scene::shared_mesh::group &ge=res.groups.back();
        ge=res.groups[i];
        ge.name="edge";
        ge.material_idx=int(res.materials.size());
        res.materials.resize(res.materials.size()+1);
        nya_scene::material &me = res.materials.back();

        nya_scene::material::pass &pe=me.get_pass(me.add_pass(nya_scene::material::default_pass));
        nya_scene::shader she;
        she.load("pmd_edge.nsh");
        pe.set_shader(she);
        pe.get_state().set_cull_face(true,nya_render::cull_face::ccw);
    }

    const ushort bones_count=reader.read<ushort>();
    if(!bones_count || !reader.check_remained(bones_count*(20+sizeof(short)+5+sizeof(nya_math::vec3))))
        return false;

    if(bones_count>gpu_skining_bones_limit)
    {
        for(int i=0;i<int(res.groups.size());++i)
        {
            nya_scene::material &me=res.materials[res.groups[i].material_idx];
            nya_scene::material::pass &pe=me.get_pass(me.add_pass(nya_scene::material::default_pass));
            nya_scene::shader sh;
            sh.load(res.groups[i].name=="edge"?"pm_edge.nsh":"pm.nsh");
            pe.set_shader(sh);
        }
    }

    std::vector<pmd_bone> bones(bones_count);
    for(int i=0;i<bones_count;++i)
    {
        pmd_bone &b=bones[i];
        b.idx=i;
        b.name=utf8_from_shiftjis(data.get_data(reader.get_offset()),20);
        reader.skip(20);
        b.parent=reader.read<short>();
        reader.skip(5); //child,kind,ik target

        read_vector(b.pos,reader);
    }

    for(int i=0;i<int(bones.size());++i)
        if(bones[i].parent>=0)
            bones[i].parent_name=bones[bones[i].parent].name;

    //dumb sort
    for(int i=0;i<int(bones.size());++i) //ToDo: less dumb
    {
        bool had_sorted=false;
        for(int j=0;j<int(bones.size());++j)
        {
            const int p=bones[j].parent;
            if(p>j)
            {
                had_sorted=true;
                std::swap(bones[j], bones[p]);
                for(int k=0;k<bones.size();++k)
                    if(bones[k].parent==j || bones[k].parent==p)
                        bones[k].parent=pmd_bone::parent_idx_by_name(bones[k].parent_name,bones);
            }
        }

        if(!had_sorted)
            break;
    }

    std::vector<int> old_bones(bones_count);
    for(int i=0;i<bones_count;++i)
        old_bones[bones[i].idx]=i;

    for(int i=0;i<bones_count;++i)
    {
        const pmd_bone &b=bones[i];

        if(res.skeleton.add_bone(b.name.c_str(),b.pos,nya_math::quat(),b.parent,true)!=i)
        {
            nya_log::log()<<"pmd load error: invalid bone\n";
            return false;
        }
    }

    const ushort iks_count=reader.read<ushort>();
    //if(!reader.check_remained(iks_count*()))
    //    return false;

    for(ushort i=0;i<iks_count;++i)
    {
        const short target=reader.read<short>();
        const short eff=reader.read<short>();
        const uchar link_count=reader.read<uchar>();
        const ushort count=reader.read<ushort>();
        const float k=reader.read<float>();

        const int ik=res.skeleton.add_ik(old_bones[target],old_bones[eff],count,k*nya_math::constants::pi);
        for(int j=0;j<link_count;++j)
        {
            const ushort link=reader.read<ushort>();
            const char *name=res.skeleton.get_bone_name(link);
            if(!name)
            {
                nya_log::log()<<"pmd load warning: invalid ik link\n";
                continue;
            }

            if(strcmp(name,"\xe5\xb7\xa6\xe3\x81\xb2\xe3\x81\x96")==0 || strcmp(name,"\xe5\x8f\xb3\xe3\x81\xb2\xe3\x81\x96")==0)
                res.skeleton.add_ik_link(ik,old_bones[link],0.001f,nya_math::constants::pi);
            else
                res.skeleton.add_ik_link(ik,old_bones[link]);
        }
    }

    add_data *ad=new add_data;
    res.add_data=ad;

    const ushort morphs_count=reader.read<ushort>();
    if(morphs_count>1)
        ad->morphs.resize(morphs_count-1);

    pmd_morph_data::morph base_morph;
    for(ushort i=0;i<ushort(ad->morphs.size());)
    {
        const std::string name=utf8_from_shiftjis(data.get_data(reader.get_offset()),20);
        reader.skip(20);
        const uint size=reader.read<uint>();
        const uchar type=reader.read<uchar>();

        pmd_morph_data::morph &m=type?ad->morphs[i]:base_morph;
        if(type)
            ++i;

        m.name=name;
        m.verts.resize(size);
        for(uint j=0;j<size;++j)
        {
            pmd_morph_data::morph_vertex &v=m.verts[j];

            const uint idx=reader.read<uint>();
            v.idx=type?base_morph.verts[idx].idx:idx;
            read_vector(v.pos,reader);
        }
    }

    res.vbo.set_vertex_data(&vertices[0],sizeof(vertices[0]),vert_count);
    res.vbo.set_normals(3*sizeof(float));
    res.vbo.set_tc(0,6*sizeof(float),2);
    res.vbo.set_tc(1,8*sizeof(float),3); //skin info

    vertices.clear();

    reader.skip(reader.read<uchar>()*sizeof(ushort));

    const uchar bone_groups_count=reader.read<uchar>();
    reader.skip(bone_groups_count*50);

    reader.skip(reader.read<uint>()*(sizeof(ushort)+sizeof(uchar)));

    if(!reader.get_remained())
        return true;

    if(reader.read<uchar>())
    {
        reader.skip(20);
        reader.skip(256);
        reader.skip(bones_count*20);

        if(morphs_count>1)
            reader.skip((morphs_count-1)*20);

        reader.skip(bone_groups_count*50);
    }

    char toon_names[10][100];
    for(int i=0;i<10;++i)
    {
        memcpy(toon_names[i],reader.get_data(),100);
        reader.skip(100);
    }

    for(int i=0;i<mat_count;++i)
    {
        nya_scene::material &m=res.materials[i];

        bool loaded=false;
        nya_scene::texture tex;

        const char idx=toon_idxs[i];
        if(idx>=0 && idx<10)
        {
            const char *name=toon_names[idx];
            if(nya_resources::get_resources_provider().has((path+name).c_str()))
                loaded=tex.load((path+name).c_str());
            else
                loaded=tex.load(name);
        }

        if(!loaded)
            tex.build("\xff\xff\xff\xff",1,1,nya_render::texture::color_bgra);

        m.set_texture("toon",tex);
    }

    const uint rigid_bodies_count=reader.read<uint>();
    ad->rigid_bodies.resize(rigid_bodies_count);
    for(uint i=0;i<rigid_bodies_count;++i)
    {
        pmd_phys_data::rigid_body &rb=ad->rigid_bodies[i];
        rb.name=utf8_from_shiftjis(data.get_data(reader.get_offset()),20);
        reader.skip(20);
        rb.bone=reader.read<short>();
        rb.collision_group=reader.read<uchar>();
        rb.collision_mask=reader.read<ushort>();
        rb.type=(pmd_phys_data::shape_type)reader.read<uchar>();
        rb.size=reader.read<nya_math::vec3>();
        read_vector(rb.pos,reader);
        read_angle(rb.rot,reader);
        rb.mass=reader.read<float>();
        rb.vel_attenuation=reader.read<float>();
        rb.rot_attenuation=reader.read<float>();
        rb.restriction=reader.read<float>();
        rb.friction=reader.read<float>();
        rb.mode=(pmd_phys_data::object_type)reader.read<uchar>();
    }

    const uint joints_count=reader.read<uint>();
    ad->joints.resize(joints_count);
    for(uint i=0;i<joints_count;++i)
    {
        pmd_phys_data::joint &j=ad->joints[i];
        j.name=utf8_from_shiftjis(data.get_data(reader.get_offset()),20);
        reader.skip(20);
        j.rigid_src=reader.read<uint>();
        j.rigid_dst=reader.read<uint>();
        read_vector(j.pos,reader);
        read_angle(j.rot,reader);
        read_vector(j.pos_max,reader);
        read_vector(j.pos_min,reader);
        read_angle(j.rot_max,reader);
        read_angle(j.rot_min,reader);

        //std::swap(j.pos_max.z,j.pos_min.z); //caused by z axis inversion
        //std::swap(j.rot_max.x,j.rot_min.x);
        //std::swap(j.rot_max.y,j.rot_min.y);

        j.pos_spring=reader.read<nya_math::vec3>();
        j.rot_spring=reader.read<nya_math::vec3>();
    }

    return true;
}

const pmd_loader::additional_data *pmd_loader::get_additional_data(const nya_scene::mesh &m)
{
    if(!m.internal().get_shared_data().is_valid())
        return 0;

    nya_scene::shared_mesh::additional_data *d=m.internal().get_shared_data()->add_data;
    if(!d)
        return 0;

    const char *type=d->type();
    if(!type || strcmp(type,"pmd")!=0)
        return 0;

    return static_cast<pmd_loader::additional_data*>(static_cast<add_data*>(d));
}
