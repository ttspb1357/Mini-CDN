#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <sstream>
#include <unordered_map>
#include <fstream>
#include <thread>
#include <arpa/inet.h>

using namespace std;

struct HttpRequest {
    string method;
    string path;
    string version;
    unordered_map<string, string> headers;
};

HttpRequest parseRequest(const string& raw) {
    HttpRequest req;
    
    // Split by \r\n to get lines
    stringstream ss(raw);
    string line;
    
    // First line: "GET /path HTTP/1.1"
    getline(ss, line, '\r');  // Read until \r
    ss.ignore(1);  // Skip the \n
    
    // Parse first line
    stringstream first_line(line);
    first_line >> req.method >> req.path >> req.version;
    
    // Remaining lines are headers
    while (getline(ss, line, '\r')) {
        ss.ignore(1);  // Skip \n
        
        if (line.empty()) break;  // Empty line = end of headers
        
        // Split header line by ":"
        size_t colon_pos = line.find(':');
        if (colon_pos != string::npos) {
            string key = line.substr(0, colon_pos);
            string value = line.substr(colon_pos + 2);  // +2 to skip ": "
            
            req.headers[key] = value;
        }
    }
    
    return req;
}

string getContentType(const string& filepath) {
    // Extract file extension
    size_t dot = filepath.rfind('.');
    if (dot == string::npos) {
        return "application/octet-stream";  // unknown type
    }
    
    string ext = filepath.substr(dot);
    
    // Map extensions to MIME types
    if (ext == ".html" || ext == ".htm") return "text/html";
    if (ext == ".css") return "text/css";
    if (ext == ".js") return "application/javascript";
    if (ext == ".json") return "application/json";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".png") return "image/png";
    if (ext == ".gif") return "image/gif";
    if (ext == ".svg") return "image/svg+xml";
    if (ext == ".txt") return "text/plain";
    if (ext == ".pdf") return "application/pdf";
    
    return "application/octet-stream";  // default for unknown
}

string serveFile(const string& path) {
    // Map requested path to actual file
    string filepath = "./public" + path;
    
    // Security: prevent path traversal attacks
    if (filepath.find("..") != string::npos) {
        return "HTTP/1.1 403 Forbidden\r\nContent-Type: text/html\r\nContent-Length: 18\r\n\r\n<h1>403 Forbidden</h1>";
    }
    
    // If path is just "/", serve index.html
    if (path.back() == '/') {
        filepath += "index.html";
    }
    
    string content_type = getContentType(filepath);

    // Try to open the file
    ifstream file(filepath, ios::binary);
    if (!file.is_open()) {
        // File not found
        string error_body = "<h1>404 Not Found</h1>";
        string response = "HTTP/1.1 404 Not Found\r\n";
        response += "Content-Type: " + content_type + "\r\n";
        response += "Content-Length: " + to_string(error_body.size()) + "\r\n";
        response += "\r\n";
        response += error_body;
        return response;
    }
    
    // Read entire file into memory
    string file_content;
    try {
        file_content = string((istreambuf_iterator<char>(file)), 
                            istreambuf_iterator<char>());
        file.close();
    } catch (const exception& e) {
        string error_body = "<h1>500 Server Error</h1>";
        string response = "HTTP/1.1 500 Internal Server Error\r\n";
        response += "Content-Type: " + content_type + "\r\n";
        response += "Content-Length: " + to_string(error_body.size()) + "\r\n";
        response += "\r\n";
        response += error_body;
        return response;
    }
    
    // Build successful response
    string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: " + content_type + "\r\n";
    response += "Content-Length: " + to_string(file_content.size()) + "\r\n";
    response += "\r\n";
    response += file_content;
    
    return response;
}

string fetchFromOrigin(const string& path, int origin_port = 8080) {
    int origin_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (origin_fd < 0) {
        cerr << "Failed to create socket for origin fetch" << endl;
        return "";
    }
    
    sockaddr_in origin_addr;
    origin_addr.sin_family = AF_INET;
    origin_addr.sin_port = htons(origin_port);
    origin_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    if (connect(origin_fd, (struct sockaddr*)&origin_addr, sizeof(origin_addr)) < 0) {
        cerr << "Failed to connect to origin server" << endl;
        close(origin_fd);
        return "";
    }
    
    string request = "GET " + path + " HTTP/1.1\r\nHost: localhost:8080\r\nConnection: close\r\n\r\n";
    write(origin_fd, request.c_str(), request.size());
    
    string response;
    char buffer[4096];
    int bytes;
    while ((bytes = read(origin_fd, buffer, 4096)) > 0) {
        response.append(buffer, bytes);
    }
    
    close(origin_fd);
    return response;
}

string getCachePath(const string& role, const string& filepath) {
    string path = filepath;
    
    // Normalize: "/" should become "/index.html" for caching
    if (path == "/" || path.empty()) {
        path = "/index.html";
    }
    // If path ends with "/", append index.html
    else if (path.back() == '/') {
        path += "index.html";
    }

    if (role == "edge_asia") {
        return "./cache_asia" + path;
    } else if (role == "edge_europe") {
        return "./cache_europe" + path;
    }
    return "";
}

