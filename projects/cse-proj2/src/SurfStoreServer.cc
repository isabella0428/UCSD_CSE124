#include <sysexits.h>
#include <string>
#include <unordered_map>

#include "rpc/server.h"

#include "logger.hpp"
#include "SurfStoreTypes.hpp"
#include "SurfStoreServer.hpp"

SurfStoreServer::SurfStoreServer(INIReader& t_config)
    : config(t_config)
{
    auto log = logger();

	// pull the address and port for the server
	string servconf = config.Get("ssd", "server", "");
	if (servconf == "") {
		log->error("Server line not found in config file");
		exit(EX_CONFIG);
	}
	size_t idx = servconf.find(":");
	if (idx == string::npos) {
		log->error("Config line {} is invalid", servconf);
		exit(EX_CONFIG);
	}
	port = strtol(servconf.substr(idx+1).c_str(), nullptr, 0);
	if (port <= 0 || port > 65535) {
		log->error("The port provided is invalid: {}", servconf);
		exit(EX_CONFIG);
	}
}

void SurfStoreServer::launch()
{
    auto log = logger();

    log->info("Launching SurfStore server");
    log->info("Port: {}", port);

	rpc::server srv(port);

	srv.bind("ping", []() {
		auto log = logger();
		log->info("ping()");
		return;
	});

	//TODO: get a block for a specific hash
	srv.bind("get_block", [&](string hash) {
		(void) hash; // prevent warning (remove before submitting)

		auto log = logger();
		log->info("get_block()");

		auto it = block_store.find(hash);
		if (it == block_store.end()) {
			log->error("Given hash doesn't exist in block_store");
			return string();
		}

		return it->second;
	});

	// TODO: store a block
	srv.bind("store_block", [&](string hash, string data){
		(void) hash; // prevent warning (remove before submitting)
		(void) data; // prevent warning (remove before submitting)

		auto log = logger();
		log->info("store_block()");

		block_store.insert(make_pair<>(hash, data));
		return;
	});

	//TODO: download a FileInfo Map from the server
	srv.bind("get_fileinfo_map", [&]() {
		auto log = logger();
		log->info("get_fileinfo_map()");

		// FileInfo file1;
		// get<0>(file1) = 42;
		// get<1>(file1) = { "h0", "h1", "h2" };

		// FileInfo file2;
		// get<0>(file2) = 20;
		// get<1>(file2) = { "h3", "h4" };

		// FileInfoMap fmap;
		// fmap["file1.txt"] = file1;
		// fmap["file2.dat"] = file2;
		return fmap;
	});

	//TODO: update the FileInfo entry for a given file
	srv.bind("update_file", [&](string filename, FileInfo finfo) {
		(void) filename; // prevent warning (remove before submitting)
		(void) finfo; // prevent warning (remove before submitting)

		std::map<string, FileInfo>::iterator it = fmap.find(filename);

		// The file never exist
		if(it == fmap.end()) {
			if (get<0>(finfo) != 1) {
				log->error("Update file refused!");
			}
			fmap.insert(std::make_pair<>(filename, finfo));
			return 0;
		}

		// Check if the version is correct
		int old_v = get<0>(it->second);
		int new_v = get<0>(finfo);

		if (new_v != old_v + 1) {
			log->error("Update file refused! Old version: {} New version{}", old_v, new_v);
			return -1;
		}

		// No matter whether it is a tombstone or a regular one
		fmap.erase(filename);
		fmap.insert(std::make_pair<>(filename, finfo));
		return 0;
	});

	// You may add additional RPC bindings as necessary

	srv.run();
}
