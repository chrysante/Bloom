#include "Debug.hpp"

#include <utl/vector.hpp>


namespace bloom {
	
	std::pair<std::unique_lock<std::mutex>, utl::vector<utl::log_message>&> globalLog() {
		static utl::vector<utl::log_message> logs;
		static std::mutex mutex;
		return { std::unique_lock(mutex), logs };
	}
	
	utl::logger& globalLogger() {
		static utl::logger l = [&]{
			utl::logger l("Bloom");
			l.add_listener(utl::make_stdout_listener());
			l.add_listener([](utl::log_message msg){
				auto [lock, log] = globalLog();
				log.push_back(std::move(msg));
			});
			return l;
		}();
		return l;
	}
	
}
