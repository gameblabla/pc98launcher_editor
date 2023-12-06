#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <curl/curl.h>

#define API_KEY "moby_8F7tJx9yjBIepD92PpEwvamWqt4"

#ifndef API_KEY
#error "Please set your Mobygames API key before compiling this app"
#endif

#ifndef MAX_PATH
#define MAX_PATH 2048
#endif

#ifndef TRUE
#define TRUE 1
#endif


#ifndef FALSE
#define FALSE 0
#endif

static int platform_id = 95;  // platform ID for PC-98 on Mobygames

long game_id;
char developer_name[128];
char release_year[128];
char genre_game[128];

struct string {
    char *ptr;
    size_t len;
};

void init_string(struct string *s) {
    s->len = 0;
    s->ptr = malloc(s->len + 1);
    if (s->ptr == NULL) {
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }
    s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s) {
    size_t new_len = s->len + size * nmemb;
    s->ptr = realloc(s->ptr, new_len + 1);
    if (s->ptr == NULL) {
        fprintf(stderr, "realloc() failed\n");
        exit(EXIT_FAILURE);
    }
    memcpy(s->ptr + s->len, ptr, size * nmemb);
    s->ptr[new_len] = '\0';
    s->len = new_len;

    return size * nmemb;
}

long parse_game_id(const char *response) {
    const char *id_str = strstr(response, "\"game_id\":");
    if (id_str) {
        return strtol(id_str + 10, NULL, 10); // 10 characters to skip over "\"game_id\":"
    }
    return -1; // Return -1 if not found
}

char *url_encode(const char *str) {
    char *encoded = curl_easy_escape(NULL, str, 0);
    if (!encoded) {
        fprintf(stderr, "Failed to encode URL\n");
        exit(EXIT_FAILURE);
    }
    return encoded;
}

void shorten_name(char *short_name, const char *game_name, int num) {
    strncpy(short_name, game_name, 6);
    short_name[6] = '\0'; // Ensure null-termination
    char num_str[3];
    snprintf(num_str, sizeof(num_str), "%02d", num);
    strcat(short_name, num_str); // Append the number
}


void download_screenshot(const char *url, int num, char* folder_name, char* destination_folder) {
    CURL *curl;
    FILE *fp;
    char filename[MAX_PATH];

    snprintf(filename, sizeof(filename), "%s\\%s_%d.png", destination_folder, folder_name, num);

    fp = fopen(filename, "wb");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open file for writing\n");
        return;
    }

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);  // Use the default write function
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);

        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }

    fclose(fp);
}

void parse_screenshot_urls(const char *response, char urls[4][512]) {
    const char *ptr = response;
    int count = 0;

    while (count < 4 && (ptr = strstr(ptr, "\"image\":\""))) {
        ptr += 9; // Skip past "\"image\":\""
        char *url_end = strchr(ptr, '\"');
        if (url_end && (url_end - ptr < 511)) {
            strncpy(urls[count], ptr, url_end - ptr);
            urls[count][url_end - ptr] = '\0';
            count++;
        }
        ptr = url_end;
    }
}

void extractDate(const char *response, const char *platformId, char *date) {
    char platformIdSearchStr[50];
    sprintf(platformIdSearchStr, "\"platform_id\":%s", platformId);
    
    // Find platform_id
    char *platformIdStart = strstr(response, platformIdSearchStr);
    if (platformIdStart == NULL) {
        strcpy(date, "Not found");
        return;
    }

    // Search backwards for first_release_date
    char *dateFieldStart = platformIdStart;
    while (dateFieldStart >= response) {
        if (strncmp(dateFieldStart, "\"first_release_date\":\"", strlen("\"first_release_date\":\"")) == 0) {
            break;
        }
        dateFieldStart--;
    }

    if (dateFieldStart < response) {
        strcpy(date, "Not found");
        return;
    }

    // Move to start of date
    dateFieldStart += strlen("\"first_release_date\":\"");

    // Find end of date
    char *dateFieldEnd = strchr(dateFieldStart, '\"');
    if (dateFieldEnd == NULL) {
        strcpy(date, "Not found");
        return;
    }

    // Calculate date length and copy date
    int dateLen = dateFieldEnd - dateFieldStart;
    strncpy(date, dateFieldStart, dateLen);
    date[dateLen] = '\0'; // Null-terminate the date string
}

void extract_developer(const char *response, char *developer, size_t max_len) {
    const char *dev_marker = "\"role\":\"Developed by\"";
    const char *dev_start = strstr(response, dev_marker);
    if (!dev_start) {
        strcpy(developer, "Developer not found");
        return;
    }

    const char *name_marker = "\"company_name\":\"";
    const char *name_end = dev_start;
    while (name_end > response && strncmp(name_end, name_marker, strlen(name_marker)) != 0) {
        name_end--;
    }

    if (name_end <= response) {
        strcpy(developer, "Developer not found");
        return;
    }

    const char *name_start = name_end + strlen(name_marker);
    name_end = strchr(name_start, '\"');
    if (!name_end) {
        strcpy(developer, "Developer not found");
        return;
    }

    size_t name_len = name_end - name_start;
    if (name_len < max_len) {
        strncpy(developer, name_start, name_len);
        developer[name_len] = '\0';
    } else {
        strncpy(developer, name_start, max_len - 1);
        developer[max_len - 1] = '\0';
    }
}

