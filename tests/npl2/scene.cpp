//https://code.google.com/p/nya-engine/

#include "scene.h"
#include "attributes.h"
#include "resources/resources.h"

#include "tsb_anim.h"

void viewer_camera::apply()
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glScalef(m_scale,m_scale,m_scale);
    glTranslatef(m_pos_x,m_pos_y,0);
    glRotatef(m_rot_y,1.0f,0.0f,0.0f);    
    glRotatef(m_rot_x,0.0f,1.0f,0.0f);
    glTranslatef(0,-8,0);
}

void viewer_camera::add_rot(float dx,float dy)
{
    m_rot_x+=dx;
    m_rot_y+=dy;

    const float max_angle=360.0f;
    
    if ( m_rot_x > max_angle )
        m_rot_x -= max_angle;
    
    if ( m_rot_x < -max_angle )
        m_rot_x += max_angle;
    
    if ( m_rot_y > max_angle )
        m_rot_y -= max_angle;
    
    if ( m_rot_y < -max_angle )
        m_rot_y += max_angle;
    
}

void viewer_camera::add_pos(float dx,float dy)
{
    m_pos_x+=dx;
    m_pos_y+=dy;
}

void viewer_camera::add_scale(float ds)
{
    m_scale *= (1.0f+ds);
    const float min_scale=0.01f;
    if(m_scale<min_scale)
        m_scale=min_scale;
}

