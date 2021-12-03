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
    query.prepare("SELECT g.name FROM authors a "
        "JOIN book_author ba ON a.id_author = ba.id_author "
        "JOIN books b ON ba.isbn = b.isbn "
        "JOIN book_genre bg ON b.isbn = bg.isbn "
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
    query.prepare("SELECT a.name FROM book_author ba JOIN authors a ON ba.id_author = a.id_author WHERE isbn = ?");
    query.addBindValue(isbn.c_str());
    query.exec();
    vector<string> authors;
    while (query.next())
        authors.push_back(query.value(0).toString().toStdString());
    return authors;
}

vector<string> getBookGenres(string isbn) {
    QSqlQuery query;
    query.prepare("SELECT g.name FROM book_genre bg JOIN genres g ON bg.id_genre = g.id_genre WHERE isbn = ?");
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

vector<string> get5isbns() {
    QSqlQuery query("SELECT isbn FROM books LIMIT 5");
    vector<string> isbns;
    while (query.next())
        isbns.push_back(query.value(0).toString().toStdString());
    return isbns;
}
