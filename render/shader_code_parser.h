//https://code.google.com/p/nya-engine/

#pragma once

#include <string>
#include <vector>
#include <map>

namespace nya_render
{

class shader_code_parser
{
public:
    const char *get_code() const { return m_code.c_str(); }
    const char *get_error() const { return m_error.c_str(); }

public:
    bool convert_to_hlsl();
    bool convert_to_glsl(); //resolves extensions
    bool convert_to_glsl_es2(const char *precision="mediump");
    bool convert_to_glsl3();

public:
    enum variable_type
    {
        type_invalid,
        type_float,
        type_vec2,
        type_vec3,
        type_vec4,
        type_mat4,
        type_sampler2d,
        type_sampler_cube
    };

    struct variable
    {
        variable_type type;
        std::string name;
        union { unsigned int array_size,idx; };

        variable():type(type_invalid),array_size(0){}
        variable(variable_type type,const char *name,unsigned int array_size):
                 type(type),name(name?name:""),array_size(array_size) {}
        bool operator < (const variable &v) const { return name<v.name; }
    };

    int get_uniforms_count();
    variable get_uniform(int idx) const;

    int get_attributes_count();
    variable get_attribute(int idx) const;

public:
    bool fix_per_component_functions();

public:
    shader_code_parser(const char *text,const char *replace_prefix_str="_nya_"):
                       m_code(text?text:""),m_replace_str(replace_prefix_str?replace_prefix_str:"") { remove_comments(); }
private:
    void remove_comments();

    bool parse_uniforms(bool remove);
    bool parse_predefined_uniforms(const char *replace_prefix_str,bool replace);
    bool parse_attributes(const char *info_replace_str,const char *code_replace_str);
    bool parse_varying(bool remove);

    bool replace_main_function_header(const char *replace_str);
    bool replace_vec_from_float(const char *func_name);
    bool replace_hlsl_types();
    bool replace_hlsl_mul(const char *func_name);
    bool replace_variable(const char *from,const char *to,size_t start_pos=0);
    bool find_variable(const char *str,size_t start_pos=0);

private:
    std::string m_code,m_replace_str,m_error;
    std::vector<variable> m_uniforms,m_attributes,m_varying;
};

}
