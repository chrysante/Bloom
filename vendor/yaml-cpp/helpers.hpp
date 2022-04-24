#pragma once

#include "yaml.h"
#include <mtl/mtl.hpp>
#include <utl/common.hpp>
#include <utl/UUID.hpp>

namespace YAML {
	
	template <typename T, std::size_t Size, mtl::vector_options O>
	YAML::Emitter& operator<<(YAML::Emitter& out, mtl::vector<T, Size, O> const& v) {
		out << YAML::Flow << YAML::BeginSeq;
		for (auto i: v) {
			out << i;
		}
		return out << YAML::EndSeq;
	}

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

	template <>
	struct YAML::convert<utl::UUID> {
		static Node encode(utl::UUID const& id) {
			Node node;
			node = id.to_string();
			return node;
		}
		 
		static bool decode(Node const& node, utl::UUID& id) {
			id = utl::UUID::from_string(node.as<std::string>());
			return true;
		}
	};
	
}

