#pragma once

#include "Main.hpp"

#include "HTTPResponseWrapper.hpp"


class Instance {
public:

	typedef sfn::TlsConnection<sfn::TcpSocket, sfn::TlsEndpointType::Server, sfn::TlsVerificationType::None> HTTPSConnection;

	enum RequestMethod {
		Get,
		Post
	};

	struct Config {
		// Use HTTPS
		bool useHTTPS = false;

		// Port the HTTP server will be listening
		Uint16 port = 5000;
		// Port the HTTPS server will be listening
		Uint16 portHTTPS = 5443;

		// Filename of the Base 64 DER encoded certificate
		wstring cert = wstring();

		// Filename of the PKCS8 private key file
		// and the optional password
		wstring key = wstring();
		string keypass = string();
	};

	// The function will return instantly, leaving worker
	// threads to handle the connections
	void start(Config&& config = Config{});

	void stop();

public:

	typedef function<HTTPResponseWrapper::Ptr(HTTPRequest)> RouteHandlerFunction;

	// Calls the callback if the two regex are both matched; calls the first one if there's multiple matches
	// RouteHandlerFunction prarmater is the HTTP requset, with URI percent-encoded
	void registerRouteRule(string regexURI, string regexHostnames, RouteHandlerFunction handler, RequestMethod request = Get) {
		if (request == Get)
			getRoutes.emplace_back(regex(regexURI), regex(regexHostnames), handler);
		else if (request == Post)
			postRoutes.emplace_back(regex(regexURI), regex(regexHostnames), handler);
	}


private:

	struct route {
		regex uri, hostname;
	};

	friend class HTTPHandler;

	void _HTTPListener();
	void _HTTPSListener();
	void _maintainer();

	void _connectionHandler(TcpSocket::Ptr socket);

	//TODO Use a better container than std::list and std::tuple
	list<tuple<regex, regex, RouteHandlerFunction>> getRoutes, postRoutes;
	HTTPResponseWrapper::Ptr _dispatchRequest(HTTPRequest& request);

	shared_ptr<thread> httpListener, httpsListener, maintainer;
	std::list<pair<sfn::TcpSocket::Ptr, shared_ptr<thread>>> sockets;
	mutex queueLock;

	atomic_bool running;

	bool useHTTPS;

	Uint16 port, portHTTPS;

	TlsCertificate::Ptr cert;
	TlsKey::Ptr key;
};


#define ROUTER(requestName) [&](HTTPRequest requestName)->HTTPResponseWrapper::Ptr

