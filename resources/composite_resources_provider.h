//https://code.google.com/p/nya-engine/

#pragma once

#include "resources.h"
#include <map>
#include <string>
#include <vector>

namespace nya_resources
{

class composite_resources_provider: public resources_provider
{
public:
    void add_provider(resources_provider *provider);
    void enable_cache();
    void set_ignore_case(bool ignore); //enables cache if true

public:
    resource_data *access(const char *resource_name);
    bool has(const char *resource_name);

public:
    int get_resources_count();
    const char *get_resource_name(int idx);

public:
    composite_resources_provider(): m_ignore_case(false),m_cache_entries(false) {}

private:
    void cache_provider(int idx);

private:
    std::vector<resources_provider*> m_providers;
    std::vector<std::string> m_resource_names;

    struct entry
    {
        std::string original_name;
        int prov_idx;
    };

    typedef std::map<std::string,entry> entries_map;
    entries_map m_cached_entries;

    bool m_ignore_case;
    bool m_cache_entries;
};

}
