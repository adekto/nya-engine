//https://code.google.com/p/nya-engine/

#include "load_pmx.h"
#include "scene/mesh.h"
#include "memory/memory_reader.h"
#include "string_encoding.h"

#include "resources/resources.h"

namespace
{

struct add_data: public pmx_loader::additional_data, nya_scene::shared_mesh::additional_data
{
    const char *type() { return "pmx"; }
};

int read_idx(nya_memory::memory_reader &reader,int size)
{
    switch(size)
    {
        case 1: return reader.read<char>();
        case 2: return reader.read<short>();
        case 4: return reader.read<int>();
    }

    return 0;
}

template<typename t> void load_vertex_morph(nya_memory::memory_reader &reader,pmd_morph_data::morph &m)
{
    for(size_t j=0;j<m.verts.size();++j)
    {
        pmd_morph_data::morph_vertex &v=m.verts[j];
        v.idx=reader.read<t>();
        v.pos.x=reader.read<float>();
        v.pos.y=reader.read<float>();
        v.pos.z=-reader.read<float>();
    }
}

}

bool pmx_loader::load(nya_scene::shared_mesh &res,nya_scene::resource_data &data,const char* name)
{
    if(!data.get_size())
        return false;

    nya_memory::memory_reader reader(data.get_data(),data.get_size());
    if(!reader.test("PMX",3))
        return false;

    reader.skip(1);
    if(reader.read<float>()!=2.0f)
    {
        nya_log::log()<<"pmx load error: invalid version\n";
        return false;
    }

    const char header_size=reader.read<char>();
    if(header_size!=sizeof(pmx_header))
    {
        nya_log::log()<<"pmx load error: invalid header\n";
        return false;
    }
    
    const pmx_header header=reader.read<pmx_header>();
    
    for(int i=0;i<4;++i)
    {
        const int size=reader.read<int>();
        reader.skip(size);
    }
    
    const int vert_count=reader.read<int>();
    if(!vert_count)
    {
        nya_log::log()<<"pmx load error: no verts found\n";
        return false;
    }

    std::vector<vert> verts(vert_count);

    for(int i=0;i<vert_count;++i)
    {
        vert &v=verts[i];
        
        v.pos.x=reader.read<float>();
        v.pos.y=reader.read<float>();
        v.pos.z=-reader.read<float>();

        v.normal.x=reader.read<float>();
        v.normal.y=reader.read<float>();
        v.normal.z=-reader.read<float>();
        
        v.tc.x=reader.read<float>();
        v.tc.y=1.0f-reader.read<float>();
        reader.skip(header.extended_uv*sizeof(float)*4);

        switch(reader.read<char>())
        {
            case 0:
                v.bone_idx[0]=read_idx(reader,header.bone_idx_size);
                v.bone_weight[0]=1.0f;
                for(int j=1;j<4;++j)
                    v.bone_weight[j]=v.bone_idx[j]=0.0f;
                break;
                
            case 1:
                v.bone_idx[0]=read_idx(reader,header.bone_idx_size);
                v.bone_idx[1]=read_idx(reader,header.bone_idx_size);
                
                v.bone_weight[0]=reader.read<float>();
                v.bone_weight[1]=1.0f-v.bone_weight[0];
                
                for(int j=2;j<4;++j)
                    v.bone_weight[j]=v.bone_idx[j]=0.0f;
                break;
                
            case 2:
                for(int j=0;j<4;++j)
                    v.bone_idx[j]=read_idx(reader,header.bone_idx_size);
                
                for(int j=0;j<4;++j)
                    v.bone_weight[j]=reader.read<float>();
                break;
                
            case 3:
                v.bone_idx[0]=read_idx(reader,header.bone_idx_size);
                v.bone_idx[1]=read_idx(reader,header.bone_idx_size);
                
                v.bone_weight[0]=reader.read<float>();
                v.bone_weight[1]=1.0f-v.bone_weight[0];
                
                for(int j=2;j<4;++j)
                    v.bone_weight[j]=v.bone_idx[j]=0.0f;

                reader.skip(sizeof(float)*3*3);
                break;

            default:
                nya_log::log()<<"pmx load error: invalid skining\n";
                return false;
        }

        reader.read<float>(); //edge
    }

    const int indices_count=reader.read<int>();
    if(header.index_size==2)
        res.vbo.set_index_data(reader.get_data(),nya_render::vbo::index2b,indices_count);
    else if(header.index_size==4)
        res.vbo.set_index_data(reader.get_data(),nya_render::vbo::index4b,indices_count);
    else
    {
        nya_log::log()<<"pmx load error: invalid index size\n";
        return false;
    }

    reader.skip(indices_count*header.index_size);

    const int textures_count=reader.read<int>();
    std::vector<std::string> tex_names(textures_count);
    for(int i=0;i<textures_count;++i)
    {
        const int str_len=reader.read<int>();
        if(header.text_encoding==0)
        {
            /*
             NSData* data = [[[NSData alloc] initWithBytes:reader.get_data()
             length:str_len] autorelease];
             NSString* str = [[NSString alloc] initWithData:data
             encoding:NSUTF16LittleEndianStringEncoding];
             tex_names[i].assign(str.UTF8String);
             */

            tex_names[i]=utf8_from_utf16le(reader.get_data(),str_len);
            
            /*
            const char *data=(const char*)reader.get_data();
            //for(int j=0;j<str_len;++j) printf("%c",data[j]); printf("\n");

            for(int j=0;j<str_len;++j)
            {
                if(data[j]!=0)
                    tex_names[i].push_back(data[j]);
            }
*/
        }
        else
            tex_names[i]=std::string((const char*)reader.get_data(),str_len);
        
        reader.skip(str_len);
    }

    //printf("_________\n");

    const int mat_count=reader.read<int>();
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

    //nya_resources::file_resources_provider frp; frp.set_folder(path.c_str()); for(nya_resources::resource_info *fri=frp.first_res_info();fri;fri=fri->get_next()) printf("%s\n",fri->get_name());

    const nya_math::vec3 mmd_light_dir=-nya_math::vec3(-0.5,-1.0,-0.5).normalize();
    //const nya_math::vec3 mmd_light_color=nya_math::vec3(154,154,154)/255.0;

    for(int i=0,offset=0;i<mat_count;++i)
    {
        nya_scene::shared_mesh::group &g=res.groups[i];
        nya_scene::material &m = res.materials[i];

        for(int j=0;j<2;++j)
        {
            const int name_len=reader.read<int>();
            if(j==1)
            {
                std::string name((const char*)reader.get_data(),name_len);
                m.set_name(name.c_str());
            }
            reader.skip(name_len);
        }

        pmx_material_params params=reader.read<pmx_material_params>();

        const char unsigned flag=reader.read<unsigned char>();
        pmx_edge_params edge=reader.read<pmx_edge_params>();

        const int tex_idx=read_idx(reader,header.texture_idx_size);
        const int sph_tex_idx=read_idx(reader,header.texture_idx_size);
        const int sph_mode=reader.read<char>();
        const char toon_flag=reader.read<char>();
        
        int toon_tex_idx=-1;
        if(toon_flag==0)
        {
            toon_tex_idx=read_idx(reader,header.texture_idx_size);
            if(toon_tex_idx>=(int)tex_names.size())
            {
                nya_log::log()<<"pmx load error: invalid toon tex idx\n";
                return false;
            }
        }
        else if(toon_flag==1)
            toon_tex_idx=reader.read<char>();
        else
        {
            nya_log::log()<<"pmx load error: invalid toon flag\n";
            return false;
        }

        if(tex_idx>=(int)tex_names.size())
        {
            nya_log::log()<<"pmx load error: invalid tex idx\n";
            return false;
        }

        if(sph_tex_idx>=(int)tex_names.size())
        {
            nya_log::log()<<"pmx load error: invalid sph tex idx\n";
            return false;
        }

        const int comment_len=reader.read<int>();
        reader.skip(comment_len);

        g.name="mesh";
        g.offset=offset;
        g.count=reader.read<int>();
        g.material_idx=i;
        offset+=g.count;

        {
            nya_scene::texture tex;
            if(tex_idx<0 || !tex.load((path+tex_names[tex_idx]).c_str()))
            {
                typedef unsigned char uchar;
                unsigned char data[4]={uchar(params.diffuse[2]*255),uchar(params.diffuse[1]*255),
                                       uchar(params.diffuse[0]*255),uchar(params.diffuse[3]*255)};
                tex.build(&data,1,1,nya_render::texture::color_bgra);
            }

            m.set_texture("diffuse",tex);
        }

        {
            bool loaded=false;
            nya_scene::texture tex;
            if(toon_flag>0)
            {
                char buf[255];
                sprintf(buf,"toon%02d.bmp",toon_tex_idx+1);
                if(nya_resources::get_resources_provider().has((path+buf).c_str()))
                    loaded=tex.load((path+buf).c_str());
                else
                    loaded=tex.load(buf);
            }
            else if(toon_tex_idx>=0)
            {
                loaded=tex.load((path+tex_names[toon_tex_idx]).c_str());
                //printf("tex %d %s ",toon_tex_idx,tex_names[toon_tex_idx].c_str());
            }

            if(!loaded)
                tex.build("\xff\xff\xff\xff",1,1,nya_render::texture::color_bgra);

            m.set_texture("toon",tex);
        }

        {
            bool add=false,mult=false;
            nya_scene::texture tex;
            if(sph_tex_idx>=0 && tex.load((path+tex_names[sph_tex_idx]).c_str()))
            {
                if(sph_mode==2)
                    m.set_texture("env add",tex),add=true;
                else if(sph_mode==1)
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
        //else
        //    printf("sp %d %d\n",sph_mode,sph_tex_idx);

        nya_scene::material::pass &p=m.get_pass(m.add_pass(nya_scene::material::default_pass));
        nya_scene::shader sh;
        sh.load("pmx.nsh");
        p.set_shader(sh);
        p.get_state().set_blend(true,nya_render::blend::src_alpha,nya_render::blend::inv_src_alpha);
        p.get_state().set_cull_face(!(flag & (1<<0)),nya_render::cull_face::cw);

        m.set_param(m.get_param_idx("light dir"),mmd_light_dir);
        m.set_param(m.get_param_idx("amb k"),params.ambient[0],params.ambient[1],params.ambient[2],1.0f);
        m.set_param(m.get_param_idx("diff k"),params.diffuse[0],params.diffuse[1],params.diffuse[2],params.diffuse[3]);
        m.set_param(m.get_param_idx("spec k"),params.specular[0],params.specular[1],params.specular[2],params.shininess);

        if(!(flag & (1<<4)))
            continue;

        res.groups.resize(res.groups.size()+1);
        nya_scene::shared_mesh::group &ge=res.groups.back();
        ge=res.groups[i];
        ge.name="edge";
        ge.material_idx=int(res.materials.size());
        res.materials.resize(res.materials.size()+1);
        nya_scene::material &me=res.materials.back();

        nya_scene::material::pass &pe=me.get_pass(me.add_pass(nya_scene::material::default_pass));
        nya_scene::shader she;
        she.load("pmx_edge.nsh");
        pe.set_shader(she);
        pe.get_state().set_blend(true,nya_render::blend::src_alpha,nya_render::blend::inv_src_alpha);
        pe.get_state().set_cull_face(true,nya_render::cull_face::ccw);
        me.set_param(me.get_param_idx("edge offset"),edge.width*0.02f,edge.width*0.02f,edge.width*0.02f,0.0f);
        me.set_param(me.get_param_idx("edge color"),edge.color[0],edge.color[1],edge.color[2],edge.color[3]);
    }

    typedef unsigned short ushort;

    const int bones_count=reader.read<int>();
    if(bones_count>pmd_loader::gpu_skining_bones_limit)
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

    std::vector<pmx_bone> bones(bones_count);
    for(int i=0;i<bones_count;++i)
    {
        pmx_bone &b=bones[i];

        const int name_len=reader.read<int>();
        b.name=header.text_encoding?std::string((const char*)reader.get_data(),name_len):
        utf8_from_utf16le(reader.get_data(),name_len);
        reader.skip(name_len);
        reader.skip(reader.read<int>());

        b.pos.x=reader.read<float>();
        b.pos.y=reader.read<float>();
        b.pos.z=-reader.read<float>();
        b.idx=i;

        b.parent=read_idx(reader,header.bone_idx_size);
        b.order=reader.read<int>();

        const flag<ushort> f(reader.read<ushort>()); //ToDo
        if(f.c(0x0001))
            read_idx(reader,header.bone_idx_size);
        else
            reader.skip(sizeof(float)*3);

        b.bound.has_rot=f.c(0x0100);
        b.bound.has_pos=f.c(0x0200);
        if(b.bound.has_rot || b.bound.has_pos)
        {
            b.bound.src_idx=read_idx(reader,header.bone_idx_size);
            b.bound.k=reader.read<float>();
        }

        if(f.c(0x0400))
            reader.skip(sizeof(float)*3); //ToDo

        if(f.c(0x0800))
            reader.skip(sizeof(float)*3*2); //ToDo

        if(f.c(0x2000))
            reader.read<int>(); //ToDo

        b.ik.has=f.c(0x0020);
        if(b.ik.has)
        {
            b.ik.eff_idx=read_idx(reader,header.bone_idx_size);
            b.ik.count=reader.read<int>();
            b.ik.k=reader.read<float>();

            const int link_count=reader.read<int>();
            b.ik.links.resize(link_count);
            for(int j=0;j<link_count;++j)
            {
                pmx_bone::ik_link &l=b.ik.links[j];
                l.idx=read_idx(reader,header.bone_idx_size);
                l.has_limits=reader.read<char>();
                if(l.has_limits)
                {
                    l.from.x=reader.read<float>();
                    l.from.y=reader.read<float>();
                    l.from.z=reader.read<float>();

                    l.to.x=reader.read<float>();
                    l.to.y=reader.read<float>();
                    l.to.z=reader.read<float>();
                }
            }
        }
    }

    for(int i=0;i<int(bones.size());++i)
        if(bones[i].parent>=0)
            bones[i].parent_name=bones[bones[i].parent].name;

    //dumb sort
    for(int i=0;i<int(bones.size());++i)
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
                {
                    if(bones[k].parent==j)
                        bones[k].parent=p;
                    else if(bones[k].parent==p)
                        bones[k].parent=j;
                }
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
        const pmx_bone &b=bones[i];

        if((b.bound.has_pos || b.bound.has_rot) && b.bound.src_idx>=0 && b.bound.src_idx<bones_count)
            res.skeleton.add_bound(old_bones[b.bound.src_idx],i,b.bound.k,b.bound.has_pos,b.bound.has_rot,true);

        if(res.skeleton.add_bone(b.name.c_str(),b.pos,nya_math::quat(),b.parent,true)!=i)
        {
            nya_log::log()<<"pmx load error: invalid bone\n";
            return false;
        }

        if(b.ik.has)
        {
            const int ik=res.skeleton.add_ik(i,old_bones[b.ik.eff_idx],b.ik.count,b.ik.k,true);
            for(int j=0;j<int(b.ik.links.size());++j)
            {
                const pmx_bone::ik_link &l=b.ik.links[j];
                if(l.has_limits)
                {
                    //res.skeleton.add_ik_link(ik,link,-to.x,-from.x,true);
                    res.skeleton.add_ik_link(ik,old_bones[l.idx],0.001f,nya_math::constants::pi,true); //ToDo
                }
                else
                    res.skeleton.add_ik_link(ik,old_bones[l.idx],true);
            }
        }
    }

    add_data *ad=new add_data;
    res.add_data=ad;

    const int morphs_count=reader.read<int>();
    ad->morphs.resize(morphs_count);

    for(int i=0;i<morphs_count;++i)
    {
        pmd_morph_data::morph &m=ad->morphs[i];
        const int name_len=reader.read<int>();
        m.name=header.text_encoding?std::string((const char*)reader.get_data(),name_len):
                                                    utf8_from_utf16le(reader.get_data(),name_len);
        reader.skip(name_len);
        reader.skip(reader.read<int>());

        m.type=pmd_morph_data::morph_type(reader.read<char>());

        //printf("%d name %s %d\n",i,m.name.c_str(),m.type);

        const char morph_type=reader.read<char>();
        switch(morph_type)
        {
            case 0:
            {
                const int size=reader.read<int>(); //ToDo: group morph
                for(int j=0;j<size;++j)
                {
                    read_idx(reader,header.morph_idx_size);
                    reader.read<float>();
                }
            }
            break;

            case 1:
            {
                const int size=reader.read<int>();
                m.verts.resize(size);

                switch(header.index_size)
                {
                    case 1: load_vertex_morph<unsigned char>(reader,m); break;
                    case 2: load_vertex_morph<unsigned short>(reader,m); break;
                    case 4: load_vertex_morph<unsigned int>(reader,m); break;
                }
            }
            break;

            case 2:
            {
                const int size=reader.read<int>(); //ToDo: bone morph
                for(int j=0;j<size;++j)
                {
                    read_idx(reader,header.bone_idx_size);
                    nya_math::vec3 pos;
                    pos.x=reader.read<float>();
                    pos.y=reader.read<float>();
                    pos.z=-reader.read<float>();
                    nya_math::quat rot;
                    rot.v.x=-reader.read<float>();
                    rot.v.y=-reader.read<float>();
                    rot.v.z=reader.read<float>();
                    rot.w=reader.read<float>();
                }
            }
            break;

            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            {
                const int size=reader.read<int>(); //ToDo: uv morph
                for(int j=0;j<size;++j)
                {
                    read_idx(reader,header.index_size);
                    reader.skip(sizeof(float)*4);
                }
            }
            break;

            case 8:
            {
                const int size=reader.read<int>(); //ToDo: material morph
                for(int j=0;j<size;++j)
                {
                    read_idx(reader,header.material_idx_size);
                    reader.read<char>();
                    reader.skip(sizeof(float)*(4+4+3+4+1+4+4+4));
                }
            }
            break;

            default:
                nya_log::log()<<"pmx load error: invalid morph type\n";
                return false;
        }
    }

    res.vbo.set_vertex_data(&verts[0],sizeof(vert),vert_count);
    int offset=0;
    res.vbo.set_vertices(offset,3); offset+=sizeof(verts[0].pos);
    res.vbo.set_normals(offset); offset+=sizeof(verts[0].normal);
    res.vbo.set_tc(0,offset,2); offset+=sizeof(verts[0].tc);
    res.vbo.set_tc(1,offset,4); offset+=sizeof(verts[0].bone_idx);
    res.vbo.set_tc(2,offset,4); //offset+=sizeof(verts[0].bone_weight);

    return true;
}

const pmx_loader::additional_data *pmx_loader::get_additional_data(const nya_scene::mesh &m)
{
    if(!m.internal().get_shared_data().is_valid())
        return 0;

    nya_scene::shared_mesh::additional_data *d=m.internal().get_shared_data()->add_data;
    if(!d)
        return 0;

    const char *type=d->type();
    if(!type || strcmp(type,"pmx")!=0)
        return 0;

    return static_cast<pmx_loader::additional_data*>(static_cast<add_data*>(d));
}
