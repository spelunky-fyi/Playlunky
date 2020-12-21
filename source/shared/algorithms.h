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
	requires range<ContainerT> && is_comparable_v<decltype(*std::begin(std::declval<ContainerT>())), ValueT>
	bool contains(ContainerT&& container, ValueT&& value) {
		const auto begin_it = begin(container);
		const auto end_it = end(container);
		return std::find(begin_it, end_it, std::forward<ValueT>(value)) != end_it;
	}
}
