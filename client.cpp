#include <iostream>
#include <cstdlib>
#include <string>
#include <filesystem>
#include <boost/asio.hpp>
#include <curl/curl.h>  // For HTTP requests to Nanopool API

// Helper function to write HTTP response from the Nanopool API
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to fetch the latest mined block proof (hash) from Nanopool using the wallet address
std::string fetch_mined_proof(const std::string& walletAddress) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        std::string url = "https://api.nanopool.org/v1/xmr/reportedhashrate/" + walletAddress;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (!readBuffer.empty()) {
            // Assuming the API provides a "hashrate" field
            size_t start = readBuffer.find("\"data\":");
            if (start != std::string::npos) {
                start += 7;  // Skip the field name
                size_t end = readBuffer.find(",", start);
                return readBuffer.substr(start, end - start);  // Return hashrate as "proof"
            }
        }
    }

    return "";  // Return empty if proof cannot be fetched
}

int main() {
    // Default settings
    const std::string defaultPoolUrl = "xmr-eu1.nanopool.org:10300";
    const std::string defaultWalletAddress = "8AxuYaNHVrGYPQTZ6e6hUT6jMB6eHB5XsLN8JUK7XYtYVq72xN7WV9wTzxyiPyZTooMeykErHv1S3ibs5sLvn5MZBg4Sa2v";
    std::string currentDir = std::filesystem::current_path().string();
    std::string defaultXmrigPath = currentDir + "/xmrig";  // Default XMRig path

    // User-configurable parameters
    std::string poolUrl;
    std::string walletAddress;
    std::string xmrigPath;

    // Prompt for user input with default values
    std::cout << "Enter the mining pool URL (default: " << defaultPoolUrl << "): ";
    std::getline(std::cin, poolUrl);
    if (poolUrl.empty()) {
        poolUrl = defaultPoolUrl;  // Use default if input is empty
    }

    std::cout << "Enter your wallet address (default: " << defaultWalletAddress << "): ";
    std::getline(std::cin, walletAddress);
    if (walletAddress.empty()) {
        walletAddress = defaultWalletAddress;  // Use default if input is empty
    }

    std::cout << "Enter the path to XMRig executable (default: " << defaultXmrigPath << "): ";
    std::getline(std::cin, xmrigPath);
    if (xmrigPath.empty()) {
        xmrigPath = defaultXmrigPath;  // Use default if input is empty
    }

    // Mining command
    std::string command = xmrigPath + " -o " + poolUrl + " -u " + walletAddress + " --donate-level=1 --algo rx/0";
    std::cout << "Starting XMRig with command: " << command << std::endl;

    int result = std::system(command.c_str());

    if (result != 0) {
        std::cerr << "Error: XMRig failed to start." << std::endl;
        return 1;
    }

    // Fetch the actual proof of mining from Nanopool (hashrate in this case)
    std::string minedProof = fetch_mined_proof(walletAddress);
    if (minedProof.empty()) {
        std::cerr << "Error: Unable to fetch mining proof." << std::endl;
        return 1;
    }

    std::cout << "Mined proof (reported hashrate): " << minedProof << std::endl;

    // Send the proof to the server for validation
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::socket socket(io_context);
    boost::asio::ip::tcp::resolver resolver(io_context);
    boost::asio::connect(socket, resolver.resolve("127.0.0.1", "8080"));  // Connect to server at localhost:8080

    // Send the proof to the server
    boost::asio::write(socket, boost::asio::buffer(minedProof + "\n"));

    // Receive the acknowledgment from the server
    boost::asio::streambuf responseBuffer;
    boost::asio::read_until(socket, responseBuffer, "\n");
    std::istream responseStream(&responseBuffer);
    std::string response;
    std::getline(responseStream, response);

    std::cout << "Server response: " << response << std::endl;

    // Check if mining was validated
    if (response.find("validated") != std::string::npos) {
        // Expecting the password to be sent after validation
        std::getline(responseStream, response);  // Read the password
        std::cout << "Received password: " << response << std::endl;
    } else {
        std::cerr << "Mining validation failed. No password received." << std::endl;
    }

    return 0;
}
