#include <unistd.h>
#include <termios.h>
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

const string CURSOR_ON = "\e[?25h";
const string CURSOR_OFF = "\e[?25l";
const string RED = "\x1B[31m";
const string GREEN = "\x1B[32m";
const string YELLOW = "\x1B[33m";
const string BLUE = "\x1B[34m";
const string PURPLE = "\x1B[35m";
const string DEFAULT = "\033[0m";
const string BOLD = "\e[1m";
const string NORMAL = "\e[0m";

void printLogo() {
    system("clear");
    cout << RED;
    cout << "  ____                _     ____             __ _ _           \n";
    cout << " |  _ \\ ___  __ _  __| |___|  _ \\ _ __ ___  / _(_) | ___ _ __ \n";
    cout << " | |_) / _ \\/ _` |/ _` / __| |_) | '__/ _ \\| |_| | |/ _ \\ '__|\n";
    cout << " |  _ <  __/ (_| | (_| \\__ \\  __/| | | (_) |  _| | |  __/ |   \n";
    cout << " |_| \\_\\___|\\__,_|\\__,_|___/_|   |_|  \\___/|_| |_|_|\\___|_|   \n\n";
    cout << DEFAULT;
}

void printBook(tuple<string, string, vector<string>, vector<string>, int, double>& book) {
    auto& [isbn, title, authors, genres, year, rating] = book;
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
    cout << BOLD << BLUE << "isbn: " << DEFAULT << NORMAL << isbn << '\n';
}

char getKey() {
    char buf = 0;
    termios old = {};
    tcgetattr(0, &old);
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &old);
    read(0, &buf, 1);
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    tcsetattr(0, TCSADRAIN, &old);
    return buf;
}

string getArrow() {
    deque<int> dq;
    while (true) {
        const char key = getKey();
        if (key == '\n') return "ENTER";
        if (key == 127) return "BACKSPACE";
        dq.push_back(key);
        if (dq.size() > 3)
            dq.pop_front();
        if (dq.size() == 3) {
            if (dq[0] == 27 && dq[1] == 91 && dq[2] == 65) return "UP";
            if (dq[0] == 27 && dq[1] == 91 && dq[2] == 66) return "DOWN";
            if (dq[0] == 27 && dq[1] == 91 && dq[2] == 67) return "RIGHT";
            if (dq[0] == 27 && dq[1] == 91 && dq[2] == 68) return "LEFT";
        }
    }
    return "";
}
