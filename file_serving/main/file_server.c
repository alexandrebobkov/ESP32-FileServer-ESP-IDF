/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* HTTP File Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include "esp_err.h"
#include "esp_log.h"

#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_http_server.h"

/* Max length a file path can have on storage */
#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN)

/* Max size of an individual file. Make sure this
 * value is same as that set in upload_script.html */
#define MAX_FILE_SIZE   (1024*1024*1024) // 1 MB (200 KB)
#define MAX_FILE_SIZE_STR "1MB"

/* Scratch buffer size */
#define SCRATCH_BUFSIZE  8192

struct file_server_data {
    /* Base path of file storage */
    char base_path[ESP_VFS_PATH_MAX + 1];

    /* Scratch buffer for temporary storage during file transfer */
    char scratch[SCRATCH_BUFSIZE];
};

static const char *TAG = "file_server";

/* Handler to redirect incoming GET request for /index.html to /
 * This can be overridden by uploading file with same name */
static esp_err_t index_html_get_handler(httpd_req_t *req)
{
    httpd_resp_set_status(req, "307 Temporary Redirect");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);  // Response body can be empty
    return ESP_OK;
}

static esp_err_t img_get_handler(httpd_req_t *req)
{
    httpd_resp_set_status(req, "307 Temporary Redirect");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_set_type(req, "image/png");
    httpd_resp_send(req, NULL, 0);  // Response body can be empty
    return ESP_OK;
}

/* Handler to respond with an icon file embedded in flash.
 * Browsers expect to GET website icon at URI /favicon.ico.
 * This can be overridden by uploading file with same name */
static esp_err_t favicon_get_handler(httpd_req_t *req)
{
    extern const unsigned char favicon_ico_start[] asm("_binary_favicon_ico_start");
    extern const unsigned char favicon_ico_end[]   asm("_binary_favicon_ico_end");
    const size_t favicon_ico_size = (favicon_ico_end - favicon_ico_start);
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, (const char *)favicon_ico_start, favicon_ico_size);
    return ESP_OK;
}

/* Send HTTP response with a run-time generated html consisting of
 * a list of all files and folders under the requested path.
 * In case of SPIFFS this returns empty list when path is any
 * string other than '/', since SPIFFS doesn't support directories */
