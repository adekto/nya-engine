//https://code.google.com/p/nya-engine/

#include "postprocess.h"
#include "formats/text_parser.h"
#include "scene.h"

namespace nya_scene
{

bool postprocess::load(const char *name)
{
    if(!scene_shared::load(name))
        return false;

    for(int i=0;i<(int)m_shared->lines.size();++i)
    {
        const shared_postprocess::line &l=m_shared->lines[i];
        if(l.type=="if")
            m_conditions[l.name]=false;
    }

    update();
    return true;
}

void postprocess::resize(unsigned int width,unsigned int height)
{
    m_width=width,m_height=height;
    update();
}

void postprocess::draw(int dt)
{
    for(size_t i=0;i<m_op.size();++i)
    {
        const size_t idx=m_op[i].idx;
        switch(m_op[i].type)
        {
            case type_draw_scene:
                draw_scene(m_op_draw_scene[idx].pass.c_str(),m_op_draw_scene[idx].tags.c_str());
                break;

            //ToDo
        }
    }
}

bool postprocess::load_text(shared_postprocess &res,resource_data &data,const char* name)
{
    nya_formats::text_parser parser;
    if(!parser.load_from_data((char *)data.get_data()))
        return false;

    res.lines.resize(parser.get_sections_count());
    for(int i=0;i<parser.get_sections_count();++i)
    {
        shared_postprocess::line &l=res.lines[i];
        l.type.assign(parser.get_section_type(i)+1);
        const char *name=parser.get_section_name(i);
        l.name.assign(name?name:"");

        l.values.resize(parser.get_subsections_count(i));
        for(int j=0;j<parser.get_subsections_count(i);++j)
        {
            l.values[j].first=parser.get_subsection_type(i,j);
            const char *value=parser.get_subsection_value(i,j);
            l.values[j].second=value?value:"";
        }
    }

    return true;
}

void postprocess::set_condition(const char *condition,bool value)
{
    if(!condition)
        return;

    conditions_map::iterator it=m_conditions.find(condition);
    if(it==m_conditions.end())
        return;

    it->second=value;
    update();
}

bool postprocess::get_condition(const char *condition) const
{
    if(!condition)
        return false;

    conditions_map::const_iterator it=m_conditions.find(condition);
    if(it==m_conditions.end())
        return false;

    return it->second;
}

void postprocess::update()
{
    if(!m_shared.is_valid())
        return;

    std::vector<bool> ifs;

    for(int i=0;i<(int)m_shared->lines.size();++i)
    {
        const shared_postprocess::line &l=m_shared->lines[i];

        if(l.type=="if")
        {
            ifs.resize(ifs.size()+1);
            ifs.back()=m_conditions[l.name];
            continue;
        }
        else if(l.type=="else")
        {
            if(ifs.empty())
            {
                log()<<"postprocess: syntax error - else without if in file "<<m_shared.get_name()<<"\n";
                return;
            }
            else
                ifs.back()=!ifs.back();
            continue;
        }
        else if(l.type=="end")
        {
            if(ifs.empty())
            {
                log()<<"postprocess: syntax error - end without if in file "<<m_shared.get_name()<<"\n";
                return;
            }
            else
                ifs.pop_back();
            continue;
        }

        bool should_contiue=false;
        for(int j=0;j<(int)ifs.size();++j)
        {
            if(!ifs[j])
            {
                should_contiue=true;
                break;
            }
        }

        if(should_contiue)
            continue;

        if(l.type=="target")
        {
            //ToDo
        }
        else if(l.type=="draw_scene")
        {
            m_op.resize(m_op.size()+1);
            m_op.back().type=type_draw_scene;
            m_op.back().idx=m_op_draw_scene.size();
            m_op_draw_scene.resize(m_op_draw_scene.size()+1);
            m_op_draw_scene.back().pass=l.name;
            if(!l.values.empty())
                m_op_draw_scene.back().tags=l.values.front().second;
        }
        else
            log()<<"postprocess: unknown operation "<<l.type<<" in file "<<m_shared.get_name()<<"\n";
    }
}

void postprocess::unload()
{
    if(m_quad.get_ref_count()==1)
        m_quad->release();
    m_quad.free();
    scene_shared::unload();
}

}
