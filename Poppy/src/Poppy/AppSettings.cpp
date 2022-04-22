#include "AppSettings.hpp"

#include <yaml-cpp/yaml.h>
#include <fstream>
#include <utl/stdio.hpp>

namespace poppy {
	
	/// MARK: AppSettings
	std::optional<std::int64_t> AppSettings::getInt(std::string key) const {
		return getImpl<std::int64_t>(ints, std::move(key));
	}
	
	bool AppSettings::setInt(std::string key, std::int64_t value) {
		return setImpl(ints, std::move(key), value);
	}
	
	std::optional<double> AppSettings::getFloat(std::string key) const {
		return getImpl<double>(floats, std::move(key));
	}
	
	bool AppSettings::setFloat(std::string key, double value) {
		return setImpl(floats, std::move(key), value);
	}
	
	std::optional<std::string> AppSettings::getString(std::string key) const {
		return getImpl<std::string>(strings, std::move(key));
	}
	
	bool AppSettings::setString(std::string key, std::string value) {
		return setImpl(strings, std::move(key), std::move(value));
	}
	
	template <typename T>
	std::optional<T> AppSettings::getImpl(auto&& map, std::string key) {
		validate(key);
		auto const itr = map.find(key);
		if (itr != map.end()) {
			return itr->second;
		}
		return std::nullopt;
	}
	
	template <typename T>
	bool AppSettings::setImpl(auto&& map, std::string key, T value) {
		validate(key);
		if (map.contains(key)) {
			return false;
		}
		map.insert({ key, value });
		return true;
	}
	
	void AppSettings::validate(std::string& s) {
		std::replace(s.begin(), s.end(), '#', '*');
	}
	
	/// MARK: AppSettingsMaster
	void AppSettingsMaster::setFilepath(std::filesystem::path file) {
		this->file = file;
	}
	
	void AppSettingsMaster::initFromFile(std::filesystem::path file) {
		setFilepath(file);
		init();
	}
	
	void AppSettingsMaster::init() {
		if (this->file.empty()) {
			return;
		}
		
		std::fstream file(this->file, std::ios::in);
		if (!file) {
			return;
		}
		
		std::stringstream sstr;
		sstr << file.rdbuf();
		
		std::string contents = sstr.str();
		
		YAML::Node node = YAML::Load(contents);

		deserializeImpl<std::string>(strings, node["Strings"]);
		deserializeImpl<std::int64_t>(ints, node["Ints"]);
		deserializeImpl<double>(floats, node["Floats"]);
		
	}
	
	void AppSettingsMaster::saveToDisk() {
		if (file.empty()) {
			return;
		}
		
		YAML::Emitter out;
		
		out << YAML::BeginMap;
		serializeImpl("Ints", ints, out);
		serializeImpl("Floats", floats, out);
		serializeImpl("Strings", strings, out);
		out << YAML::EndMap;
		
		std::fstream file(this->file, std::ios::out | std::ios::trunc);
		assert(file);
		file << out.c_str();
	}
	
	void AppSettingsMaster::serializeImpl(std::string_view name, auto&& map, auto& out) {
		out << YAML::Key << name.data() << YAML::Value << YAML::BeginMap;
		for (auto&& [key, value]: map) {
			out << YAML::Key << key << YAML::Value << value;
		}
		out << YAML::EndMap;
	}
	
	template <typename T>
	void AppSettingsMaster::deserializeImpl(auto& map, auto&& node) {
		for (auto&& element: node) {
			auto const key = element.first.template as<std::string>();
			auto const value = element.second.template as<T>();
			map.insert({ key, value });
		}
	}
	
	
	
}
