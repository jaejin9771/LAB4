#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/wait.h>
#include <ctype.h>

#define BUFFER_SIZE 8192
#define MAX_HEADERS 100
#define SERVER_PORT 8080
#define SERVER_ROOT "./www"  // 웹 루트 디렉토리
#define CGI_ROOT "./cgi-bin" // CGI 프로그램 디렉토리

// HTTP 헤더 구조체
typedef struct {
    char *name;
    char *value;
} HttpHeader;

// HTTP 요청 구조체
typedef struct {
    char method[10];
    char path[255];
    char query_string[255];
    char protocol[10];
    HttpHeader headers[MAX_HEADERS];
    int header_count;
    char *body;
    long content_length;
} HttpRequest;

// HTTP 응답 헤더 생성
void send_response_headers(int client_sock, int status_code, const char *content_type, long content_length) {
    char buffer[BUFFER_SIZE];
    char *status_text;
    
    switch(status_code) {
        case 200: status_text = "OK"; break;
        case 404: status_text = "Not Found"; break;
        case 500: status_text = "Internal Server Error"; break;
        default: status_text = "Unknown";
    }
    
    // 현재 시간 가져오기
    time_t now = time(NULL);
    char time_str[100];
    strftime(time_str, sizeof(time_str), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));
    
    // 응답 헤더 생성
    sprintf(buffer,
            "HTTP/1.1 %d %s\r\n"
            "Date: %s\r\n"
            "Server: SimpleWebServer/1.0\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %ld\r\n"
            "Connection: close\r\n"
            "\r\n",
            status_code, status_text,
            time_str,
            content_type,
            content_length);
    
    write(client_sock, buffer, strlen(buffer));
}

