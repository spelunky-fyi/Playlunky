#pragma once

#include <type_traits>

namespace SigScan
{
	template<typename FunT>
	struct Function {
		using type = FunT;

		const char* Signature{ "" };
		FunT Func{ nullptr };

		template<class... Args>
		requires std::is_invocable_v<FunT, Args&&...>
		auto operator()(Args&&... args) {
			return Func(std::forward<Args>(args)...);
		}
	};
};