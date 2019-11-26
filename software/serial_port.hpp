#pragma once

#include <string_view>
#include <memory>

#include "types.hpp"

struct serial_port_props
{
	dword baud_rate { 115200 };
	byte	stop_bits { 1 };
	byte	data_bits { 8 };
	enum { 
		parity_none, 
		parity_mark, 
		parity_space, 
		parity_even, 
		parity_odd 
	} parity { parity_none } ;

};

struct serial_port
{
	serial_port (std::string_view name, const serial_port_props& props = {});
	~serial_port ();

	std::size_t push(const void* buf, std::size_t len);
	std::size_t pull(void* buf, std::size_t len);

	template <typename _Ttype, std::enable_if_t<std::is_pod_v<_Ttype>, int> = 0>
	bool push(const _Ttype& value)
	{
		return push(&value, sizeof(value)) == sizeof(value);
	}

	template <typename _Ttype, std::enable_if_t<std::is_pod_v<_Ttype>, int> = 0>
	bool pull(_Ttype& value)
	{
		return pull(&value, sizeof(value)) == sizeof(value);
	}

	template <typename _Ttype, std::size_t _Nsize, std::enable_if_t<std::is_pod_v<_Ttype>, int> = 0>
	bool push(const _Ttype (&value) [_Nsize])
	{
		return push(&value, sizeof(value)) == sizeof(value);
	}

	template <typename _Ttype, std::size_t _Nsize, std::enable_if_t<std::is_pod_v<_Ttype>, int> = 0>
	bool pull(_Ttype (&value) [_Nsize])
	{
		return pull(&value, sizeof(value)) == sizeof(value);
	}

private:
	std::unique_ptr<struct serial_port_impl> _impl;
};



