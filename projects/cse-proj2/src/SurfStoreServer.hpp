#ifndef SURFSTORESERVER_HPP
#define SURFSTORESERVER_HPP

#include <unordered_map>
#include "inih/INIReader.h"
#include "logger.hpp"
#include "SurfStoreTypes.hpp"

using namespace std;

class SurfStoreServer {
public:
    SurfStoreServer(INIReader& t_config);

    void launch();

	const int NUM_THREADS = 8;

protected:
    INIReader& config;
	int port;
    map<string, string> block_store;
    FileInfoMap fmap;
};

#endif // SURFSTORESERVER_HPP
