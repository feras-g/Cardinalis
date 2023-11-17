#pragma once

#include <functional>

struct Material
{
	int texture_base_color_idx = -1;
	int texture_normal_map_idx = -1;
	int texture_metalness_roughness_idx = -1;
	int texture_emissive_map_idx = -1;
    glm::vec4 base_color;
    glm::vec2 metalness_roughness;
};

/* https://live.boost.org/doc/libs/1_42_0/boost/functional/hash/hash.hpp */
template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

struct MaterialHash
{
    std::size_t operator()(const Material& m) const noexcept
    {
        size_t h = 0;
        hash_combine(h, m.texture_base_color_idx);
        hash_combine(h, m.texture_normal_map_idx);
        hash_combine(h, m.texture_metalness_roughness_idx);
        hash_combine(h, m.texture_emissive_map_idx);
        hash_combine(h, m.base_color);
        return h;
    }
};


