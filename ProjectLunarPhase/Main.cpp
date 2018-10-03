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
	db.connect();

	Start(8);

	Instance instance;

	setFrameFile(L"./html/frame.html");

	instance.registerRouteRule("/", ".*", ROUTER(){
		return redirect("/index");
	});

	instance.registerRouteRule("/index", ".*", ROUTER(){
		return filetemplate("./html/show_entries.html", { { "%TITLE%", "Index" } });
	});

	instance.registerRouteRule("/static/.*", ".*", ROUTER(request){
		return file('.' + decodePercentEncoding(request.GetURI()));
	});

	instance.registerRouteRule("/sanae", ".*", ROUTER(){
		string body = u8"<img src=\"static/sanae.png\" alt=\"sanae.png\" style=\"width: 350px; background-color: #fff\">"
			"</img><br />東風谷早苗 / 东风谷早苗<br />Kochiya Sanae\r\n";
		return htmltext(body, true, { { "%TITLE%", "Kochiya Sanae" } });
	});

	instance.registerRouteRule("/login", ".*", ROUTER(){
		return filetemplate(L"./html/login.html", { { "%TITLE%", "Login" } });
	});

	instance.registerRouteRule("/login", ".*", ROUTER(request){
		if (request.GetHeaderValue("Content-Type") != "application/x-www-form-urlencoded")
			return error(404);
		auto instances = decodeFormUrlEncoded(request.GetBody());
		if (db.verifyUser(instances["username"], instances["password"])) {
			string cookie = generateCookie();
			db.setUserCookie(instances["username"], cookie);
			return redirect("/posts", 302, { { "id", cookie } });
		}
		else
			return redirect("/login", 302);

	}, Instance::Post);

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

		return redirect("/posts", 302);
	}, Instance::Post);

	instance.registerRouteRule("/posts", ".*", ROUTER(request){
		// Craft the posts body
		string cont = R"(
		<div class="post">
        	<a style="float: left;">%TITLE%</a>
          <a style="float: right;">%USER%</a>
            <br /><hr />
            <a>%CONT%</a>
        </div>
)";
		string entries;
		auto posts = db.getPosts();
		for (auto&&[id, username, title, body] : posts)
			entries = StringParser::replaceSubString(cont, {
				{ "%TITLE%", title }, { "%USER%", username }, { "%CONT%", StringParser::replaceSubString(body, { { "\r\n", "<br />\r\n" } }) } })
				+ entries;
		if (entries.empty())
			entries = u8"<div class=\"post\">这里似乎没有内容</div>";

		// Get the username
		string nav = "<a href=\"/login\">Login</a>";
		if (string cookie = decodeCookieSequence(request.GetHeaderValue("Cookie"))["id"]; !cookie.empty())
			if (string username = db.getCookieUsername(cookie); !username.empty())
				nav = "Logged in: " + username;

		return filetemplate(L"./html/show_entries.html", { { "%TITLE%", "Posts" }, { "%BODY%", entries }, { "%NAV%", nav } });
	});

	instance.start(Instance::Config{ true, 5000, 5443, L"./ssl/127.0.0.1.cer", L"./ssl/127.0.0.1.key" });

	cin.ignore(10000, '\n');

	instance.stop();

	Stop();
}

