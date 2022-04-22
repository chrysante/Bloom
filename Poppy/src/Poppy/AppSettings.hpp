#pragma once

#include <string>
#include <utl/hashmap.hpp>
#include <filesystem>
#include <optional>

namespace poppy {
	
	class AppSettings {
		friend class AppSettingsMaster;
		
	public:
		std::optional<std::int64_t> getInt(std::string key) const;
		bool setInt(std::string key, std::int64_t value);
		
		std::optional<double> getFloat(std::string key) const;
		bool setFloat(std::string key, double value);
		
		std::optional<std::string> getString(std::string key) const;
		bool setString(std::string key, std::string value);
		
	private:
		template <typename T>
		static std::optional<T> getImpl(auto&& map, std::string key);
		template <typename T>
		static bool setImpl(auto&& map, std::string key, T value);
		
		static void validate(std::string&);
		
	private:
		utl::hashmap<std::string, std::int64_t> ints;
		utl::hashmap<std::string, double> floats;
		utl::hashmap<std::string, std::string> strings;
	};
	
	
	
	class AppSettingsMaster: public AppSettings {
	public:
		void setFilepath(std::filesystem::path);
		void initFromFile(std::filesystem::path);
		void init();
		void saveToDisk();
		
	private:
		void serializeImpl(std::string_view name, auto&& map, auto& emitter);
		template <typename T>
		void deserializeImpl(auto& map, auto&& node);
		
	private:
		std::filesystem::path file;
	};
	
}
