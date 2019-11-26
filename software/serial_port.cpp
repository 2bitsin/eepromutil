#include <stdexcept>
#include "serial_port.hpp"

#include <Windows.h>

auto format_last_error ()
-> std::string
{
	auto lem = GetLastError();
	const char* lem_buff {nullptr};
	FormatMessageA (
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		lem,
		MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lem_buff,
		0, NULL);
	std::string lem_sstr = lem_buff;
	LocalFree((HLOCAL)lem_buff);
	return lem_sstr;
}

struct serial_port_impl
{
	serial_port_impl (std::string_view name)
	: _handle (_port_open (name))
	{}

	~serial_port_impl ()
	{}

	void set_props(const serial_port_props& props)
	{
		DCB dcb = {0};
		dcb.DCBlength = sizeof(dcb);

		if (!GetCommState(_handle, &dcb))
			throw std::runtime_error(format_last_error());

		dcb.BaudRate = props.baud_rate;
		dcb.StopBits = props.stop_bits;
		dcb.Parity = _map_parity(props);
		dcb.ByteSize = props.data_bits;

		if (!SetCommState(_handle, &dcb))
			throw std::runtime_error(format_last_error());
	}

	std::size_t push(const void* buf, std::size_t len)
	{
		DWORD olen = 0u;
		if (!WriteFile(_handle, buf, (DWORD)len, &olen, nullptr))
			throw std::runtime_error(format_last_error());
		return olen;
	}
	std::size_t pull(void* buf, std::size_t len)
	{
		DWORD olen = 0u;
		if (!ReadFile(_handle, buf, (DWORD)len, &olen, nullptr))
			throw std::runtime_error(format_last_error());
		return olen;
	}

	static auto _map_parity (const serial_port_props& props)
		-> int
	{
		switch(props.parity)
		{
		case serial_port_props::parity_none:
			return NOPARITY;
		case serial_port_props::parity_even:
			return EVENPARITY;
		case serial_port_props::parity_odd:
			return ODDPARITY;
		case serial_port_props::parity_mark:
			return MARKPARITY;
		case serial_port_props::parity_space:
			return SPACEPARITY;
		}
		return NOPARITY;
	}

	static auto _port_open (std::string_view name)
		-> HANDLE
	{
		auto _handle = CreateFileA (
			name.data (),
			GENERIC_READ | GENERIC_WRITE,
			0,
			nullptr,
			OPEN_EXISTING,
			0,
			nullptr);
		if (!_handle)
			throw std::runtime_error (format_last_error ());
		return _handle;
	}

	HANDLE _handle{nullptr};
};

serial_port::serial_port (std::string_view name, const serial_port_props& props)
: _impl{std::make_unique<serial_port_impl> (name)}
{
	_impl->set_props(props);
}

serial_port::~serial_port ()
{}

std::size_t serial_port::push (const void* buf, std::size_t len)
{
	return _impl->push(buf, len);
}

std::size_t serial_port::pull (void* buf, std::size_t len)
{
	return _impl->pull(buf, len);
}
