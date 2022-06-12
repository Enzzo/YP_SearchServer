#pragma once

#include <chrono>
#include <string>
#include <iostream>

#define PROFILE_CONCAT_INTERNAL(X, Y) X ## Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION(x) LogDuration UNIQUE_VAR_NAME_PROFILE(x)
#define LOG_DURATION_STREAM(x, y) LogDuration UNIQUE_VAR_NAME_PROFILE(x, y)

using namespace std::literals;

class LogDuration {
	using Clock = std::chrono::steady_clock;

	Clock::time_point begin_time_ = Clock::now();
	const std::string operation_;
	std::ostream& out_;

public:	

	LogDuration() : LogDuration(""s){};
	
	LogDuration(const std::string& operation) : LogDuration(operation, std::cout) {};

	LogDuration(const std::string& operation, std::ostream& out) : operation_(operation + ": "s), out_(out) {};
	
	~LogDuration() {
		out_ << this->Duration() << std::endl;
	}

	inline const std::string Duration()const;
};

inline const std::string LogDuration::Duration()const {
	Clock::time_point end_time = Clock::now();
	Clock::duration dur = end_time - begin_time_;

	return operation_ + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(dur).count()) + " ms"s;
}