static esp_err_t http_resp_dir_html(httpd_req_t *req, const char *dirpath)
{
    char entrypath[FILE_PATH_MAX];
    char entrysize[16];
    const char *entrytype;

    struct dirent *entry;
    struct stat entry_stat;

    DIR *dir = opendir(dirpath);
    const size_t dirpath_len = strlen(dirpath);

    /* Retrieve the base path of file storage to construct the full path */
    strlcpy(entrypath, dirpath, sizeof(entrypath));

    if (!dir) {
        ESP_LOGE(TAG, "Failed to stat dir : %s", dirpath);
        /* Respond with 404 Not Found */
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Directory does not exist");
        return ESP_FAIL;
    }

    /* Send HTML file header */
    httpd_resp_sendstr_chunk(req, "<!DOCTYPE html><head><meta charset=\"utf-8\"><title>ESP32 Private Portable Files Server</title>"
    "<style>"
    "html {font-family: sans-serif;-ms-text-size-adjust: 100%;-webkit-text-size-adjust: 100%;}"
    "body {margin: 0;}"
    "article,aside,details,figcaption,figure,footer,header,hgroup,main,menu,nav,section,summary {display: block;}"
    "a {background-color: transparent;}"
    "a:active,a:hover {outline: 0;}"
    "b,strong {font-weight: bold;}"
    "h1 {font-size: 2em;margin: 0.67em 0;}"
    "mark {background: #ff0;color: #000;}"
    "small {font-size: 80%;}"
    "hr {-moz-box-sizing: content-box;box-sizing: content-box;height: 0;}"
    "button,input,optgroup,select,textarea {color: inherit;font: inherit;margin: 0;}"
    "button {overflow: visible;}"
    "button,select {text-transform: none;}"
    "button,html input[type=\"button\"],input[type=\"reset\"],input[type=\"submit\"] {-webkit-appearance: button;cursor: pointer;}"
    "button::-moz-focus-inner,input::-moz-focus-inner {border: 0;padding: 0;}"
    "input {line-height: normal;}"
    "textarea {overflow: auto;}"
    "table {border-collapse: collapse;border-spacing: 0;}"
    "td,th {padding: 0;}"
    ".container {position: relative;width: 100%;max-width: 960px;margin: 0 auto;padding: 0 20px;box-sizing: border-box;}"
    ".column,.columns {width: 100%;float: left;box-sizing: border-box;margin-bottom: 1.5rem; }"
    "@media (min-width: 400px) {.container {width: 85%;padding: 0; }}"
    "@media (min-width: 550px) {"
    ".container {width: 80%; }.column,.columns {margin-left: 4%;}.column:first-child,.columns:first-child {margin-left: 0;}"
    ".one.column,.one.columns{width:4.66666666667%;}.two.columns{width: 13.3333333333%;}.three.columns{width: 22%;}.four.columns{width: 30.6666666667%;}.five.columns{ width:39.3333333333%;}.six.columns{width: 48%;}.seven.columns{ width: 56.6666666667%;}.eight.columns{width: 65.3333333333%;}.nine.columns{ width:74.0%;}.ten.columns{width: 82.6666666667%;}.eleven.columns{ width:91.3333333333%;}.twelve.columns{width: 100%;margin-left: 0;}.one-third.column{ width:30.6666666667%;}.two-thirds.column{width:65.3333333333%;}.one-half.column{ width:48%;}"
    ".offset-by-one.column,.offset-by-one.columns{margin-left: 8.66666666667%;}.offset-by-two.column,.offset-by-two.columns{margin-left: 17.3333333333%; }.offset-by-three.column,.offset-by-three.columns{margin-left: 26%;}.offset-by-four.column,.offset-by-four.columns{margin-left: 34.6666666667%; }.offset-by-five.column,.offset-by-five.columns{margin-left: 43.3333333333%;}.offset-by-six.column,.offset-by-six.columns{margin-left: 52%;}.offset-by-seven.column,.offset-by-seven.columns{margin-left: 60.6666666667%;}.offset-by-eight.column,  .offset-by-eight.columns{margin-left: 69.3333333333%;}.offset-by-nine.column,.offset-by-nine.columns{ margin-left: 78.0%;}.offset-by-ten.column,.offset-by-ten.columns{margin-left: 86.6666666667%; }.offset-by-eleven.column,.offset-by-eleven.columns{margin-left: 95.3333333333%; }.offset-by-one-third.column,.offset-by-one-third.columns{margin-left: 34.6666666667%;}.offset-by-two-thirds.column,.offset-by-two-thirds.columns{margin-left: 69.3333333333%; }.offset-by-one-half.column,.offset-by-one-half.columns{margin-left: 52%;}"
    "}"
    "html {font-size: 62.5%; }"
    "body {font-size: 1.5em;line-height: 1.6;font-weight: 400;font-family: \"Raleway\", \"HelveticaNeue\", \"Helvetica Neue\", Helvetica, Arial, sans-serif;color: #222; }"
    "h1, h2, h3, h4, h5, h6 {margin-top: 0;margin-bottom: 2rem;font-weight: 300; }"
    "h1 { font-size: 4.0rem; line-height: 1.2;  letter-spacing: -.1rem;}"
    "h2 { font-size: 3.6rem; line-height: 1.25; letter-spacing: -.1rem; }"
    "h3 { font-size: 3.0rem; line-height: 1.3;  letter-spacing: -.1rem; }"
    "h4 { font-size: 2.4rem; line-height: 1.35; letter-spacing: -.08rem; }"
    "h5 { font-size: 1.8rem; line-height: 1.5;  letter-spacing: -.05rem; }"
    "h6 { font-size: 1.5rem; line-height: 1.6;  letter-spacing: 0; }"
    "@media (min-width: 550px) {h1 { font-size: 5.0rem; }h2 { font-size: 4.2rem; }h3 { font-size: 3.6rem; }h4 { font-size: 3.0rem; }h5 { font-size: 2.4rem; }h6 { font-size: 1.5rem; }}"
    "p {margin-top: 0; }"
    "a {color: #1EAEDB; }"
    "a:hover {color: #0FA0CE; }"
    ".button,button,input[type=\"submit\"],input[type=\"reset\"],input[type=\"button\"] {display: inline-block;height: 38px;padding: 0 30px;color: #555;text-align: center;font-size: 11px;font-weight: 600;line-height: 38px;letter-spacing: .1rem;text-transform: uppercase;text-decoration: none;white-space: nowrap;background-color: transparent;border-radius: 4px;border: 1px solid #bbb;cursor: pointer;box-sizing: border-box; }"
    ".button:hover,button:hover,input[type=\"submit\"]:hover,input[type=\"reset\"]:hover,input[type=\"button\"]:hover,.button:focus,button:focus,input[type=\"submit\"]:focus,input[type=\"reset\"]:focus,input[type=\"button\"]:focus {color: #333;border-color: #888;outline: 0; }"
    ".button.button-primary,button.button-primary,input[type=\"submit\"].button-primary,input[type=\"reset\"].button-primary,input[type=\"button\"].button-primary {color: #FFF;background-color: #33C3F0;border-color: #33C3F0; }"
    ".button.button-delete,button.button-delete,input[type=\"submit\"].button-delete,input[type=\"reset\"].button-delete,input[type=\"button\"].button-delete {color: #333;background-color: #fff;border-color: #e01010; }"
    ".button.button-primary:hover,button.button-primary:hover,input[type=\"submit\"].button-primary:hover,input[type=\"reset\"].button-primary:hover,input[type=\"button\"].button-primary:hover,.button.button-primary:focus,button.button-primary:focus,input[type=\"submit\"].button-primary:focus,input[type=\"reset\"].button-primary:focus,input[type=\"button\"].button-primary:focus {color: #FFF;background-color: #1EAEDB;border-color: #1EAEDB; }"
    ".button.button-delete:hover,button.button-delete:hover,input[type=\"submit\"].button-delete:hover,input[type=\"reset\"].button-delete:hover,input[type=\"button\"].button-delete:hover,.button.button-delete:focus,button.button-delete:focus,input[type=\"submit\"].button-delete:focus,input[type=\"reset\"].button-delete:focus,input[type=\"button\"].button-delete:focus {color: #333;background-color: #e01010;border-color: #e01010; }"
    "input[type=\"email\"],input[type=\"number\"],input[type=\"search\"],input[type=\"text\"],input[type=\"tel\"],input[type=\"url\"],input[type=\"password\"],textarea,select {height: 38px;padding: 6px 10px;background-color: #fff;border: 1px solid #D1D1D1;border-radius: 4px;box-shadow: none;box-sizing: border-box;}input[type=\"email\"],input[type=\"number\"],input[type=\"search\"],input[type=\"text\"],input[type=\"tel\"],input[type=\"url\"],input[type=\"password\"],textarea {-webkit-appearance: none;-moz-appearance: none;appearance: none;}textarea {min-height: 65px;padding-top: 6px;padding-bottom: 6px; }input[type=\"email\"]:focus,input[type=\"number\"]:focus,input[type=\"search\"]:focus,input[type=\"text\"]:focus,input[type=\"tel\"]:focus,input[type=\"url\"]:focus,input[type=\"password\"]:focus,textarea:focus,select:focus {border: 1px solid #33C3F0;outline: 0; }label,legend {display: block;margin-bottom: .5rem;font-weight: 600; }fieldset {padding: 0;border-width: 0; }input[type=\"checkbox\"],input[type=\"radio\"] {display: inline; }label > .label-body {display: inline-block;margin-left: .5rem;font-weight: normal; }"
    "ul {list-style: circle inside; }ol {list-style: decimal inside; }ol, ul {padding-left: 0;margin-top: 0; }ul ul,ul ol,ol ol,ol ul {margin: 1.5rem 0 1.5rem 3rem;font-size: 90%; }li {margin-bottom: 1rem; }"
    "code {padding: .2rem .5rem;margin: 0 .2rem;font-size: 90%;white-space: nowrap;background: #F1F1F1;border: 1px solid #E1E1E1;border-radius: 4px; }pre > code {display: block;padding: 1rem 1.5rem;white-space: pre; }"
    "th,td {padding: 12px 15px;text-align: left;border-bottom: 1px solid #E1E1E1; }th:first-child,td:first-child {padding-left: 0; }th:last-child,td:last-child {padding-right: 0; }"
    "button,.button {margin-bottom: 1rem; }input,textarea,select,fieldset {margin-bottom: 1.5rem; }pre,blockquote,dl,figure,table,p,ul,ol,form {margin-bottom: 2.5rem; }"
    ".container:after,.row:after,.u-cf {content: "";display: table;clear: both; }"
    "</style>"
    "<link href=\"//fonts.googleapis.com/css?family=Raleway:400,300,600\" rel=\"stylesheet\" type=\"text/css\">"
    "</head><html><body><div class=\"container\">");
    /*"hr {margin-top: 3rem;margin-bottom: 3.5rem;border-width: 0;border-top: 1px solid #000000; }"*/
    /* Get handle to embedded file upload script */
    extern const unsigned char upload_script_start[] asm("_binary_upload_script_html_start");
    extern const unsigned char upload_script_end[]   asm("_binary_upload_script_html_end");
    const size_t upload_script_size = (upload_script_end - upload_script_start);

    /* Add file upload form and script which on execution sends a POST request to /upload */
    httpd_resp_send_chunk(req, (const char *)upload_script_start, upload_script_size);

    /* Send file-list table definition and column labels */
    /*httpd_resp_sendstr_chunk(req,
        "<table class=\"fixed\" border=\"1\">"
        "<col width=\"800px\" /><col width=\"300px\" /><col width=\"300px\" /><col width=\"100px\" />"
        "<thead><tr><th>Name</th><th>Type</th><th>Size (Bytes)</th><th>Delete</th></tr></thead>"
        "<tbody>");*/
    httpd_resp_sendstr_chunk(req,   
        //"<div class=\"container\">"     
        "<hr>"
        "<div class=\"row\">"
        "<div class=\"six columns\" style=\"text-align: center;\"><b>Name</b></div>"
        "<div class=\"two columns\" style=\"text-align: center;\"><b>Type</b></div>"
        "<div class=\"two columns\" style=\"text-align: center;\"><b>Size (Bytes)</b></div>"
        "<div class=\"two columns\" style=\"text-align: center;\"><b>Delete</b></div>"
        "</div>"
        "<hr>"
    );

    /* Iterate over all files / folders and fetch their names and sizes */
    while ((entry = readdir(dir)) != NULL) {
        entrytype = (entry->d_type == DT_DIR ? "directory" : "file");

        strlcpy(entrypath + dirpath_len, entry->d_name, sizeof(entrypath) - dirpath_len);
        if (stat(entrypath, &entry_stat) == -1) {
            ESP_LOGE(TAG, "Failed to stat %s : %s", entrytype, entry->d_name);
            continue;
        }
        sprintf(entrysize, "%ld", entry_stat.st_size);
        ESP_LOGI(TAG, "Found %s : %s (%s bytes)", entrytype, entry->d_name, entrysize);

        // Send chunk of HTML file containing table entries with file name and size 
        //httpd_resp_sendstr_chunk(req, "<tr><td><a href=\"");
        // Display file name
        httpd_resp_sendstr_chunk(req, "<div class=\"row\"><div class=\"six columns\"><a href=\"");
        httpd_resp_sendstr_chunk(req, req->uri);
        httpd_resp_sendstr_chunk(req, entry->d_name);
        if (entry->d_type == DT_DIR) {
            httpd_resp_sendstr_chunk(req, "/");
        }
        httpd_resp_sendstr_chunk(req, "\">");
        httpd_resp_sendstr_chunk(req, entry->d_name);
        // Display file type
        httpd_resp_sendstr_chunk(req, "</a></div><div class=\"two columns\" style=\"text-align: center;\">");
        httpd_resp_sendstr_chunk(req, entrytype);
        // Display file size
        httpd_resp_sendstr_chunk(req, "</div><div class=\"two columns\" style=\"text-align: right;\">");
        httpd_resp_sendstr_chunk(req, entrysize);
        // Display file delete button
        httpd_resp_sendstr_chunk(req, "</div><div class=\"two columns\">");
        httpd_resp_sendstr_chunk(req, "<form method=\"post\" action=\"/delete");
        httpd_resp_sendstr_chunk(req, req->uri);
        httpd_resp_sendstr_chunk(req, entry->d_name);
        httpd_resp_sendstr_chunk(req, "\"><button type=\"submit\" class=\"button-delete\">Delete</button></form>");
        //httpd_resp_sendstr_chunk(req, "</td></tr>\n");
        // Close row div
        httpd_resp_sendstr_chunk(req, "</div></div>\n");
    }
    closedir(dir);
    httpd_resp_sendstr_chunk(req, "<hr>\n");

    /* Finish the file list table */
    //httpd_resp_sendstr_chunk(req, "</tbody></table>");

    /* Send remaining chunk of HTML file to complete it */
    httpd_resp_sendstr_chunk(req, "</div></body></html>");

    /* Send empty chunk to signal HTTP response completion */
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

#define IS_FILE_EXT(filename, ext) \
    (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename)
{
    if (IS_FILE_EXT(filename, ".pdf")) {
        return httpd_resp_set_type(req, "application/pdf");
    } else if (IS_FILE_EXT(filename, ".html")) {
        return httpd_resp_set_type(req, "text/html");
    } else if (IS_FILE_EXT(filename, ".jpeg")) {
        return httpd_resp_set_type(req, "image/jpeg");
    } else if (IS_FILE_EXT(filename, ".ico")) {
        return httpd_resp_set_type(req, "image/x-icon");
    }
    /* This is a limited set only */
    /* For any other type always set as plain text */
    return httpd_resp_set_type(req, "text/plain");
}

/* Copies the full path into destination buffer and returns
 * pointer to path (skipping the preceding base path) */
static const char* get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize)
{
    const size_t base_pathlen = strlen(base_path);
    size_t pathlen = strlen(uri);

    const char *quest = strchr(uri, '?');
    if (quest) {
        pathlen = MIN(pathlen, quest - uri);
    }
    const char *hash = strchr(uri, '#');
    if (hash) {
        pathlen = MIN(pathlen, hash - uri);
    }

    if (base_pathlen + pathlen + 1 > destsize) {
        /* Full path string won't fit into destination buffer */
        return NULL;
    }

    /* Construct full path (base + path) */
    strcpy(dest, base_path);
    strlcpy(dest + base_pathlen, uri, pathlen + 1);

    /* Return pointer to path, skipping the base */
    return dest + base_pathlen;
}

