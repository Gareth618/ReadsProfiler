#include <QtSql>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <bits/stdc++.h>

using std::cin;
using std::cout;
using std::string;
using std::vector;
using std::deque;
using std::set;
using std::map;
using std::pair;
using std::tuple;
using std::to_string;
using std::stoi;
using std::stod;
using std::function;
using std::get;
using std::make_pair;
using std::invalid_argument;

vector<string> getBookAuthors(QSqlDatabase& db, string isbn) {
    QSqlQuery query(db);
    query.prepare("SELECT a.name FROM book_author ba JOIN authors a ON ba.id_author = a.id_author WHERE ba.isbn = ?");
    query.addBindValue(isbn.c_str());
    query.exec();
    vector<string> authors;
    while (query.next())
        authors.push_back(query.value(0).toString().toStdString());
    return authors;
}

vector<string> getBookGenres(QSqlDatabase& db, string isbn) {
    QSqlQuery query(db);
    query.prepare("SELECT g.name FROM book_genre bg JOIN genres g ON bg.id_genre = g.id_genre WHERE bg.isbn = ?");
    query.addBindValue(isbn.c_str());
    query.exec();
    vector<string> genres;
    while (query.next())
        genres.push_back(query.value(0).toString().toStdString());
    return genres;
}

vector<tuple<string, string, vector<string>, vector<string>, int, double>> getBooks(
    QSqlDatabase& db,
    vector<string> isbns,
    vector<string> titles,
    vector<string> authors,
    vector<string> genres,
    vector<int> years,
    vector<double> ratings
) {
    QSqlQuery query(db);
    map<string, tuple<string, vector<string>, vector<string>, int, double, int>> results;
    auto search = [&]() {
        query.exec();
        while (query.next()) {
            const string isbn = query.value(0).toString().toStdString();
            if (results.count(isbn))
                get<5>(results[isbn])++;
            else
                results[isbn] = make_tuple(
                    query.value(1).toString().toStdString(),
                    getBookAuthors(db, isbn),
                    getBookGenres(db, isbn),
                    query.value(2).toInt(),
                    query.value(3).toDouble(),
                    1
                );
        }
    };

    for (string isbn : isbns) {
        query.prepare("SELECT * FROM books WHERE isbn = ?");
        query.addBindValue(isbn.c_str());
        search();
    }
    for (string title : titles) {
        query.prepare("SELECT * FROM books WHERE LOWER(title) = LOWER(?)");
        query.addBindValue(title.c_str());
        search();
    }
    for (string author : authors) {
        query.prepare("SELECT b.isbn, b.title, b.year, b.rating FROM books b JOIN book_author ba ON b.isbn = ba.isbn JOIN authors a ON ba.id_author = a.id_author WHERE LOWER(a.name) = LOWER(?)");
        query.addBindValue(author.c_str());
        search();
    }
    for (string genre : genres) {
        query.prepare("SELECT b.isbn, b.title, b.year, b.rating FROM books b JOIN book_genre bg ON b.isbn = bg.isbn JOIN genres g ON bg.id_genre = g.id_genre WHERE LOWER(g.name) = LOWER(?)");
        query.addBindValue(genre.c_str());
        search();
    }
    for (int year : years) {
        query.prepare("SELECT * FROM books WHERE year = ?");
        query.addBindValue(year);
        search();
    }
    for (double rating : ratings) {
        query.prepare("SELECT * FROM books WHERE ABS(rating - ?) < .25");
        query.addBindValue(rating);
        search();
    }

    vector<pair<int, string>> order;
    for (auto& [isbn, book] : results)
        order.emplace_back(get<5>(book), isbn);
    sort(order.rbegin(), order.rend());

    vector<tuple<string, string, vector<string>, vector<string>, int, double>> books;
    for (auto [score, isbn] : order)
        books.emplace_back(
            isbn,
            get<0>(results[isbn]),
            get<1>(results[isbn]),
            get<2>(results[isbn]),
            get<3>(results[isbn]),
            get<4>(results[isbn])
        );
    return books;
}

