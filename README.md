## ReadsProfiler

Initialize database:

```sh
cd Init && qmake Init.pro && make && ./Init.app/Contents/MacOS/Init
```

Run server:

```sh
cd Server && qmake Server.pro && make && ./Server.app/Contents/MacOS/Server
```

Run client:

```sh
cd Client && g++ -std=c++17 -Wall -Wextra main.cpp -o main.o && ./main.o
```
