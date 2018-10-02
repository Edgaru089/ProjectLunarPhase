#pragma once

#include "Main.hpp"
#include "MySql/MySql.hpp"


class DatabaseHandler {
public:

	DatabaseHandler() :mysql(nullptr) {}

	void connect(string&& hostname = "localhost",
				 Uint16 port = 3306,
				 string&& username = "bndsdb",
				 string&& password = "bnds-db-admin-ScienceAndTechnolgy",
				 string&& databaseName = "bndsdb") {
		mysql = make_shared<MySql>(hostname.c_str(), username.c_str(), password.c_str(), databaseName.c_str(), port);
		_prepareStatements();
	}

	void ensureConnected(string&& hostname = "localhost",
						 Uint16 port = 3306,
						 string&& username = "bndsdb",
						 string&& password = "bnds-db-admin-ScienceAndTechnolgy",
						 string&& databaseName = "bndsdb") {
		if (!mysql) {
			mysql = make_shared<MySql>(hostname.c_str(), username.c_str(), password.c_str(), databaseName.c_str(), port);
			_prepareStatements();
		}
	}

	bool connected() { return (bool)mysql; }
	void disconnect() { mysql = nullptr; }


	struct User {
		int id = 0;
		string username;
		string password;
		string cursession;
	};

	struct Post {
		int id = 0;
		string username;
		string title;
		string body;
	};

	User getUser(int id) {
		ensureConnected();

	}

	void addPost(const string& username, const string& title, const string& contents) {
		ensureConnected();
		//mysql->runCommand(*insertPost, 0, username, title, contents);
		mysql->runCommand("INSERT INTO `posts` (`id`, `username`, `title`, `body`) VALUES (?, ?, ?, ?)", 0, username, title, contents);
	}

	// id, username, title, body
	typedef vector<tuple<int, string, string, string>> PostVectorTuple;
	const PostVectorTuple& getPosts() {
		ensureConnected();
		if (chrono::steady_clock::now() - cachedPostTime > recachePostDuartion) {
			try {
				cachedPosts.clear();
				//mysql->runQuery(&q, *listPosts);
				mysql->runQuery(&cachedPosts, "SELECT * FROM `posts`");
				mlog << "Database.GetPosts(): Queried size: " << cachedPosts.size() << dlog;
				return cachedPosts;
			}
			catch (MySqlException e) {
				mlog << Log::Error << "There is a database exception: " << e.what() << dlog;
			}
		}
		return cachedPosts;
	}


private:

	void _prepareStatements() {
		try {
			// Set UTF-8 Encoding
			mysql->runCommand("SET NAMES 'utf8';");
			insertPost = make_shared<MySqlPreparedStatement>(mysql->prepareStatement("INSERT INTO `posts` (`id`, `username`, `title`, `body`) VALUES (?, ?, ?, ?)"));
			listPosts = make_shared<MySqlPreparedStatement>(mysql->prepareStatement("SELECT * FROM `posts`"));
		}
		catch (MySqlException e) {
			mlog << Log::Error << "There is a database exception: " << e.what() << dlog;
		}
	}

	shared_ptr<MySql> mysql;
	shared_ptr<MySqlPreparedStatement> insertPost, listPosts;

	PostVectorTuple cachedPosts;
	chrono::steady_clock::time_point cachedPostTime;
	const chrono::seconds recachePostDuartion = chrono::seconds(1);


};

