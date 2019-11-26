#include <iostream>
#include <string>
#include <string_view>

#include "serial_port.hpp"

int main() try
{
	serial_port port {R"(\\.\COM9)", { .baud_rate = 115200 }};

	char buff [8] = { 0 };

	port.push('CNYS');
	port.push('DIFS');
	port.pull(buff);
	
	std::cout << std::string(buff, buff+8);
	return 0;
}
catch(const std::exception& ex)
{
	std::cout << ex.what () << "\n";
	return 0;
}
