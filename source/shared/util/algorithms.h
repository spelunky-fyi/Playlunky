#pragma once

#include <type_traits>
#include <utility>

namespace algo {
	template <class T, class U, typename = void>
	struct is_comparable : std::false_type {};
	template <class T, class U>
	struct is_comparable<T, U, decltype((std::declval<T>() == std::declval<U>()), void())>
		: std::true_type {};
	template<class T, class U>
	inline constexpr auto is_comparable_v = is_comparable<T, U>::value;

	template<class T>
	concept range = requires (T&& cont){
		begin(cont);
		end(cont);
	};

	template<class ContainerT, class ValueT>
	requires range<ContainerT> && is_comparable_v<decltype(*begin(std::declval<ContainerT>())), ValueT>
	bool contains(ContainerT&& container, ValueT&& value) {
		const auto begin_it = begin(container);
		const auto end_it = end(container);
		return std::find(begin_it, end_it, std::forward<ValueT>(value)) != end_it;
	}

	template<class ContainerT, class FunT>
	requires range<ContainerT> && std::is_invocable_v<FunT, decltype(*begin(std::declval<ContainerT>()))>
	bool contains_if(ContainerT&& container, FunT&& value) {
		const auto begin_it = begin(container);
		const auto end_it = end(container);
		return std::find_if(begin_it, end_it, std::forward<FunT>(value)) != end_it;
	}

	template<class ContainerT, class FunT>
	requires range<ContainerT> && std::is_invocable_v<FunT, decltype(*begin(std::declval<ContainerT>()))>
	auto find_if(ContainerT&& container, FunT&& fun) -> decltype(&*begin(std::declval<ContainerT>())) {
		const auto begin_it = begin(container);
		const auto end_it = end(container);
		auto found_it = std::find_if(begin_it, end_it, std::forward<FunT>(fun));
		if (found_it != end_it) {
			return &*found_it;
		}
		return nullptr;
	}

	template<class ContainerT, class FunT>
	requires range<ContainerT> && std::is_invocable_v<FunT, decltype(*begin(std::declval<ContainerT>()))>
	auto count_if(ContainerT&& container, FunT&& fun) {
		const auto begin_it = begin(container);
		const auto end_it = end(container);
		return std::count_if(begin_it, end_it, std::forward<FunT>(fun));
	}

	inline bool is_sub_path(const std::filesystem::path& path, const std::filesystem::path& base) {
		const auto first_mismatch = std::mismatch(path.begin(), path.end(), base.begin(), base.end());
		return first_mismatch.second == base.end();
	}
}
