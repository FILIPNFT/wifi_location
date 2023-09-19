#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <json-c/json.h>

struct string {
    char *ptr;
    size_t len;
};

void init_string(struct string *s) {
    s->len = 0;
    s->ptr = malloc(s->len+1);
    if (s->ptr == NULL) {
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }
    s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s) {
    size_t new_len = s->len + size*nmemb;
    s->ptr = realloc(s->ptr, new_len+1);
    if (s->ptr == NULL) {
        fprintf(stderr, "realloc() failed\n");
        exit(EXIT_FAILURE);
    }
    memcpy(s->ptr+s->len, ptr, size*nmemb);
    s->ptr[new_len] = '\0';
    s->len = new_len;

    return size*nmemb;
}

char* getWiFiData() {
    FILE *fp;
    char *line = NULL;
    size_t len = 0;

    char *jsonPayload = malloc(4096 * sizeof(char)); // Increased buffer size
    if (!jsonPayload) {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    strcpy(jsonPayload, "{\"considerIp\":\"false\",\"wifiAccessPoints\":[");

    system("sudo /System/Library/PrivateFrameworks/Apple80211.framework/Resources/airport -s > output.txt");

    fp = fopen("output.txt", "r");
    if (fp == NULL) {
        perror("Failed to open file");
        free(jsonPayload);
        exit(EXIT_FAILURE);
    }

    // Skip the first line (it contains the header)
    if (getline(&line, &len, fp) == -1) {
        perror("Failed to read line");
        free(jsonPayload);
        pclose(fp);
        exit(EXIT_FAILURE);
    }

    while (getline(&line, &len, fp) != -1) {
        char *ssid = strtok(line, " ");
        char *bssid = strtok(NULL, " ");
        char *rssi = strtok(NULL, " ");


        // Ensuring there is enough space to add more data to jsonPayload
        if (strlen(jsonPayload) + strlen(ssid) + strlen(bssid) + strlen(rssi) + 100 >= 4096) {
            fprintf(stderr, "Buffer size is insufficient\n");
            free(line);
            free(jsonPayload);
            pclose(fp);
            exit(EXIT_FAILURE);
        }

        strcat(jsonPayload, "{\"macAddress\":");
        strcat(jsonPayload,"\"");
        strcat(jsonPayload, bssid);
        strcat(jsonPayload,"\",");
        strcat(jsonPayload, "\"signalStrength\":");
        strcat(jsonPayload,"\"");
        strcat(jsonPayload, rssi);
        strcat(jsonPayload,"\",");
        strcat(jsonPayload, "\"ssid\":");
        strcat(jsonPayload,"\"");
        strcat(jsonPayload, ssid);
        strcat(jsonPayload,"\"");
        strcat(jsonPayload, "},");
    }

    // Replace the last comma with a closing bracket to end the JSON array
    jsonPayload[strlen(jsonPayload) - 1] = ']';
    strcat(jsonPayload, "}");

    fclose(fp);

    if (line) {
        free(line);
    }

    return jsonPayload;
}


int main() {
    CURL *curl;
    CURLcode res;

    struct curl_slist *headers = NULL;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if(curl) {
        struct string s;
        init_string(&s);

        char *jsonData = getWiFiData();
        //printf("%s", jsonData);

        curl_easy_setopt(curl, CURLOPT_URL, "https://www.googleapis.com/geolocation/v1/geolocate?key=AIzaSyDOZ8eV3dLiiqwpqGk1Fy9Hjsw-L89wNyE");

        // Set the header content-type
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

        res = curl_easy_perform(curl);

        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        else {
            // Print the raw JSON response for debugging
            //printf("Raw JSON response: %s\n", s.ptr);

            // Parse the JSON response
            json_object *parsed_json;
            json_object *location;
            json_object *lat;
            json_object *lng;
            json_object *accuracy;

            parsed_json = json_tokener_parse(s.ptr);

            json_object_object_get_ex(parsed_json, "location", &location);
            json_object_object_get_ex(location, "lat", &lat);
            json_object_object_get_ex(location, "lng", &lng);
            json_object_object_get_ex(parsed_json, "accuracy", &accuracy);

            printf("Latitude: %s\n", json_object_get_string(lat));
            printf("Longitude: %s\n", json_object_get_string(lng));
            printf("Accuracy: %s meters\n", json_object_get_string(accuracy));
            //printf("Locaton: %s meters\n", json_object_get_string(location));

            // Clean up JSON object
            json_object_put(parsed_json);
        }
        curl_slist_free_all(headers);

        curl_easy_cleanup(curl);
        free(s.ptr);
        free(jsonData);
    }

    curl_global_cleanup();

    return 0;
}