void extract_genre(const char *response, char *genre, size_t max_len) {
    const char *category_marker = "\"genre_category\":\"Gameplay\"";
    const char *category_start = strstr(response, category_marker);
    if (!category_start) {
        strcpy(genre, "Genre not found");
        return;
    }

    const char *name_marker = "\"genre_name\":\"";
    const char *name_start = strstr(category_start, name_marker);
    if (!name_start) {
        strcpy(genre, "Genre not found");
        return;
    }
    name_start += strlen(name_marker);

    const char *name_end = strchr(name_start, '\"');
    if (!name_end) {
        strcpy(genre, "Genre not found");
        return;
    }

    size_t name_len = name_end - name_start;
    if (name_len < max_len) {
        strncpy(genre, name_start, name_len);
        genre[name_len] = '\0';
    } else {
        strncpy(genre, name_start, max_len - 1);
        genre[max_len - 1] = '\0';
    }
}

void fetch_game_details(long game_id) {
    CURL *curl;
    CURLcode res;
    char url[512];
    struct string s;
    init_string(&s);
    
    char platformid_name[64];
    snprintf(platformid_name, sizeof(platformid_name), "%d", platform_id);
        
    #if defined (_WIN32) || defined(__CYGWIN__) || defined(WIN32)
    Sleep(1000);
    #else
	sleep(1);  // Pause for 1 second between downloads
    #endif
    

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl) {
        snprintf(url, sizeof(url), "https://api.mobygames.com/v1/games/%ld?api_key=%s", game_id, API_KEY);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            //printf("Full GET Response:\n%s\n", s.ptr);
        }
    }

	extractDate(s.ptr, platformid_name, release_year);
	extract_genre(s.ptr, genre_game, sizeof(genre_game));
	
	//,{"genre_category":"Gameplay","genre_category_id":4,"genre_id":22,"genre_name":"Shooter"}
	
	free(s.ptr);
	init_string(&s);
	
    #if defined (_WIN32) || defined(__CYGWIN__) || defined(WIN32)
    Sleep(1000);
    #else
	sleep(1);  // Pause for 1 second between downloads
    #endif
	
    if (curl) {
		snprintf(url, sizeof(url), "https://api.mobygames.com/v1/games/%ld/platforms/%d?api_key=%s", game_id, platform_id, API_KEY);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            //printf("Full GET Response:\n%s\n", s.ptr);
        }
    }
    
    extract_developer(s.ptr, developer_name, sizeof(developer_name));

	// Use developer_name and release_year as needed
	//printf("Developer: %s\n", developer_name);
	//printf("Release Year: %s\n", release_year);

    free(s.ptr);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
}

char* Get_Developer_Game()
{
	return developer_name;
}

char* Get_Year_Game()
{
	return release_year;
}

char* Get_Genre_Game()
{
	return genre_game;
}

int fetch_and_download_screenshots(char* game_name, char* folder_name, char* destination_folder, int plid)
{
	CURL *curl;
    CURLcode res;
    char url[512];
    struct string s;
    init_string(&s);
    
    platform_id = plid;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl) {
        char *encoded_title = url_encode(game_name);
        snprintf(url, sizeof(url), "https://api.mobygames.com/v1/games?title=%s&platform=%d&api_key=%s", encoded_title, platform_id, API_KEY);
        curl_free(encoded_title);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
			game_id = parse_game_id(s.ptr);
            if (game_id != -1) {
                printf("Game ID: %ld\n", game_id);
                // Further processing to get screenshots
            } else {
                printf("Game ID not found.\n");
                return 1;
            }
        }
    }
    
    free(s.ptr);
    
    #if defined (_WIN32) || defined(__CYGWIN__) || defined(WIN32)
    Sleep(1000);
    #else
	sleep(1);  // Pause for 1 second between downloads
    #endif
    
    if (game_id != -1) {
        // Initialize a new string structure for the screenshots response
        struct string screenshots_response;
        init_string(&screenshots_response);

        // Make a new request to get screenshots
        char screenshots_url[512];
        snprintf(screenshots_url, sizeof(screenshots_url), "https://api.mobygames.com/v1/games/%ld/platforms/%d/screenshots?api_key=%s", game_id, platform_id, API_KEY);
        curl_easy_setopt(curl, CURLOPT_URL, screenshots_url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);  // Use the default write function
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &screenshots_response);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            return 1;
        } else {
            // Parse the response to extract screenshot URLs
            char screenshot_urls[4][512];
            parse_screenshot_urls(screenshots_response.ptr, screenshot_urls);
            

            for (int i = 0; i < 4; i++) {
                if (strlen(screenshot_urls[i]) > 0) {
                    download_screenshot(screenshot_urls[i], i + 1, folder_name, destination_folder);
					#if defined (_WIN32) || defined(__CYGWIN__) || defined(WIN32)
					Sleep(1000);
					#else
					sleep(1);  // Pause for 1 second between downloads
					#endif
                }
            }
        }

        // Clean up
        free(screenshots_response.ptr);

    } else {
        printf("Game ID not found.\n");
    }

	curl_easy_cleanup(curl); // Clean up the CURL handle
	curl_global_cleanup(); // Clean up the CURL library	
	
	fetch_game_details(game_id);
	
	return 0;
}


#ifdef STANDALONE
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <game name> <short name>\n", argv[0]);
        return 1;
    }

	
	fetch_and_download_screenshots(argv[1], argv[2], "", platform_id);
 
	//fetch_game_details(game_id, argv[1], argv[2], "");
	
    return 0;
}
#endif
