//https://code.google.com/p/nya-engine/

/*
    left-bottom alligned
*/

#pragma once

#include "log/log.h"
#include "memory/shared_ptr.h"

#include <string>
#include <list>
#include <deque>

namespace nya_ui
{

void set_log(nya_log::log *l);
nya_log::log &get_log();

typedef unsigned int uint;

uint clamp(int v,uint from,uint to);
float clamp(float v,float from,float to);

struct point
{
    uint x; uint y;

    point(): x(0),y(0) {}
    point(uint x,uint y): x(x),y(y) {}
};

struct rect
{
    uint x; uint y; uint w; uint h;

    bool check_point(const point &p) const { return check_point(p.x,p.y); }
    bool check_point(uint px, uint py) const { return !(px<x || py<y || px>x+w || py>y+h); }

    rect(): x(0),y(0),w(0),h(0) {}
    rect(uint x,uint y,uint w,uint h): x(x),y(y),w(w),h(h) {}
};

class layer;
class widget;
class layout
{
public:
    virtual void add_widget(widget &w);
    virtual void draw_widgets(layer &l);
    virtual void resize(uint width,uint height);
    virtual void move(int x,int y);
    virtual void remove_widget() {}

public:
    enum mbutton
    {
        left_button,
        middle_button,
        right_button
    };

    virtual bool mouse_button(mbutton button,bool pressed);
    virtual bool mouse_move(uint x,uint y);
    virtual void mouse_left();
    virtual bool mouse_scroll(uint dx,uint dy);

public:
    virtual uint get_width() { return m_width; }
    virtual uint get_height() { return m_height; }
    virtual point get_mouse_pos() { return point(m_mouse_x,m_mouse_y); }

public:
    struct event { std::string sender; std::string type; };

public:
    virtual void send_event(event &e) { printf("send_event %s %s\n",e.sender.c_str(),e.type.c_str() ); }
    void send_event(const char *sender_id,const char *event)
    {
        if(!sender_id || !event)
            return;

        layout::event e;
        e.sender.assign(sender_id);
        e.type.assign(event);
        send_event(e);
    }

    virtual void process_events(event &e);

public:
    layout(): m_x(0),m_y(0),m_width(0),m_height(0),m_mouse_x(0),m_mouse_y(0) {}

protected:
    typedef std::list<widget*> widgets_list;
    widgets_list m_widgets;
    int m_x;
    int m_y;
    uint m_width;
    uint m_height;
    uint m_mouse_x;
    uint m_mouse_y;
};

class widget
{
    friend class layout;

public:
    virtual void set_id(const char *id) { if(id) m_id.assign(id); }

    virtual void set_pos(int x,int y)
    {
        m_pos_left=x, m_pos_bottom=y;

        calc_pos_markers();
        update_mouse_over();
    }

    virtual void set_size(uint width,uint height)
    {
        m_width=width, m_height=height;

        calc_pos_markers();
        update_mouse_over();
    }

    virtual void set_visible(bool visible)
    {
        if(m_visible==visible)
            return;

        update_mouse_over();
        m_visible=visible;
    }

    virtual void set_align(bool left,bool right,bool top,bool bottom)
    {
        m_align_left=left, m_align_right=right;
        m_align_top=top, m_align_bottom=bottom;

        calc_pos_markers();
        update_mouse_over();
    }

    enum keep_aspect
    {
        none,
        from_width,
        from_height
    };

    virtual void set_keep_aspect(keep_aspect a)
    {
        m_keep_aspect=a;
        calc_pos_markers();
        update_mouse_over();
    }

    virtual bool is_visible() { return m_visible; }
    virtual bool is_mouse_over() { return m_mouse_over; }

    virtual const char *get_id() { return m_id.c_str(); }