/* Handler to download a file kept on the server */
static esp_err_t download_get_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];
    FILE *fd = NULL;
    struct stat file_stat;

    const char *filename = get_path_from_uri(filepath, ((struct file_server_data *)req->user_ctx)->base_path,
                                             req->uri, sizeof(filepath));
    if (!filename) {
        ESP_LOGE(TAG, "Filename is too long");
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Filename too long");
        return ESP_FAIL;
    }

    /* If name has trailing '/', respond with directory contents */
    if (filename[strlen(filename) - 1] == '/') {
        return http_resp_dir_html(req, filepath);
    }

    if (stat(filepath, &file_stat) == -1) {
        /* If file not present on SPIFFS check if URI
         * corresponds to one of the hardcoded paths */
        if (strcmp(filename, "/index.html") == 0) {
            return index_html_get_handler(req);
        } else if (strcmp(filename, "/favicon.ico") == 0) {
            return favicon_get_handler(req);
        } else if (strcmp(filename, "/bot.png") == 0) {
            return img_get_handler(req);
        }
        ESP_LOGE(TAG, "Failed to stat file : %s", filepath);
        /* Respond with 404 Not Found */
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File does not exist");
        return ESP_FAIL;
    }

    fd = fopen(filepath, "r");
    if (!fd) {
        ESP_LOGE(TAG, "Failed to read existing file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Sending file : %s (%ld bytes)...", filename, file_stat.st_size);
    set_content_type_from_file(req, filename);

    /* Retrieve the pointer to scratch buffer for temporary storage */
    char *chunk = ((struct file_server_data *)req->user_ctx)->scratch;
    size_t chunksize;
    do {
        /* Read file in chunks into the scratch buffer */
        chunksize = fread(chunk, 1, SCRATCH_BUFSIZE, fd);

        if (chunksize > 0) {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK) {
                fclose(fd);
                ESP_LOGE(TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
               return ESP_FAIL;
           }
        }

        /* Keep looping till the whole file is sent */
    } while (chunksize != 0);

    /* Close file after sending complete */
    fclose(fd);
    ESP_LOGI(TAG, "File sending complete");

    /* Respond with an empty chunk to signal HTTP response completion */
#ifdef CONFIG_EXAMPLE_HTTPD_CONN_CLOSE_HEADER
    httpd_resp_set_hdr(req, "Connection", "close");
#endif
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

/* Handler to upload a file onto the server */
static esp_err_t upload_post_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];
    FILE *fd = NULL;
    struct stat file_stat;

    /* Skip leading "/upload" from URI to get filename */
    /* Note sizeof() counts NULL termination hence the -1 */
    const char *filename = get_path_from_uri(filepath, ((struct file_server_data *)req->user_ctx)->base_path,
                                             req->uri + sizeof("/upload") - 1, sizeof(filepath));
    if (!filename) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Filename too long");
        return ESP_FAIL;
    }

    /* Filename cannot have a trailing '/' */
    if (filename[strlen(filename) - 1] == '/') {
        ESP_LOGE(TAG, "Invalid filename : %s", filename);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid filename");
        return ESP_FAIL;
    }

    if (stat(filepath, &file_stat) == 0) {
        ESP_LOGE(TAG, "File already exists : %s", filepath);
        /* Respond with 400 Bad Request */
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "File already exists");
        return ESP_FAIL;
    }

    /* File cannot be larger than a limit */
    if (req->content_len > MAX_FILE_SIZE) {
        ESP_LOGE(TAG, "File too large : %d bytes", req->content_len);
        /* Respond with 400 Bad Request */
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                            "File size must be less than "
                            MAX_FILE_SIZE_STR "!");
        /* Return failure to close underlying connection else the
         * incoming file content will keep the socket busy */
        return ESP_FAIL;
    }

    fd = fopen(filepath, "w");
    if (!fd) {
        ESP_LOGE(TAG, "Failed to create file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to create file");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Receiving file : %s...", filename);

    /* Retrieve the pointer to scratch buffer for temporary storage */
    char *buf = ((struct file_server_data *)req->user_ctx)->scratch;
    int received;

    /* Content length of the request gives
     * the size of the file being uploaded */
    int remaining = req->content_len;

    while (remaining > 0) {

        ESP_LOGI(TAG, "Remaining size : %d", remaining);
        /* Receive the file part by part into a buffer */
        if ((received = httpd_req_recv(req, buf, MIN(remaining, SCRATCH_BUFSIZE))) <= 0) {
            if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry if timeout occurred */
                continue;
            }

            /* In case of unrecoverable error,
             * close and delete the unfinished file*/
            fclose(fd);
            unlink(filepath);

            ESP_LOGE(TAG, "File reception failed!");
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive file");
            return ESP_FAIL;
        }

        /* Write buffer content to file on storage */
        if (received && (received != fwrite(buf, 1, received, fd))) {
            /* Couldn't write everything to file!
             * Storage may be full? */
            fclose(fd);
            unlink(filepath);

            ESP_LOGE(TAG, "File write failed!");
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to write file to storage");
            return ESP_FAIL;
        }

        /* Keep track of remaining size of
         * the file left to be uploaded */
        remaining -= received;
    }

    /* Close file upon upload completion */
    fclose(fd);
    ESP_LOGI(TAG, "File reception complete");

    /* Redirect onto root to see the updated file list */
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
#ifdef CONFIG_EXAMPLE_HTTPD_CONN_CLOSE_HEADER
    httpd_resp_set_hdr(req, "Connection", "close");
