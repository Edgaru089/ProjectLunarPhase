#pragma once

#include "Main.hpp"

#include "HTTPResponseWrapper.hpp"


class Instance {
public:

	enum RequestMethod {
		Get,
		Post
	};

	struct Config {
		Config() {}

		// Port the HTTP server will be listening
		Uint16 port = 5000;

		// Filename of the Base 64 DER encoded certificate
		wstring cert = wstring();

		// Filename of the PKCS8 private key file
		// and the optional password
		wstring key = wstring();
		string keypass = string();
	};

	// The function will return instantly, leaving worker
	// threads to handle the connections
	void start(Config&& config = Config());

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

	void _HTTPListener(shared_ptr<TcpListener> listener);
	void _maintainer();

	void _connectionHandler(shared_ptr<TcpSocket> socket, shared_ptr<atomic_bool> connected);

	//TODO Use a better container than std::list and std::tuple
	list<tuple<regex, regex, RouteHandlerFunction>> getRoutes, postRoutes;
	HTTPResponseWrapper::Ptr _dispatchRequest(HTTPRequest& request);

	shared_ptr<thread> httpListener;
	shared_ptr<thread> maintainer;
	std::list<tuple<shared_ptr<TcpSocket>, shared_ptr<thread>, shared_ptr<atomic_bool>>> sockets;
	mutex queueLock;

	atomic_bool running;

	Uint16 port;
};


#define ROUTER(requestName) [&](const HTTPRequest& requestName)->HTTPResponseWrapper::Ptr

