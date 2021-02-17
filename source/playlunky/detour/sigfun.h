#pragma once

#include <string_view>
#include <type_traits>

[[nodiscard]] inline constexpr std::string_view operator"" _sig(const char* _Str, size_t _Len) noexcept {
	return std::string_view(_Str, _Len);
}

namespace SigScan
{
	template<typename FunT>
	struct Function {
		using type = FunT;

		std::string_view Signature{ "" };
		const char* ProcName{ nullptr };
		const char* Module{ "Spel2.exe" };
		FunT Func{ nullptr };

		template<class... Args>
		requires std::is_invocable_v<FunT, Args&&...>
		auto operator()(Args&&... args) {
			return Func(std::forward<Args>(args)...);
		}
	};
};