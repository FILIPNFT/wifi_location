## How It Works

- **Dynamic String Structure**: A custom `string` structure is used to handle dynamic string operations.
- **Initialization Function**: `init_string` initializes the `string` structure by allocating memory and setting the initial length to 0.
- **Write Callback Function**: `WriteCallback` is a libcurl function that appends data from HTTP responses to the `string` structure, resizing it as needed.
- **Fetch Address from Coordinates**: `get_address_from_coordinates` sends a request to the Google Maps Geocoding API to convert latitude and longitude into a formatted address.
- **Main Function**:
  - Sends a request to the Google Geolocation API to obtain the current geographical coordinates.
  - Parses the response to extract latitude and longitude.
  - Calls `get_address_from_coordinates` to convert the coordinates into a human-readable address.

The program demonstrates how to use libcurl for HTTP requests and jansson for JSON parsing to interact with external APIs and process their responses.

## Example Usage

- **Setup**: Ensure you have libcurl and jansson libraries installed.
- **Run**: Compile and run the program to see the current location's latitude, longitude, and formatted address. Make sure to replace the placeholder with your Google API key in the `api_url`.
