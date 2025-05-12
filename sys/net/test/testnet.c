#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

#define BUFFER_SIZE 8192
#define MAX_PRODUCTS 100
#define MAX_URL_LENGTH 1024
#define MAX_TITLE_LENGTH 512
#define MAX_PRICE_LENGTH 64
#define MAX_DESCRIPTION_LENGTH 2048
#define MAX_IMAGES 10
#define MAX_IMAGE_URL_LENGTH 1024
#define USER_AGENT "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36"
#define HOST "www.daraz.pk"
#define PORT 443

/* Product structure */
typedef struct {
    char title[MAX_TITLE_LENGTH];
    char price[MAX_PRICE_LENGTH];
    char description[MAX_DESCRIPTION_LENGTH];
    char product_url[MAX_URL_LENGTH];
    char image_urls[MAX_IMAGES][MAX_IMAGE_URL_LENGTH];
    int image_count;
} Product;

/* Function prototypes */
int create_socket(const char* host, int port);
int send_request(int sockfd, const char* host, const char* path);
int receive_response(int sockfd, char* buffer, size_t buffer_size);
void parse_products(const char* html, Product* products, int* product_count);
void write_csv(const Product* products, int product_count, const char* filename);
char* extract_between(const char* src, const char* start_tag, const char* end_tag);
void cleanup_html(char* str);
void url_decode(char* str);
int establish_ssl_connection(const char* host, int port);
void extract_images(const char* html, Product* product);

/**
 * Main function
 */
int main() {
    int sockfd;
    char* buffer;
    size_t buffer_size = 1024 * 1024 * 5; // 5MB buffer for the HTML content
    Product products[MAX_PRODUCTS];
    int product_count = 0;
    
    /* Allocate buffer */
    buffer = (char*)malloc(buffer_size);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate memory\n");
        return EXIT_FAILURE;
    }
    memset(buffer, 0, buffer_size);
    
    printf("Initializing socket...\n");
    sockfd = establish_ssl_connection(HOST, PORT);
    if (sockfd < 0) {
        fprintf(stderr, "Failed to connect to %s:%d\n", HOST, PORT);
        free(buffer);
        return EXIT_FAILURE;
    }
    
    printf("Sending HTTP request...\n");
    if (send_request(sockfd, HOST, "/") <= 0) {
        fprintf(stderr, "Failed to send request\n");
        close(sockfd);
        free(buffer);
        return EXIT_FAILURE;
    }
    
    printf("Receiving response...\n");
    int bytes = receive_response(sockfd, buffer, buffer_size);
    if (bytes <= 0) {
        fprintf(stderr, "Failed to receive response\n");
        close(sockfd);
        free(buffer);
        return EXIT_FAILURE;
    }
    
    printf("Received %d bytes of data\n", bytes);
    printf("Parsing products...\n");
    parse_products(buffer, products, &product_count);
    printf("Found %d products\n", product_count);
    
    /* For each product, fetch its page to get more details */
    for (int i = 0; i < product_count && i < 5; i++) { // Limit to 5 products for testing
        printf("Fetching details for product %d: %s\n", i+1, products[i].title);
        close(sockfd);
        
        /* Extract product ID from URL */
        char product_path[MAX_URL_LENGTH];
        strncpy(product_path, products[i].product_url, MAX_URL_LENGTH);
        
        sockfd = establish_ssl_connection(HOST, PORT);
        if (sockfd < 0) {
            fprintf(stderr, "Failed to connect for product details\n");
            continue;
        }
        
        if (send_request(sockfd, HOST, product_path) <= 0) {
            fprintf(stderr, "Failed to send request for product details\n");
            close(sockfd);
            continue;
        }
        
        memset(buffer, 0, buffer_size);
        bytes = receive_response(sockfd, buffer, buffer_size);
        if (bytes <= 0) {
            fprintf(stderr, "Failed to receive product details\n");
            close(sockfd);
            continue;
        }
        
        /* Extract product description and images from the product page */
        char* desc = extract_between(buffer, "<meta name=\"description\" content=\"", "\"");
        if (desc) {
            strncpy(products[i].description, desc, MAX_DESCRIPTION_LENGTH - 1);
            free(desc);
        }
        
        extract_images(buffer, &products[i]);
    }
    
    printf("Writing products to CSV...\n");
    write_csv(products, product_count, "daraz_products.csv");
    
    printf("Cleaning up...\n");
    close(sockfd);
    free(buffer);
    
    printf("Done!\n");
    return EXIT_SUCCESS;
}

