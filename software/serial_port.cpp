#include <stdexcept>
#include "serial_port.hpp"

#include <Windows.h>

inline auto format_last_error ()
-> std::string
{
	auto lem = GetLastError ();
	const char* lem_buff{nullptr};
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
	LocalFree ((HLOCAL)lem_buff);
	return lem_sstr;
}

inline void throw_last_error ()
{
	throw std::runtime_error (format_last_error ());
}

struct serial_port_impl
{
	serial_port_impl (std::string_view name)
	: _handle (_port_open (name))
	{}

	~serial_port_impl ()
	{
		if (_handle != nullptr)
			CloseHandle (_handle);
	}

	void set_props (const serial_port_props& props)
	{
		DCB dcb = {0};
		dcb.DCBlength = sizeof (dcb);

		if (!GetCommState (_handle, &dcb))
			throw_last_error ();

		dcb.BaudRate = props.baud_rate;
		dcb.StopBits = props.stop_bits;
		dcb.Parity = _map_parity (props);
		dcb.ByteSize = props.data_bits;

		if (!SetCommState (_handle, &dcb))
			throw_last_error ();
	}

	std::size_t push (const void* buf, std::size_t len, bool wait)
	{
		DWORD olen = 0u;
		bool status = true;
		if (wait)
		{
			status = status && SetCommMask(_handle, EV_TXEMPTY);
		}
		status = status && WriteFile (_handle, buf, (DWORD)len, &olen, nullptr);
		if (wait)
		{
			DWORD ev {0};
			status = status && WaitCommEvent(_handle, &ev, nullptr);
			status = status && SetCommMask(_handle, 0);
		}
		if (!status)
			throw_last_error();
		return olen;
	}

	std::size_t pull (void* buf, std::size_t len, bool wait)
	{
		DWORD olen = 0u;
		bool status = true;
		if (wait)
		{
			DWORD ev {0};
			status = status && SetCommMask(_handle, EV_RXCHAR);
			status = status && WaitCommEvent(_handle, &ev, nullptr);
		}
		status = status && ReadFile (_handle, buf, (DWORD)len, &olen, nullptr);
		if (wait)
		{
			status = status && SetCommMask(_handle, 0);
		}
		if (!status)
				throw_last_error();
		return olen;
	}

	static auto _map_parity (const serial_port_props& props)
		-> int
	{
		switch (props.parity)
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
			throw_last_error ();
		return _handle;
	}

	HANDLE	_handle{nullptr};
	DWORD		_evmask{0};
};

serial_port::serial_port (std::string_view name, const serial_port_props& props)
: _impl{std::make_unique<serial_port_impl> (name)}
{
	_impl->set_props (props);
}

serial_port::~serial_port ()
{}

void serial_port::set_props (const serial_port_props& props)
{
	_impl->set_props (props);
}

std::size_t serial_port::push (const void* buf, std::size_t len, bool wait)
{
	return _impl->push (buf, len, wait);
}

std::size_t serial_port::pull (void* buf, std::size_t len, bool wait)
{
	return _impl->pull (buf, len, wait);
}
