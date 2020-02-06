#include <sysexits.h>
#include <sys/stat.h>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <map>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <fcntl.h>

#include <sys/types.h>
#include <dirent.h>
#include <assert.h>


#include "rpc/server.h"
#include "picosha2/picosha2.h"

#include "logger.hpp"
#include "SurfStoreTypes.hpp"
#include "SurfStoreClient.hpp"

using namespace std;

SurfStoreClient::SurfStoreClient(INIReader& t_config)
    : config(t_config), c(nullptr)
{
    auto log = logger();

	// pull the address and port for the server
	string serverconf = config.Get("ssd", "server", "");
	if (serverconf == "") {
		log->error("The server was not found in the config file");
		exit(EX_CONFIG);
	}
	size_t idx = serverconf.find(":");
	if (idx == string::npos) {
		log->error("Config line {} is invalid", serverconf);
		exit(EX_CONFIG);
	}
	serverhost = serverconf.substr(0,idx);
	serverport = strtol(serverconf.substr(idx+1).c_str(), nullptr, 0);
	if (serverport <= 0 || serverport > 65535) {
		log->error("The port provided is invalid: {}", serverconf);
		exit(EX_CONFIG);
	}

  base_dir = config.Get("ss", "base_dir", "");
  blocksize = config.GetInteger("ss", "blocksize", 4096);

  log->info("Launching SurfStore client");
  log->info("Server host: {}", serverhost);
  log->info("Server port: {}", serverport);

	c = new rpc::client(serverhost, serverport);
  chdir(base_dir.c_str());
}

SurfStoreClient::~SurfStoreClient()
{
	if (c) {
		delete c;
		c = nullptr;
	}
}

void SurfStoreClient::sync() {
  auto log = logger();

  // Test if we are connected to the server
	log->info("Calling ping");
	c->call("ping");

  // TODO: check file deletion
  vector<string> local_filenames = scanDir();
  for (auto it = local_filenames.begin(); it != local_filenames.end(); ++it) {
    // Check if we have local modification (Compare file content and index)
    string local_filename = *it;
    log->info("Target file {}",local_filename);

    FileInfo local_index = get_local_fileinfo(local_filename);

    // The client create a new file locally --> Upload the file
    if (get<0>(local_index) == -1) {
      upload_file(local_filename);
    }

    // Compare hashes from file content and hashes from local index file
    char path[300];
    realpath(local_filename.c_str(), path);

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
      log->error("Sync failed: Cannot open file in client base_dir!");
      return;
    }
    char* buf = (char *)malloc(sizeof(char) * blocksize);
    list<string> file_hashkey;
    int num;
    while((num = read(fd, buf, blocksize)) != 0) {
      string data(&buf[0], num);
      string hash = picosha2::hash256_hex_string(data);
      file_hashkey.push_back(hash);
    }

    free(buf);

    list<string> index_hashkey = get<1>(get_local_fileinfo(local_filename));
    int result = (file_hashkey == index_hashkey);

    // The file is locally modified
    if (!result) {
      FileInfoMap fmap = c->call("get_fileinfo_map").as<FileInfoMap>();
      // Check if the version of the local index file is the same as the one of server
      int local_v = get<0>(get_local_fileinfo(local_filename));
      int remote_v = get<0>(fmap[local_filename]);

      // Okay to commit local change
      if (local_v == remote_v) {
        upload_file(local_filename);

      } else {
        download_from_server(local_filename);
      }
      continue;
    }

    // The file is not locally modified --> Check if we have higher version
    log->info("Check if there are new versions");
    FileInfoMap fmap = c->call("get_fileinfo_map").as<FileInfoMap>();
    int remote_v = get<0>(fmap[local_filename]);
    int local_v  = get<0>(get_local_fileinfo(local_filename));
    if (remote_v > local_v) {
      download_from_server(local_filename);
    }
  }

  // Check if there are files deleted locally
  vector<string> filenames_local_index = filenames_from_local_index();
  for(auto it = filenames_local_index.begin(); it != filenames_local_index.end(); ++it) {
    if (find(local_filenames.begin(), local_filenames.end(), *it) == local_filenames.end()) {
      // File is deleted locally
      list<string> hashes;
      hashes.push_back("0");

      int old_v = get<0>(get_local_fileinfo(*it));
      FileInfo new_finfo = make_tuple<>(old_v + 1, hashes);

      // Update remotely
      if(c->call("update_file", *it, new_finfo).as<int>() != 0) {
        download_from_server(*it);
      } else {
        set_local_fileinfo(*it, new_finfo);
      }
    }
  }

  // Check if there are new files remotely
  FileInfoMap fmap = c->call("get_fileinfo_map").as<FileInfoMap>();
  for (auto it = fmap.begin(); it != fmap.end(); ++it)
  {
    string remote_filename = it->first;
    FileInfo local_index = get_local_fileinfo(remote_filename);
    if (get<0>(local_index) == -1)
    {
      download_from_server(remote_filename);
    }
  }
}