// URL 디코딩 함수
void url_decode(char *dst, const char *src) {
    char a, b;
    while (*src) {
        if (*src == '%' && ((a = src[1]) && (b = src[2])) && 
            isxdigit(a) && isxdigit(b)) {
            if (a >= 'a') a -= 'a'-'A';
            if (a >= 'A') a -= ('A' - 10);
            else a -= '0';
            if (b >= 'a') b -= 'a'-'A';
            if (b >= 'A') b -= ('A' - 10);
            else b -= '0';
            *dst++ = 16 * a + b;
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

// MIME 타입 결정
const char* get_mime_type(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot) return "text/plain";
    if (strcasecmp(dot, ".html") == 0 || strcasecmp(dot, ".htm") == 0) return "text/html";
    if (strcasecmp(dot, ".jpg") == 0 || strcasecmp(dot, ".jpeg") == 0) return "image/jpeg";
    if (strcasecmp(dot, ".png") == 0) return "image/png";
    if (strcasecmp(dot, ".css") == 0) return "text/css";
    if (strcasecmp(dot, ".js") == 0) return "application/javascript";
    return "text/plain";
}

// HTTP 요청 파싱
void parse_http_request(char *request_str, HttpRequest *request) {
    char *line = strtok(request_str, "\r\n");
    
    // 요청 라인 파싱
    sscanf(line, "%s %s %s", request->method, request->path, request->protocol);
    
    // URL에서 쿼리 스트링 분리
    char *query = strchr(request->path, '?');
    if (query) {
        *query = '\0';
        strcpy(request->query_string, query + 1);
    } else {
        request->query_string[0] = '\0';
    }
    
    // URL 디코딩
    char decoded_path[255];
    url_decode(decoded_path, request->path);
    strcpy(request->path, decoded_path);
    
    // 헤더 파싱
    request->header_count = 0;
    while ((line = strtok(NULL, "\r\n")) && strlen(line) > 0) {
        char *value = strchr(line, ':');
        if (value) {
            *value = '\0';
            value++;
            while (*value == ' ') value++;
            
            request->headers[request->header_count].name = strdup(line);
            request->headers[request->header_count].value = strdup(value);
            request->header_count++;
        }
    }
    
    // POST 요청인 경우 body 파싱
    request->body = NULL;
    request->content_length = 0;
    if (strcasecmp(request->method, "POST") == 0) {
        for (int i = 0; i < request->header_count; i++) {
            if (strcasecmp(request->headers[i].name, "Content-Length") == 0) {
                request->content_length = atol(request->headers[i].value);
                if (request->content_length > 0) {
                    request->body = strdup(line + 2);  // body는 빈 줄 다음에 시작
                }
                break;
            }
        }
    }
}

// CGI 프로그램 실행
void execute_cgi(int client_sock, HttpRequest *request) {
    char cgi_path[512];
    sprintf(cgi_path, "%s%s", CGI_ROOT, request->path);
    
    // CGI 환경변수 설정
    setenv("REQUEST_METHOD", request->method, 1);
    setenv("QUERY_STRING", request->query_string, 1);
    if (request->content_length > 0) {
        char content_length_str[16];
        sprintf(content_length_str, "%ld", request->content_length);
        setenv("CONTENT_LENGTH", content_length_str, 1);
    }
    
    int pipe_fd[2];
    if (pipe(pipe_fd) < 0) {
        perror("pipe");
        return;
    }
    
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }
    
    if (pid == 0) {  // 자식 프로세스
        close(pipe_fd[0]);  // 읽기 끝 닫기
        dup2(pipe_fd[1], STDOUT_FILENO);  // 표준 출력을 파이프로 리다이렉션
        
        execl(cgi_path, cgi_path, NULL);
        exit(1);
    } else {  // 부모 프로세스
        close(pipe_fd[1]);  // 쓰기 끝 닫기
        
        // CGI 출력을 클라이언트에게 전송
        char buffer[BUFFER_SIZE];
        ssize_t n;
        while ((n = read(pipe_fd[0], buffer, BUFFER_SIZE)) > 0) {
            write(client_sock, buffer, n);
        }
        
        close(pipe_fd[0]);
        waitpid(pid, NULL, 0);
    }
}

// 정적 파일 처리
void serve_file(int client_sock, const char *path) {
    char full_path[512];
    sprintf(full_path, "%s%s", SERVER_ROOT, path);
    
    // 기본 페이지 처리
    if (path[strlen(path)-1] == '/') {
        strcat(full_path, "index.html");
    }
    
    // 파일 열기
    int fd = open(full_path, O_RDONLY);
    if (fd < 0) {
        // 404 Not Found
        const char *not_found = "<html><body><h1>404 Not Found</h1></body></html>";
        send_response_headers(client_sock, 404, "text/html", strlen(not_found));
        write(client_sock, not_found, strlen(not_found));
        return;
    }
    
    // 파일 크기 확인
    struct stat st;
    fstat(fd, &st);
    
    // 응답 헤더 전송
    send_response_headers(client_sock, 200, get_mime_type(full_path), st.st_size);
    
    // 파일 내용 전송
    char buffer[BUFFER_SIZE];
    ssize_t n;
    while ((n = read(fd, buffer, BUFFER_SIZE)) > 0) {
        write(client_sock, buffer, n);
    }
    
    close(fd);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    
    // 소켓 생성
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        exit(1);
    }
    
    // 서버 주소 설정
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);
    
    // 소켓에 주소 바인딩
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(1);
    }
    
    // 연결 대기
    if (listen(server_sock, 5) < 0) {
        perror("listen");
        exit(1);
    }
    
    printf("Server started on port %d\n", SERVER_PORT);
    
    while (1) {
        // 클라이언트 연결 수락
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_sock < 0) {
            perror("accept");
            continue;
        }
        
        // HTTP 요청 수신
        ssize_t n = read(client_sock, buffer, BUFFER_SIZE-1);
        if (n < 0) {
            perror("read");
            close(client_sock);
            continue;
        }
        buffer[n] = '\0';
        
        // HTTP 요청 파싱
        HttpRequest request;
        parse_http_request(buffer, &request);
        
        printf("Request: %s %s %s\n", request.method, request.path, request.protocol);
        
        // CGI 요청 처리
        if (strncmp(request.path, "/cgi-bin/", 9) == 0) {
            execute_cgi(client_sock, &request);
        }
        // 정적 파일 요청 처리
        else {
            serve_file(client_sock, request.path);
        }
        
        // 연결 종료
        close(client_sock);
        
        // 메모리 해제
        for (int i = 0; i < request.header_count; i++) {
            free(request.headers[i].name);
            free(request.headers[i].value);
        }
        if (request.body) free(request.body);
    }
    
    close(server_sock);
    return 0;
}