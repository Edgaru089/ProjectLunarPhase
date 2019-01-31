
#include "Instance.hpp"
#include "HTTPParser.hpp"


namespace {

	class DisconnectedException :public exception {
		char const* what() const noexcept override { return "Socket disconnected when waiting for contents"; }
	};

	unsigned char getchar(shared_ptr<TcpSocket> socket) {
		unsigned char c = 0;
		size_t i = 0;

		Socket::Status stat;
		while ((stat = socket->receive(&c, 1, i)) != Socket::Disconnected&&stat != Socket::Error && i <= 0) {
			if (stat == Socket::NotReady || stat == Socket::Partial)
				sleep(milliseconds(2));
			i = 0;
		}
		if (stat == Socket::Disconnected || stat == Socket::Error)
			throw DisconnectedException();

		return c;
	}

}


void Instance::start(Instance::Config&& conf) {
	mlog << "Server Starting..." << dlog;

	port = conf.port;

	mlog << "HTTP Port: " << port << ", HTTPS Turned Off" << dlog;
	mlog << "IPv4 Listening: " << (true ? "On" : "Off") << ", IPv6 Listening: " << (false ? "On" : "Off") << '\n' << dlog;

	running = true;

	httpListener = make_shared<thread>([this]() {
		auto listener = make_shared<TcpListener>();
		listener->listen(port);
		_HTTPListener(listener);
	});

	maintainer = make_shared<thread>(&Instance::_maintainer, this);
}


void Instance::stop() {
	mlog << "Stopping..." << dlog;
	running = false;
	for (auto&[connection, handler, connected] : sockets) {
		connection->disconnect();
		if (handler->joinable())
			handler->join();
	}

	if (httpListener && httpListener->joinable())
		httpListener->join();
	if (maintainer&&maintainer->joinable())
		maintainer->join();

	sockets.clear();
	httpListener = nullptr;
	mlog << "Server Stopped" << dlog;
}


void Instance::_maintainer() {
	while (running) {
		this_thread::sleep_for(chrono::milliseconds(300));
		queueLock.lock();
		for (auto i = sockets.begin(); i != sockets.end();)
			if (!(*get<2>(*i))) {
				if (get<1>(*i)->joinable())
					get<1>(*i)->join();
				i = sockets.erase(i);
			} else
				i++;
			queueLock.unlock();
	}
}


void Instance::_HTTPListener(shared_ptr<TcpListener> listener) {
	shared_ptr<TcpSocket> socket = nullptr;
	Socket::Status stat;
	listener->setBlocking(false);
	while (running) {
		if (!socket)
			socket = make_shared<TcpSocket>();
		if ((stat = listener->accept(*socket)) == Socket::Done) {
			// Create a handler thread and store the connection.
			queueLock.lock();
			auto connflag = make_shared<atomic_bool>(true);
			sockets.emplace_back(socket,
								 make_shared<thread>(&Instance::_connectionHandler, this, socket, connflag),
								 connflag);
			mloge << "HTTP Connected: [" << socket->getRemoteAddress().toString() << "]:" << socket->getRemotePort() << dlog;
			queueLock.unlock();
			socket = nullptr;
		}

		this_thread::sleep_for(chrono::milliseconds(1));
	}

	// Close the listener socket.
	listener->close();
}


void Instance::_connectionHandler(shared_ptr<TcpSocket> socket, shared_ptr<atomic_bool> connected) {
	IpAddress remoteIp = socket->getRemoteAddress();
	Uint16 remotePort = socket->getRemotePort();
	while (*connected) {
		int CRLFCount = 0;
		char c;
		string header;
		header.clear();
		// Receive a new request
		socket->setBlocking(true);
		try {
			while (!isalpha(c = getchar(socket)));
			header.push_back(c);
			while (CRLFCount < 4) {
				c = getchar(socket);
				header.push_back(c);
				if (c == '\r' || c == '\n')
					CRLFCount++;
				else
					CRLFCount = 0;
			}
		} catch (DisconnectedException) {
			(*connected) = false;
			break;
		}

		// Parse the request
		HTTPRequest request;
		try {
			request = HTTPParser::parseRequest(header);
		} catch (HTTPParser::BadRequestException) {
			// Create a 400 Bad Request response and send it (therefore closing the connection)
			HTTPResponseError(400).send(socket);
			break;
		}

		// Read a body if there is one
		if (string lenstr = request.GetHeaderValue("Content-Length"); !lenstr.empty()) {
			size_t len = atoi(lenstr.c_str());
			string body(len, '\0');
			char* bodydata = body.data();
			size_t readlen;
			socket->receive(bodydata, len - (bodydata - body.data()), readlen);
			request.SetBody(body);
		}

		// Request ok, dispatch it
		mloge << "Endpoint [" << remoteIp.toString() << "] sended a request" << dlog;
		mloge << "    Method: " << request.GetMethod() << ", URI: " << request.GetURI() << dlog;
		mloge << "    Body: " << (request.GetBody().empty() ? "(empty)"s : request.GetBody()) << dlog;
		HTTPResponseWrapper::Ptr response = _dispatchRequest(request);

		// Send the response
		response->send(socket);
		mloge << "Data Sent: " << response->what() << dlog;

		// If Connection: close, close the connection
		if (response->GetHeaderValue("Connection") == "close") {
			socket->disconnect();
			(*connected) = false;
		}
	}
	socket->disconnect();
	mloge << "Endpoint " << remoteIp.toString() << " closed." << dlog;
}


HTTPResponseWrapper::Ptr Instance::_dispatchRequest(HTTPRequest& request) {
	// Get the URI and hostname
	const string& uri = request.GetURI(), host = request.GetHeaderValue("Host");

	// Dispatch the reqeust
	// HACK Assume all non-POST requests as GET ones
	auto& handlers = (request.GetMethod() == "POST" ? postRoutes : getRoutes);
	for (auto&[uriRegex, hostRegex, handler] : handlers)
		if (regex_match(uri, uriRegex) && regex_match(host, hostRegex))
			return handler(request);

	// Return a 404 Not Found if no handler was matched
	return make_shared<HTTPResponseError>(404);
}

