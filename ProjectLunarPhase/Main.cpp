#include "Main.hpp"
#include "Instance.hpp"
#include "DatabaseHandler.hpp"

Log dlog;
ofstream logout;


int main(int argc, char* argv[]) try {
	// Open a binary output stream for logs
	time_t curtime = time(NULL);
	wchar_t buffer[64] = {};
	char signature[] = u8"\ufeff";
	wcsftime(buffer, 63, L"logs/%Y-%m-%d-%H.%M.%S.log", localtime(&curtime));
	logout.open(buffer, ofstream::binary);
	logout.write(signature, sizeof(signature) - 1);

	locale::global(locale("", LC_CTYPE));
	wcout.imbue(locale("", LC_CTYPE));
	dlog.addOutputStream(wcout);
	dlog.addOutputHandler([&](const wstring& str) {
		string u8str = wstringToUtf8(str) + "\r\n";
		logout.write(u8str.data(), u8str.size());
		logout.flush();
	});

	//MySql mysql("localhost","bndsdb","bnds-db-admin-ScienceAndTechnolgy");
	DatabaseHandler db;

	Start(8);

	Instance instance;

	setFrameFile(L"./html/frame.html");

	auto databaseErrorPage = [&](char const * what) {
		return filetemplate("./html/database_error.html", { { "%WHAT%", what } });
	};

	instance.registerRouteRule("/", ".*", ROUTER(){
		return redirect("/posts");
	});

	instance.registerRouteRule("/static/.*", ".*", ROUTER(request){
		return file('.' + decodePercentEncoding(request.GetURI()));
	});

	instance.registerRouteRule("/sanae", ".*", ROUTER(){
		string body = u8"<img src=\"static/sanae.png\" alt=\"sanae.png\" style=\"width: 350px; background-color: #fff\">"
			"</img><br />東風谷早苗 / 东风谷早苗<br />Kochiya Sanae\r\n";
		return htmltext(body, true, { { "%TITLE%", "Kochiya Sanae" } });
	});

	instance.registerRouteRule("/login", ".*", ROUTER(request){
		try {
			// Get the username, redirect if not empty
			string username;
			if (string cookie = decodeCookieSequence(request.GetHeaderValue("Cookie"))["id"]; !cookie.empty())
				if (!(username = db.getCookieUsername(cookie)).empty())
					return redirect("/posts", 303);
			return filetemplate(L"./html/login.html", { { "%TITLE%", "Login" } });
		} catch (MySqlException& e) {
			return databaseErrorPage(e.what());
		}
	});

	instance.registerRouteRule("/login", ".*", ROUTER(request){
		try {
			if (request.GetHeaderValue("Content-Type") != "application/x-www-form-urlencoded")
				return error(404);
			auto instances = decodeFormUrlEncoded(request.GetBody());
			if (db.verifyUser(instances["username"], instances["password"])) {
				string cookie = generateCookie();
				db.setUserCookie(instances["username"], cookie);
				return redirect("/posts", 303, { { "id", cookie } });
			} else
				return redirect("/login", 303);
		} catch (MySqlException& e) {
			return databaseErrorPage(e.what());
		}

	}, Instance::Post);

	instance.registerRouteRule("/register", ".*", ROUTER(request){
		try {
		// Get the username, redirect if not empty
			string username;
			if (string cookie = decodeCookieSequence(request.GetHeaderValue("Cookie"))["id"]; !cookie.empty())
				if (!(username = db.getCookieUsername(cookie)).empty())
					return redirect("/posts", 303);
			return filetemplate(L"./html/register.html", { { "%TITLE%", "Register" } });
		} catch (MySqlException& e) {
			return databaseErrorPage(e.what());
		}
	});

	instance.registerRouteRule("/register", ".*", ROUTER(request){
		try {
			if (request.GetHeaderValue("Content-Type") != "application/x-www-form-urlencoded")
				return error(404);
			auto instances = decodeFormUrlEncoded(request.GetBody());
			if (db.getUser(instances["username"]).username == "") {
				string cookie = generateCookie();
				db.addUser(instances["username"], instances["password"], cookie);
				return redirect("/posts", 303, { { "id", cookie } });
			} else
				return redirect("/register", 303);
		} catch (MySqlException& e) {
			return databaseErrorPage(e.what());
		}
	}, Instance::Post);

	instance.registerRouteRule("/logout", ".*", ROUTER(request){
		// Remove the id cookie by setting the expiry date in the past;
		// See https://stackoverflow.com/questions/20320549/how-can-you-delete-a-cookie-in-an-http-response
		return redirect("/posts", 303, { { "id", "deleted; Expires=Wed, 21 Oct 2015 07:28:00 GMT" } });
	});

	instance.registerRouteRule("/share", ".*", ROUTER(request){
		try {
			if (request.GetHeaderValue("Content-Type") != "application/x-www-form-urlencoded")
				return error(404);
			auto data = decodeFormUrlEncoded(request.GetBody());

			string username, title, body;
			for (auto&[id, data] : data) {
				if (id == "username")
					username = data;
				else if (id == "title")
					title = data;
				else if (id == "body")
					body = data;
			}
			if (!username.empty() && !title.empty() && !body.empty())
				db.addPost(username, title, body);
			return redirect("/posts", 303);
		} catch (MySqlException& e) {
			return databaseErrorPage(e.what());
		}
	}, Instance::Post);

	instance.registerRouteRule("/posts", ".*", ROUTER(request){
		try {
			// Craft the posts body
			string cont = readFileBinaryCached(L"./html/post_frame.html");
			string entries;
			auto posts = db.getPosts();
			for (auto&&[id, username, title, body] : posts)
				entries = StringParser::replaceSubString(cont, {
					{ "%TITLE%", title }, { "%USER%", username }, { "%CONT%", StringParser::replaceSubString(body, { { "\r\n", "<br />\r\n" } }) } })
					+ entries;
			if (entries.empty())
				entries = u8"<div class=\"post\">这里似乎没有内容</div>";

			// Get the username
			string nav = "<a href=\"/login\">Login</a> <a href=\"/register\">Register</a>", username;
			if (string cookie = decodeCookieSequence(request.GetHeaderValue("Cookie"))["id"]; !cookie.empty())
				if (!(username = db.getCookieUsername(cookie)).empty())
					nav = "Logged in: " + username + " <a href=\"/logout\">Logout</a>";

			// Add the postnew frame
			string postnew;
			if (username.empty())
				postnew = readFileBinaryCached(L"./html/postnew_frame_empty.html");
			else
				postnew = StringParser::replaceSubString(readFileBinaryCached(L"./html/postnew_frame.html"), { { "%USER%", username } });

			return filetemplate(L"./html/show_entries.html", { { "%TITLE%", "Posts" }, { "%BODY%", entries }, { "%NAV%", nav }, { "%POST%", postnew } });
		} catch (MySqlException& e) {
			return databaseErrorPage(e.what());
		}
	});

	instance.start(Instance::Config{ true, 5000, 5443, L"./ssl/127.0.0.1.cer", L"./ssl/127.0.0.1.key" });

	// Pause for a newline
	cin.ignore(10000, '\n');

	instance.stop();

	Stop();

} catch (exception& e) {
	mlog << Log::Error << "The main closure failed with an uncaught exception: " << e.what() << dlog;
	mlog << Log::Error << "Exception typename: " << typeid(e).name() << " (" << typeid(e).raw_name() << ")" << dlog;
	mlog << Log::Error << "The program will now fail." << dlog;
	return 1;
}

