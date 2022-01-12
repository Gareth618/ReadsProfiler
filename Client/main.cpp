#include "cli.hpp"
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

const string ADDRESS = "127.0.0.1";
const int PORT = 8080;

int sd;
string username;

class Tokenizer {
    int index;
    vector<string> tokens;
public:
    Tokenizer(string str) : index(0), tokens(1) {
        for (char chr : str)
            if (chr == '\n')
                tokens.emplace_back();
            else
                tokens.back() += chr;
    }
    string next() {
        return tokens[index++];
    }
};

string signIn(string username, string password) {
    string str = username + "\n" + password;
    int length = str.size();
    write(sd, &length, sizeof(int));
    const string type = "si";
    write(sd, type.c_str(), 2);
    write(sd, str.c_str(), length);
    char buff[4096];
    read(sd, &length, sizeof(int));
    read(sd, buff, length);
    buff[length] = '\0';
    str = buff;
    Tokenizer tk(str);
    return tk.next();
}

pair<int, vector<tuple<string, string, vector<string>, vector<string>, int, double>>> searchBook(string username, string isbn, string title, string author, string genre, int year, double rating) {
    string str = username + "\n" + isbn + "\n" + title + "\n" + author + "\n" + genre + "\n" + to_string(year) + "\n" + to_string(rating);
    int length = str.size();
    write(sd, &length, sizeof(int));
    const string type = "sb";
    write(sd, type.c_str(), 2);
    write(sd, str.c_str(), length);
    char buff[4096];
    read(sd, &length, sizeof(int));
    read(sd, buff, length);
    buff[length] = '\0';
    str = buff;
    Tokenizer tk(str);
    pair<int, vector<tuple<string, string, vector<string>, vector<string>, int, double>>> result;
    result.first = stoi(tk.next());
    result.second.resize(stoi(tk.next()));
    for (auto& [isbn, title, authors, genres, year, rating] : result.second) {
        isbn = tk.next();
        title = tk.next();
        authors.resize(stoi(tk.next()));
        for (string& author : authors)
            author = tk.next();
        genres.resize(stoi(tk.next()));
        for (string& genre : genres)
            genre = tk.next();
        year = stoi(tk.next());
        rating = stod(tk.next());
    }
    return result;
}

void downloadBook(string title) {
    string str = title;
    int length = str.size();
    write(sd, &length, sizeof(int));
    const string type = "db";
    write(sd, type.c_str(), 2);
    write(sd, str.c_str(), length);
    const int fd = open(("/Users/gareth618/Desktop/ReadsProfiler/Downloads/" + title + ".txt").c_str(), O_WRONLY | O_CREAT, 0666);
    char buff[4096];
    while (true) {
        read(sd, &length, sizeof(int));
        if (!length) break;
        read(sd, buff, length);
        write(fd, buff, length);
    }
    close(fd);
}

void updateDownloads(int idSearch, string isbn) {
    string str = to_string(idSearch) + "\n" + isbn;
    int length = str.size();
    write(sd, &length, sizeof(int));
    const string type = "ud";
    write(sd, type.c_str(), 2);
    write(sd, str.c_str(), length);
    char buff[4096];
    read(sd, &length, sizeof(int));
    read(sd, buff, length);
    buff[length] = '\0';
    str = buff;
}

vector<tuple<string, string, vector<string>, vector<string>, int, double>> youMayLike(string username) {
    string str = username;
    int length = str.size();
    write(sd, &length, sizeof(int));
    const string type = "ml";
    write(sd, type.c_str(), 2);
    write(sd, str.c_str(), length);
    char buff[4096];
    read(sd, &length, sizeof(int));
    read(sd, buff, length);
    buff[length] = '\0';
    str = buff;
    Tokenizer tk(str);
    vector<tuple<string, string, vector<string>, vector<string>, int, double>> result;
    result.resize(stoi(tk.next()));
    for (auto& [isbn, title, authors, genres, year, rating] : result) {
        isbn = tk.next();
        title = tk.next();
        authors.resize(stoi(tk.next()));
        for (string& author : authors)
            author = tk.next();
        genres.resize(stoi(tk.next()));
        for (string& genre : genres)
            genre = tk.next();
        year = stoi(tk.next());
        rating = stod(tk.next());
    }
    return result;
}

