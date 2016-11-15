#include <tinysocket.h>
#include <iostream>
#include <sstream>
#include <cstring>

int main()
{
	try 
	{
		ts::socket server(ts::address_famaly::internet_network, ts::socket_type::stream, ts::protocol_type::tcp);
		server.bind(ts::ip_socket_address(ts::ip_address_any, 8080));
		server.listen(0);
	
		
	
		while(true)
		{
			ts::socket client = server.accept();
			
			std::stringstream res;
		
			res << 
			"<!DOCTYPE html>"
			"<html>"
			"<body>"
			"<h1>You ip address</h1>"
			"<p>" << client.remote_endpoint().get_address() << ", client port: " << client.remote_endpoint().get_port() << "</p>"
			"</body>"
			"</html>";
			
			
				
			
			std::stringstream header;
			header << "HTTP/1.0 200 Document follows\r\n"
			"Server: tiny socket web server\r\n"
			"Content-Length: " <<  res.str().size() << "\r\n\r\n";
			
			
			
			client.send(header.str().c_str(), header.str().size());
			client.send(res.str().c_str(), res.str().size());
		}
	
	
	}
	catch (std::exception& ex) 
	{
		std::cout << ex.what() << std::endl;
		std::cin.get();
	}
	

    return 0;
}

