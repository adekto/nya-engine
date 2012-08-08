//https://code.google.com/p/nya-engine/

#include "ui/button.h"

namespace nya_ui
{

void button::draw(layer &layer)
{
    rect r=get_draw_rect();
    if(!r.w || !r.h)
        return;

    layer::rect_style rs;

    if(m_mouse_over)
        rs.border_color.set(0.7,0.6,1.0,1.0);
    else
        rs.border_color.set(0.4,0.3,1.0,1.0);

    rs.border=true;

    rs.solid_color=rs.border_color;
    rs.solid_color.a=0.5f;
    rs.solid=true;
    
    layer.draw_rect(r,rs);
 
    if(!m_text.empty())
        layer.draw_text(r.x+r.w/2,r.y+r.h/2,m_text.c_str(),layer::center,layer::center);
}

}