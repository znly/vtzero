#pragma once

#include "protozero/pbf_builder.hpp"
#include "protozero/pbf_message.hpp"
#include "vtzero/builder.hpp"

namespace vtzero {

struct mesh_data {
    std::vector<int32_t> coordinates;
    std::vector<uint32_t> triangles;
    std::vector<uint8_t> normal_map;
};

class mesh_layer {
public:
    static constexpr uint32_t extension_id = 16;

    enum class tag : protozero::pbf_tag_type {
        type = 1,
        elevation_min = 10,
        elevation_max = 11,
        coordinates = 20,
        triangles = 21,
        normal_map = 40,
    };

    static std::string encode(const mesh_data &mesh) {
        std::string data;
        protozero::pbf_builder<tag> builder { data };

        builder.add_packed_sint32(tag::coordinates, std::begin(mesh.coordinates), std::end(mesh.coordinates));
        builder.add_packed_uint32(tag::triangles, std::begin(mesh.triangles), std::end(mesh.triangles));
        // if (!mesh.normal_map.empty()) {
        //     builder.add_bytes(tag::normal_map, std::begin(mesh.triangles), std::end(mesh.triangles));
        // }

        return data;
    }

    static bool decode(protozero::data_view view, mesh_data &mesh) {
        protozero::pbf_message<tag> message { view };

        while (message.next()) {
            switch (message.tag_and_type()) {
                case protozero::tag_and_type(tag::type, protozero::pbf_wire_type::varint):
                    message.skip();
                    break;
                case protozero::tag_and_type(tag::coordinates, protozero::pbf_wire_type::length_delimited): {
                    auto pi = message.get_packed_sint32();
                    for (auto it = pi.begin(); it != pi.end(); ++it) {
                        mesh.coordinates.push_back(*it);
                    }
                    break;
                }
                case protozero::tag_and_type(tag::triangles, protozero::pbf_wire_type::length_delimited): {
                    auto pi = message.get_packed_uint32();
                    for (auto it = pi.begin(); it != pi.end(); ++it) {
                        mesh.triangles.push_back(*it);
                    }
                    break;
                }
                default:
                    printf("skipping decode!\n");
                    message.skip();
            }
        }
        return true;
    }
};

class mesh_layer_builder : public vtzero::layer_builder {
public:
    mesh_layer_builder(vtzero::tile_builder &tile, const std::string &name, uint32_t version = 2, uint32_t extent = 4096)
        : layer_builder(tile, name, version, extent) {
    }

    bool set_mesh(const mesh_data &mesh) {

        add_extension(mesh_layer::extension_id, mesh_layer::encode(mesh));

        return true;
    }
};

class mesh_feature_builder : public vtzero::feature_builder {
public:
    explicit mesh_feature_builder(mesh_layer_builder &layer)
        : feature_builder(&layer.get_layer_impl()) {

        m_feature_writer.add_enum(vtzero::detail::pbf_feature::type, static_cast<int32_t>(vtzero::GeomType::UNKNOWN));
    }

    void set_mesh(const std::vector<uint32_t> &triangles) {
        // vtzero_assert(m_feature_writer.valid() &&
        //               "Can not add geometry after commit() or rollback()");
        // vtzero_assert(!m_pbf_geometry.valid() &&
        //               !m_pbf_tags.valid() &&
        //               "add_point() can only be called once");
        m_pbf_geometry = { m_feature_writer, vtzero::detail::pbf_feature::geometry };
        for (auto &t : triangles) {
            m_pbf_geometry.add_element(t);
        }
    }
};
}
