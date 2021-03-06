#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

#include <string>
#include <boost/algorithm/string.hpp>

namespace bblocks {

//............................................................................... SocketOptions ....

/**
 * @class SocketOptions
 *
 * Abstraction to manipulate socket options.
 *
 */
class SocketOptions
{
public:

	static bool SetTcpNoDelay(const int fd, const bool enable)
	{
		const int flag = enable;
		int status = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
		return status != -1;
	}

	static bool GetTcpNoDelay(const int fd)
	{
		int flag = 0;
		socklen_t len = sizeof(char);
		int status = getsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, &len);
		INVARIANT(status != -1);
		return flag;
	}

	static bool SetTcpWindow(const int fd, const int size)
	{
		// Set out buffer size
		int status = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
		if (status == -1) return false;

		// Set in buffer size
		status = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
		return status != -1;
	}

	static int GetTcpRcvBuffer(const int fd)
	{
		int size = 0;
		socklen_t len = sizeof(int);
		int status = getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &size, &len);
		INVARIANT(status != -1);
		return size;
	}

	static int GetTcpSendBuffer(const int fd)
	{
		int size = 0;
		socklen_t len = sizeof(int);
		int status = getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, &len);
		INVARIANT(status != -1);
		return size;
	}
};

// .............................................................................. SocketAddress ....

/**
 * @class SocketAddress
 *
 * Socket address abstraction.
 *
 */
class SocketAddress
{
public:

	/*.... Static Function ....*/

	/**
	 * Create socket for accepting connection
	 */
	 static SocketAddress ServerSocketAddr(const string & hostname, const short port)
	 {
		SocketAddress addr;
		addr.laddr_ =  GetAddr(hostname, port);
		return addr;
	 }

	 static SocketAddress ServerSocketAddr(const sockaddr_in & laddr)
	 {
		SocketAddress addr;
		addr.laddr_ =  laddr;
		return addr;
	 }


	/**
	 * Convert hostname:port to sockaddr_in
	 */
	static sockaddr_in GetAddr(const string & hostname, const short port)
	{
		sockaddr_in addr;

		addrinfo hints;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = 0;
		hints.ai_protocol = 0;

		addrinfo * result = NULL;
		int status = getaddrinfo(hostname.c_str(), NULL, &hints, &result);
		INVARIANT(status != -1);
		ASSERT(result);
		ASSERT(result->ai_addrlen == sizeof(sockaddr_in));

		memset(&addr, 0, sizeof(sockaddr_in)); 
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = ((sockaddr_in *)result->ai_addr)->sin_addr.s_addr;

		freeaddrinfo(result);

		return addr;
	}

	/**
	 * Convert IP:port to sockaddr_in
	 */
	static sockaddr_in GetAddr(const in_addr_t & addr, const short port)
	{
		sockaddr_in sockaddr;
		memset(&sockaddr, /*ch=*/ 0, sizeof(sockaddr_in));
		sockaddr.sin_family = AF_INET;
		sockaddr.sin_port = htons(port);
		sockaddr.sin_addr.s_addr = addr;

		return sockaddr;
	}

	/**
	 * Convert hostname:port string to sockaddr_in
	 */
	static sockaddr_in GetAddr(const string & saddr)
	{
		vector<string> tokens;
		boost::split(tokens, saddr, boost::is_any_of(":"));

		ASSERT(tokens.size() == 2);

		const string host = tokens[0];
		const short port = atoi(tokens[1].c_str());

		return GetAddr(host, port);
	}

	/**
	 * Construct a connection (local binding-remote binding) form laddr and
	 * raddr strings of format host:port
	 */
	static SocketAddress GetAddr(const string & laddr,
				     const string & raddr)
	{
		return SocketAddress(GetAddr(laddr), GetAddr(raddr));
	}

	/*.... ctor/dtor ....*/

	explicit SocketAddress(const sockaddr_in & raddr)
	    : laddr_(GetAddr(INADDR_ANY, /*port=*/ 0)), raddr_(raddr)
	{}

	explicit SocketAddress(const sockaddr_in & laddr, const sockaddr_in & raddr)
	    : laddr_(laddr), raddr_(raddr)
	{}

	/*.... get/set ....*/

	/**
	 * Get local binding socket address
	 */
	const sockaddr_in & LocalAddr() const { return laddr_; }

	/**
	 * Get remote socket address
	 */
	const sockaddr_in & RemoteAddr() const { return raddr_; }

private:

	SocketAddress() {}

	struct sockaddr_in laddr_;
	struct sockaddr_in raddr_;
};

}