/**
 * Establishes SSL connection with the server
 * Note: This is a simplified version that doesn't actually implement SSL/TLS
 * In a real application, we would use OpenSSL or another SSL library
 */
int establish_ssl_connection(const char* host, int port) {
    fprintf(stderr, "WARNING: This implementation doesn't support HTTPS!\n");
    fprintf(stderr, "In a real application, use OpenSSL or another SSL library.\n");
    fprintf(stderr, "For demonstration purposes, we're proceeding with plain HTTP.\n");
    
    return create_socket(host, port);
}

/**
 * Creates a socket and connects to the specified host and port
 */
int create_socket(const char* host, int port) {
    int sockfd;
    struct hostent* server;
    struct sockaddr_in serv_addr;
    
    /* Create socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Failed to create socket");
        return -1;
    }
    
    /* Get host by name */
    server = gethostbyname(host);
    if (server == NULL) {
        fprintf(stderr, "Failed to resolve host: %s\n", host);
        close(sockfd);
        return -1;
    }
    
    /* Prepare server address structure */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    
    /* Connect to server */
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Failed to connect to server");
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

/**
 * Sends an HTTP request to the specified host and path
 */
int send_request(int sockfd, const char* host, const char* path) {
    char request[BUFFER_SIZE];
    
    /* Format HTTP request */
    snprintf(request, BUFFER_SIZE,
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "User-Agent: %s\r\n"
             "Accept: text/html,application/xhtml+xml\r\n"
             "Connection: close\r\n"
             "\r\n",
             path, host, USER_AGENT);
    
    /* Send request */
    return send(sockfd, request, strlen(request), 0);
}

/**
 * Receives HTTP response from the server
 */
int receive_response(int sockfd, char* buffer, size_t buffer_size) {
    int total_bytes = 0;
    int bytes_received;
    
    /* Receive data in chunks */
    while ((bytes_received = recv(sockfd, buffer + total_bytes, buffer_size - total_bytes - 1, 0)) > 0) {
        total_bytes += bytes_received;
        if (total_bytes >= buffer_size - 1)
            break;
    }
    
    if (bytes_received < 0) {
        perror("Error receiving data");
        return -1;
    }
    
    buffer[total_bytes] = '\0';
    
    /* Skip HTTP headers */
    char* body = strstr(buffer, "\r\n\r\n");
    if (body) {
        body += 4; /* Skip the \r\n\r\n */
        
        /* Check if the response is chunked */
        if (strstr(buffer, "Transfer-Encoding: chunked") != NULL) {
            /* Simple chunked encoding handling (very simplified) */
            /* In a real application, you would need proper chunk parsing */
            char* content = malloc(total_bytes);
            if (!content) {
                fprintf(stderr, "Failed to allocate memory for chunked content\n");
                return -1;
            }
            
            int content_len = 0;
            char* chunk = body;
            
            while (1) {
                char* chunk_size_end = strstr(chunk, "\r\n");
                if (!chunk_size_end) break;
                
                /* Parse chunk size (hex) */
                int chunk_size = 0;
                sscanf(chunk, "%x", &chunk_size);
                
                if (chunk_size == 0) break; /* Last chunk */
                
                chunk = chunk_size_end + 2; /* Skip the size line */
                memcpy(content + content_len, chunk, chunk_size);
                content_len += chunk_size;
                
                chunk += chunk_size + 2; /* Skip chunk data and CRLF */
            }
            
            content[content_len] = '\0';
            memcpy(buffer, content, content_len + 1);
            free(content);
            return content_len;
        } else {
            /* Move body to the beginning of the buffer */
            memmove(buffer, body, strlen(body) + 1);
            return strlen(buffer);
        }
    }
    
    return total_bytes;
}

/**
 * Parses HTML to extract products
 */
void parse_products(const char* html, Product* products, int* product_count) {
    const char* ptr = html;
    *product_count = 0;
    
    /* Look for product containers */
    while ((ptr = strstr(ptr, "<div class=\"box--pRqdD")) != NULL && *product_count < MAX_PRODUCTS) {
        Product* product = &products[*product_count];
        memset(product, 0, sizeof(Product));
        
        /* Extract title */
        char* title = extract_between(ptr, "<div class=\"title--wFj93\">", "</div>");
        if (title) {
            cleanup_html(title);
            strncpy(product->title, title, MAX_TITLE_LENGTH - 1);
            free(title);
        }
        
        /* Extract price */
        char* price = extract_between(ptr, "<div class=\"price--NVB62\">", "</div>");
        if (price) {
            cleanup_html(price);
            strncpy(product->price, price, MAX_PRICE_LENGTH - 1);
            free(price);
        }
        
        /* Extract product URL */
        char* url = extract_between(ptr, "<a href=\"", "\"");
        if (url) {
            if (strncmp(url, "//", 2) == 0) {
                /* Handle protocol-relative URLs */
                char full_url[MAX_URL_LENGTH];
                snprintf(full_url, MAX_URL_LENGTH, "https:%s", url);
                strncpy(product->product_url, full_url, MAX_URL_LENGTH - 1);
            } else if (strncmp(url, "/", 1) == 0) {
                /* Handle relative URLs */
                strncpy(product->product_url, url, MAX_URL_LENGTH - 1);
            } else {
                strncpy(product->product_url, url, MAX_URL_LENGTH - 1);
            }
            free(url);
        }
        
        /* Extract first image */
        char* img = extract_between(ptr, "<img src=\"", "\"");
        if (img) {
            if (strncmp(img, "//", 2) == 0) {
                /* Handle protocol-relative URLs */
                char full_url[MAX_IMAGE_URL_LENGTH];
                snprintf(full_url, MAX_IMAGE_URL_LENGTH, "https:%s", img);
                strncpy(product->image_urls[0], full_url, MAX_IMAGE_URL_LENGTH - 1);
            } else {
                strncpy(product->image_urls[0], img, MAX_IMAGE_URL_LENGTH - 1);
            }
            product->image_count = 1;
            free(img);
        }
        
        /* Move to next product */
        ptr++;
        (*product_count)++;
    }
}

/**
 * Extracts multiple images from product page
 */
void extract_images(const char* html, Product* product) {
    const char* ptr = html;
    int count = product->image_count; /* Start from existing count */
    
    /* Find the image gallery section */
    const char* gallery = strstr(html, "pdp-block module-gallery");
    if (!gallery) return;
    
    /* Extract all image URLs */
    while ((ptr = strstr(ptr, "<img src=\"")) != NULL && count < MAX_IMAGES) {
        ptr += 10; /* Skip "<img src=\"" */
        
        /* Find the end of the URL */
        const char* end = strchr(ptr, '\"');
        if (!end) continue;
        
        int len = end - ptr;
        if (len > 0 && len < MAX_IMAGE_URL_LENGTH) {
            char url[MAX_IMAGE_URL_LENGTH];
            strncpy(url, ptr, len);
            url[len] = '\0';
            
            /* Skip non-product images (icons, etc.) */
            if (strstr(url, "png") != NULL || len < 30) {
                ptr = end + 1;
                continue;
            }
            
            /* Handle protocol-relative URLs */
            if (strncmp(url, "//", 2) == 0) {
                char full_url[MAX_IMAGE_URL_LENGTH];
                snprintf(full_url, MAX_IMAGE_URL_LENGTH, "https:%s", url);
                strncpy(product->image_urls[count], full_url, MAX_IMAGE_URL_LENGTH - 1);
            } else {
                strncpy(product->image_urls[count], url, MAX_IMAGE_URL_LENGTH - 1);
            }
            
            count++;
            if (count >= MAX_IMAGES) break;
        }
        
        ptr = end + 1;
    }
    
    product->image_count = count;
}

/**
 * Writes products to a CSV file
 */
void write_csv(const Product* products, int product_count, const char* filename) {
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        perror("Failed to open CSV file");
        return;
    }
    
    /* Write CSV header */
    fprintf(fp, "Title,Price,Description,Product URL");
    for (int i = 0; i < MAX_IMAGES; i++) {
        fprintf(fp, ",Image %d", i + 1);
    }
    fprintf(fp, "\n");
    
    /* Write products */
    for (int i = 0; i < product_count; i++) {
        const Product* p = &products[i];
        
        /* Escape CSV fields */
        char title[MAX_TITLE_LENGTH * 2];
        char price[MAX_PRICE_LENGTH * 2];
        char description[MAX_DESCRIPTION_LENGTH * 2];
        char product_url[MAX_URL_LENGTH * 2];
        
        /* Simple CSV escaping - replace commas and quotes, then quote the field */
        char* t = title;
        for (const char* s = p->title; *s; s++) {
            if (*s == ',' || *s == '\"' || *s == '\n' || *s == '\r') {
                *t++ = '\"';
                if (*s == '\"') *t++ = '\"'; /* Double the quotes for CSV escaping */
                else *t++ = *s;
                *t++ = '\"';
            } else {
                *t++ = *s;
            }
        }
        *t = '\0';
        
        /* Repeat for other fields */
        /* In a real application, use a proper CSV library or create a reusable function */
        /* Simplified for this example */
        snprintf(price, sizeof(price), "\"%s\"", p->price);
        snprintf(description, sizeof(description), "\"%s\"", p->description);
        snprintf(product_url, sizeof(product_url), "\"%s\"", p->product_url);
        
        fprintf(fp, "%s,%s,%s,%s", title, price, description, product_url);
        
        /* Write image URLs */
        for (int j = 0; j < MAX_IMAGES; j++) {
            if (j < p->image_count && p->image_urls[j][0] != '\0') {
                fprintf(fp, ",\"%s\"", p->image_urls[j]);
            } else {
                fprintf(fp, ",");
            }
        }
        
        fprintf(fp, "\n");
    }
    
    fclose(fp);
}

