# ATM Simulation System (C++)

**Author: Areen Chandel**

A console-based ATM simulation built in C++ as a Summer Training Program (DSA) project. It models the real workflow of an ATM machine — inserting a card, entering a PIN, and performing common banking operations — with data saved to a file so balances are remembered between runs.

## Features

| # | Feature | Description |
|---|---------|-------------|
| 1 | **Card + PIN login** | Log in with a card number and 4-digit PIN. After 3 wrong PIN attempts the card is **blocked**. |
| 2 | **Balance enquiry** | View available balance. |
| 3 | **Cash withdrawal** | Withdraw any amount in multiples of 100. Checks balance, a daily limit (Rs. 40,000) and the cash available inside the ATM. |
| 4 | **Fast cash** | Quick fixed-amount withdrawal (500 / 1000 / 2000 / 5000 / 10000). |
| 5 | **Note dispensing** | Shows how the amount is made up of real note denominations (2000, 500, 200, 100) using a greedy algorithm. |
| 6 | **Cash deposit** | Add money to the account. |
| 7 | **Fund transfer** | Transfer money to another account, with confirmation. |
| 8 | **Change PIN** | Change the PIN securely (verifies old PIN + confirms new one). |
| 9 | **Mini statement** | Shows the last transactions with date/time, type, amount and balance. |
| 10 | **Data persistence** | All accounts are stored in `accounts.txt`, so balances survive after the program closes. |

## Data Structures & Algorithms Used

| Concept | Where it is used | Why |
|---------|------------------|-----|
| **Structure (`struct Transaction`)** | Stores one transaction: date/time, type, amount, balance after | Groups related fields of different types into one record |
| **Class (`Account`, `ATM`)** | Models a bank account and the machine itself | Encapsulates data with the operations that act on it |
| **Dynamic array (`vector<Account>`)** | Holds all accounts loaded from the file | Grows at runtime; O(1) indexed access |
| **Dynamic array (`vector<Transaction>`)** | Per-account transaction history | Acts as an append-only log; the mini statement reads the last *n* entries |
| **Linear search** | Finding an account by card number for login and fund transfer | O(n) scan over the accounts vector |
| **Greedy algorithm** | Breaking a withdrawal amount into note denominations | Repeatedly takes the largest note that fits, giving the fewest notes |
| **String parsing (`stringstream`)** | Reading and writing the `accounts.txt` records | Splits each saved line back into fields |
| **File handling (`fstream`)** | Loading data on start, saving on exit | Gives persistence without a database |

**Complexity summary**

- Account lookup (login / transfer): **O(n)**, where n = number of accounts
- Deposit / withdrawal / balance update: **O(1)**
- Note dispensing: **O(d)**, where d = number of denominations (constant, 4)
- Mini statement: **O(k)**, where k = number of transactions shown
- Loading / saving the file: **O(n × t)**, accounts × their transactions

**Possible optimisation:** replacing the linear search with a hash map (`unordered_map<string, Account>`) keyed on card number would bring account lookup down to **O(1)** average — the natural next step if the bank had thousands of customers.

## How to compile and run

**Windows (MinGW / g++)**
```bash
g++ -std=c++17 atm.cpp -o atm.exe
atm.exe
```

**Linux / macOS**
```bash
g++ -std=c++17 atm.cpp -o atm
./atm
```

## Demo accounts

On the very first run the program creates two sample accounts automatically:

| Card Number | PIN  | Holder        | Balance    |
|-------------|------|---------------|------------|
| 1001        | 1234 | Rahul Sharma  | Rs. 25,000 |
| 1002        | 5678 | Priya Verma   | Rs. 50,000 |

Use these to log in. (Delete `accounts.txt` any time to reset to these defaults.)

## Project structure

```
DSA_ATM_SIMULATION_AREEN/
├── atm.cpp        # the full program (single file, well commented)
├── accounts.txt   # auto-generated data file (created on first run)
└── README.md      # this file
```

## How the code is organised

- **`struct Transaction`** — one entry in an account's history.
- **`class Account`** — a single bank account (card number, PIN, holder name, balance, transaction history).
- **`class ATM`** — the machine: loads/saves accounts, runs the menus, and performs every operation (withdraw, deposit, transfer, change PIN, etc.).
- **`main()`** — creates an `ATM` object and starts it.

## Future enhancements

- Hash the PIN instead of storing it in plain text.
- Use an `unordered_map` for O(1) account lookup.
- Reset the daily withdrawal limit automatically each new day.
- Add an admin mode to refill ATM cash and add new customers.
- Replace the text file with a real database (SQLite).

---

**Author:** Areen Chandel
**Program:** Summer Training Program — DSA
