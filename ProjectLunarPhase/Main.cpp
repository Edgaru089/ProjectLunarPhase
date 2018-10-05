#include "Main.hpp"
#include "Instance.hpp"
#include "DatabaseHandler.hpp"

Log dlog;


int main(int argc, char* argv[]) {
	DatabaseHandler db;
	locale::global(locale("", LC_CTYPE));
	wcout.imbue(locale("", LC_CTYPE));
	dlog.addOutputStream(wcout);

	//MySql mysql("localhost","bndsdb","bnds-db-admin-ScienceAndTechnolgy");

	Start(8);

	Instance instance;

	setFrameFile(L"./html/frame.html");

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
		// Get the username, redirect if not empty
		string username;
		if (string cookie = decodeCookieSequence(request.GetHeaderValue("Cookie"))["id"]; !cookie.empty())
			if (!(username = db.getCookieUsername(cookie)).empty())
				return redirect("/posts", 303);
		return filetemplate(L"./html/login.html", { { "%TITLE%", "Login" } });
	});

	instance.registerRouteRule("/login", ".*", ROUTER(request){
		if (request.GetHeaderValue("Content-Type") != "application/x-www-form-urlencoded")
			return error(404);
		auto instances = decodeFormUrlEncoded(request.GetBody());
		if (db.verifyUser(instances["username"], instances["password"])) {
			string cookie = generateCookie();
			db.setUserCookie(instances["username"], cookie);
			return redirect("/posts", 303, { { "id", cookie } });
		}
		else
			return redirect("/login", 303);

	}, Instance::Post);

	instance.registerRouteRule("/logout", ".*", ROUTER(request){
		// Remove the id cookie by setting the expiry date in the past;
		// See https://stackoverflow.com/questions/20320549/how-can-you-delete-a-cookie-in-an-http-response
		return redirect("/posts", 303, { { "id", "deleted; Expires=Wed, 21 Oct 2015 07:28:00 GMT" } });
	});

	instance.registerRouteRule("/share", ".*", ROUTER(request){
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
	}, Instance::Post);

	instance.registerRouteRule("/posts", ".*", ROUTER(request){
		// Craft the posts body
		string cont = readFileBinary(L"./html/post_frame.html");
		string entries;
		auto posts = db.getPosts();
		for (auto&&[id, username, title, body] : posts)
			entries = StringParser::replaceSubString(cont, {
				{ "%TITLE%", title }, { "%USER%", username }, { "%CONT%", StringParser::replaceSubString(body, { { "\r\n", "<br />\r\n" } }) } })
				+ entries;
		if (entries.empty())
			entries = u8"<div class=\"post\">这里似乎没有内容</div>";

		// Get the username
		string nav = "<a href=\"/login\">Login</a>", username;
		if (string cookie = decodeCookieSequence(request.GetHeaderValue("Cookie"))["id"]; !cookie.empty())
			if (!(username = db.getCookieUsername(cookie)).empty())
				nav = "Logged in: " + username + " <a href=\"/logout\">Logout</a>";

		// Add the postnew frame
		string postnew;
		if (username.empty())
			postnew = readFileBinary(L"./html/postnew_frame_empty.html");
		else
			postnew = StringParser::replaceSubString(readFileBinary(L"./html/postnew_frame.html"), { { "%USER%", username } });

		return filetemplate(L"./html/show_entries.html", { { "%TITLE%", "Posts" }, { "%BODY%", entries }, { "%NAV%", nav }, { "%POST%", postnew } });
	});

	instance.start(Instance::Config{ true, 5000, 5443, L"./ssl/127.0.0.1.cer", L"./ssl/127.0.0.1.key" });

	cin.ignore(10000, '\n');

	instance.stop();

	Stop();
}

