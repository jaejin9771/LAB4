#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void parse_query_string(const char *query, double *num1, double *num2, char *op) {
    char *token;
    char query_copy[1024];
    strcpy(query_copy, query);
    
    token = strtok(query_copy, "&");
    while (token) {
        if (strncmp(token, "num1=", 5) == 0)
            *num1 = atof(token + 5);
        else if (strncmp(token, "num2=", 5) == 0)
            *num2 = atof(token + 5);
        else if (strncmp(token, "op=", 3) == 0)
            *op = token[3];
        
        token = strtok(NULL, "&");
    }
}

int main() {
    char *query_string = getenv("QUERY_STRING");
    double num1 = 0, num2 = 0, result = 0;
    char op = '+';
    
    printf("Content-type: text/html\n\n");
    printf("<!DOCTYPE html>\n"
           "<html>\n"
           "<head>\n"
           "<title>계산기</title>\n"
           "<style>\n"
           "body { font-family: Arial, sans-serif; max-width: 600px; margin: 0 auto; padding: 20px; }\n"
           ".calculator { background-color: #f0f0f0; padding: 20px; border-radius: 5px; }\n"
           ".result { font-size: 1.2em; margin: 20px 0; padding: 10px; background-color: #fff; }\n"
           "</style>\n"
           "</head>\n"
           "<body>\n"
           "<h1>간단한 계산기</h1>\n"
           "<div class=\"calculator\">\n"
           "<form method=\"GET\" action=\"/cgi-bin/calculator\">\n"
           "<p>첫 번째 숫자: <input type=\"number\" step=\"any\" name=\"num1\" required></p>\n"
           "<p>연산자: \n"
           "<select name=\"op\">\n"
           "<option value=\"+\">+</option>\n"
           "<option value=\"-\">-</option>\n"
           "<option value=\"*\">×</option>\n"
           "<option value=\"/\">÷</option>\n"
           "</select></p>\n"
           "<p>두 번째 숫자: <input type=\"number\" step=\"any\" name=\"num2\" required></p>\n"
           "<p><input type=\"submit\" value=\"계산\"></p>\n"
           "</form>\n");
    
    if (query_string && strlen(query_string) > 0) {
        parse_query_string(query_string, &num1, &num2, &op);
        
        printf("<div class=\"result\">\n"
               "<strong>계산 결과:</strong><br>\n"
               "%.2f %c %.2f = ", num1, op, num2);
        
        switch (op) {
            case '+': result = num1 + num2; break;
            case '-': result = num1 - num2; break;
            case '*': result = num1 * num2; break;
            case '/':
                if (num2 != 0) result = num1 / num2;
                else {
                    printf("오류: 0으로 나눌 수 없습니다.");
                    printf("</div>\n</div>\n</body>\n</html>\n");
                    return 0;
                }
                break;
        }
        
        printf("%.2f\n", result);
        printf("</div>\n");
    }
    
    printf("</div>\n"
           "</body>\n"
           "</html>\n");
    
    return 0;
}