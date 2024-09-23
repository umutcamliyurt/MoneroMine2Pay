# MoneroMine2Pay
## A private payment and PoW tool by mining Monero
<!-- DESCRIPTION -->
## Description:

Monero-based proof of work (PoW) offers significant benefits for server maintenance and spam prevention by leveraging computational challenges to validate new client connections. This process requires participants to expend computational resources, deterring malicious actors from overwhelming servers with spam or denial-of-service attacks.

<!-- FEATURES -->
## Features:

- Prevents spam and abuse

- Supports server maintenance by mining Monero

- Server calculates balance of wallet to validate PoW challenge

- Built in C++

<!-- INSTALLATION -->
## Installation:

Install [Xmrig](https://github.com/xmrig/xmrig/releases) and move it into the same directory with MoneroMine2Pay

```
git clone https://github.com/umutcamliyurt/MoneroMine2Pay
cd MoneroMine2Pay/
sudo apt-get install libboost-all-dev libssl-dev libcurl4-openssl-dev
g++ server.cpp -o server -lcurl -lboost_system -lboost_program_options -I./include
g++ client.cpp -o client -lboost_system -lcurl -lpthread
```

<!-- DEMO -->
## Demo:

### Client:

```
./client
[2024-09-23 11:07:40.730]  net      new job from xmr-eu1.nanopool.org:10300 diff 480045 algo rx/0 height 3243641 (9 tx)
[2024-09-23 11:07:48.319]  cpu      accepted (2/0) diff 480045 (211 ms)
[2024-09-23 11:07:51.748]  net      new job from xmr-eu1.nanopool.org:10300 diff 480045 algo rx/0 height 3243641 (10 tx)
[2024-09-23 11:08:24.191]  miner    speed 10s/60s/15m 1451.0 1556.3 n/a H/s max 1682.1 H/s
[2024-09-23 11:08:31.872]  net      new job from xmr-eu1.nanopool.org:10300 diff 480045 algo rx/0 height 3243641 (23 tx)
[2024-09-23 11:09:03.750]  net      new job from xmr-eu1.nanopool.org:10300 diff 480045 algo rx/0 height 3243642 (8 tx)
[2024-09-23 11:09:13.054]  signal   Ctrl+C received, exiting
[2024-09-23 11:09:13.058]  cpu      stopped (3 ms)
Server response: Mining proof rejected. Less than 0.100000 USD worth of Monero mined.
Mining validation failed. No password received.
```

### Server:

```
./server --password 3425234 --min-usd 0.1
Server is running and waiting for connections on port 8080...
Received mined proof (reported hashrate): 0
Checking balance for wallet: 8AxuYaNHVrGYPQTZ6e6hUT6jMB6eHB5XsLN8JUK7XYtYVq72xN7WV9wTzxyiPyZTooMeykErHv1S3ibs5sLvn5MZBg4Sa2v
Current balance: 2.8353e-06
Previous balance: 2.8353e-06
Mined Monero: 2.8353e-06 XMR (0.000491613 USD)
Mining validation failed: Less than 0.1 USD worth of Monero mined.
```

<!-- LICENSE -->
## License

Distributed under the MIT License. See `LICENSE` for more information.