void scene::init()
{    
    get_shared_anims().should_unload_unused(false);
    //get_shared_models().set_lru_limit(20);
    //get_shared_models().should_unload_unused(false);
    
    m_anim_list.push_back("event_01");
    m_anim_list.push_back("event_02");
    m_anim_list.push_back("event_03");
    m_anim_list.push_back("event_04");
    m_anim_list.push_back("event_05");
    m_anim_list.push_back("event_06");
    m_anim_list.push_back("umauma00_F");
    
    std::vector<char> script_buf;
    std::string script_str;
    nya_resources::resource_info *info=nya_resources::get_resources_provider().first_res_info();
    while(info)
    {
        if(info->check_extension(".txt"))
        {
            //nya_log::get_log()<<"script: "<<info->get_name()<<"\n";
            nya_resources::resource_data *data=info->access();
            if(data)
            {
                script_buf.resize(data->get_size());
                data->read_all(&script_buf[0]);
                script_buf.push_back(0);
                script_str.assign(&script_buf[0]);
                
                size_t last_pos=0;
                size_t pos=script_str.find("%mm");
                while(pos!=std::string::npos)
                {
                    if(pos+5>=script_str.size())
                        break;
                    
                    size_t pos2=script_str.find(";",pos+5);
                    if(pos2==std::string::npos)
                        break;
                    
                    const std::string anim_name=script_str.substr(pos+5,pos2-(pos+5));
                    //nya_log::get_log()<<"anim: "<<anim_name.c_str()<<"\n";
                    
                    char num = script_str[pos+3];
                    if(num=='0')
                    {
                        m_anim_list.push_back(anim_name.c_str());
                        
                        nya_log::get_log()<<"anim0: "<<anim_name.c_str()<<"\n";
                        
                        size_t loc_pos=script_str.rfind("%mp0,",pos);
                        if(loc_pos!=std::string::npos)
                        {
                            size_t loc_pos2=script_str.find(";",loc_pos);
                            std::string pos=script_str.substr(loc_pos+5,loc_pos2-(loc_pos+5));
                            nya_log::get_log()<<"\tpos0: "<<pos.c_str()<<"\n";
                        }
                        
                        last_pos=pos2;
                    }
                    else if(num<='9')
                    {
                        m_anim_list.back().name[num-'1'+1]=anim_name;
                        if(num=='1')
                        {
                            size_t loc_pos=script_str.rfind("%mp1,",pos);
                            if(loc_pos!=std::string::npos)
                            {
                                size_t loc_pos2=script_str.find(";",loc_pos);
                                std::string pos=script_str.substr(loc_pos+5,loc_pos2-(loc_pos+5));
                                nya_log::get_log()<<"\tpos1: "<<pos.c_str()<<"\n";
                            }
                        }
                    }
                    
                    pos=script_str.find("%mm",pos2);
                }
                
                data->release();
            }
        }
        
        info=info->get_next();
    }
    
    m_curr_anim = m_anim_list.begin();
    if(m_curr_anim!=m_anim_list.end())
        m_imouto.set_anim(m_curr_anim->name[0].c_str());
    
    nya_resources::resource_data *model_res = nya_resources::get_resources_provider().access("ani_bodyA_00.tmb");
    if(model_res)
    {
        m_aniki.load(model_res);
        model_res->release();
    }

    
    m_shader_scenery.add_program(nya_render::shader::vertex,
                                 
                                 "mat3 get_rot(mat4 m)"
                                 "{"
                                 "  return mat3(m[0].xyz,m[1].xyz,m[2].xyz);"
                                 "}"
                                 
                                 "void main()"
                                 "{"
                                 "  gl_TexCoord[0]=gl_MultiTexCoord0;"
                                 "  gl_TexCoord[3]=gl_MultiTexCoord3;"
                                 "  gl_TexCoord[2]=gl_Color;"
                                 
                                 "  gl_TexCoord[1].xyz=get_rot(gl_ModelViewMatrix)*gl_Normal.xyz;"
                                 
                                 "  gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex;"
                                 "}");
    
    m_shader_scenery.add_program(nya_render::shader::pixel,
                                 "uniform sampler2D base_map;"
                                 "void main(void)"
                                 "{"
                                 "  vec4 color=gl_TexCoord[2];"
                                 "  vec4 vcolor=gl_TexCoord[3];"
                                 "  vec4 base=texture2D(base_map,gl_TexCoord[0].xy)*color;"
                                 //"  float l=dot(normalize(vec3(0,0,1.0)),normalize(gl_TexCoord[1].xyz));"
                                 //"  float ls=dot(normalize(vec3(-0.3,0,1.0)),normalize(gl_TexCoord[1].xyz));"
                                 //"  gl_FragColor=vec4((0.15+max(0.0,l*0.85))*base.rgb+pow(max(ls,0.0),90.0)*vec3(0.06),base.a);"
                                 //"  gl_FragColor=vec4(base.rgb+pow(max(ls,0.0),90.0)*vec3(0.06),base.a);"
                                 "  gl_FragColor=base*color*vcolor;"
                                 //"  gl_FragColor=vcolor;"
                                 "}");

    const char *vprogram=
    "uniform mat4 bones[200];"
    
    "mat3 get_rot(mat4 m)"
    "{"
    "  return mat3(m[0].xyz,m[1].xyz,m[2].xyz);"
    "}"
    
    "void main()"
    "{"
    "  gl_TexCoord[0]=gl_MultiTexCoord0;"
    "  vec4 bone_idx=gl_MultiTexCoord1;"
    "  vec4 bone_weight=gl_MultiTexCoord2;"
    
    "  mat4 bone0=bones[int(bone_idx.x)]*bone_weight.x;"
    "  mat4 bone1=bones[int(bone_idx.y)]*bone_weight.y;"
    "  mat4 bone2=bones[int(bone_idx.z)]*bone_weight.z;"
    "  mat4 bone3=bones[int(bone_idx.w)]*bone_weight.w;"
    
    "  vec4 pos=bone0*gl_Vertex;"
    "  pos+=bone1*gl_Vertex;"
    "  pos+=bone2*gl_Vertex;"
    "  pos+=bone3*gl_Vertex;"
    
    "  vec3 nor=get_rot(bone0)*gl_Normal.xyz;"
    "  nor+=get_rot(bone1)*gl_Normal.xyz;"
    "  nor+=get_rot(bone2)*gl_Normal.xyz;"
    "  nor+=get_rot(bone3)*gl_Normal.xyz;"
    
    "  gl_TexCoord[2]=gl_Color;"
    //"  gl_TexCoord[3]=gl_MultiTexCoord3;"
    
    "  gl_TexCoord[1].xyz=get_rot(gl_ModelViewMatrix)*nor;"
    
    "  gl_Position=gl_ModelViewProjectionMatrix*pos;"
    "}";
    
    //for(int i=0;i<128;++i) printf("%c|%i/",i,i);
    
    m_shader.add_program(nya_render::shader::vertex,vprogram);
    m_shader.add_program(nya_render::shader::pixel,
                         "uniform sampler2D base_map;"
                         "void main(void)"
                         "{"
                         "  vec4 color=gl_TexCoord[2];"
                         //"  vec4 vcolor=gl_TexCoord[3];"
                         "  vec4 base=texture2D(base_map,gl_TexCoord[0].xy)*color;"
                         //"  float l=dot(normalize(vec3(0,0,1.0)),normalize(gl_TexCoord[1].xyz));"
                         "  float ls=dot(normalize(vec3(-0.3,0,1.0)),normalize(gl_TexCoord[1].xyz));"
                         //"  gl_FragColor=vec4((0.85+max(0.0,l*0.15))*base.rgb+pow(max(ls,0.0),90.0)*vec3(0.06),base.a);"
                         "  gl_FragColor=vec4(base.rgb+pow(max(ls,0.0),90.0)*vec3(0.06),base.a);"
                         //"  gl_FragColor=base;"
                         "}");
    
    m_shader.set_sampler("base_map",0);
    m_sh_mat_uniform=m_shader.get_handler("bones");
    
    m_shader_black.add_program(nya_render::shader::vertex,vprogram);
    m_shader_black.add_program(nya_render::shader::pixel,
                               "void main(void)"
                               "{"
                               "  gl_FragColor=vec4(0,0,0,0.3);"
                               "}");
    m_shbl_mat_uniform=m_shader_black.get_handler("bones");
    

    
    m_imouto.set_attrib("COORDINATE","ok_vocaloid00");
    m_imouto.set_attrib("BODY","imo_bodyB_00");
}

