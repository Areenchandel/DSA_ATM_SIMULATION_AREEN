/**************************************************************************
 *  ATM SIMULATION SYSTEM  (Console based, C++)
 *  --------------------------------------------------------------------
 *  Summer Training Program - DSA Project
 *  Author: Areen Chandel
 *  --------------------------------------------------------------------
 *
 *  Features (modelled on a real ATM):
 *    1.  Card number + 4-digit PIN login (3 attempts, then card blocked)
 *    2.  Balance enquiry
 *    3.  Cash withdrawal  - checks balance, daily limit and ATM cash,
 *                           dispenses using real note denominations
 *    4.  Fast cash        - quick fixed-amount withdrawal
 *    5.  Cash deposit
 *    6.  Fund transfer    - move money to another account
 *    7.  Change PIN
 *    8.  Mini statement   - last transactions
 *    9.  Data persistence - all accounts saved to a text file, so the
 *                           data is remembered the next time you run it
 *
 *  Data structures used:
 *    struct Transaction        - one record of a transaction
 *    class  Account            - a single bank account
 *    class  ATM                - the machine itself
 *    vector<Account>           - dynamic array holding all accounts
 *    vector<Transaction>       - per-account transaction history (log)
 *    Linear search             - to find an account by card number
 *    Greedy algorithm          - to break an amount into note denominations
 *
 *  Compile :  g++ -std=c++17 atm.cpp -o atm
 *  Run     :  ./atm      (Linux/Mac)   or   atm.exe   (Windows)
 **************************************************************************/

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include <limits>
#include <ctime>

using namespace std;

/* ----------------------------------------------------------------------
 *  Global settings of the machine
 * ------------------------------------------------------------------- */
const string DATA_FILE      = "accounts.txt";
const double DAILY_LIMIT    = 40000.0;   // max a customer can take per day
const int    MAX_PIN_TRIES  = 3;         // wrong PINs before card is blocked
const int    STATEMENT_ROWS = 10;        // how many rows the mini statement shows

/* ----------------------------------------------------------------------
 *  A single transaction record kept in an account's history.
 * ------------------------------------------------------------------- */
struct Transaction {
    string dateTime;      // when it happened
    string type;          // Deposit / Withdraw / Transfer-Out / Transfer-In
    double amount;        // how much money
    double balanceAfter;  // balance left after this transaction

    Transaction() : amount(0), balanceAfter(0) {}

    Transaction(string dt, string t, double amt, double bal)
        : dateTime(dt), type(t), amount(amt), balanceAfter(bal) {}
};

/* ----------------------------------------------------------------------
 *  Returns the current date and time as a short string, e.g. 23-07-26 14:05
 * ------------------------------------------------------------------- */
string currentDateTime() {
    time_t now = time(nullptr);
    tm *lt = localtime(&now);
    char buf[32];
    strftime(buf, sizeof(buf), "%d-%m-%y %H:%M", lt);
    return string(buf);
}

/* ----------------------------------------------------------------------
 *  class Account : one customer's bank account.
 * ------------------------------------------------------------------- */
class Account {
public:
    string cardNumber;
    string pin;
    string holderName;
    double balance;
    double withdrawnToday;
    bool   blocked;
    vector<Transaction> history;   // dynamic array acting as a log

    Account() : balance(0), withdrawnToday(0), blocked(false) {}

    Account(string card, string p, string name, double bal)
        : cardNumber(card), pin(p), holderName(name),
          balance(bal), withdrawnToday(0), blocked(false) {}

    /* add a transaction to this account's history */
    void addTransaction(const string &type, double amount) {
        history.push_back(Transaction(currentDateTime(), type, amount, balance));
    }
};

/* ----------------------------------------------------------------------
 *  Small input helpers - they stop the program from crashing or looping
 *  forever if the user types letters where a number is expected.
 * ------------------------------------------------------------------- */
int readInt(const string &prompt) {
    int value;
    while (true) {
        cout << prompt;
        if (cin >> value) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return value;
        }
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "  !! Please enter a valid number.\n";
    }
}

double readDouble(const string &prompt) {
    double value;
    while (true) {
        cout << prompt;
        if (cin >> value) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return value;
        }
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "  !! Please enter a valid amount.\n";
    }
}

string readLine(const string &prompt) {
    cout << prompt;
    string s;
    getline(cin, s);
    return s;
}

void line(char c = '=', int n = 60) {
    cout << string(n, c) << "\n";
}

void pause() {
    cout << "\n  Press ENTER to continue...";
    cin.get();
}

/* ----------------------------------------------------------------------
 *  class ATM : the machine. Holds every account, loads and saves the
 *  data file, shows the menus and performs all the operations.
 * ------------------------------------------------------------------- */
