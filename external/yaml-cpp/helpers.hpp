#pragma once

#include "yaml.h"
#include <mtl/mtl.hpp>
#include <utl/common.hpp>
#include <utl/UUID.hpp>
#include <filesystem>
#include <type_traits>

namespace YAML {
	
	/// MARK: mtl::complex
	template <typename T>
	struct YAML::convert<mtl::complex<T>> {
		static Node encode(mtl::complex<T> const& z) {
			return convert<mtl::vector<T, 2>>::encode(z);
		}
		 
		static bool decode(Node const& node, mtl::complex<T>& z) {
			return convert<mtl::vector<T, 2>>::decode(node, z);
		}
	};
	
	/// MARK: mtl::quaternion
	template <typename T>
	struct YAML::convert<mtl::quaternion<T>> {
		static Node encode(mtl::quaternion<T> const& z) {
			return convert<mtl::vector<T, 4>>::encode(z);
		}
		 
		static bool decode(Node const& node, mtl::quaternion<T>& z) {
			return convert<mtl::vector<T, 4>>::decode(node, z);
		}
	};
	
	/// MARK: mtl::vector
	template <typename T, std::size_t Size, mtl::vector_options O>
	struct YAML::convert<mtl::vector<T, Size, O>> {
		static Node encode(mtl::vector<T, Size, O> const& v) {
			Node node;
			for (auto i: v) {
				node.push_back(i);
			}
			return node;
		}
		 
		static bool decode(Node const& node, mtl::vector<T, Size, O>& v) {
			if (!node.IsSequence() || node.size() != Size) { return false; }
			 
			UTL_WITH_INDEX_SEQUENCE((I, Size), {
				v = { node[I].as<T>()... };
			});
			return true;
		}
	};
	
	/// MARK: mtl::matrix
	template <typename T, std::size_t Rows, std::size_t Columns, mtl::vector_options O>
	struct YAML::convert<mtl::matrix<T, Rows, Columns, O>> {
		static Node encode(mtl::matrix<T, Rows, Columns, O> const& m) {
			Node node;
			for (auto i: m) {
				node.push_back(i);
			}
			return node;
		}
		 
		static bool decode(Node const& node, mtl::matrix<T, Rows, Columns, O>& m) {
			if (!node.IsSequence() || node.size() != Rows * Columns) { return false; }
			 
			UTL_WITH_INDEX_SEQUENCE((I, Rows * Columns), {
				m = { node[I].as<T>()... };
			});
			return true;
		}
	};

	/// MARK: utl::uuid
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
	
	/// MARK: std::filesystem::path
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
	
	/// MARK: Enums
	template <typename E> requires (std::is_enum_v<E>)
	struct YAML::convert<E> {
		static Node encode(E const& e) {
			Node node;
			node = utl::to_underlying(e);
			return node;
		}
		 
		static bool decode(Node const& node, E& e) {
			e = (E)node.as<std::underlying_type_t<E>>();
			return true;
		}
	};
	
}

