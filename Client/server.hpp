#include <set>
#include <QtSql>
using namespace std;

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
