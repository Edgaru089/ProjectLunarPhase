#pragma once

#include "Main.hpp"
#include "MySql/MySql.hpp"


class DatabaseHandler {
public:

	DatabaseHandler(const char* hostname = "127.0.0.1",
		Uint16 port = 3306,
		const char* username = "bndsdb",
		const char* password = "bnds-db-admin-ScienceAndTechnolgy",
		const char* databaseName = "bndsdb")
		:mysql(hostname, username, password, databaseName, port),
		stmtGetUser(mysql.prepareStatement("SELECT * FROM `users` WHERE `username` = ?;")),
		stmtAddUser(mysql.prepareStatement("INSERT INTO `users` (`id`, `username`, `password`, `cursession`) VALUES (?, ?, ?, ?)")),
		stmtVerifyUser(mysql.prepareStatement("SELECT `password` FROM `users` WHERE `username` = ?;")),
		stmtAddPost(mysql.prepareStatement("INSERT INTO `posts` (`id`, `username`, `title`, `body`) VALUES (?, ?, ?, ?);")),
		stmtGetPosts(mysql.prepareStatement("SELECT * FROM `posts`;")),
		stmtGetPostsByUsername(mysql.prepareStatement("SELECT * FROM `posts` WHERE `username` = ?;")),
		stmtGetPostById(mysql.prepareStatement("SELECT * FROM `posts` WHERE `id` = ?;")),
		stmtGetPostUsernameById(mysql.prepareStatement("SELECT `username` FROM `posts` WHERE `id` = ?;")),
		stmtRemovePost(mysql.prepareStatement("DELETE FROM `posts` WHERE `id` = ?;")),
		stmtUpdatePost(mysql.prepareStatement("UPDATE `posts` SET `title` = ?, `body` = ? WHERE `id` = ?;")),
		stmtSetUserCookie(mysql.prepareStatement("UPDATE `users` SET `cursession` = ? WHERE `users`.`username` = ?;")),
		stmtGetCookieUsername(mysql.prepareStatement("SELECT `username` FROM `users` WHERE `cursession` = ?;"))
	{
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
		mysql.runQuery(&q, stmtGetUser, username);
		if (q.size() != 1)
			return User{};
		else {
			auto&[id, username, password, cursession] = q[0];
			return User{ id, username, password, cursession };
		}
	}

	void addUser(const string& username, const string& password, const string& session) {
		if (getUser(username) != User{})
			return;
		mysql.runCommand(stmtAddUser, 0, username, password, session);
	}

	bool verifyUser(const string& username, const string& password) {
		vector<tuple<string>> q;
		mysql.runQuery(&q, stmtVerifyUser, username);
		if (q.size() != 1 || get<0>(q[0]) != password)
			return false;
		else
			return true;
	}

	void addPost(const string& username, const string& title, const string& contents) {
		mysql.runCommand(stmtAddPost, 0, username, title, contents);
	}

	// id, username, title, body
	typedef vector<tuple<int, string, string, string>> PostVectorTuple;
	const PostVectorTuple& getPosts() {
		if (chrono::steady_clock::now() - cachedPostTime > recachePostDuartion) {
			cachedPostTime = chrono::steady_clock::now();
			cachedPosts.clear();
			//mysql->runQuery(&cachedPosts, *listPosts);
			mysql.runQuery(&cachedPosts, stmtGetPosts);
			mlog << "Database.GetPosts(): Queried size: " << cachedPosts.size() << dlog;
			return cachedPosts;
		}
		return cachedPosts;
	}

	void getPostsByUsername(const string& username, PostVectorTuple& vec) {
		vec.clear();
		mysql.runQuery(&vec, stmtGetPostsByUsername, username);
	}

	Post getPostById(int id) {
		PostVectorTuple qans;
		mysql.runQuery(&qans, stmtGetPostById, id);
		if (qans.size() != 1)
			return Post();
		else {
			auto&[id, username, title, body] = qans[0];
			return Post{ id, username, title, body };
		}
	}

	string getPostUsernameById(int id) {
		vector<tuple<string>> qans;
		mysql.runQuery(&qans, stmtGetPostUsernameById, id);
		if (qans.size() != 1)
			return "";
		else
			return get<0>(qans[0]);
	}

	void removePost(int id) {
		mysql.runCommand(stmtRemovePost, id);
		cachedPosts.clear();
	}

	void updatePost(int id, const string& title, const string& contents) {
		mysql.runCommand(stmtUpdatePost, title, contents, id);
		cachedPosts.clear();
	}

	void setUserCookie(const string& username, const string& cookie) {
		mysql.runCommand(stmtSetUserCookie, cookie, username);
	}

	string getCookieUsername(const string& cookie) {
		vector<tuple<string>> q;
		mysql.runQuery(&q, stmtGetCookieUsername, cookie);
		if (q.size() != 1)
			return "";
		else
			return get<0>(q[0]);
	}

private:

	MySql mysql;

	MySqlPreparedStatement
		stmtGetUser,
		stmtAddUser,
		stmtVerifyUser,
		stmtAddPost,
		stmtGetPosts,
		stmtGetPostsByUsername,
		stmtGetPostById,
		stmtGetPostUsernameById,
		stmtRemovePost,
		stmtUpdatePost,
		stmtSetUserCookie,
		stmtGetCookieUsername;

	PostVectorTuple cachedPosts;
	chrono::steady_clock::time_point cachedPostTime;
	const chrono::seconds recachePostDuartion = chrono::seconds(2);


};

