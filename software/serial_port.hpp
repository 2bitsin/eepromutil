#pragma once

#include <string_view>
#include <memory>

#include <types.hpp>

struct serial_port_props
{
	enum parity_type
	{
		parity_none,
		parity_mark,
		parity_space,
		parity_even,
		parity_odd
	};

	dword				baud_rate {115200};
	byte				stop_bits {1};
	byte				data_bits {8};
	parity_type parity		{parity_type::parity_none};
};

struct serial_port
{
	serial_port (std::string_view name, const serial_port_props& props = {});
	~serial_port ();

	void set_props(const serial_port_props& props);

	std::size_t pull (      void* buf, std::size_t len, bool wait = true);
	std::size_t push (const void* buf, std::size_t len, bool wait = true);

	template <typename _Ttype, std::enable_if_t<std::is_pod_v<_Ttype>, int> = 0>
	std::size_t push (const _Ttype* buf, std::size_t len, bool wait = false)
	{
		return push((const void*)buf, len*sizeof(_Ttype), wait)/sizeof(_Ttype);
	}

	template <typename _Ttype, std::enable_if_t<std::is_pod_v<_Ttype>, int> = 0>
	std::size_t pull (_Ttype* buf, std::size_t len, bool wait = false)
	{
		return pull((			 void*)buf, len*sizeof(_Ttype), wait)/sizeof(_Ttype);
	}

	template <typename _Ttype, std::enable_if_t<std::is_pod_v<_Ttype>, int> = 0>
	std::size_t push (const _Ttype& value, bool wait = false)
	{
		return push (&value, 1u, wait);
	}

	template <typename _Ttype, std::enable_if_t<std::is_pod_v<_Ttype>, int> = 0>
	std::size_t pull (_Ttype& value, bool wait = false)
	{
		return pull (&value, 1u, wait);
	}

	template <typename _Ttype, std::size_t _Nsize, std::enable_if_t<std::is_pod_v<_Ttype>, int> = 0>
	std::size_t push (const _Ttype (&value) [_Nsize], bool wait = false)
	{
		return push (value, _Nsize, wait);
	}

	template <typename _Ttype, std::size_t _Nsize, std::enable_if_t<std::is_pod_v<_Ttype>, int> = 0>
	std::size_t pull (_Ttype (&value) [_Nsize], bool wait = false)
	{
		return pull (value, _Nsize, wait);
	}

private:
	std::unique_ptr<struct serial_port_impl> _impl;
};