vector<string> get5isbns(QSqlDatabase& db, string username) {
    QSqlQuery query(db);
    query.prepare("SELECT isbn FROM books EXCEPT SELECT d.isbn FROM history h JOIN downloads d ON h.id_search = d.id_search WHERE h.username = ? LIMIT 5");
    query.addBindValue(username.c_str());
    query.exec();
    vector<string> isbns;
    while (query.next())
        isbns.push_back(query.value(0).toString().toStdString());
    return isbns;
}

int updateHistory(QSqlDatabase& db, string username, string isbn, string title, int year, double rating, string author, string genre) {
    QSqlQuery query("SELECT COUNT(*) FROM history", db);
    query.next();
    const int idSearch = query.value(0).toInt() + 1;
    query.prepare((
        string("INSERT INTO history VALUES (?, ?, ")
        + (isbn.empty() ? "NULL, " : "?, ")
        + (title.empty() ? "NULL, " : "?, ")
        + (!year ? "NULL, " : "?, ")
        + (!rating ? "NULL, " : "?, ")
        + (author.empty() ? "NULL, " : "?, ")
        + (genre.empty() ? "NULL" : "?")
        + ")"
    ).c_str());
    query.addBindValue(idSearch);
    query.addBindValue(username.c_str());
    if (!isbn.empty()) query.addBindValue(isbn.c_str());
    if (!title.empty()) query.addBindValue(title.c_str());
    if (year) query.addBindValue(year);
    if (rating) query.addBindValue(rating);
    if (!author.empty()) query.addBindValue(author.c_str());
    if (!genre.empty()) query.addBindValue(genre.c_str());
    query.exec();
    return idSearch;
}

const int PORT = 8080;

struct Thread {
    int thread;
    int client;
};

string signIn(QSqlDatabase& db, string username, string password) {
    QSqlQuery query(db);
    query.prepare("SELECT password FROM users WHERE username = ?");
    query.addBindValue(username.c_str());
    query.exec();
    if (!query.next())
        return "un";
    if (password != query.value(0).toString().toStdString())
        return "pw";
    return "ok";
}

pair<int, vector<tuple<string, string, vector<string>, vector<string>, int, double>>> searchBook(QSqlDatabase& db, string username, string isbn, string title, string author, string genre, int year, double rating) {
    const int idSearch = updateHistory(db, username, isbn, title, year, rating, author, genre);
    vector<string> isbns; if (!isbn.empty()) isbns.push_back(isbn);
    vector<string> titles; if (!title.empty()) titles.push_back(title);
    vector<string> authors; if (!author.empty()) authors.push_back(author);
    vector<string> genres; if (!genre.empty()) genres.push_back(genre);
    vector<int> years; if (year) years.push_back(year);
    vector<double> ratings; if (rating) ratings.push_back(rating);
    return make_pair(idSearch, getBooks(db, isbns, titles, authors, genres, years, ratings));
}

void updateDownloads(QSqlDatabase& db, int idSearch, string isbn) {
    QSqlQuery query(db);
    query.prepare("INSERT INTO downloads VALUES (?, ?)");
    query.addBindValue(idSearch);
    query.addBindValue(isbn.c_str());
    query.exec();
}