/**
 * Helper function to extract text between two tags
 */
char* extract_between(const char* src, const char* start_tag, const char* end_tag) {
    const char* start = strstr(src, start_tag);
    if (!start) return NULL;
    
    start += strlen(start_tag);
    const char* end = strstr(start, end_tag);
    if (!end) return NULL;
    
    size_t len = end - start;
    char* result = (char*)malloc(len + 1);
    if (!result) return NULL;
    
    strncpy(result, start, len);
    result[len] = '\0';
    
    return result;
}

/**
 * Cleans up HTML entities and tags
 */
void cleanup_html(char* str) {
    char* dest = str;
    char* src = str;
    int inside_tag = 0;
    
    while (*src) {
        if (*src == '<') {
            inside_tag = 1;
            src++;
            continue;
        }
        
        if (*src == '>') {
            inside_tag = 0;
            src++;
            continue;
        }
        
        if (inside_tag) {
            src++;
            continue;
        }
        
        /* Handle HTML entities */
        if (*src == '&') {
            if (strncmp(src, "&amp;", 5) == 0) {
                *dest++ = '&';
                src += 5;
            } else if (strncmp(src, "&lt;", 4) == 0) {
                *dest++ = '<';
                src += 4;
            } else if (strncmp(src, "&gt;", 4) == 0) {
                *dest++ = '>';
                src += 4;
            } else if (strncmp(src, "&quot;", 6) == 0) {
                *dest++ = '\"';
                src += 6;
            } else if (strncmp(src, "&apos;", 6) == 0) {
                *dest++ = '\'';
                src += 6;
            } else if (strncmp(src, "&nbsp;", 6) == 0) {
                *dest++ = ' ';
                src += 6;
            } else {
                /* Unknown entity or not an entity */
                *dest++ = *src++;
            }
            continue;
        }
        
        /* Convert newlines to spaces */
        if (*src == '\n' || *src == '\r') {
            *dest++ = ' ';
            src++;
            continue;
        }
        
        /* Copy regular character */
        *dest++ = *src++;
    }
    
    *dest = '\0'; /* Ensure null termination */
    
    /* Replace multiple spaces with a single space */
    dest = str;
    src = str;
    
    while (*src) {
        if (*src == ' ') {
            *dest++ = ' ';
            while (*src == ' ') src++; /* Skip additional spaces */
        } else {
            *dest++ = *src++;
        }
    }
    
    *dest = '\0'; /* Ensure null termination */
    
    /* Trim leading and trailing spaces */
    dest = str;
    while (*dest == ' ') dest++;
    
    if (dest != str) {
        memmove(str, dest, strlen(dest) + 1);
    }
    
    dest = str + strlen(str) - 1;
    while (dest >= str && *dest == ' ') {
        *dest-- = '\0';
    }
}

/**
 * Decodes URL-encoded strings
 */
void url_decode(char* str) {
    char* dest = str;
    char* src = str;
    
    while (*src) {
        if (*src == '%' && src[1] && src[2]) {
            /* Decode hex value */
            int hex;
            sscanf(src + 1, "%2x", &hex);
            *dest++ = (char)hex;
            src += 3;
        } else if (*src == '+') {
            /* Convert plus to space */
            *dest++ = ' ';
            src++;
        } else {
            /* Copy regular character */
            *dest++ = *src++;
        }
    }
    
    *dest = '\0'; /* Ensure null termination */
}
