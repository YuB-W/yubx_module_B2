#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <queue>

class CCommunication {
public:
	void Initialize();
};

void Init_main();

inline auto Communication = std::make_unique<CCommunication>();