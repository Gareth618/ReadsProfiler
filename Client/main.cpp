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
    const string password = getpass((BOLD + GREEN + "password: " + DEFAULT + NORMAL).c_str());
    cout << CURSOR_OFF;
    const string result = signIn(username, password);
    if (result == "username") {
        cout << RED << "User " << username << " doesn't exist!\n" << DEFAULT;
        getKey();
        return screenSignIn();
    }
    if (result == "password") {
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
    const int idSearch = updateHistory(username, isbn, title, year, rating, author, genre);

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
            updateDownloads(idSearch, get<0>(results[index]));
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
    auto results = getRecommendations(username);
    function<void(int)> screenResult = [&](int index) {
        printLogo();
        cout << BOLD << BLUE << "You May Like… 💡\n\n" << DEFAULT << NORMAL;
        printBook(results[index]);
        cout << '\n' << index + 1 << '/' << results.size() << '\n';
        const string arrow = getArrow();
        if (arrow == "ENTER")
            return screenResult(index);
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
    cout << BOLD << BLUE << "Explore Genres 📚\n\n" << DEFAULT << NORMAL;
    auto tree = getGenreHierarchy();
    auto& roots = tree.first;
    auto& edges = tree.second;
    function<void(string, int)> dfs = [&](string node, int tab) {
        for (int i = 0; i < tab; i++)
            cout << ' ';
        cout << YELLOW << "- " << DEFAULT << node << '\n';
        for (string son : edges[node])
            dfs(son, tab + 2);
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
