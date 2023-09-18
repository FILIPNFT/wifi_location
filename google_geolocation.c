#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>

// Pravimo novu strukturu koja radi dinamicke operacije sa stringovima.
struct string {
    char *ptr; // predstavlja pokazivac na odredeni karakter
    size_t len; // vodi racuna o duzini odredenog stringa
};

// Funkcija za inicijalizovanje strukture koja alocira memoriju i postavlja duzinu na 0
void init_string(struct string *s) {
    s->len = 0;
    s->ptr = malloc(s->len + 1);
    if (s->ptr == NULL) {
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }
    s->ptr[0] = '\0';
}

// Ovo je funkcija koja pripada biblioteci libcurl, njena svrha je da radi sa podacima
// dovijenim iz HTTP respons-a. Svhra joj je da dodeljena podatke dodaje na ptr promanjivu i
// povecava duzinu istog.
size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total_size = size * nmemb;
    struct string *s = userp;
    s->ptr = realloc(s->ptr, s->len + total_size + 1);
    if (s->ptr == NULL) {
        fprintf(stderr, "realloc() failed\n");
        exit(EXIT_FAILURE);
    }
    memcpy(s->ptr+s->len, contents, total_size);
    s->ptr[s->len + total_size] = '\0';
    s->len += total_size;

    return total_size;
}
// Sledeca funkcija dobija podatke od kordinatama koje pomocu GoogleAPI
// pretvara u Google maps lokaciju koja je tipa Stringa.
void get_address_from_coordinates(double lat, double lng) {
    CURL *curl;
    CURLcode res;
    char api_url[256];

    struct string s;
    init_string(&s);

    snprintf(api_url, sizeof(api_url), "https://maps.googleapis.com/maps/api/geocode/json?latlng=%f,%f&key=AIzaSyDOZ8eV3dLiiqwpqGk1Fy9Hjsw-L89wNyE", lat, lng);

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, api_url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            json_t *root;
            json_error_t error;

            root = json_loads(s.ptr, 0, &error);

            if(!root) {
                fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
            } else {
                json_t *results, *first_result, *formatted_address;
                results = json_object_get(root, "results");
                if(results) {
                    first_result = json_array_get(results, 0);
                    if(first_result) {
                        formatted_address = json_object_get(first_result, "formatted_address");
                        if(formatted_address) {
                            printf("Address: %s\n", json_string_value(formatted_address));
                        } else {
                            printf("No address found\n");
                        }
                    } else {
                        printf("No results found\n");
                    }
                } else {
                    printf("No results key in JSON response\n");
                }
                json_decref(root);
            }
        }

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    free(s.ptr);
}
// Ovaj kod je kostur programa koji nalazi kordinate koje se dalje parsiraju za tacnu lokaciju.
int main(void) {
    CURL *curl;
    CURLcode res;
    char* api_url = "https://www.googleapis.com/geolocation/v1/geolocate?key=AIzaSyDOZ8eV3dLiiqwpqGk1Fy9Hjsw-L89wNyE";

    struct string s;
    init_string(&s);

    char* postData = "{}";

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if(curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, api_url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            json_t *root;
            json_error_t error;

            root = json_loads(s.ptr, 0, &error);

            if(!root) {
                fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
            } else {
                json_t *location, *lat, *lng;
                location = json_object_get(root, "location");
                if(location) {
                    lat = json_object_get(location, "lat");
                    lng = json_object_get(location, "lng");
                    if(lat && lng) {
                        double lat_val = json_real_value(lat);
                        double lng_val = json_real_value(lng);

                        printf("Latitude: %f\n", lat_val);
                        printf("Longitude: %f\n", lng_val);
                        get_address_from_coordinates(lat_val, lng_val);
                    } else {
                        printf("Latitude and/or longitude not found in JSON response\n");
                    }
                } else {
                    printf("No location key in JSON response\n");
                }

                json_decref(root);
            }
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }

    curl_global_cleanup();
    free(s.ptr);
    return 0;
}