#endif
    httpd_resp_sendstr(req, "File uploaded successfully");
    return ESP_OK;
}

/* Handler to delete a file from the server */
static esp_err_t delete_post_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];
    struct stat file_stat;

    /* Skip leading "/delete" from URI to get filename */
    /* Note sizeof() counts NULL termination hence the -1 */
    const char *filename = get_path_from_uri(filepath, ((struct file_server_data *)req->user_ctx)->base_path,
                                             req->uri  + sizeof("/delete") - 1, sizeof(filepath));
    if (!filename) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Filename too long");
        return ESP_FAIL;
    }

    /* Filename cannot have a trailing '/' */
    if (filename[strlen(filename) - 1] == '/') {
        ESP_LOGE(TAG, "Invalid filename : %s", filename);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid filename");
        return ESP_FAIL;
    }

    if (stat(filepath, &file_stat) == -1) {
        ESP_LOGE(TAG, "File does not exist : %s", filename);
        /* Respond with 400 Bad Request */
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "File does not exist");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Deleting file : %s", filename);
    /* Delete file */
    unlink(filepath);

    /* Redirect onto root to see the updated file list */
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
#ifdef CONFIG_EXAMPLE_HTTPD_CONN_CLOSE_HEADER
    httpd_resp_set_hdr(req, "Connection", "close");