void scene::set_bkg(const char *name)
{
    for(int i=0;i<max_bkg_models;++i)
        m_bkg_models[i].release();
    
    m_has_scenery=false;

    attribute *atr=get_attribute_manager().get("BG",name);
    if(!atr)
        return;

    int to=max_bkg_models;
    if(to>10)
        to=10;

    for(int i=0;i<to;++i)
    {
        char key[16]="FILE_0";
        key[5]='0'+i;

        const char *name=atr->get_value(key);
        if(!name)
        {
            nya_log::get_log()<<"invalid scnery file name";
            continue;
        }

        if(strcmp(name,"nil")==0)
            continue;

        nya_resources::resource_data *scenery_res = nya_resources::get_resources_provider().access(name);
        if(scenery_res)
        {
            m_bkg_models[i].load(scenery_res);
            scenery_res->release();
        }
    }
    
    m_has_scenery=true;
}

void scene::draw()
{
    if(m_has_scenery)
        glClearColor(0,0,0,0);
    else
        glClearColor(0.2,0.4,0.5,0);
    
    character &imouto=m_preview?m_imouto_preview:m_imouto;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    m_camera.apply();
  
	glEnable     ( GL_DEPTH_TEST );
	glDepthFunc(GL_LESS);
    
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(1.0f);    

    glEnable(GL_TEXTURE_2D);
    
	glColor4f(1,1,1,1);
    
    glEnable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    
    m_shader_scenery.bind();
    for(int i=0;i<max_bkg_models;++i)
        m_bkg_models[i].draw(true);

    m_shader_scenery.unbind();
    
    m_anim_time+=1.0f;
    
    const size_t frames_count=imouto.get_frames_count();
    if(m_anim_time>=frames_count)
        m_anim_time=0;
    
    if(frames_count)
    {
        m_shader.bind();
        m_shader.set_uniform16_array(m_sh_mat_uniform,
                                     imouto.get_buffer(int(m_anim_time)),
                                     imouto.get_bones_count());
    }
    
    imouto.draw(true);
    
    //bro
    const size_t bro_frames_count=m_aniki.get_frames_count();
    if(bro_frames_count)
    {
        m_shader.set_uniform16_array(m_sh_mat_uniform,
                                     m_aniki.get_buffer(int(m_anim_time)),
                                     m_aniki.get_bones_count());

        //glTranslatef(m_bro_dpos_x,m_bro_dpos_y,m_bro_dpos_z);
        m_aniki.draw(true);
        //glTranslatef(-m_bro_dpos_x,-m_bro_dpos_y,-m_bro_dpos_z);
    }
    /*
     struct test_vertex
     {
     float pos[3];
     float bone_idx[3];
     float bone_weight[3];
     };
     
     test_vertex vertices[200];
     for(int i=0;i<200;++i)
     {
     test_vertex &v=vertices[i];
     v.pos[0]=v.pos[1]=v.pos[2]=0;
     v.bone_idx[0]=(float)i;
     v.bone_idx[1]=v.bone_idx[2]=0;
     v.bone_weight[0]=1.0f;
     v.bone_weight[1]=v.bone_weight[2]=0;
     }
     
     glPointSize(4);
     
     glEnableClientState(GL_VERTEX_ARRAY);
     glVertexPointer(3,GL_FLOAT,sizeof(test_vertex),vertices);
     glClientActiveTexture(GL_TEXTURE1);
     glEnableClientState(GL_TEXTURE_COORD_ARRAY);
     glTexCoordPointer(3,GL_FLOAT,sizeof(test_vertex),&(vertices[0].bone_idx[0]));
     glClientActiveTexture(GL_TEXTURE2);
     glEnableClientState(GL_TEXTURE_COORD_ARRAY);
     glTexCoordPointer(3,GL_FLOAT,sizeof(test_vertex),&(vertices[0].bone_weight[0]));
     glDrawArrays(GL_POINTS,0,200);
     glDisableClientState(GL_VERTEX_ARRAY);
     */
    if(frames_count)
        m_shader.unbind();
    
    glEnable(GL_CULL_FACE);
    
    glDisable(GL_TEXTURE_2D);

	glColor4f(0,0,0,0.3f);    
    
    glPolygonMode(GL_BACK,GL_LINE);
    glCullFace(GL_FRONT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    
    //m_scenery.draw(false);
    
    if(frames_count)
    {
        m_shader_black.bind();  
        m_shader_black.set_uniform16_array(m_shbl_mat_uniform,
                                           imouto.get_buffer(int(m_anim_time)),
                                           imouto.get_bones_count());
    }
    
    imouto.draw(false);
    
    //bro
    if(bro_frames_count)
    {
        m_shader_black.set_uniform16_array(m_shbl_mat_uniform,
                                           m_aniki.get_buffer(int(m_anim_time)),
                                           m_aniki.get_bones_count());
        
        //glTranslatef(m_bro_dpos_x,m_bro_dpos_y,m_bro_dpos_z);
        m_aniki.draw(false);
        //glTranslatef(-m_bro_dpos_x,-m_bro_dpos_y,-m_bro_dpos_z);
    }
    
    if(frames_count)
        m_shader_black.unbind();
    
    glDisable(GL_BLEND);   
    glCullFace(GL_BACK);
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    
    glDisable( GL_DEPTH_TEST );
}

void scene::set_imouto_attr(const char *key,const char *value,int num)
{
    m_imouto.set_attrib(key,value,num);
}

void scene::set_imouto_preview(const char *key,const char *value,int num)
{
    m_imouto_preview.copy_attrib(m_imouto);
    m_imouto_preview.set_attrib(key,value,num);
    m_imouto_preview.set_anim(m_imouto.get_anim());
    m_preview=true;
}

void scene::finish_imouto_preview()
{
    m_preview=false;
}

void scene::prev_anim()
{
    if(m_anim_list.empty())
        return;
    
    if(m_curr_anim-- == m_anim_list.begin())
        m_curr_anim= --m_anim_list.end();

    
    m_imouto.set_anim(m_curr_anim->name[0].c_str());
    m_anim_time=0;
    
    std::string bro_anim=m_curr_anim->name[1];
    
    if(bro_anim.empty())
    {
        m_aniki.apply_anim(0);
        return;
    }
    
    bro_anim.append(".tsb");
    
    anim_ref a=get_shared_anims().access(bro_anim.c_str());
    m_aniki.apply_anim(a.get());
}

void scene::next_anim()
{
    if(m_anim_list.empty())
        return;
    
    if(++m_curr_anim==m_anim_list.end())
        m_curr_anim=m_anim_list.begin();
    
    m_imouto.set_anim(m_curr_anim->name[0].c_str());
    m_anim_time=0;
    
    std::string bro_anim=m_curr_anim->name[1];
    
    if(bro_anim.empty())
    {
        m_aniki.apply_anim(0);
        return;
    }
    
    bro_anim.append(".tsb");
    
    anim_ref a=get_shared_anims().access(bro_anim.c_str());
    m_aniki.apply_anim(a.get());
}

void scene::set_anim(const char *name)
{
}

void scene::release()
{
    for(int i=0;i<max_bkg_models;++i)
        m_bkg_models[i].release();

    m_imouto.release();
    m_aniki.release();

    m_shader.release();
    m_shader_scenery.release();
}

scene &get_scene()
{
    static scene scene;
    return scene;
}