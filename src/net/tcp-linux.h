#ifndef _TCPSERVER_H_
#define _TCPSERVER_H_

#include <boost/algorithm/string.hpp>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>

#include <netdb.h>
#include <list>

#include "util.hpp"
#include "async.h"
#include "schd/thread-pool.h"
#include "net/epoll.h"
#include "net/fdpoll.h"
#include "buf/buffer.h"
#include "perf/perf-counter.h"
#include "net/transport.h"

namespace dh_core {

//.................................................................................. TCPChannel ....

/**
 * @class TCPChannel
 */
class TCPChannel : public CompletionHandle, public UnicastTransportChannel
{
public:

	friend class TCPConnector;
	friend class TCPServer;

	using This = TCPChannel;

	using UnicastTransportChannel::ReadDoneHandle;
	using UnicastTransportChannel::WriteDoneHandle;
	using UnicastTransportChannel::StopDoneHandle;

	explicit TCPChannel(const std::string & fqn, int fd, FdPoll & epoll);
	virtual ~TCPChannel();

	virtual int Peek(IOBuffer & data, const ReadDoneHandle & h) override;
	virtual int Read(IOBuffer & buf, const ReadDoneHandle & h) override;
	virtual int Write(IOBuffer & buf, const WriteDoneHandle & h) override;
	virtual int Stop(const StopDoneHandle & cb) override;

    private:

	__DISABLE_ASSIGN_AND_COPY__(TCPChannel)
	__STATELESS_ASYNC_PROCESSOR__

	/**
	 * represent read operation context
	 */
	struct ReadCtx
	{
		ReadCtx() : bytesRead_(0) {}

		ReadCtx(const IOBuffer & buf, const ReadDoneHandle & h, const bool isPeek)
			: buf_(buf)
			, bytesRead_(0)
			, h_(h)
			, isPeek_(isPeek)
		{}

		void Reset()
		{
			buf_.Reset();
			bytesRead_ = 0;
			h_ = NULL;
		}

		IOBuffer buf_;
		uint32_t bytesRead_;
		ReadDoneHandle h_;
		bool isPeek_;
	};

	/**
	 * Represent write operation
	 */
	struct WriteCtx
	{
		WriteCtx() {}

		WriteCtx(const IOBuffer & buf, const WriteDoneHandle & h)
		    : buf_(buf), h_(h)
		{
			ASSERT(buf);
		}

		IOBuffer buf_;
		WriteDoneHandle h_;
	};

	int Read(const IOBuffer & buf, const ReadDoneHandle & h, const bool peek);
	void HandleFdEvent(int fd, uint32_t events) __intr_fn__;
	int ReadDataFromSocket(const bool isasync);
	int WriteDataToSocket(const bool isasync);
	void BarrierDone(int);
	void FailOps();
	void Close();


	const std::string fqn_;
	SpinMutex lock_;
	int fd_;
	FdPoll & epoll_;
	std::list<WriteCtx> wpending_;
	ReadCtx rpending_;
	StopDoneHandle stoph_;

	PerfCounter statReadSize_;
	PerfCounter statWriteSize_;
};

//................................................................................... TCPServer ....

/**
 * @class TCPServer
 *
 * Asynchronous TCP server implementation
 *
 * Provides a TCP listener implementation. Helps accept connections
 * from clients asynchronously. Designed on the acceptor design pattern.
 */
class TCPServer : public CompletionHandle, public UnicastAcceptor
{
public:

	using This = TCPServer;

	using UnicastAcceptor::AcceptDoneHandle;
	using UnicastAcceptor::StopDoneHandle;

	TCPServer(FdPoll & epoll)
		: fqn_(fqn()), lock_(fqn_), epoll_(epoll)
	{}

	virtual ~TCPServer() {}

	/*
	 * Accept --> epoll.Add(fd, event) --> kernel
	 * kernel --> epoll --> HandleFdEvent *--> AcceptDoneHandle
	 * Stop *--> BarrierDone *--> StopDoneHandle 
	 */

	virtual int Accept(const SocketAddress & addr, const AcceptDoneHandle & h) override;
	virtual int Stop(const StopDoneHandle & h) override;

    private:

	static const size_t MAXBACKLOG = 1024;

	typedef int socket_t;

	void HandleFdEvent(int fd, uint32_t events) __intr_fn__;
	void BarrierDone(StopDoneHandle h);

	const std::string fqn() const { return "/tcpserver/" + STR(this); }
	const std::string fqn(int fd) const { return fqn() + "/ch/" + STR(fd); }

	const std::string fqn_;
	SpinMutex lock_;
	FdPoll & epoll_;
	socket_t sockfd_;
	AcceptDoneHandle h_;
};

//................................................................................ TCPConnector ....

/**
 * @class TCPConnector
 *
 * TCP connection provider
 *
 * Helps establish TCP connections asynchronously. This follows the connector design pattern.
 */
class TCPConnector : public CompletionHandle, public UnicastConnector
{
public:

	using This = TCPConnector;

	using UnicastConnector::ConnectDoneHandle;
	using UnicastConnector::StopDoneHandle;

	TCPConnector(FdPoll & epoll, const std::string & fqn = "/tcp/connector")
	    : fqn_(fqn), lock_(fqn), epoll_(epoll)
	{}

	virtual ~TCPConnector()
	{
	    INVARIANT(pendingConnects_.empty());
	}

	/*
	 * Connect() --> epoll.Add(fd, event) --> kernel
	 * kernel --> epoll --> HandleFdEvent(fd, event) *--> ConnectDoneHandle
	 * Stop *--> BarrierDone *--> ConnectDoneHandle (if pending)
	 */

	virtual int Connect(const SocketAddress & addr, const ConnectDoneHandle & h) override;
	virtual int Stop(const StopDoneHandle & h) override;

    private:

	__DISABLE_ASSIGN_AND_COPY__(TCPConnector);

	typedef std::map<fd_t, ConnectDoneHandle> pending_map_t;

	void HandleFdEvent(int fd, uint32_t events) __intr_fn__;
	void BarrierDone(StopDoneHandle h);

	const std::string fqn_;
	SpinMutex lock_;
	FdPoll & epoll_;
	pending_map_t pendingConnects_;
};

} // namespace kware {

#endif