pair<set<string>, map<string, vector<string>>> exploreGenres() {
    string str = "";
    int length = str.size();
    write(sd, &length, sizeof(int));
    const string type = "eg";
    write(sd, type.c_str(), 2);
    write(sd, str.c_str(), length);
    char buff[4096];
    read(sd, &length, sizeof(int));
    read(sd, buff, length);
    buff[length] = '\0';
    str = buff;
    Tokenizer tk(str);
    pair<set<string>, map<string, vector<string>>> result;
    const int n = stoi(tk.next());
    for (int i = 0; i < n; i++)
        result.first.insert(tk.next());
    const int m = stoi(tk.next());
    for (int i = 0; i < m; i++) {
        const string u = tk.next();
        const string v = tk.next();
        result.second[u].push_back(v);
    }
    return result;
}

pair<string, vector<string>> exploreAuthor(string author) {
    string str = author;
    int length = str.size();
    write(sd, &length, sizeof(int));
    const string type = "ea";
    write(sd, type.c_str(), 2);
    write(sd, str.c_str(), length);
    char buff[4096];
    read(sd, &length, sizeof(int));
    read(sd, buff, length);
    buff[length] = '\0';
    str = buff;
    Tokenizer tk(str);
    pair<string, vector<string>> result;
    result.first = tk.next();
    result.second.resize(stoi(tk.next()));
    for (string& genre : result.second)
        genre = tk.next();
    return result;
}

void screenSignIn();
void screenHome(int);
void screenSearchBook();
void screenYouMayLike();
void screenExploreGenres();
void screenExploreAuthor();

void screenSignIn() {
    printLogo();
    cout << CURSOR_ON;
    cout << BOLD << GREEN << "username: " << DEFAULT << NORMAL; getline(cin, username);
    const string password = getpass((BOLD + GREEN + "password: " + DEFAULT + NORMAL).c_str());
    cout << CURSOR_OFF;
    const string result = signIn(username, password);
    if (result == "un") {
        cout << RED << "User " << username << " doesn't exist!\n" << DEFAULT;
        getKey();
        return screenSignIn();
    }
    if (result == "pw") {
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
        make_pair("explore author", [&]() { screenExploreAuthor(); screenHome(item); }),
        make_pair("sign-out", []() { })
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

    int idSearch;
    vector<tuple<string, string, vector<string>, vector<string>, int, double>> results;
    tie(idSearch, results) = searchBook(username, isbn, title, author, genre, year, rating);
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
            downloadBook(get<1>(results[index]));
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
    auto results = youMayLike(username);
    function<void(int)> screenResult = [&](int index) {
        printLogo();
        cout << BOLD << BLUE << "You May Like… 💡\n\n" << DEFAULT << NORMAL;
        printBook(results[index]);
        cout << '\n' << index + 1 << '/' << results.size() << '\n';
        const string arrow = getArrow();
        if (arrow == "ENTER") {
            downloadBook(get<1>(results[index]));
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
    cout << BOLD << BLUE << "Explore Genres 📚\n\n" << DEFAULT << NORMAL;
    auto tree = exploreGenres();
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

void screenExploreAuthor() {
    printLogo();
    cout << BOLD << BLUE << "Explore Author ✍️\n" << DEFAULT << NORMAL;
    cout << BOLD << GREEN << "author: " << DEFAULT << NORMAL;
    cout << CURSOR_ON;
    string author;
    getline(cin, author);
    cout << CURSOR_OFF;
    auto authorGenres = exploreAuthor(author);
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
    sockaddr_in server;
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ADDRESS.c_str());
    server.sin_port = htons(PORT);
    sd = socket(AF_INET, SOCK_STREAM, 0);
    connect(sd, (sockaddr*)&server, sizeof(sockaddr));

    auto onClose = [](int) {
        cout << DEFAULT;
        cout << NORMAL;
        cout << CURSOR_ON;
        system("clear");
        const int length = 0;
        write(sd, &length, sizeof(int));
        const string type = "so";
        write(sd, type.c_str(), 2);
        close(sd);
        exit(0);
    };
    signal(SIGINT, onClose);
    cout << CURSOR_OFF;
    screenSignIn();
    onClose(0);
}
