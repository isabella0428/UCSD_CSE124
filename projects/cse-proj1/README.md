## CSE124 Proj1  Building a Web Server

#### Overview

At a high level, the web server that I built listens for connections on a socket (bound to a specific address and port on a host machine). Clients connect to this socket and use the [TritonHTTP](https://cseweb.ucsd.edu/~gmporter/classes/wi19/cse124/post/2019/01/12/tritonhttp-specification/) protocol to retrieve files from the server



#### Characteristics

1. ##### Response adopts TritionHTTP routine

   ```
   // Example response
   HTTP/1.1 200 OK<CR><LF>
   
   Server: Myserver 1.0<CR><LF>
   
   Last-Modified: Sun, 19 Aug 18 18:02:49 -0700<CR><LF>
   
   Content-Length: 12812<CR><LF>
   
   Content-Type: image/jpeg<CR><LF>
   
   <CR><LF>
   ```

* Support MIME types

  ```
  Content-Type: image/jpeg<CR><LF>
  ```

  

* Error Response

  ```
  "403":	No file read permission
  "404":	Other errors, file doesn't exist etc
  "200":	Success
  ```



2. ##### Support all kinds of files

   In the sample_htocs, we have html, jpg and png files

   

3. ##### Multi-thread processing

   Use c++11 thread library.

   

4. ##### Automatically download data file to specified path

   In the sample client, I set the file name accepted from the server to be the same as the one from the server.



#### Environment

c++11



#### Run

```
1. Enter progrom folder
	 cd src
1. build the files
	 make
2. In one terminal, run the server code
	 ./httpd myconfig.ini
3. In the other terminal, run the client code
	 ./client
```

##### Server:

```
(base) ➜  src git:(master) ✗ ./httpd myconfig.ini 
[11:41:23.268] [info] [thread 28199912] Web server enabled
[11:41:23.271] [info] [thread 28199912] Launching web server
[11:41:23.271] [info] [thread 28199912] Port: 8080
[11:41:23.271] [info] [thread 28199912] doc_root: ../
[11:41:23.271] [info] [thread 28199912] Socket created
[11:41:23.272] [info] [thread 28199912] Socket Binded
[11:41:23.272] [info] [thread 28199912] Start Listening
[11:41:27.018] [info] [thread 28199968] Start Processing Request
[11:41:27.038] [info] [thread 28199968] Finish Transmitting File
[11:42:02.612] [info] [thread 28200534] Start Processing Request
```

##### Client:

```
(base) ➜  src git:(master) ✗ ./client   
Client side
Socket Created
Connected to Server
Start receiving response
----------------------
Response received: 
HTTP/1.1 200 OK
Server: Myserver 1.0
Filename: UCSD_Seal.png
Last modified: Mon, 03 Feb 20 11:42:02 +0800
Content-Length: 637155
Content-Type: image/png

File saved to: /Users/isabella/Desktop/MOOCS/UCSD_CSE124/project/cse-proj1/download/UCSD_Seal.png
```



