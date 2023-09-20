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

    char *jsonPayload = malloc(4096 * sizeof(char));
    if (!jsonPayload) {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    strcpy(jsonPayload, "{\"considerIp\":\"false\",\"wifiAccessPoints\":[\n");

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
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    while (getline(&line, &len, fp) != -1) {
        char *ssid = strtok(line, ":");
        char *bssid = strtok(NULL, " ");
        char *rssi = strtok(NULL, " ");

        size_t ssid_len = strlen(ssid);

        char new_bssid[18];
        if(ssid_len >= 3) {
            char ssid_end[3];
            strncpy(ssid_end, &ssid[ssid_len - 2], 2);
            ssid_end[2] = '\0';

            snprintf(new_bssid, sizeof(new_bssid), "%s:%s", ssid_end, bssid);

            ssid[ssid_len - 3] = '\0';

            //printf("New SSID: %s\n", ssid);
            //printf("New BSSID: %s\n", new_bssid);
            //printf("RSSI: %s\n", rssi);
        }
        else {
            strncpy(new_bssid, bssid, sizeof(new_bssid) - 1);
            new_bssid[sizeof(new_bssid) - 1] = '\0';
        }

        if (strlen(jsonPayload) + strlen(ssid) + strlen(new_bssid) + strlen(rssi) + 100 >= 4096) {
            fprintf(stderr, "Buffer size is insufficient\n");
            free(line);
            free(jsonPayload);
            fclose(fp);
            exit(EXIT_FAILURE);
        }

        strcat(jsonPayload, "{\"macAddress\":");
        strcat(jsonPayload,"\"");
        strcat(jsonPayload, new_bssid);
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
        strcat(jsonPayload, "\n");
    }

    jsonPayload[strlen(jsonPayload) - 2] = ']';
    strcat(jsonPayload, "\n}");

    fclose(fp);

    if (line) {
        free(line);
    }

    return jsonPayload;
}
json_object* getAddressDetails(const char* lat, const char* lng) {
    CURL *curl;
    CURLcode res;
    json_object *parsed_json = NULL;

    curl = curl_easy_init();
    if(curl) {
        struct string s;
        init_string(&s);

        char url[256];
        snprintf(url, sizeof(url), "https://maps.googleapis.com/maps/api/geocode/json?latlng=%s,%s&key=", lat, lng);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        res = curl_easy_perform(curl);

        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        else {
            parsed_json = json_tokener_parse(s.ptr);
        }

        curl_easy_cleanup(curl);
        free(s.ptr);
    }
    return parsed_json;
}

json_object* getElevationDetails(const char* lat, const char* lng) {
    CURL *curl;
    CURLcode res;
    json_object *parsed_json = NULL;

    curl = curl_easy_init();
    if(curl) {
        struct string s;
        init_string(&s);

        char url[256];
        snprintf(url, sizeof(url), "https://maps.googleapis.com/maps/api/elevation/json?locations=%s,%s&key=", lat, lng);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        res = curl_easy_perform(curl);

        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        else {
            parsed_json = json_tokener_parse(s.ptr);
        }

        curl_easy_cleanup(curl);
        free(s.ptr);
    }
    return parsed_json;
}


int main() {
    CURL *curl;
    CURLcode res;

    struct curl_slist *headers = NULL;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if (curl) {
        struct string s;
        init_string(&s);

        char *jsonData = getWiFiData();
        //printf("%s\n", jsonData);

        curl_easy_setopt(curl, CURLOPT_URL,"https://www.googleapis.com/geolocation/v1/geolocate?key=");

        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            //printf("Raw JSON response: %s\n", s.ptr);

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
            json_object *addressDetails = getAddressDetails(json_object_get_string(lat), json_object_get_string(lng));
            if (addressDetails != NULL) {
                json_object *formatted_address;
                json_object_object_get_ex(addressDetails, "results", &formatted_address);
                formatted_address = json_object_array_get_idx(formatted_address, 0);
                json_object_object_get_ex(formatted_address, "formatted_address", &formatted_address);
                printf("Address: %s\n", json_object_get_string(formatted_address));

                json_object_put(addressDetails);
            }

            json_object *elevationDetails = getElevationDetails(json_object_get_string(lat),json_object_get_string(lng));

            if (elevationDetails != NULL) {
                json_object *elevation;
                json_object_object_get_ex(elevationDetails, "results", &elevation);
                elevation = json_object_array_get_idx(elevation, 0);
                json_object_object_get_ex(elevation, "elevation", &elevation);
                printf("Elevation: %f meters\n", json_object_get_double(elevation));

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
}