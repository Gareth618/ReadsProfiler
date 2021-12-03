#include "cli.hpp"
#include "server.hpp"
#include <signal.h>

string username;
QSqlDatabase db;

void screenSignIn();
void screenHome(int);
void screenSearchBook();
void screenYouMayLike();
void screenExploreGenres();
void screenExploreAuthors();

void screenSignIn() {
    printLogo();
    cout << CURSOR_ON;
    cout << BOLD << GREEN << "username: " << DEFAULT << NORMAL; getline(cin, username);
    string password = getpass((BOLD + GREEN + "password: " + DEFAULT + NORMAL).c_str());
    cout << CURSOR_OFF;
    QSqlQuery query;
    query.prepare("SELECT password FROM users WHERE username = ?");
    query.addBindValue(username.c_str());
    query.exec();
    if (!query.next()) {
        cout << RED << "User " << username << " doesn't exist!\n" << DEFAULT;
        getKey();
        return screenSignIn();
    }
    if (password != query.value(0).toString().toStdString()) {
        cout << RED << "Wrong password!\n" << DEFAULT;
        getKey();
        return screenSignIn();
    }
    return screenHome(0);
}

void screenHome(int item) {
    printLogo();
    vector<pair<string, function<void()>>> items = {
        make_pair("search book", [&]() { screenSearchBook(); screenHome(item); }),
        make_pair("you may like…", [&]() { screenYouMayLike(); screenHome(item); }),
        make_pair("explore genres", [&]() { screenExploreGenres(); screenHome(item); }),
        make_pair("explore authors", [&]() { screenExploreAuthors(); screenHome(item); }),
        make_pair("sign-out", []() { system("clear"); })
    };
    for (int i = 0; i < int(items.size()); i++) {
        const auto [str, fun] = items[i];
        if (i == item) cout << BOLD << GREEN;
        cout << '[' << (i == item ? 'x' : ' ') << "] " << str << '\n';
        if (i == item) cout << DEFAULT << NORMAL;
    }
    const string arrow = getArrow();
    if (item < int(items.size()) - 1 && arrow == "DOWN")
        return screenHome(item + 1);
    if (item > 0 && arrow == "UP")
        return screenHome(item - 1);
    if (arrow == "ENTER")
        return items[item].second();
    return screenHome(item);
}

void screenSearchBook() {
    printLogo();
    cout << BOLD << BLUE << "Search Book 🔍\n" << DEFAULT << NORMAL;
    cout << CURSOR_ON;
    cout << BOLD << GREEN << "isbn: " << DEFAULT << NORMAL; string isbn; getline(cin, isbn);
    cout << BOLD << GREEN << "title: " << DEFAULT << NORMAL; string title; getline(cin, title);
    cout << BOLD << GREEN << "author: " << DEFAULT << NORMAL; string author; getline(cin, author);
    cout << BOLD << GREEN << "genre: " << DEFAULT << NORMAL; string genre; getline(cin, genre);
    cout << BOLD << GREEN << "year: " << DEFAULT << NORMAL; string yearStr; getline(cin, yearStr);
    cout << BOLD << GREEN << "rating: " << DEFAULT << NORMAL; string ratingStr; getline(cin, ratingStr);
    cout << CURSOR_OFF;
    int year = 0; try { year = stoi(yearStr); } catch (invalid_argument exc) { }
    double rating = 0; try { rating = stod(ratingStr); } catch (invalid_argument exc) { }

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

    vector<string> isbns; if (!isbn.empty()) isbns.push_back(isbn);
    vector<string> titles; if (!title.empty()) titles.push_back(title);
    vector<string> authors; if (!author.empty()) authors.push_back(author);
    vector<string> genres; if (!genre.empty()) genres.push_back(genre);
    vector<int> years; if (year) years.push_back(year);
    vector<double> ratings; if (rating) ratings.push_back(rating);

    auto results = getBooks(isbns, titles, authors, genres, years, ratings);
    if (results.empty()) {
        cout << RED << "Your query didn't return any results!\n" << DEFAULT;
        getKey();
        return;
    }
    function<void(int)> screenResult = [&](int index) {
        printLogo();
        cout << BOLD << BLUE << "Search Book 🔍\n\n" << DEFAULT << NORMAL;
        printBook(results[index]);
        cout << '\n' << index + 1 << '/' << results.size() << '\n';
        const string arrow = getArrow();
        if (arrow == "ENTER") {
            QSqlQuery query;
            query.prepare("INSERT INTO downloads VALUES (?, ?)");
            query.addBindValue(idSearch);
            query.addBindValue(get<0>(results[index]).c_str());
            query.exec();
            return screenResult(index);
        }
        if (arrow == "BACKSPACE")
            return;
        if (arrow == "RIGHT" && index < int(results.size()) - 1)
            return screenResult(index + 1);
        if (arrow == "LEFT" && index > 0)
            return screenResult(index - 1);
        return screenResult(index);
    };
    return screenResult(0);
}

