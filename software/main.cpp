#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <fstream>
#include <cstdio>

#include "serial_port.hpp"
#include <protocol.hpp>
#include <crc32.hpp>

struct controller
{	
	controller()
	{}

	~controller()
	{}

	word _read_software_id(serial_port& port) const
	{
		dword resp {0u};
		dword sid {0u}; 
		port.push(SYNC_MAGIC, true);
		port.push(CMD_SOFT_INFO, true);
		port.pull(resp);
		if (resp != CMD_SOFT_INFO)
			throw std::runtime_error("Failed operation");
		port.pull(sid);
		return sid;
	}

	int cmd_erase_chip(serial_port& port)
	{
		dword resp;
		port.push(SYNC_MAGIC, true);
		port.push(CMD_ERASE_CHIP, true);
		port.pull(resp);
		if (resp != CMD_ERASE_CHIP)
			throw std::runtime_error("Failed operation");
		return 0;
	}

	int cmd_erase_sector(byte sector, serial_port& port)
	{
		dword resp;
		port.push(SYNC_MAGIC, true);
		port.push(CMD_ERASE_SECTOR, true);
		port.push(dword(sector));
		port.pull(resp);
		if (resp != CMD_ERASE_SECTOR)
			throw std::runtime_error("Failed operation");
		return 0;
	}

	int cmd_read_software_id(serial_port& port)
	{
		auto sid = _read_software_id(port);
		std::printf("Chip ID : %04x\n", sid);
		return 0;
	}

	int cmd_read(std::string_view fn, dword base, dword last, serial_port& port)
	{
		std::ofstream ofs(fn.data(), std::ios::binary);
		dword crcc, resp;
		for(auto addr = base; addr < last; addr += BUFFER_SIZE)
		{		
			std::printf("Reading sector %05X ... ", addr);
			port.push(SYNC_MAGIC, true);
			port.push(CMD_READ_BYTES, true);
			port.push(addr, true);

			port.pull(resp);
			if (resp != CMD_READ_BYTES) 
				throw std::runtime_error("Failed operation (bad response).");
			byte buff[BUFFER_SIZE];
			port.pull(crcc);
			port.pull(buff);		
			if (crcc != xcrc32(buff, BUFFER_SIZE, 0u))
				throw std::runtime_error("Failed operation (crc check fail).");
			std::printf("[crc: 0x%08X] ok\n", crcc);
			ofs.write((const char*)buff, sizeof(buff));
		}
		return 0;
	}

	int cmd_write(std::string_view fn, dword base, dword last, serial_port& port)
	{
		std::ifstream ifs(fn.data(), std::ios::binary);
		dword crcc, resp;
		for(auto addr = base; addr < last; addr += BUFFER_SIZE)
		{		
			std::printf("Writting sector %05X ... ", addr);

			byte buff[BUFFER_SIZE];
			std::memset(buff, 0xffu, sizeof(buff));
			ifs.read((char*)buff, sizeof(buff));
			crcc = xcrc32(buff, BUFFER_SIZE, 0u);

			port.push(SYNC_MAGIC, true);
			port.push(CMD_WRITE_BYTES, true);
			port.push(addr, true);
			port.push(crcc, true);
			port.push(buff, true);

			port.pull(resp);
			printf ("%.*s", 4, (char*)&resp);
			if (resp != CMD_WRITE_BYTES) 
				throw std::runtime_error("Failed operation (bad response).");
			std::printf("[crc: 0x%08X] ok\n", crcc);			
		}
		return 0;

	}
};

int main(int argc, char** argv) try
{
	using namespace std::string_view_literals;
	using namespace std::string_literals;

	std::vector<std::string_view> args{argv, argv+argc};
	serial_port port {R"(\\.\COM9)", { 
		.baud_rate = 115200,
		.stop_bits = 1,
		.data_bits = 8,
		.parity = serial_port_props::parity_none
	}};
	controller ctrl;	

	int r = -1;
	if (args.size() < 2u || args[1] == "info"sv)
		r = ctrl.cmd_read_software_id(port);
	else if (args[1] == "erase_all"sv)
		r = ctrl.cmd_erase_chip(port);	
	else if (args[1] == "erase_sector"sv)
	{
		if (args.size () >= 3u)
			throw std::runtime_error("Error: too few arguments.\n");
		auto sec = std::stoul(args[2].data(), nullptr, 16u);			
		r = ctrl.cmd_erase_sector((byte)sec, port);
	}
	else if (args[1] == "read"sv || argv[1] == "write"sv)
	{
		auto fn = "all.bin"sv;
		if (args.size() >= 3u)
			fn = argv[2];
		dword a0 = 0x0u;
		dword a1 = 0x80000u;
		if (args.size() >= 4u)
		{
			a0 = std::stoul(args[3].data(), nullptr, 16);
			a1 = a0 + BUFFER_SIZE;
		}
		if (args.size() >= 5u)
			a1 = std::stoul(args[4].data(), nullptr, 16);
		if (a0 > a1)
			std::swap(a0, a1);
		if (args[1] == "read"sv)
			r = ctrl.cmd_read(fn, a0, a1, port);
		else if (args[1] == "write"sv)
			r = ctrl.cmd_write(fn, a0, a1, port);
	}
	return r;
}
catch(const std::exception& ex)
{
	std::cout << ex.what () << "\n";
	return 0;
}
