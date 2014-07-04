//https://code.google.com/p/nya-engine/

#pragma once

#include <cstddef>
#include <string.h>

namespace nya_memory
{

class memory_reader
{
public:
    template <typename t> t read()
    {
        t a;
        const size_t size=sizeof(t);
        if(size>m_size-m_offset)
        {
            m_offset=m_size;
            memset(&a,0,size);
            return a;
        }

        memcpy(&a,m_data+m_offset,size);
        m_offset+=size;

        return a;
    }
    
    bool test(const void*data,size_t size)
    {
        if(size>m_size-m_offset)
        {
            //m_offset=m_size;
            return false;
        }

        if(memcmp(m_data+m_offset,data,size)!=0)
        {
            //m_offset+=size;
            return false;
        }

        m_offset+=size;
        return true;
    }

    bool check_remained(size_t size) const { return size<=m_size-m_offset; }

    bool seek(size_t offset)
    {
        if(offset>=m_size)
        {
            m_offset=m_size;
            return false;
        }

        m_offset=offset;
        return true;
    }

    bool skip(size_t offset)
    {
        m_offset+=offset;
        if(m_offset<offset || m_offset > m_size) //overflow
            m_offset=m_size;

        return m_offset<m_size;
    }

    bool rewind(size_t offset)
    {
        if(offset>m_offset)
        {
            m_offset=0;
            return false;
        }

        m_offset-=offset;
        return true;
    }

    size_t get_offset() const { return m_offset; }

    size_t get_remained() const
    {
        if(m_offset>=m_size)
            return 0;

        return m_size-m_offset;
    }

    const void *get_data()  const
    {
        if(m_offset>=m_size)
            return 0;

        return m_data+m_offset;
    }

    memory_reader(const void *data,size_t size)
    {
        if(data)
        {
            m_data=(char*)data;
            m_size=size;
        }
        else
        {
            m_data=0;
            m_size=0;
        }

        m_offset=0;
    }

private:
    const char *m_data;
    size_t m_size;
    size_t m_offset;
};

}
