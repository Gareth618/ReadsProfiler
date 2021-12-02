#include "cli.hpp"
#include "server.hpp"
#include <signal.h>

string username;
QSqlDatabase db;

void screenSignIn();
void screenHome(int item);
void screenSearchBook();
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
        make_pair("you may like…", [&]() { screenHome(item); }),
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
    double rating = 0; try { year = stod(ratingStr); } catch (invalid_argument exc) { }

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

    if (!isbn.empty()) {
        query.prepare("SELECT * FROM books WHERE isbn = ?");
        query.addBindValue(isbn.c_str());
        search();
    }
    if (!title.empty()) {
        query.prepare("SELECT * FROM books WHERE LOWER(title) = LOWER(?)");
        query.addBindValue(title.c_str());
        search();
    }
    if (!author.empty()) {
        query.prepare("SELECT b.isbn, b.title, b.year, b.rating FROM books b JOIN book_author ba ON b.isbn = ba.isbn JOIN authors a ON ba.id_author = a.id_author WHERE LOWER(a.name) = LOWER(?)");
        query.addBindValue(author.c_str());
        search();
    }
    if (!genre.empty()) {
        query.prepare("SELECT b.isbn, b.title, b.year, b.rating FROM books b JOIN book_genre bg ON b.isbn = bg.isbn JOIN genres g ON bg.id_genre = g.id_genre WHERE LOWER(g.name) = LOWER(?)");
        query.addBindValue(genre.c_str());
        search();
    }
    if (year) {
        query.prepare("SELECT * FROM books WHERE year = ?");
        query.addBindValue(year);
        search();
    }
    if (rating) {
        query.prepare("SELECT * FROM books WHERE ABS(rating - ?) < .25");
        query.addBindValue(rating);
        search();
    }

    if (results.empty()) {
        cout << RED << "Your query didn't return any results!\n" << DEFAULT;
        getKey();
        return;
    }
    vector<pair<int, string>> order;
    for (auto& [isbn, book] : results)
        order.emplace_back(get<5>(book), isbn);
    sort(order.rbegin(), order.rend());

    function<void(int)> screenResult = [&](int index) {
        printLogo();
        auto& [title, authors, genres, year, rating, score] = results[order[index].second];
        cout << BOLD << GREEN << title << DEFAULT << NORMAL;
        cout << PURPLE << " (" << year << ")\n" << DEFAULT;
        cout << YELLOW << rating << " rating " << DEFAULT;
        for (int i = 1; i <= rating; i++)
            cout << "⭐";
        cout << '\n';
        cout << BOLD << BLUE << "author" << (authors.size() == 1 ? "" : "s") << ": " << DEFAULT << NORMAL;
        for (int i = 0; i < int(authors.size()) - 1; i++)
            cout << authors[i] << ", ";
        cout << authors.back() << '\n';
        cout << BOLD << BLUE << "genre" << (genres.size() == 1 ? "" : "s") << ": " << DEFAULT << NORMAL;
        for (int i = 0; i < int(genres.size()) - 1; i++)
            cout << genres[i] << ", ";
        cout << genres.back() << '\n';
        cout << '\n' << index + 1 << '/' << results.size() << '\n';

        const string arrow = getArrow();
        if (arrow == "ENTER") {
            QSqlQuery query;
            query.prepare("INSERT INTO downloads VALUES (?, ?)");
            query.addBindValue(idSearch);
            query.addBindValue(order[index].second.c_str());
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

void screenExploreGenres() {
    printLogo();
    cout << BOLD << BLUE << "Explore Genres 📚\n" << DEFAULT << NORMAL;
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
