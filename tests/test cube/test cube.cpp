//https://code.google.com/p/nya-engine/

#include "log/log.h"
#include "system/app.h"
#include "system/system.h"
#include "render/vbo.h"
#include "render/shader.h"
#include "render/transform.h"
#include "render/render.h"

#include "stdio.h"

class test_cube: public nya_system::app
{
private:
    void on_init_splash()
    {
        nya_log::get_log()<<"on_init_splash\n";

        nya_render::set_clear_color(0.0f,0.6f,0.7f,1.0f);
    }

    void on_splash(unsigned int dt)
    {
        nya_log::get_log()<<"on_splash\n";

        nya_render::clear(true,true);
    }

    void on_init()
    {
        nya_log::get_log()<<"on_init\n";

        nya_render::set_clear_color(0.2f,0.4f,0.5f,0.0f);
        nya_render::set_clear_depth(1.0f);
        nya_render::depth_test::enable();

        float vertices[] = 
        {
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
            -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
            -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
            -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
             0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
             0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 1.0f,
             0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
             0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f
        };

        unsigned short indices[] = 
        {
            0,2,1, 1,2,3, // -x
            4,5,6, 5,7,6, // +x
            0,1,5, 0,5,4, // -y
            2,6,7, 2,7,3, // +y
            0,4,6, 0,6,2, // -z
            1,3,7, 1,7,5, // +z
        };

        m_vbo.set_vertex_data(vertices,sizeof(float)*6,8);
        m_vbo.set_vertices(0,3);
        m_vbo.set_colors(sizeof(float)*3,3);
        m_vbo.set_index_data(indices,nya_render::vbo::index2b,
			     sizeof(indices)/sizeof(unsigned short));

        const char *vs_code=
            "varying vec4 color;"
            "void main()"
            "{"
                "color=gl_Color;"
                "gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex;"
            "}";

        const char *ps_code=
            "varying vec4 color;"
            "void main()"
            "{"
                "gl_FragColor=color;"
            "}";

        m_shader.add_program(nya_render::shader::vertex,vs_code);
        m_shader.add_program(nya_render::shader::pixel,ps_code);
    }

    void on_process(unsigned int dt)
    {
        m_rot+=dt*0.05f;

        static unsigned int fps_counter=0;
        static unsigned int fps_update_timer=0;

        ++fps_counter;

        fps_update_timer+=dt;
        if(fps_update_timer>1000)
        {
            char name[255];
            sprintf(name,"test cube %d fps",fps_counter);
            set_title(name);

            fps_update_timer%=1000;
            fps_counter=0;
        }
    }

    void on_draw()
    {
        nya_render::clear(true,true);

        nya_math::mat4 mv;
        mv.translate(0,0,-2.0f);
        mv.rotate(30.0f,1.0f,0.0f,0.0f);
        mv.rotate(m_rot,0.0f,1.0f,0.0f);
        nya_render::transform::get().set_modelview_matrix(mv);

        m_shader.bind();
        m_vbo.bind();
        m_vbo.draw();
        m_vbo.unbind();
        m_shader.unbind();
    }

    void on_resize(unsigned int w,unsigned int h)
    {
        nya_log::get_log()<<"on_resize "<<w<<" "<<h<<"\n";

        if(!w || !h)
            return;

        nya_math::mat4 proj;
        proj.perspective(70.0f,float(w)/h,0.01f,100.0f);
        nya_render::transform::get().set_projection_matrix(proj);
    }

    void on_free()
    {
        nya_log::get_log()<<"on_free\n";

        m_vbo.release();
        m_shader.release();
    }

public:
    test_cube(): m_rot(0.0f) {}

private:
    nya_render::vbo m_vbo;
    nya_render::shader m_shader;
    float m_rot;
};

#ifdef _WIN32
    int CALLBACK WinMain(HINSTANCE,HINSTANCE,LPSTR,int)
#else
    int main(int argc, char **argv)
#endif
{
    /*
    nya_log::plain_file_log log;
    log.open((std::string(nya_system::get_app_path())+"log.txt").c_str());

    nya_log::set_log(&log);
    */

    nya_log::get_log()<<"test cube started from path "
                      <<nya_system::get_app_path()<<"\n";

    test_cube app;
    app.set_title("Loading, please wait...");
    app.start_windowed(100,100,640,480,0);

//  log.close();

    return 0;
}