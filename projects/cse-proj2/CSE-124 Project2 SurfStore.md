## CSE-124	Project2	SurfStore

### Overview

In this project, I built a cloud-based file storage system called SurfStore. SurfStore is a networked file storage application, and lets you sync file to and from the “cloud”. I implement the cloud service, and a client which interacts with your service via RPC. 



### Specification

#### File Storage

<img src="CSE-124 Project2 SurfStore.assets/image-20200206125840647.png">

In the cloud, I split the file into fixed-sized chunks. Files are stored in these way in the cloud, not as a whole. 

We use library **picosha2** to generate hash value of different file chunks and keep hashvalue list of a single file in a map alone with the filename. So that the client can find the corresponding data chunks hash value list with the filename.



#### Client

Each client has a unique path to store their files, which is specified in different **ini** files. 

We also have an index file in each client's basic directory, which stores filename, version(the last time synced), and also the hash value which specifies the content when last synced.  

Compare index file and local directory file content, we can determine if we have local modifications.



#### File version

Each file/filename is associated with a *version*, which is a monotonically increasing non-negative integer. The version is incremented any time the file is created, modified, or deleted. The purpose of the version is so that clients can detect when they have an out-of-date view of the file hierarchy.

For example, imagine that Tahani wants to update a spreadsheet file that tracks conference room reservations. Ideally, they would perform the following actions:

<img src="CSE-124 Project2 SurfStore.assets/image-20200206130643929.png">



If we have version conflict, first sync first wins.

More specification to tick [here](https://cseweb.ucsd.edu/~gmporter/classes/wi19/cse124/projects/pa2/).



#### File format

All binary data should work fine. I tested .txt, .png, .jpg, .html.



### Sync conditions

I sum up 6 different conditions that may occur and their solutions.

| State                                      | Local Directory | Local Index File                                     | Remote Index           | Action                                                       |
| ------------------------------------------ | --------------- | ---------------------------------------------------- | ---------------------- | ------------------------------------------------------------ |
| New file in the cloud                      | Not exist       | Not exist                                            | Exist                  | 1. Download file     2. Reconstitute file locally 3. Set local index |
| Create new file locally                    | Exist           | Not exist                                            | Not exist              | 1. Upload data block 2. Update remote index 3. Update local index |
| New modification of existing file remotely | Exist           | Exist                                                | Exist (higher version) | 1. Download blocks 2. Reconstitue file locally 3. Set local index |
| Have illegal local modification            | Exist           | Exist (local version is not equal to remote version) | Exist                  | failed to commit, download blocks like when there is a new file in the cloud |
| Have legal local modification              | Exist           | Exist (local version is equal to remote version)     | Exist                  | Commit file, like when u are  creating a file                |
| Delete file locally                        | Not exist       | Exist                                                | Exist                  | 1. Update remote index 2. Set local index                    |



#### Environment

* Dependency

  * inih
  * json
  * picosha2
  * rpc
  * spdlog

* Run time environment

  c++11



#### Run

1. Get into the target folder

   ```
   cd src
   ```

2. Make file

   ```
   make
   ```

3. Run server

   ```
   ./ssd myconfig1.ini
   ```

4. Run client1

   ```
   ./ss myconfig1.ini
   ```

5. Run client2

   ```
   ./ss myconfig2.ini
   ```

   



