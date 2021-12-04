#include <set>
#include <QtSql>
using namespace std;

pair<set<string>, map<string, vector<string>>> getGenreHierarchy() {
    QSqlQuery query;
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

pair<string, vector<string>> getAuthorGenres(string author) {
    QSqlQuery query;
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

vector<string> getBookAuthors(string isbn) {
    QSqlQuery query;
    query.prepare("SELECT a.name FROM book_author ba JOIN authors a ON ba.id_author = a.id_author WHERE ba.isbn = ?");
    query.addBindValue(isbn.c_str());
    query.exec();
    vector<string> authors;
    while (query.next())
        authors.push_back(query.value(0).toString().toStdString());
    return authors;
}

vector<string> getBookGenres(string isbn) {
    QSqlQuery query;
    query.prepare("SELECT g.name FROM book_genre bg JOIN genres g ON bg.id_genre = g.id_genre WHERE bg.isbn = ?");
    query.addBindValue(isbn.c_str());
    query.exec();
    vector<string> genres;
    while (query.next())
        genres.push_back(query.value(0).toString().toStdString());
    return genres;
}

vector<tuple<string, string, vector<string>, vector<string>, int, double>> getBooks(
    vector<string> isbns,
    vector<string> titles,
    vector<string> authors,
    vector<string> genres,
    vector<int> years,
    vector<double> ratings
) {
    QSqlQuery query;
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
                    getBookAuthors(isbn),
                    getBookGenres(isbn),
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

vector<string> get5isbns(string username) {
    QSqlQuery query;
    query.prepare("SELECT isbn FROM books EXCEPT SELECT d.isbn FROM history h JOIN downloads d ON h.id_search = d.id_search WHERE h.username = ? LIMIT 5");
    query.addBindValue(username.c_str());
    query.exec();
    vector<string> isbns;
    while (query.next())
        isbns.push_back(query.value(0).toString().toStdString());
    return isbns;
}

string signIn(string username, string password) {
    QSqlQuery query;
    query.prepare("SELECT password FROM users WHERE username = ?");
    query.addBindValue(username.c_str());
    query.exec();
    if (!query.next())
        return "username";
    if (password != query.value(0).toString().toStdString())
        return "password";
    return "ok";
}

int updateHistory(string username, string isbn, string title, int year, double rating, string author, string genre) {
    QSqlQuery query("SELECT COUNT(*) FROM history");
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

void updateDownloads(int idSearch, string isbn) {
    QSqlQuery query;
    query.prepare("INSERT INTO downloads VALUES (?, ?)");
    query.addBindValue(idSearch);
    query.addBindValue(isbn.c_str());
    query.exec();
}

vector<tuple<string, string, vector<string>, vector<string>, int, double>> getRecommendations(string username) {
    QSqlQuery query;
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
    auto results1 = getBooks(vector<string>(), vector<string>(), authors, genres, vector<int>(), vector<double>());
    auto results2 = getBooks(isbns, titles, vector<string>(), vector<string>(), years, ratings);
    auto results3 = getBooks(get5isbns(username), vector<string>(), vector<string>(), vector<string>(), vector<int>(), vector<double>());
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