vector<tuple<string, string, vector<string>, vector<string>, int, double>> youMayLike(QSqlDatabase& db, string username) {
    QSqlQuery query(db);
    query.prepare("SELECT d.isbn FROM users u JOIN history h ON u.username = h.username JOIN downloads d ON h.id_search = d.id_search WHERE u.username = ?");
    query.addBindValue(username.c_str());
    query.exec();
    set<string> used;
    while (query.next())
        used.insert(query.value(0).toString().toStdString());

    vector<string> isbns;
    query.prepare("SELECT h.isbn FROM users u JOIN history h ON u.username = h.username WHERE u.username = ? AND h.isbn IS NOT NULL");
    query.addBindValue(username.c_str());
    query.exec();
    while (query.next())
        isbns.push_back(query.value(0).toString().toStdString());
    vector<string> titles;
    query.prepare("SELECT h.title FROM users u JOIN history h ON u.username = h.username WHERE u.username = ? AND h.title IS NOT NULL");
    query.addBindValue(username.c_str());
    query.exec();
    while (query.next())
        titles.push_back(query.value(0).toString().toStdString());
    vector<string> authors;
    query.prepare("SELECT h.author FROM users u JOIN history h ON u.username = h.username WHERE u.username = ? AND h.author IS NOT NULL");
    query.addBindValue(username.c_str());
    query.exec();
    while (query.next())
        authors.push_back(query.value(0).toString().toStdString());
    vector<string> genres;
    query.prepare("SELECT h.genre FROM users u JOIN history h ON u.username = h.username WHERE u.username = ? AND h.genre IS NOT NULL");
    query.addBindValue(username.c_str());
    query.exec();
    while (query.next())
        genres.push_back(query.value(0).toString().toStdString());
    vector<int> years;
    query.prepare("SELECT h.year FROM users u JOIN history h ON u.username = h.username WHERE u.username = ? AND h.year IS NOT NULL");
    query.addBindValue(username.c_str());
    query.exec();
    while (query.next())
        years.push_back(query.value(0).toInt());
    vector<double> ratings;
    query.prepare("SELECT h.rating FROM users u JOIN history h ON u.username = h.username WHERE u.username = ? AND h.rating IS NOT NULL");
    query.addBindValue(username.c_str());
    query.exec();
    while (query.next())
        ratings.push_back(query.value(0).toDouble());

    vector<tuple<string, string, vector<string>, vector<string>, int, double>> results;
    auto results1 = getBooks(db, vector<string>(), vector<string>(), authors, genres, vector<int>(), vector<double>());
    auto results2 = getBooks(db, isbns, titles, vector<string>(), vector<string>(), years, ratings);
    auto results3 = getBooks(db, get5isbns(db, username), vector<string>(), vector<string>(), vector<string>(), vector<int>(), vector<double>());
    for (int i = 0; results.size() < 5 && i < int(results1.size()); i++)
        if (!used.count(get<0>(results1[i]))) {
            results.push_back(results1[i]);
            used.insert(get<0>(results1[i]));
        }
    for (int i = 0; results.size() < 5 && i < int(results2.size()); i++)
        if (!used.count(get<0>(results2[i]))) {
            results.push_back(results2[i]);
            used.insert(get<0>(results2[i]));
        }
    for (int i = 0; results.size() < 5 && i < int(results3.size()); i++)
        if (!used.count(get<0>(results3[i]))) {
            results.push_back(results3[i]);
            used.insert(get<0>(results3[i]));
        }
    return results;
}

pair<set<string>, map<string, vector<string>>> exploreGenres(QSqlDatabase& db) {
    QSqlQuery query(db);
    query.exec("SELECT name FROM genres");
    set<string> roots;
    while (query.next())
        roots.insert(query.value(0).toString().toStdString());
    query.exec("SELECT g1.name, g2.name FROM genres g1 JOIN genres g2 ON g2.parent = g1.id_genre");
    map<string, vector<string>> edges;
    while (query.next()) {
        const string fath = query.value(0).toString().toStdString();
        const string node = query.value(1).toString().toStdString();
        edges[fath].push_back(node);
        roots.erase(node);
    }
    return make_pair(roots, edges);
}

pair<string, vector<string>> exploreAuthor(QSqlDatabase& db, string author) {
    QSqlQuery query(db);
    query.prepare("SELECT name FROM authors WHERE LOWER(name) = LOWER(?)");
    query.addBindValue(author.c_str());
    query.exec();
    if (query.next())
        author = query.value(0).toString().toStdString();
    query.prepare("SELECT DISTINCT g.name FROM authors a "
        "JOIN book_author ba ON a.id_author = ba.id_author "
        "JOIN book_genre bg ON ba.isbn = bg.isbn "
        "JOIN genres g ON bg.id_genre = g.id_genre "
    "WHERE a.name = ?");
    query.addBindValue(author.c_str());
    query.exec();
    vector<string> genres;
    while (query.next())
        genres.push_back(query.value(0).toString().toStdString());
    return make_pair(author, genres);
}