bool isFileCached(const string& role, const string& filepath) {
    string cache_path = getCachePath(role, filepath);
    if (cache_path.empty()) return false;
    
    ifstream file(cache_path, ios::binary);
    return file.is_open();
}

void cacheFile(const string& role, const string& filepath, const string& content) {
    string cache_path = getCachePath(role, filepath);
    if (cache_path.empty()) return;
    
    size_t pos = cache_path.rfind('/');
    if (pos != string::npos) {
        string dir = cache_path.substr(0, pos);
        system(("mkdir -p " + dir).c_str());
    }
    
    ofstream file(cache_path, ios::binary);
    file << content;
    file.close();
    
    cout << "[" << role << "] Cached: " << filepath << endl;
}

string serveFromCache(const string& role, const string& filepath) {
    string cache_path = getCachePath(role, filepath);
    if (cache_path.empty()) return "";
    
    ifstream file(cache_path, ios::binary);
    if (!file.is_open()) return "";
    
    // Read entire file into memory
    string file_content;
    try {
        file_content = string((istreambuf_iterator<char>(file)), 
                            istreambuf_iterator<char>());
        file.close();
    } catch (const exception& e) {
        string error_body = "<h1>500 Server Error</h1>";
        string response = "HTTP/1.1 500 Internal Server Error\r\n";
        response += "Content-Type: " + getContentType(cache_path) + "\r\n";
        response += "Content-Length: " + to_string(error_body.size()) + "\r\n";
        response += "\r\n";
        response += error_body;
        return response;
    }
    
    // Build successful response
    string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: " + getContentType(cache_path) + "\r\n";
    response += "Content-Length: " + to_string(file_content.size()) + "\r\n";
    response += "\r\n";
    response += file_content;
    
    return response;
}

void handleClient(int client_fd, const string& role) {
    char buffer[4096] = {0};
    int bytes_read = read(client_fd, buffer, 4096);
    
    if (bytes_read < 0) {
        cerr << "Read failed" << endl;
        close(client_fd);
        return;
    }
    
    HttpRequest req = parseRequest(buffer);
    
    cout << "[" << role << "] " << req.method << " " << req.path << endl;
    
    string response;
    
    if (role == "edge_asia" || role == "edge_europe") 
    {
        if (isFileCached(role, req.path)) 
        {
            cout << "[" << role << "] Cache hit: " << req.path << endl;
            response = serveFromCache(role, req.path);
        } 
        else 
        {
            cout << "[" << role << "] Cache miss: " << req.path << " - fetching from origin" << endl;
            response = fetchFromOrigin(req.path, 8080);
            
            if (!response.empty() && response.find("HTTP/1.1 200 OK") != string::npos) {
                size_t body_start = response.find("\r\n\r\n");
                if (body_start != string::npos) {
                    body_start += 4;
                    string body = response.substr(body_start);
                    cacheFile(role, req.path, body);
                }
            } 
            else if(response.empty()) {
                response = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: "+ getContentType(req.path) + "\r\n\r\n<h1>500 Error</h1>";
            }
            else {
                response = "HTTP/1.1 404 Not Found\r\nContent-Type: "+ getContentType(req.path) + "\r\n\r\n<h1>404 Not Found</h1>";
            }
        }
    } 
    else {
        response = serveFile(req.path);
    }
    
    write(client_fd, response.c_str(), response.size());
    
    cout << "[" << role << "] Response sent" << endl;
    
    close(client_fd);
}

int main(int argc, char* argv[]) {
    int port = 8080;
    
    if (argc > 1) {
        port = stoi(argv[1]);
    }
    
    string role;
    if (port == 8080) {
        role = "origin";
    } else if (port == 8081) {
        role = "edge_asia";
    } else if (port == 8082) {
        role = "edge_europe";
    } else {
        role = "unknown";
    }
    
    cout << "Starting server on port " << port << " as " << role << endl;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (server_fd < 0) {
        cerr << "Socket creation failed" << endl;
        return 1;
    }
    
    // Enable port reuse immediately after socket creation
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        cerr << "setsockopt(SO_REUSEADDR) failed" << endl;
        close(server_fd);
        return 1;
    }
    
    cout << "Socket created successfully" << endl;
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);  // USE PORT VARIABLE
    
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        cerr << "Bind failed" << endl;
        return 1;
    }
    
    cout << "Socket bound to port " << port << endl;  // USE PORT VARIABLE
    
    if (listen(server_fd, 10) < 0) {
        cerr << "Listen failed" << endl;
        return 1;
    }
    
    cout << "Server listening on port " << port << "..." << endl;  // USE PORT VARIABLE
    
    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        
        if (client_fd < 0) {
            cerr << "Accept failed" << endl;
            continue;
        }
        
        cout << "Client connected" << endl;
        
        thread t(handleClient, client_fd, role);
        t.detach();
    }
    
    close(server_fd);
    return 0;
}