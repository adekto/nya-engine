//https://code.google.com/p/nya-engine/

#pragma once

#include <stddef.h>

namespace nya_formats
{

struct dds
{
    unsigned int width;
    unsigned int height;

    unsigned int mipmap_count;
    bool need_generate_mipmaps;

    enum texture_type
    {
        texture_2d,
        texture_cube
    };

    texture_type type;

    enum pixel_format
    {
        dxt1,
        dxt2,
        dxt3,
        dxt4,
        dxt5,
        bgra,
        bgr,
        palette4_rgba,
        palette8_rgba,
        greyscale
    };

    pixel_format pf;

    const void *data;
    size_t data_size;
    size_t mip0_data_size;

    dds(): width(0),height(0),mipmap_count(0),need_generate_mipmaps(false),
           data(0),data_size(0),mip0_data_size(0) {}

public:
    size_t decode_header(const void *data,size_t size); //0 if invalid
    void flip_vertical(const void *from_data,void *to_data);
    void decode_palette8_rgba(const void *from_data,void *to_data); //width*height*4 to_data buf required
};

}