void screenYouMayLike() {
    QSqlQuery query;
    vector<string> isbns;
    query.prepare("SELECT h.isbn FROM users u JOIN history h ON u.username = h.username JOIN books b ON h.isbn = b.isbn WHERE username = ?");
    query.addBindValue(username.c_str());
    query.exec();
    while (query.next())
        isbns.push_back(query.value(0).toString().toStdString());
    vector<string> titles;
    query.prepare("SELECT LOWER(h.title) FROM users u JOIN history h ON u.username = h.username JOIN books b ON LOWER(h.title) = LOWER(b.title) WHERE username = ?");
    query.addBindValue(username.c_str());
    query.exec();
    while (query.next())
        titles.push_back(query.value(0).toString().toStdString());
    vector<string> authors;
    query.prepare("SELECT LOWER(h.author) FROM users u JOIN history h ON u.username = h.username JOIN books b ON LOWER(h.author) = LOWER(b.author) WHERE username = ?");
    query.addBindValue(username.c_str());
    query.exec();
    while (query.next())
        authors.push_back(query.value(0).toString().toStdString());
    vector<string> genres;
    query.prepare("SELECT LOWER(h.genre) FROM users u JOIN history h ON u.username = h.username JOIN books b ON LOWER(h.genre) = LOWER(b.genre) WHERE username = ?");
    query.addBindValue(username.c_str());
    query.exec();
    while (query.next())
        genres.push_back(query.value(0).toString().toStdString());
    vector<int> years;
    query.prepare("SELECT h.year FROM users u JOIN history h ON u.username = h.username JOIN books b ON h.year = b.year WHERE username = ?");
    query.addBindValue(username.c_str());
    query.exec();
    while (query.next())
        years.push_back(query.value(0).toInt());
    vector<double> ratings;
    query.prepare("SELECT h.rating FROM users u JOIN history h ON u.username = h.username JOIN books b ON ABS(h.rating - b.rating) < .25 WHERE username = ?");
    query.addBindValue(username.c_str());
    query.exec();
    while (query.next())
        ratings.push_back(query.value(0).toDouble());

    auto results1 = getBooks(vector<string>(), vector<string>(), authors, genres, vector<int>(), vector<double>());
    auto results2 = getBooks(isbns, titles, vector<string>(), vector<string>(), years, ratings);
    auto results3 = getBooks(get5isbns(), vector<string>(), vector<string>(), vector<string>(), vector<int>(), vector<double>());
    for (int i = 0; results1.size() < 5 && i < int(results2.size()); i++)
        results1.push_back(results2[i]);
    for (int i = 0; results1.size() < 5 && i < int(results3.size()); i++)
        results1.push_back(results3[i]);

    function<void(int)> screenResult = [&](int index) {
        printLogo();
        cout << BOLD << BLUE << "You may like… 💡\n\n" << DEFAULT << NORMAL;
        printBook(results1[index]);
        cout << '\n' << index + 1 << '/' << results1.size() << '\n';
        const string arrow = getArrow();
        if (arrow == "ENTER")
            return screenResult(index);
        if (arrow == "BACKSPACE")
            return;
        if (arrow == "RIGHT" && index < int(results1.size()) - 1)
            return screenResult(index + 1);
        if (arrow == "LEFT" && index > 0)
            return screenResult(index - 1);
        return screenResult(index);
    };
    return screenResult(0);
}

void screenExploreGenres() {
    printLogo();
    cout << BOLD << BLUE << "Explore Genres 📚\n\n" << DEFAULT << NORMAL;
    auto tree = getGenreHierarchy();
    auto& roots = tree.first;
    auto& edges = tree.second;
    function<void(string, int)> dfs = [&](string fath, int tab) {
        for (int i = 0; i < tab; i++)
            cout << ' ';
        cout << YELLOW << "- " << DEFAULT << fath << '\n';
        for (string node : edges[fath])
            dfs(node, tab + 2);
    };
    for (string root : roots)
        dfs(root, 0);
    getKey();
}

void screenExploreAuthors() {
    printLogo();
    cout << BOLD << BLUE << "Explore Authors ✍️\n" << DEFAULT << NORMAL;
    cout << BOLD << GREEN << "author: " << DEFAULT << NORMAL;
    cout << CURSOR_ON;
    string author;
    getline(cin, author);
    cout << CURSOR_OFF;
    auto authorGenres = getAuthorGenres(author);
    author = authorGenres.first;
    auto& genres = authorGenres.second;
    if (genres.empty()) {
        cout << RED << "Author " << author << " doesn't exist!\n" << DEFAULT;
        getKey();
        return;
    }
    cout << '\n' << BOLD << YELLOW << author << " prefers the following genres:\n" << DEFAULT << NORMAL;
    for (string genre : genres)
        cout << YELLOW << "- " << DEFAULT << genre << '\n';
    getKey();
}

int main() {
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("/Users/gareth618/Desktop/ReadsProfiler/db.sqlite");
    signal(SIGINT, [](int) {
        db.close();
        cout << DEFAULT;
        cout << NORMAL;
        cout << CURSOR_ON;
        system("clear");
        exit(0);
    });
    db.open();
    cout << CURSOR_OFF;
    screenSignIn();
    cout << CURSOR_ON;
    db.close();
    return 0;
}
