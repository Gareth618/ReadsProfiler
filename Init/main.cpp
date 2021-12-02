#include <QtSql>
using namespace std;

void createTables() {
    QString createBooks = (
        "CREATE TABLE books ("
        "  isbn CHAR(13) NOT NULL PRIMARY KEY,"
        "  title VARCHAR(100) NOT NULL,"
        "  year INTEGER NOT NULL,"
        "  rating NUMBER"
        ")"
    );
    QString createAuthors = (
        "CREATE TABLE authors ("
        "  id_author INTEGER NOT NULL PRIMARY KEY,"
        "  name VARCHAR(50) NOT NULL"
        ")"
    );
    QString createGenres = (
        "CREATE TABLE genres ("
        "  id_genre INTEGER NOT NULL PRIMARY KEY,"
        "  name VARCHAR(50) NOT NULL,"
        "  parent INTEGER"
        ")"
    );
    QString createBookAuthor = (
        "CREATE TABLE book_author ("
        "  isbn CHAR(13) NOT NULL,"
        "  id_author INTEGER NOT NULL"
        ")"
    );
    QString createBookGenre = (
        "CREATE TABLE book_genre ("
        "  isbn CHAR(13) NOT NULL,"
        "  id_genre INTEGER NOT NULL"
        ")"
    );
    QString createUsers = (
        "CREATE TABLE users ("
        "  username VARCHAR(50) NOT NULL PRIMARY KEY,"
        "  password VARCHAR(50) NOT NULL"
        ")"
    );
    QString createHistory = (
        "CREATE TABLE history ("
        "  id_search INTEGER NOT NULL PRIMARY KEY,"
        "  username VARCHAR(50) NOT NULL,"
        "  isbn CHAR(13),"
        "  title VARCHAR(100),"
        "  year INTEGER,"
        "  rating NUMBER,"
        "  author VARCHAR(50),"
        "  genre VARCHAR(50)"
        ")"
    );
    QString createDownloads = (
        "CREATE TABLE downloads ("
        "  id_search INTEGER NOT NULL,"
        "  isbn CHAR(13) NOT NULL"
        ")"
    );
    QSqlQuery query;
    query.exec("DROP TABLE books");
    query.exec("DROP TABLE authors");
    query.exec("DROP TABLE genres");
    query.exec("DROP TABLE book_author");
    query.exec("DROP TABLE book_genre");
    query.exec("DROP TABLE users");
    query.exec("DROP TABLE history");
    query.exec("DROP TABLE downloads");
    query.exec(createBooks);
    query.exec(createAuthors);
    query.exec(createGenres);
    query.exec(createBookAuthor);
    query.exec(createBookGenre);
    query.exec(createUsers);
    query.exec(createHistory);
    query.exec(createDownloads);
}

void addUser(string username, string password) {
    QSqlQuery query;
    query.prepare("INSERT INTO users VALUES (?, ?)");
    query.addBindValue(username.c_str());
    query.addBindValue(password.c_str());
    query.exec();
}

int getIdAuthor(string author) {
    QSqlQuery query;
    query.prepare("SELECT id_author FROM authors WHERE name = ?");
    query.addBindValue(author.c_str());
    query.exec();
    if (query.next())
        return query.value(0).toInt();
    query.exec("SELECT COUNT(*) FROM authors");
    query.next();
    const int id = query.value(0).toInt() + 1;
    query.prepare("INSERT INTO authors VALUES (?, ?)");
    query.addBindValue(id);
    query.addBindValue(author.c_str());
    query.exec();
    return id;
}

int getIdGenre(string genre) {
    QSqlQuery query;
    query.prepare("SELECT id_genre FROM genres WHERE name = ?");
    query.addBindValue(genre.c_str());
    query.exec();
    if (query.next())
        return query.value(0).toInt();
    query.exec("SELECT COUNT(*) FROM genres");
    query.next();
    const int id = query.value(0).toInt() + 1;
    query.prepare("INSERT INTO genres VALUES (?, ?, NULL)");
    query.addBindValue(id);
    query.addBindValue(genre.c_str());
    query.exec();
    return id;
}

void addBook(string isbn, string title, int year, double rating, vector<string> authors, vector<string> genres) {
    QSqlQuery query;
    query.prepare("INSERT INTO books VALUES (?, ?, ?, ?)");
    query.addBindValue(isbn.c_str());
    query.addBindValue(title.c_str());
    query.addBindValue(year);
    query.addBindValue(rating);
    query.exec();
    for (string author : authors) {
        query.prepare("INSERT INTO book_author VALUES (?, ?)");
        query.addBindValue(isbn.c_str());
        query.addBindValue(getIdAuthor(author));
        query.exec();
    }
    for (string genre : genres) {
        query.prepare("INSERT INTO book_genre VALUES (?, ?)");
        query.addBindValue(isbn.c_str());
        query.addBindValue(getIdGenre(genre));
        query.exec();
    }
}

void addGenreEdge(string genre, string subgenre) {
    QSqlQuery query;
    query.prepare("UPDATE genres SET parent = ? WHERE name = ?");
    query.addBindValue(getIdGenre(genre));
    query.addBindValue(subgenre.c_str());
    query.exec();
}

void populateDatabase() {
    addUser("gareth618", "wu-tang");
    addUser("bunul20", "lol4life");
    addUser("juve45", "icpc_wf");
    addUser("denis2111", "treeGCD");
    addUser("bogdan4tz", "pcr.social");
    addBook("9780262033848", "Introduction to Algorithms", 2009, 4.5, {"Thomas Cormen", "Charles Leiserson", "Ronald Rivest", "Clifford Stein"}, {"Science", "Computer Science"});
    addBook("9780201896831", "The Art of Computer Programming", 1968, 4, {"Donald Knuth"}, {"Science", "Computer Science"});
    addBook("9781473695993", "Brief Answers to the Big Questions", 2018, 5, {"Stephen Hawking"}, {"Science", "Physics", "Astronomy"});
    addGenreEdge("Science", "Computer Science");
    addGenreEdge("Science", "Physics");
    addGenreEdge("Science", "Astronomy");
}

int main() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("/Users/gareth618/Desktop/ReadsProfiler/db.sqlite");
    db.open();
    createTables();
    populateDatabase();
    db.close();
    return 0;
}