    virtual void get_pos(int &x,int &y) { x=m_pos_left; y=m_pos_bottom; }
    virtual void get_size(uint &width,uint &height) { width=m_width; height=m_height; }

protected:
    virtual void parent_moved(int x,int y)
    {
        m_parent_pos_x=x, m_parent_pos_y=y;

        m_cached_rect=false;
        update_mouse_over();
    }

    virtual void parent_resized(uint width,uint height)
    {
        m_parent_width=width, m_parent_height=height;

        m_cached_rect=false;
        update_mouse_over();
    }

protected:
    virtual void update_mouse_over()
    {
        const bool over=m_parent?get_rect().check_point(m_parent->get_mouse_pos()):false;
        if(over)
        {
            if(!m_mouse_over)
            {
                on_mouse_over();
                m_mouse_over=true;
            }
        }
        else if(m_mouse_over)
        {
            on_mouse_left();
            m_mouse_over=false;
        }
    }

protected:
    virtual void on_mouse_over() { send_to_parent("mouse_over"); }
    virtual void on_mouse_left() { send_to_parent("mouse_left"); }
    virtual bool on_mouse_move(uint x,uint y,bool inside) { return false; }
    virtual bool on_mouse_button(layout::mbutton button,bool pressed) { return false; }
    virtual bool on_mouse_scroll(uint x,uint y) { return false; }

protected:
    virtual void draw(layer &l) {}
    virtual void process_events(layout::event &e) {}

protected:
    virtual void send_to_parent(layout::event &e)
    {
        if(!m_parent)
            return;

        m_parent->send_event(e); printf("send_to_parent %s %s\n",e.sender.c_str(),e.type.c_str() );
    }

    void send_to_parent(const char *event) { if(!m_id.empty()) send_to_parent(m_id.c_str(),event); }

    void send_to_parent(const char *sender,const char *event)
    {
        if(!sender || !event)
            return;

        layout::event e;
        e.sender.assign(sender);
        e.type.assign(event);
        send_to_parent(e);
    }

protected:
    virtual const rect &get_rect();

protected:
    virtual void calc_pos_markers()
    {
        if(m_align_right)
            m_pos_right=m_parent_width-m_width-m_pos_left;
        else if(!m_align_left)
            m_center_x=m_parent_width/2-(m_pos_left+m_width/2);

        if(m_align_top)
            m_pos_top=m_parent_height-m_height-m_pos_bottom;
        else if(!m_align_bottom)
            m_center_y=m_parent_height/2-(m_pos_bottom+m_height/2);

        m_cached_rect=false;
    }

public:
    widget(): m_pos_left(0),m_pos_right(0),m_pos_top(0),m_pos_bottom(0),
              m_center_x(0),m_center_y(0),m_width(0),m_height(0),
              m_parent(0), m_parent_pos_x(0),m_parent_pos_y(0),
              m_parent_width(0),m_parent_height(0),
              m_align_left(true),m_align_right(false),
              m_align_top(false),m_align_bottom(true),
              m_keep_aspect(none),m_visible(true), m_mouse_pressed(false),
              m_mouse_over(false), m_cached_rect(false) {}

protected:
    std::string m_id;

    int m_pos_left;
    int m_pos_right;
    int m_pos_top;
    int m_pos_bottom;
    int m_center_x;
    int m_center_y;

    uint m_width;
    uint m_height;

    layout *m_parent;

    int m_parent_pos_x;
    int m_parent_pos_y;
    uint m_parent_width;
    uint m_parent_height;

    bool m_align_left;
    bool m_align_right;
    bool m_align_top;
    bool m_align_bottom;

    keep_aspect m_keep_aspect;

    bool m_visible;
    bool m_mouse_over;
    bool m_mouse_pressed;

    bool m_cached_rect;
    rect m_rect;
};

class layer: public layout
{
public:
    virtual void draw();
    virtual void resize(uint width,uint height);
    virtual void process();

private:
    virtual void process_events(event &e) {}

public:
    virtual void send_event(event &e);

private:
    typedef std::deque<event> events_deque;
    events_deque m_events;
};

}