string answer(QSqlDatabase& db, string type, string request) {
    int index = 0;
    auto next = [&]() {
        string str;
        while (index < int(request.size()) && request[index] != '\n')
            str += request[index++];
        index++;
        return str;
    };

    auto encodeBookVector = [](vector<tuple<string, string, vector<string>, vector<string>, int, double>> books) {
        string str = to_string(books.size());
        for (auto& [isbn, title, authors, genres, year, rating] : books) {
            str += "\n" + isbn + "\n" + title;
            str += "\n" + to_string(authors.size());
            for (string author : authors)
                str += "\n" + author;
            str += "\n" + to_string(genres.size());
            for (string genre : genres)
                str += "\n" + genre;
            str += "\n" + to_string(year);
            str += "\n" + to_string(rating);
        }
        return str;
    };

    if (type == "si") {
        const string username = next();
        const string password = next();
        return signIn(db, username, password);
    }
    if (type == "sb") {
        const string username = next();
        const string isbn = next();
        const string title = next();
        const string author = next();
        const string genre = next();
        const int year = stoi(next());
        const double rating = stod(next());
        auto [idSearch, books] = searchBook(db, username, isbn, title, author, genre, year, rating);
        return to_string(idSearch) + "\n" + encodeBookVector(books);
    }
    if (type == "ud") {
        const int idSearch = stoi(next());
        const string isbn = next();
        updateDownloads(db, idSearch, isbn);
        return "";
    }
    if (type == "ml") {
        const string username = next();
        return encodeBookVector(youMayLike(db, username));
    }
    if (type == "eg") {
        auto [genres, tree] = exploreGenres(db);
        string str = to_string(genres.size());
        for (string genre : genres)
            str += "\n" + genre;
        vector<pair<string, string>> edges;
        for (auto& [node, nghbs] : tree)
            for (string nghb : nghbs)
                edges.emplace_back(node, nghb);
        str += "\n" + to_string(edges.size());
        for (auto [node, nghb] : edges)
            str += "\n" + node + "\n" + nghb;
        return str;
    }
    if (type == "ea") {
        auto [author, genres] = exploreAuthor(db, next());
        string str = author + "\n" + to_string(genres.size());
        for (string genre : genres)
            str += "\n" + genre;
        return str;
    }
    return "";
}

void answer(Thread thread) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", to_string(thread.thread).c_str());
    db.setDatabaseName("/Users/gareth618/Desktop/ReadsProfiler/db.sqlite");
    db.open();
    char buff[4096];
    while (true) {
        cout << "[thread " << thread.thread << "] waiting for request\n";
        int length;
        read(thread.client, &length, sizeof(int));
        char type[3];
        read(thread.client, type, 2);
        type[2] = '\0';
        read(thread.client, buff, length);
        buff[length] = '\0';
        if (!strcmp(type, "so")) break;
        if (!strcmp(type, "db")) {
            const int fd = open(("/Users/gareth618/Desktop/ReadsProfiler/Books/" + string(buff) + ".txt").c_str(), O_RDONLY);
            char buff[4096];
            while (true) {
                const int bytes = read(fd, buff, 4096);
                write(thread.client, &bytes, sizeof(int));
                if (!bytes) break;
                write(thread.client, buff, bytes);
            }
            continue;
        }
        cout << "[thread " << thread.thread << "] request received\n";
        const string ans = answer(db, type, buff);
        length = ans.size();
        write(thread.client, &length, sizeof(int));
        write(thread.client, ans.c_str(), length);
        cout << "[thread " << thread.thread << "] answer sent\n";
    }
    db.close();
}

void* treat(void* arg) {
    pthread_detach(pthread_self());
    answer(*((Thread*)arg));
    close(intptr_t(arg));
    return nullptr;
}

int main() {
    signal(SIGINT, [](int) {
        system("clear");
        exit(0);
    });

    sockaddr_in server;
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);

    const int sd = socket(AF_INET, SOCK_STREAM, 0);
    int on = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    bind(sd, (sockaddr*)&server, sizeof(sockaddr));
    listen(sd, 2);

    vector<pthread_t> threads;
    while (true) {
        cout << "[server] listening at port " << PORT << '\n';
        sockaddr_in client;
        bzero(&client, sizeof(client));
        socklen_t size = sizeof(client);
        Thread *thread = new Thread;
        thread->thread = threads.size();
        thread->client = accept(sd, (sockaddr*)&client, &size);
        threads.emplace_back();
        pthread_create(&threads.back(), nullptr, &treat, thread);
    }
}