FileInfo SurfStoreClient::get_local_fileinfo(string filename) {
  auto log = logger();
  log->info ("get_local_fileinfo {}", filename);

  char path[200];
  realpath("index.txt", path);

  ifstream f(path);
  if (f.fail()) {
    int v = -1;
  	list<string> blocklist;
  	FileInfo ret = make_tuple(v, list<string>());
    return ret;
  }
  do {
    vector<string> parts;
    string x;
    getline(f,x);
    stringstream ss(x);
    string tok;
    while(getline(ss, tok, ' ')) {
      parts.push_back(tok);
    }
    if (parts.size() > 0 && parts[0] == filename) {
      list<string> hl (parts.begin() + 2, parts.end());
      int v = stoi(parts[1]);
      return make_tuple(v,hl);
    }
  } while(!f.eof());
  int v = -1;
	list<string> blocklist;
	FileInfo ret = make_tuple(v, list<string>());
  return ret;
}

void SurfStoreClient::set_local_fileinfo(string filename, FileInfo finfo) {
  auto log = logger();
  log->info("set local file info");

  char old_index_path[200], new_index_path[200];
  realpath("index.txt", old_index_path);
  realpath("index.txt.new", new_index_path);

  std::ifstream f(old_index_path);
  std::ofstream out(new_index_path);
  int v = get<0>(finfo);
  list<string> hl = get<1>(finfo);
  bool set = false;
  do {
    string x;
    vector<string> parts; 
    getline(f,x);
    stringstream ss(x);
    string tok;
    while(getline(ss, tok, ' ')) {
      parts.push_back(tok);
    }
    if (parts.size() > 0) {
        if ( parts[0] == filename) {
          set = true;
          out << filename << " "<<v<<" ";
          for (auto it : hl) out<<it<<" ";
          out.seekp(-1,ios_base::cur);
          out <<"\n";
        }
        else {
          out << x<<"\n";
        }
      }
    else break;
  } while(!f.eof());
  if (!set) {
    out << filename <<" "<< v<< " ";
    for (auto it : hl) out<<it<<" ";
    out.seekp(-1,ios_base::cur);
    out <<"\n";
  }
  out.close();
  f.close();
  string real = string(old_index_path);
  string bkp = string(new_index_path);

  remove(real.c_str());
  rename(bkp.c_str(),real.c_str());
}

void SurfStoreClient::download_from_server(string filename){
  auto log = logger();

  log->info("Download file {}", filename);
  FileInfoMap fmap = c->call("get_fileinfo_map").as<FileInfoMap>();
  auto it = fmap.find(filename);
  if (it == fmap.end()) {
    log->error("Failed to download from server: Can't find file remotely");
    return;
  }

  char path[300];
  realpath(filename.c_str(), path);

  FileInfo remote_finfo = it->second;
  list<string> hashes = get<1>(remote_finfo);

  string hash0 = *hashes.begin();
  if ((hashes.size() == 1) && (hash0.compare("0") == 0))
  {
      // Delete file
    log->info("delete file {}", filename);
    remove(path);
    set_local_fileinfo(filename, remote_finfo);
    return;
  }

  int fd = open(path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd == -1) {
    log->error("Failed to download from server: Can't open or create the file");
    return;
  }

  // Reconstitute the file locally
  for(auto it = hashes.begin(); it != hashes.end(); ++it) {
    string hash = *it;
    string data = c->call("get_block", hash).as<string>();
    write(fd, data.c_str(), data.length());
  }

  // Update info in local index file
  set_local_fileinfo(filename, remote_finfo);
}

void SurfStoreClient::upload_file(string filename)
{
  auto log = logger();

  log->info("Upload file {}", filename);

  char path[300];
  realpath(filename.c_str(), path);

  int fd = open(path, O_RDONLY);
  char *buf = (char *)malloc(sizeof(char) * blocksize);
  list<string> hash_keys;
  int num;
  while ((num = read(fd, buf, blocksize)) > 0)
  {
    string data(&buf[0], num);
    string key = picosha2::hash256_hex_string(data);
    hash_keys.push_back(key);
    c->call("store_block", key, data); // Upload block data
  }

  // Upload remote index
  int v = get<0>(get_local_fileinfo(filename));
  if (v == -1) {
    v = 1;
  } else {
    ++v;
  }

  FileInfo new_finfo = make_tuple<>(v, hash_keys);
  if (c->call("update_file", filename, new_finfo).as<int>() != 0) {
    // We have a version conflict --> Download file version remotely
    download_from_server(filename);
  }
  free(buf);

  // Set local index
  set_local_fileinfo(filename, new_finfo);
}

vector<std::string> SurfStoreClient::scanDir()
{
  vector<std::string> filenames;
  FILE *proc = popen("/bin/ls", "r");
  char buf[1024];

  logger()->info("File scanning");
  while (!feof(proc) && fgets(buf, sizeof(buf), proc))
  {
    char *temp = strtok(buf, "\r\n");
    std::string filename = temp;
    if (strcmp(filename.c_str(), "index.txt") == 0) {
      continue;
    }
    logger()->info(filename);
    filenames.push_back(filename);
  }
  logger()->info("Finish file scanning");
  return filenames;
}

vector<string> SurfStoreClient::filenames_from_local_index() {
  char path[200];
  realpath("index.txt", path);
  
  if (access(path, F_OK) != 0) {
    vector<string> null_filenames;
    return null_filenames;
  }

  ifstream f(path);
  vector<string> filenames;
  do
  {
    vector<string> parts;
    string x;
    getline(f, x);
    stringstream ss(x);
    string tok;
    getline(ss, tok, ' ');
    if (tok != "\n") {
      parts.push_back(tok);
    }
    filenames.push_back(parts[0]);
  } while (!f.eof());
  filenames.pop_back();
  return filenames;
}
