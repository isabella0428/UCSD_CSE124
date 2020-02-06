#ifndef SURFSTORECLIENT_HPP
#define SURFSTORECLIENT_HPP

#include <string>
#include <list>


#include "inih/INIReader.h"
#include "rpc/client.h"

#include "logger.hpp"
#include "SurfStoreTypes.hpp"

using namespace std;

class SurfStoreClient {
public:
    SurfStoreClient(INIReader& t_config);
    ~SurfStoreClient();

	void sync(); 						// sync the base_dir with the cloud
	void upload_file(string);			// update a new local file
	void delete_file(string);			// delete a file

	const uint64_t RPC_TIMEOUT = 100; 	// milliseconds

protected:

    INIReader& config;
	string serverhost;
	int serverport;
	string base_dir;
	int blocksize;
	unordered_map<std::string, std::string> block_map;

	rpc::client * c;

	// helper functions to get/set from the local index file
	FileInfo get_local_fileinfo(string filename);
	void set_local_fileinfo(string filename, FileInfo finfo);

	// helper functions to get/set blocks to/from local files
	list<string> get_blocks_from_file(string filename);

	// helper functions to update local file from the server
	void download_from_server(string filename);

	// helper function to get all filenames from local base directory/local index file
	vector<string> scanDir();
	vector<string> filenames_from_local_index();
};

#endif // SURFSTORECLIENT_HPP
