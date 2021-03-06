
#include "Instance.hpp"
#include "HTTPParser.hpp"


namespace {

	class DisconnectedException :public exception {
		char const* what() const override { return "Socket disconnected when waiting for contents"; }
	};

	// Blocking
	unsigned char getchar(TcpSocket::Ptr socket) {
		unsigned char c = 0;
		while (!((socket->LocalHasShutdown() || socket->RemoteHasShutdown()) && socket->BytesToReceive() == 0) && socket->Receive(&c, 1) == 0)
			this_thread::sleep_for(chrono::milliseconds(75));
		if ((socket->LocalHasShutdown() || socket->RemoteHasShutdown()) && socket->BytesToReceive() == 0)
			throw DisconnectedException();
		//#ifndef DISABLE_ALL_LOGS
		//		putchar(c);
		//#endif
		return c;
	}

}


void Instance::start(Instance::Config&& conf) {
	mlog << "Server Starting..." << dlog;

	useHTTPS = conf.useHTTPS;
	listenIPv6 = conf.listenIPv6;
	port = conf.port;
	portHTTPS = conf.portHTTPS;

	if (useHTTPS) {
		cert = nullptr;
		key = nullptr;
		if (string str = readFileBinaryCached(conf.cert); !str.empty())
			cert = TlsCertificate::Create(str);
		if (string str = readFileBinaryCached(conf.key); !str.empty())
			key = TlsKey::Create(str, conf.keypass);
		if (!cert)
			mlog << Log::Error << "Certificate loading failed! Filename: " << conf.cert << dlog;
		if (!key)
			mlog << Log::Error << "Certificate private key loading failed! Filename: " << conf.key << dlog;
		if (!cert || !key)
			useHTTPS = false;
	}

	if (useHTTPS)
		mlog << "HTTP Port: " << port << ", HTTPS Port: " << portHTTPS << dlog;
	else
		mlog << "HTTP Port: " << port << ", HTTPS Turned Off" << dlog;
	mlog << "IPv4 Listening: " << (listenIPv4 ? "On" : "Off") << ", IPv6 Listening: " << (listenIPv6 ? "On" : "Off") << '\n' << dlog;

	running = true;

	if (listenIPv4) {
		httpListener = make_shared<thread>([this]() {
			auto listener = TcpListener::Create();
			listener->Listen(sfn::Endpoint(sfn::IpAddress("0.0.0.0"), port));
			_HTTPListener(listener);
		});
		if (useHTTPS)
			httpsListener = make_shared<thread>([this]() {
			auto listener = TcpListener::Create();
			listener->Listen(sfn::Endpoint(sfn::IpAddress("0.0.0.0"), port));
			_HTTPSListener(listener);
		});
	}
	if (listenIPv6) {
		httpListenerV6 = make_shared<thread>([this]() {
			auto listener = TcpListener::Create();
			listener->Listen(sfn::Endpoint(sfn::IpAddress("::"), port));
			_HTTPListener(listener);
		});
		if (useHTTPS)
			httpsListenerV6 = make_shared<thread>([this]() {
			auto listener = TcpListener::Create();
			listener->Listen(sfn::Endpoint(sfn::IpAddress("::"), port));
			_HTTPSListener(listener);
		});
	}
}


void Instance::stop() {
	mlog << "Stopping..." << dlog;
	running = false;
	for (auto&[connection, handler] : sockets) {
		connection->Shutdown();
		if (handler->joinable())
			handler->join();
	}

	if (httpListener && httpListener->joinable())
		httpListener->join();
	if (httpsListener && httpsListener->joinable())
		httpsListener->join();

	sockets.clear();
	httpListener = httpsListener = nullptr;
	mlog << "Server Stopped" << dlog;
}


void Instance::_maintainer() {
	while (running) {
		this_thread::sleep_for(chrono::milliseconds(300));
		queueLock.lock();
		for (auto i = sockets.begin(); i != sockets.end();)
			if (i->first->RemoteHasShutdown() && i->first->BytesToSend() == 0) {
				if (i->second->joinable())
					i->second->join();
				i = sockets.erase(i);
			} else
				i++;
			queueLock.unlock();
	}
}


void Instance::_HTTPListener(TcpListener::Ptr listener) {
	sfn::TcpSocket::Ptr socket;
	while (running) {
		socket = nullptr;
		// Dequeue any pending connections from the listener.
		while ((socket = listener->GetPendingConnection())) {
			// Turn off connection lingering.
			socket->SetLinger(0);
			// Create a handler thread and store the connection.
			queueLock.lock();
			sockets.emplace_back(socket,
								 make_shared<thread>(&Instance::_connectionHandler, this, socket));
			mloge << "HTTP Connected: [" << ansiToWstring((string)socket->GetRemoteEndpoint().GetAddress()) << "]:" << socket->GetRemoteEndpoint().GetPort() << dlog;
			queueLock.unlock();
		}

		this_thread::sleep_for(chrono::milliseconds(200));
	}

	// Close the listener socket.
	listener->Close();
}


void Instance::_HTTPSListener(TcpListener::Ptr listener) {
	typedef sfn::TlsConnection<sfn::TcpSocket, sfn::TlsEndpointType::Server, sfn::TlsVerificationType::None> Connection;

	Connection::Ptr connection;
	while (running) {
		connection = nullptr;
		// Dequeue any connections from the listener.
		while ((connection = listener->GetPendingConnection<Connection>())) {
			// Set the server certificate and key pair.
			connection->SetCertificateKeyPair(cert, key);
			// Turn off connection lingering.
			connection->SetLinger(0);
			// Create a handler thread and store the connection.
			queueLock.lock();
			sockets.emplace_back(connection,
								 make_shared<thread>(&Instance::_connectionHandler, this, connection));
			mloge << "HTTPS Connected: [" << ansiToWstring((string)connection->GetRemoteEndpoint().GetAddress()) << "]:" << connection->GetRemoteEndpoint().GetPort() << dlog;
			queueLock.unlock();
		}

		this_thread::sleep_for(chrono::milliseconds(200));
	}

	// Close the listener socket.
	listener->Close();
}


void Instance::_connectionHandler(TcpSocket::Ptr socket) {
	Endpoint remote = socket->GetRemoteEndpoint();
	while (!(socket->LocalHasShutdown() || socket->RemoteHasShutdown())) {
		int CRLFCount = 0;
		char c;
		string header;
		header.clear();
		// Receive a new request
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
			while (!(socket->LocalHasShutdown() || socket->RemoteHasShutdown()) && bodydata < body.data() + len)
				bodydata += socket->Receive(bodydata, len - (bodydata - body.data()));
			request.SetBody(body);
		}

		// Request ok, dispatch it
		mloge << "Endpoint " << (string)remote.GetAddress() << " sended a request" << dlog;
		mloge << "    Method: " << request.GetMethod() << ", URI: " << request.GetURI() << dlog;
		mloge << "    Body: " << (request.GetBody().empty() ? "(empty)"s : request.GetBody()) << dlog;
		HTTPResponseWrapper::Ptr response = _dispatchRequest(request);

		// Send the response
		response->send(socket);
		mloge << "Data Sent: " << response->what() << dlog;

		// If Connection: close, close the connection
		if (response->GetHeaderValue("Connection") == "close") {
			socket->Close();
		}
	}
	if (!socket->LocalHasShutdown())
		socket->Shutdown();
	mloge << "Endpoint " << (string)remote.GetAddress() << " closed." << dlog;
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

