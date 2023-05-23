#pragma once
#include <type_traits>
#include <functional>
#include <vector>
#include <string>
#include <unordered_map>

class Argument
{
public:
	void* data;
	bool is_const;
	bool is_lvalue;
	const std::type_info& type_id;

	template<typename T>
	Argument(T&& arg)
		:is_const(std::is_const_v<T>),
		is_lvalue(std::is_lvalue_reference_v<T>),
		type_id(typeid(std::decay_t<T>))
	{
		if (is_lvalue)
		{
			data = &arg;
		}
		else
		{
			data = new std::decay_t<T>(arg);
		}
	}

	~Argument()
	{
		if (!is_lvalue && data != nullptr)
		{
			delete data;
		}
	}

	template<typename U>
	U Get() const
	{
		constexpr bool is_U_const = std::is_const_v<U>;
		if ((!is_U_const && is_const) || !(type_id == typeid(std::decay_t<U>)))
		{
			throw std::bad_cast();
		}
		return *(std::decay_t<U>*)data;
	}
};

using ArgumentPack = std::vector<Argument>;

class Delegate
{
public:
	std::function<void(ArgumentPack)> func;

	template <typename F>
	Delegate(F&& f)
	{
		init_func(std::function(f));
	}

	template <typename... Params>
	inline void init_func(std::function<void(Params...)>&& f)
	{
		init_func_impl(std::move(f), std::make_index_sequence<sizeof...(Params)>());
	}

	template <typename... Params, size_t... Index>
	void init_func_impl(std::function<void(Params...)>&& f, std::index_sequence<Index...>)
	{
		func = [f](ArgumentPack args)
		{
			f(args[Index].Get<Params>()...);
		};
	}

	template <typename Class, typename... Params>
	Delegate(Class* obj, void(Class::* mem_func)(Params...))
	{
		init_func_impl(obj, mem_func, std::make_index_sequence<sizeof...(Params)>());
	}

	template <typename Class, typename... Params, size_t... Index>
	void init_func_impl(Class* obj, void(Class::* mem_func)(Params...), std::index_sequence<Index...>)
	{
		func = [obj, mem_func](ArgumentPack args)
		{
			(obj->*mem_func)(args[Index].Get<Params>()...);
		};
	}

	template <typename... Args>
	void invoke(Args&&... args)
	{
		func(ArgumentPack{ args... });
	}
};

using MulticastDelegate = std::vector<Delegate>;

class EventSystem
{
public:
	std::unordered_map<std::string, MulticastDelegate> event_map;

	template <typename F>
	void listen(const std::string& event_key, F&& f)
	{
		event_map[event_key].emplace_back(Delegate(f));
	}
	template <typename Class, typename... Params>
	void listen(const std::string& event_key, Class* obj, void(Class::* mem_func)(Params...))
	{
		event_map[event_key].emplace_back(Delegate(obj, mem_func));
	}

	template <typename... Args>
	void send(const std::string& event_key, Args&&... args)
	{
		auto it = event_map.find(event_key);
		if(it != event_map.end())
		{
			for (auto& d : it->second)
			{
				d.invoke(args...);
			}
		}
	}
};