class ATM {
private:
    vector<Account> accounts;   // all customers
    double atmCash;             // cash physically inside the machine
    int    currentIndex;        // index of the logged-in account (-1 = none)

public:
    ATM() : atmCash(500000.0), currentIndex(-1) {
        loadData();
    }

    /* -------------------- FILE HANDLING -------------------- */

    /* Load every account from accounts.txt. If the file does not exist,
       create two demo accounts so the program can be tested straight away. */
    void loadData() {
        ifstream fin(DATA_FILE);
        if (!fin) {
            createDemoAccounts();
            return;
        }

        accounts.clear();
        string line;
        Account *current = nullptr;

        while (getline(fin, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string tag;
            getline(ss, tag, '|');

            if (tag == "ACC") {
                Account a;
                string bal, wt, blk;
                getline(ss, a.cardNumber, '|');
                getline(ss, a.pin,        '|');
                getline(ss, a.holderName, '|');
                getline(ss, bal,          '|');
                getline(ss, wt,           '|');
                getline(ss, blk,          '|');
                a.balance        = atof(bal.c_str());
                a.withdrawnToday = atof(wt.c_str());
                a.blocked        = (blk == "1");
                accounts.push_back(a);
                current = &accounts.back();
            }
            else if (tag == "TXN" && current != nullptr) {
                Transaction t;
                string amt, bal;
                getline(ss, t.dateTime, '|');
                getline(ss, t.type,     '|');
                getline(ss, amt,        '|');
                getline(ss, bal,        '|');
                t.amount       = atof(amt.c_str());
                t.balanceAfter = atof(bal.c_str());
                current->history.push_back(t);
            }
            else if (tag == "CASH") {
                string c;
                getline(ss, c, '|');
                atmCash = atof(c.c_str());
            }
        }
        fin.close();

        if (accounts.empty()) createDemoAccounts();
    }

    /* Write every account (and its history) back to the file. */
    void saveData() {
        ofstream fout(DATA_FILE);
        if (!fout) {
            cout << "  !! Warning: could not save data to file.\n";
            return;
        }
        fout << "CASH|" << fixed << setprecision(2) << atmCash << "|\n";

        for (size_t i = 0; i < accounts.size(); i++) {
            const Account &a = accounts[i];
            fout << "ACC|" << a.cardNumber << "|" << a.pin << "|"
                 << a.holderName << "|" << fixed << setprecision(2) << a.balance
                 << "|" << a.withdrawnToday << "|" << (a.blocked ? 1 : 0) << "|\n";

            for (size_t j = 0; j < a.history.size(); j++) {
                const Transaction &t = a.history[j];
                fout << "TXN|" << t.dateTime << "|" << t.type << "|"
                     << fixed << setprecision(2) << t.amount << "|"
                     << t.balanceAfter << "|\n";
            }
        }
        fout.close();
    }

    void createDemoAccounts() {
        accounts.clear();
        accounts.push_back(Account("1001", "1234", "Rahul Sharma", 25000.0));
        accounts.push_back(Account("1002", "5678", "Priya Verma",  50000.0));
        saveData();
    }

    /* -------------------- SEARCHING -------------------- */

    /* Linear search : go through the accounts one by one and compare the
       card number. Returns the index, or -1 if it is not found.  O(n)     */
    int findAccount(const string &card) {
        for (size_t i = 0; i < accounts.size(); i++) {
            if (accounts[i].cardNumber == card) return (int)i;
        }
        return -1;
    }

    /* -------------------- SCREENS -------------------- */

    void run() {
        while (true) {
            cout << "\n";
            line('=');
            cout << "                 WELCOME TO CIPHER BANK ATM\n";
            line('=');
            cout << "   1. Insert Card (Login)\n";
            cout << "   2. Exit\n";
            line('-');
            int choice = readInt("   Choose an option: ");

            if (choice == 1) {
                login();
            } else if (choice == 2) {
                saveData();
                cout << "\n   Thank you for using Cipher Bank ATM. Goodbye!\n\n";
                return;
            } else {
                cout << "   !! Invalid option.\n";
            }
        }
    }

    void login() {
        cout << "\n";
        line('-');
        string card = readLine("   Enter Card Number : ");
        int idx = findAccount(card);

        if (idx == -1) {
            cout << "   !! Card not recognised by this machine.\n";
            return;
        }
        if (accounts[idx].blocked) {
            cout << "   !! This card is BLOCKED. Please contact your branch.\n";
            return;
        }

        for (int attempt = 1; attempt <= MAX_PIN_TRIES; attempt++) {
            string pin = readLine("   Enter 4-digit PIN : ");
            if (pin == accounts[idx].pin) {
                currentIndex = idx;
                cout << "\n   Login successful. Welcome, "
                     << accounts[idx].holderName << "!\n";
                mainMenu();
                return;
            }
            int left = MAX_PIN_TRIES - attempt;
            if (left > 0) {
                cout << "   !! Wrong PIN. Attempts remaining: " << left << "\n";
            } else {
                accounts[idx].blocked = true;
                saveData();
                cout << "\n   !! 3 wrong attempts. YOUR CARD HAS BEEN BLOCKED.\n";
            }
        }
    }

    void mainMenu() {
        while (currentIndex != -1) {
            Account &a = accounts[currentIndex];
            cout << "\n";
            line('=');
            cout << "   MAIN MENU  |  " << a.holderName
                 << "  |  Card: " << a.cardNumber << "\n";
            line('=');
            cout << "   1. Balance Enquiry\n";
            cout << "   2. Cash Withdrawal\n";
            cout << "   3. Fast Cash\n";
            cout << "   4. Cash Deposit\n";
            cout << "   5. Fund Transfer\n";
            cout << "   6. Change PIN\n";
            cout << "   7. Mini Statement\n";
            cout << "   8. Logout\n";
            line('-');
            int ch = readInt("   Choose an option: ");

            switch (ch) {
                case 1: balanceEnquiry(); break;
                case 2: withdraw();       break;
                case 3: fastCash();       break;
                case 4: deposit();        break;
                case 5: transfer();       break;
                case 6: changePin();      break;
                case 7: miniStatement();  break;
                case 8:
                    saveData();
                    cout << "\n   Card ejected. Please collect your card.\n";
                    currentIndex = -1;
                    break;
                default:
                    cout << "   !! Invalid option.\n";
            }
        }
    }

    /* -------------------- OPERATIONS -------------------- */

    void balanceEnquiry() {
        Account &a = accounts[currentIndex];
        cout << "\n";
        line('-');
        cout << "   Account Holder    : " << a.holderName << "\n";
        cout << "   Card Number       : " << a.cardNumber << "\n";
        cout << "   Available Balance : Rs. " << fixed << setprecision(2)
             << a.balance << "\n";
        line('-');
        pause();
    }

    /* Greedy algorithm: always take the biggest note that still fits.
       This gives the customer the smallest number of notes.  */
    void dispenseNotes(double amount) {
        int notes[] = {2000, 500, 200, 100};
        int amt = (int)amount;
        cout << "\n   Dispensing cash:\n";
        for (int i = 0; i < 4; i++) {
            int count = amt / notes[i];
            if (count > 0) {
                cout << "      " << setw(4) << count << " x Rs. " << notes[i] << "\n";
                amt %= notes[i];
            }
        }
        cout << "\n   *** Please collect your cash ***\n";
    }

    void doWithdraw(double amount) {
        Account &a = accounts[currentIndex];

        if (amount <= 0) {
            cout << "   !! Invalid amount.\n"; return;
        }
        if ((int)amount % 100 != 0) {
            cout << "   !! Amount must be in multiples of 100.\n"; return;
        }
        if (amount > a.balance) {
            cout << "   !! Insufficient balance. Available: Rs. "
                 << fixed << setprecision(2) << a.balance << "\n";
            return;
        }
        if (a.withdrawnToday + amount > DAILY_LIMIT) {
            cout << "   !! Daily limit of Rs. " << fixed << setprecision(2)
                 << DAILY_LIMIT << " exceeded. Already withdrawn today: Rs. "
                 << a.withdrawnToday << "\n";
            return;
        }
        if (amount > atmCash) {
            cout << "   !! This ATM does not have enough cash right now.\n";
            return;
        }

        a.balance        -= amount;
        a.withdrawnToday += amount;
        atmCash          -= amount;
        a.addTransaction("Withdraw", amount);
        saveData();

        dispenseNotes(amount);
        cout << "   Remaining Balance : Rs. " << fixed << setprecision(2)
             << a.balance << "\n";
    }

    void withdraw() {
        cout << "\n";
        line('-');
        cout << "   CASH WITHDRAWAL   (multiples of 100 only)\n";
        line('-');
        double amt = readDouble("   Enter amount: Rs. ");
        doWithdraw(amt);
        pause();
    }

    void fastCash() {
        cout << "\n";
        line('-');
        cout << "   FAST CASH\n";
        line('-');
        cout << "   1. Rs.   500      2. Rs.  1000\n";
        cout << "   3. Rs.  2000      4. Rs.  5000\n";
        cout << "   5. Rs. 10000      6. Back\n";
        line('-');
        int ch = readInt("   Choose an option: ");
        double amt = 0;
        switch (ch) {
            case 1: amt = 500;   break;
            case 2: amt = 1000;  break;
            case 3: amt = 2000;  break;
            case 4: amt = 5000;  break;
            case 5: amt = 10000; break;
            case 6: return;
            default:
                cout << "   !! Invalid option.\n";
                pause();
                return;
        }
        doWithdraw(amt);
        pause();
    }

    void deposit() {
        Account &a = accounts[currentIndex];
        cout << "\n";
        line('-');
        cout << "   CASH DEPOSIT\n";
        line('-');
        double amt = readDouble("   Enter amount to deposit: Rs. ");

        if (amt <= 0) {
            cout << "   !! Invalid amount.\n";
        } else if ((int)amt % 100 != 0) {
            cout << "   !! Deposit must be in multiples of 100.\n";
        } else {
            a.balance += amt;
            atmCash   += amt;
            a.addTransaction("Deposit", amt);
            saveData();
            cout << "\n   Deposit successful.\n";
            cout << "   Updated Balance : Rs. " << fixed << setprecision(2)
                 << a.balance << "\n";
        }
        pause();
    }

    void transfer() {
        Account &sender = accounts[currentIndex];
        cout << "\n";
        line('-');
        cout << "   FUND TRANSFER\n";
        line('-');
        string card = readLine("   Enter beneficiary card number: ");

        int idx = findAccount(card);
        if (idx == -1) {
            cout << "   !! Beneficiary account not found.\n"; pause(); return;
        }
        if (idx == currentIndex) {
            cout << "   !! You cannot transfer money to your own account.\n"; pause(); return;
        }

        cout << "   Beneficiary Name : " << accounts[idx].holderName << "\n";
        double amt = readDouble("   Enter amount: Rs. ");

        if (amt <= 0) {
            cout << "   !! Invalid amount.\n"; pause(); return;
        }
        if (amt > sender.balance) {
            cout << "   !! Insufficient balance.\n"; pause(); return;
        }

        string ok = readLine("   Confirm transfer? (y/n): ");
        if (ok != "y" && ok != "Y") {
            cout << "   Transfer cancelled.\n"; pause(); return;
        }

        sender.balance -= amt;
        sender.addTransaction("Transfer-Out", amt);

        accounts[idx].balance += amt;
        accounts[idx].addTransaction("Transfer-In", amt);

        saveData();
        cout << "\n   Transfer successful!\n";
        cout << "   Rs. " << fixed << setprecision(2) << amt << " sent to "
             << accounts[idx].holderName << "\n";
        cout << "   Your Balance : Rs. " << sender.balance << "\n";
        pause();
    }

    void changePin() {
        Account &a = accounts[currentIndex];
        cout << "\n";
        line('-');
        cout << "   CHANGE PIN\n";
        line('-');
        string oldPin = readLine("   Enter current PIN : ");
        if (oldPin != a.pin) {
            cout << "   !! Current PIN is incorrect.\n"; pause(); return;
        }
        string newPin = readLine("   Enter new 4-digit PIN : ");
        if (newPin.length() != 4) {
            cout << "   !! PIN must be exactly 4 digits.\n"; pause(); return;
        }
        string confirm = readLine("   Confirm new PIN : ");
        if (newPin != confirm) {
            cout << "   !! The two PINs did not match.\n"; pause(); return;
        }
        a.pin = newPin;
        saveData();
        cout << "\n   PIN changed successfully.\n";
        pause();
    }

    void miniStatement() {
        Account &a = accounts[currentIndex];
        cout << "\n";
        line('=');
        cout << "   MINI STATEMENT  |  " << a.holderName << "\n";
        line('=');

        if (a.history.empty()) {
            cout << "   No transactions yet.\n";
        } else {
            cout << "   " << left << setw(18) << "DATE / TIME"
                 << setw(15) << "TYPE"
                 << right << setw(12) << "AMOUNT"
                 << setw(14) << "BALANCE" << "\n";
            line('-');

            int start = (int)a.history.size() - STATEMENT_ROWS;
            if (start < 0) start = 0;

            for (size_t i = start; i < a.history.size(); i++) {
                const Transaction &t = a.history[i];
                cout << "   " << left << setw(18) << t.dateTime
                     << setw(15) << t.type
                     << right << setw(12) << fixed << setprecision(2) << t.amount
                     << setw(14) << t.balanceAfter << "\n";
            }
        }
        line('=');
        cout << "   Current Balance : Rs. " << fixed << setprecision(2)
             << a.balance << "\n";
        pause();
    }
};

/* ----------------------------------------------------------------------
 *  Program starts here.
 * ------------------------------------------------------------------- */
int main() {
    ATM machine;
    machine.run();
    return 0;
}
