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
    addBook("9780123618013", "Twenty Thousand Leagues Under the Sea", 1872, 4.75, {"Jules Verne"}, {"Adventure"});
    addBook("9780123618014", "Around the World in Eighty Days", 1873, 4, {"Jules Verne"}, {"Adventure"});
    addBook("9780123618017", "Journey to the Center of the Earth", 1871, 5, {"Jules Verne"}, {"Fiction", "Science Fiction", "Adventure"});
    addBook("9780978824693", "Rich Dad Poor Dad", 2000, 1.618, {"Robert Kiyosaki"}, {"Self-Help", "Finance"});
    addBook("9781439167346", "How to Win Friends and Influence People", 1937, 3, {"Dale Carnegie"}, {"Self-Help"});
    addBook("9780671035975", "How to Stop Worrying and Start Living", 1948, 4.3, {"Dale Carnegie"}, {"Self-Help"});
    addBook("9780525501220", "The Four", 2017, 4, {"Scott Galloway"}, {"Self-Help", "Business"});
    addBook("9780973922311", "Ion", 1920, 5, {"Liviu Rebreanu"}, {"Novel"});
    addBook("9786069335505", "Baltagul", 1930, 4.4, {"Mihail Sadoveanu"}, {"Novel"});
    addBook("9786186186186", "Gareth and the Strange UFO", 2022, 5, {"Gareth", "Oracolul"}, {"Fiction", "Science Fiction", "Adventure", "Fantasy"});
    addBook("9786186186185", "The Phantom of the Opera", 1910, 4, {"Gaston Leroux"}, {"Fiction", "Gothic Fiction"});
    addBook("9786186186184", "Pride and Prejudice", 1813, 4, {"Jane Austen"}, {"Novel", "Romance"});
    addBook("9786186186183", "Alone in the World", 1878, 4, {"Hector Malot"}, {"Novel"});
    addBook("9786186186182", "The Invisible Man", 1897, 4, {"Herbert George Wells"}, {"Novel", "Horror", "Fiction", "Science Fiction"});
    addGenreEdge("Science", "Computer Science");
    addGenreEdge("Science", "Physics");
    addGenreEdge("Science", "Astronomy");
    addGenreEdge("Self-Help", "Finance");
    addGenreEdge("Self-Help", "Business");
    addGenreEdge("Novel", "Horror");
    addGenreEdge("Novel", "Romance");
    addGenreEdge("Fiction", "Fantasy");
    addGenreEdge("Fiction", "Gothic Fiction");
    addGenreEdge("Fiction", "Science Fiction");
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