#endif
    httpd_resp_sendstr(req, "File deleted successfully");
    return ESP_OK;
}

/* Function to start the file server */
esp_err_t example_start_file_server(const char *base_path)
{
    static struct file_server_data *server_data = NULL;

    if (server_data) {
        ESP_LOGE(TAG, "File server already started");
        return ESP_ERR_INVALID_STATE;
    }

    /* Allocate memory for server data */
    server_data = calloc(1, sizeof(struct file_server_data));
    if (!server_data) {
        ESP_LOGE(TAG, "Failed to allocate memory for server data");
        return ESP_ERR_NO_MEM;
    }
    strlcpy(server_data->base_path, base_path,
            sizeof(server_data->base_path));

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* Use the URI wildcard matching function in order to
     * allow the same handler to respond to multiple different
     * target URIs which match the wildcard scheme */
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "Starting HTTP Server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start file server!");
        return ESP_FAIL;
    }

    /* URI handler for getting uploaded files */
    httpd_uri_t file_download = {
        .uri       = "/*",  // Match all URIs of type /path/to/file
        .method    = HTTP_GET,
        .handler   = download_get_handler,
        .user_ctx  = server_data    // Pass server data as context
    };
    httpd_register_uri_handler(server, &file_download);

    /* URI handler for uploading files to server */
    httpd_uri_t file_upload = {
        .uri       = "/upload/*",   // Match all URIs of type /upload/path/to/file
        .method    = HTTP_POST,
        .handler   = upload_post_handler,
        .user_ctx  = server_data    // Pass server data as context
    };
    httpd_register_uri_handler(server, &file_upload);

    /* URI handler for deleting files from server */
    httpd_uri_t file_delete = {
        .uri       = "/delete/*",   // Match all URIs of type /delete/path/to/file
        .method    = HTTP_POST,
        .handler   = delete_post_handler,
        .user_ctx  = server_data    // Pass server data as context
    };
    httpd_register_uri_handler(server, &file_delete);

    return ESP_OK;
}
