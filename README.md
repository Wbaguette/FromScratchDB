# FromScratchDB

My implementation of a KV database using B+trees from scratch in C++, following this resource: https://build-your-own.org/database.

Goals of this project:
1. Familiarize myself with the underlying data structures and algorithms of databases. 
2. Develop my C++ design and programming skills.

View the `examples` directory to see usage.

### Compile and Run
1. Go to project root

2. Create build folder `mkdir build`
3. Configure `cmake -B build`
4. Build `cmake --build ./build`
5. Run `./build/db`
6. Subsequent builds and runs `cmake --build ./build && ./build/db`
