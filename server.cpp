#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <boost/program_options.hpp>
#include <cmath>

using json = nlohmann::json;
namespace po = boost::program_options;

// Helper function to write HTTP response from the API
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to fetch the wallet balance from Nanopool using the wallet address
std::string get_xmr_balance(const std::string& walletAddress) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        std::string url = "https://api.nanopool.org/v1/xmr/balance/" + walletAddress;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (!readBuffer.empty()) {
            return readBuffer;  // Return the full API response
        }
    }

    return "";  // Return empty if the balance cannot be fetched
}

// Function to extract balance from the JSON response
double extract_balance(const std::string& balance_data) {
    try {
        auto json_response = json::parse(balance_data);
        if (json_response.contains("status") && json_response["status"] == true) {
            return json_response["data"].get<double>();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing balance data: " << e.what() << std::endl;
    }

    return 0.0;  // Return 0.0 if unable to extract balance
}

// Function to fetch the current price of Monero in USD from CoinGecko
double get_xmr_price_in_usd() {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        std::string url = "https://api.coingecko.com/api/v3/simple/price?ids=monero&vs_currencies=usd";

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (!readBuffer.empty()) {
            try {
                auto json_response = json::parse(readBuffer);
                if (json_response.contains("monero") && json_response["monero"].contains("usd")) {
                    return json_response["monero"]["usd"].get<double>();
                }
            } catch (const std::exception& e) {
                std::cerr << "Error parsing XMR price data: " << e.what() << std::endl;
            }
        }
    }

    return 0.0;  // Return 0.0 if unable to fetch price data
}

// Function to load previous balance from a file (persistent storage)
double load_previous_balance(const std::string& filepath) {
    std::ifstream infile(filepath);
    double balance = 0.0;

    if (infile.is_open()) {
        infile >> balance;
        infile.close();
    }

    return balance;
}

// Function to save current balance to a file (persistent storage)
void save_current_balance(const std::string& filepath, double balance) {
    std::ofstream outfile(filepath);
    if (outfile.is_open()) {
        outfile << balance;
        outfile.close();
    }
}

// Function to compare two floating-point numbers with a tolerance
bool is_equal(double a, double b, double tolerance = 1e-8) {
    return std::fabs(a - b) < tolerance;
}

// Function to handle the client connection and receive mining proof
void handle_client(boost::asio::ip::tcp::socket& socket, const std::string& password, double min_usd) {
    try {
        boost::asio::streambuf buffer;
        boost::asio::read_until(socket, buffer, "\n");

        // Extracting mining proof (reported hashrate) from the client
        std::istream input(&buffer);
        std::string minedProof;
        std::getline(input, minedProof);

        // Output the received proof
        std::cout << "Received mined proof (reported hashrate): " << minedProof << std::endl;

        // Check wallet balance
        std::string wallet_address = "8AxuYaNHVrGYPQTZ6e6hUT6jMB6eHB5XsLN8JUK7XYtYVq72xN7WV9wTzxyiPyZTooMeykErHv1S3ibs5sLvn5MZBg4Sa2v"; // Monero wallet address

        std::cout << "Checking balance for wallet: " << wallet_address << std::endl;

        // Fetch balance data from Nanopool API
        std::string balance_data = get_xmr_balance(wallet_address);

        if (!balance_data.empty()) {
            // Extract the current balance from the API response
            double current_balance = extract_balance(balance_data);
            std::cout << "Current balance: " << current_balance << std::endl;

            // Load the previous balance from a file
            std::string balance_file = "previous_balance.txt";
            double previous_balance = load_previous_balance(balance_file);
            std::cout << "Previous balance: " << previous_balance << std::endl;

            // Check if balances are the same
            if (is_equal(current_balance, previous_balance)) {
                std::cerr << "Error: Current balance is the same as previous balance. Proof of work will not be accepted." << std::endl;
                std::string response = "Proof of work rejected: No change in balance.\n";
                boost::asio::write(socket, boost::asio::buffer(response));
                return; // Exit the function
            }

            // Fetch Monero price in USD
            double monero_price_in_usd = get_xmr_price_in_usd();
            if (monero_price_in_usd > 0.0) {
                // Calculate how much Monero has been mined
                double mined_xmr = current_balance - previous_balance;
                double mined_usd = mined_xmr * monero_price_in_usd;

                std::cout << "Mined Monero: " << mined_xmr << " XMR (" << mined_usd << " USD)" << std::endl;

                // Validate the client proof by checking if at least the configured minimum USD worth of Monero has been mined
                if (mined_usd >= min_usd) {
                    std::cout << "Mining validated: At least " << min_usd << " USD worth of Monero mined." << std::endl;
                    std::string response = "Mining proof validated. At least " + std::to_string(min_usd) + " USD worth of Monero mined.\n";
                    response += password + "\n";  // Send the password to the client
                    boost::asio::write(socket, boost::asio::buffer(response));

                    // Save the current balance for future comparisons
                    save_current_balance(balance_file, current_balance);
                } else {
                    std::cerr << "Mining validation failed: Less than " << min_usd << " USD worth of Monero mined." << std::endl;
                    std::string response = "Mining proof rejected. Less than " + std::to_string(min_usd) + " USD worth of Monero mined.\n";
                    boost::asio::write(socket, boost::asio::buffer(response));
                }
            } else {
                std::cerr << "Error: Unable to fetch Monero price." << std::endl;
                std::string response = "Error: Unable to fetch Monero price.\n";
                boost::asio::write(socket, boost::asio::buffer(response));
            }
        } else {
            std::cerr << "Error: Unable to fetch balance data from Nanopool API." << std::endl;
            std::string response = "Error: Unable to fetch balance data.\n";
            boost::asio::write(socket, boost::asio::buffer(response));
        }

        // Close the socket connection after handling
        socket.close();
    }
    catch (std::exception& e) {
        std::cerr << "Error handling client: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    try {
        // Declare command-line options
        std::string password;
        double min_usd = 1.0;  // Default minimum USD threshold
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help", "produce help message")
            ("password", po::value<std::string>(&password)->required(), "password to be sent to client on proof of work")
            ("min-usd", po::value<double>(&min_usd)->default_value(1.0), "minimum USD value to validate mining");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 1;
        }

        po::notify(vm);  // Throws an error if a required option is missing

        boost::asio::io_context io_context;

        // Create a TCP acceptor to listen on port 8080
        boost::asio::ip::tcp::acceptor acceptor(io_context,
            boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 8080));

        std::cout << "Server is running and waiting for connections on port 8080..." << std::endl;

        while (true) {
            // Accept incoming client connection
            boost::asio::ip::tcp::socket socket(io_context);
            acceptor.accept(socket);

            // Handle the client in a separate function and pass the password and minimum USD
            handle_client(socket, password, min_usd);
        }
    }
    catch (std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
    }

    return 0;
}
