#pragma once

#include "Main.hpp"
#include "MySql/MySql.hpp"


class DatabaseHandler {
public:

	DatabaseHandler(const char* hostname = "localhost",
					Uint16 port = 3306,
					const char* username = "bndsdb",
					const char* password = "bnds-db-admin-ScienceAndTechnolgy",
					const char* databaseName = "bndsdb") :mysql(hostname, username, password, databaseName, port) {
		// Set UTF-8 Encoding
		mysql.runCommand("SET NAMES 'utf8';");
	}

	struct User {
		int id = 0;
		string username;
		string password;
		string cursession;
		bool operator ==(const User& right) const { return id == right.id; }
		bool operator != (const User& right) const { return id != right.id; }
	};

	struct Post {
		int id = 0;
		string username;
		string title;
		string body;
	};

	typedef vector<tuple<int, string, string, string>> UserVectorTuple;
	User getUser(string username) {
		UserVectorTuple q;
		mysql.runQuery(&q, "SELECT * FROM `users` WHERE `username` = ?;", username);
		if (q.size() != 1)
			return User{};
		else {
			auto&[id, username, password, cursession] = q[0];
			return User{ id, username, password, cursession };
		}
	}

	void addUser(string username, string password) {
		if (getUser(username) != User{})
			return;
		mysql.runCommand("INSERT INTO `users` (`id`, `username`, `password`, `cursession`) VALUES (?, ?, ?, ?)", 0, username, password, ""s);
	}

	bool verifyUser(string username, string password) {
		vector<tuple<string>> q;
		mysql.runQuery(&q, "SELECT `password` FROM `users` WHERE `username` = ?;", username);
		if (q.size() != 1 || get<0>(q[0]) != password)
			return false;
		else
			return true;
	}

	void addPost(const string& username, const string& title, const string& contents) {
		//mysql->runCommand(*insertPost, 0, username, title, contents);
		mysql.runCommand("INSERT INTO `posts` (`id`, `username`, `title`, `body`) VALUES (?, ?, ?, ?);", 0, username, title, contents);
	}

	// id, username, title, body
	typedef vector<tuple<int, string, string, string>> PostVectorTuple;
	const PostVectorTuple& getPosts() {
		if (chrono::steady_clock::now() - cachedPostTime > recachePostDuartion) {
			try {
				cachedPosts.clear();
				//mysql->runQuery(&cachedPosts, *listPosts);
				mysql.runQuery(&cachedPosts, "SELECT * FROM `posts`;");
				mlog << "Database.GetPosts(): Queried size: " << cachedPosts.size() << dlog;
				return cachedPosts;
			}
			catch (MySqlException e) {
				mlog << Log::Error << "There is a database exception: " << e.what() << dlog;
			}
		}
		return cachedPosts;
	}

	void setUserCookie(const string& username, const string& cookie) {
		mysql.runCommand("UPDATE `users` SET `cursession` = ? WHERE `users`.`username` = ?;", cookie, username);
	}

	string getCookieUsername(const string& cookie) {
		vector<tuple<string>> q;
		mysql.runQuery(&q, "SELECT `username` FROM `users` WHERE `cursession` = ?;", cookie);
		if (q.size() != 1)
			return "";
		else
			return get<0>(q[0]);
	}

private:

	MySql mysql;

	PostVectorTuple cachedPosts;
	chrono::steady_clock::time_point cachedPostTime;
	const chrono::seconds recachePostDuartion = chrono::seconds(2);


};

