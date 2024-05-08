#ifndef BLOOM_CORE_YAML_H
#define BLOOM_CORE_YAML_H

#include <filesystem>
#include <type_traits>

#include <magic_enum.hpp>
#include <utl/UUID.hpp>
#include <utl/common.hpp>
#include <vml/vml.hpp>
#include <yaml-cpp/yaml.h>

template <typename T>
struct YAML::convert<vml::complex<T>> {
    static Node encode(vml::complex<T> const& z) {
        return convert<vml::vector<T, 2>>::encode(z);
    }

    static bool decode(Node const& node, vml::complex<T>& z) {
        return convert<vml::vector<T, 2>>::decode(node, z);
    }
};

template <typename T>
struct YAML::convert<vml::quaternion<T>> {
    static Node encode(vml::quaternion<T> const& z) {
        return convert<vml::vector<T, 4>>::encode(z);
    }

    static bool decode(Node const& node, vml::quaternion<T>& z) {
        return convert<vml::vector<T, 4>>::decode(node, z);
    }
};

template <typename T, std::size_t Size, vml::vector_options O>
struct YAML::convert<vml::vector<T, Size, O>> {
    static Node encode(vml::vector<T, Size, O> const& v) {
        Node node;
        for (auto i: v) {
            node.push_back(i);
        }
        return node;
    }

    static bool decode(Node const& node, vml::vector<T, Size, O>& v) {
        if (!node.IsSequence() || node.size() != Size) {
            return false;
        }

        UTL_WITH_INDEX_SEQUENCE((I, Size), { v = { node[I].as<T>()... }; });
        return true;
    }
};

template <typename T, std::size_t Rows, std::size_t Columns,
          vml::vector_options O>
struct YAML::convert<vml::matrix<T, Rows, Columns, O>> {
    static Node encode(vml::matrix<T, Rows, Columns, O> const& m) {
        Node node;
        for (auto i: m) {
            node.push_back(i);
        }
        return node;
    }

    static bool decode(Node const& node, vml::matrix<T, Rows, Columns, O>& m) {
        if (!node.IsSequence() || node.size() != Rows * Columns) {
            return false;
        }

        UTL_WITH_INDEX_SEQUENCE((I, Rows * Columns),
                                { m = { node[I].as<T>()... }; });
        return true;
    }
};

template <>
struct YAML::convert<utl::uuid> {
    static Node encode(utl::uuid const& id) {
        Node node;
        node = id.to_string();
        return node;
    }

    static bool decode(Node const& node, utl::uuid& id) {
        id = utl::uuid::from_string(node.as<std::string>());
        return true;
    }
};

template <>
struct YAML::convert<std::filesystem::path> {
    static Node encode(std::filesystem::path const& path) {
        Node node;
        node = path.string();
        return node;
    }

    static bool decode(Node const& node, std::filesystem::path& path) {
        path = std::filesystem::path(node.as<std::string>());
        return true;
    }
};

template <typename E>
    requires(std::is_enum_v<E>)
struct YAML::convert<E> {
    static Node encode(E e) {
        Node node;
        node = magic_enum::enum_name(e);
        return node;
    }

    static bool decode(Node const& node, E& e) {
        if (auto value = magic_enum::enum_cast<E>(node.as<std::string>())) {
            e = *value;
            return true;
        }
        return false;
    }
};

#endif // BLOOM_CORE_YAML_H
