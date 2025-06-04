#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <ctime>
#include <iomanip>
#include <sstream>
using namespace std;

const int MAX_TRANSACTIONS = 10;
const int DIFFICULTY = 6;

unsigned long djb2(const string &str)
{
    unsigned long hash = 5381;
    for (char c : str)
        hash = ((hash << 5) + hash) + c;
    return hash;
}

struct Transaction
{
    string sender;
    string recipient;
    double amount;
    time_t timestamp;

    Transaction(string s, string r, double amt)
        : sender(move(s)), recipient(move(r)), amount(amt), timestamp(time(nullptr)) {}
};

class Block
{
public:
    int index;
    vector<Transaction> transactions;
    time_t timestamp;
    string previous_hash;
    string hash;
    int nonce;
    Block *next;

    Block(int idx, const vector<Transaction> &txs, const string &prev_hash)
        : index(idx), transactions(txs), timestamp(time(nullptr)),
          previous_hash(prev_hash), nonce(0), next(nullptr)
    {
        mine();
    }

    void mine()
    {
        string target(DIFFICULTY, '0');
        while (true)
        {
            hash = calculate_hash();
            if (hash.substr(0, DIFFICULTY) == target)
            {
                break;
            }
            nonce++;
        }
        cout << "Block mined: " << hash << endl;
    }

    string calculate_hash() const
    {
        stringstream ss;
        ss << index << previous_hash << timestamp << nonce;
        for (const auto &tx : transactions)
        {
            ss << tx.sender << tx.recipient << fixed << setprecision(2)
               << tx.amount << tx.timestamp;
        }
        unsigned long hashed = djb2(ss.str());
        stringstream hs;
        hs << setw(12) << setfill('0') << hex << hashed;
        return hs.str();
    }

    void print() const
    {
        cout << "Block #" << index << endl;
        cout << "Timestamp: " << timestamp << endl;
        cout << "Previous Hash: " << previous_hash << endl;
        cout << "Hash: " << hash << endl;
        cout << "Nonce: " << nonce << endl;
        cout << "Transactions (" << transactions.size() << "):\n";
        for (const auto &tx : transactions)
            cout << "  " << tx.sender << " -> " << tx.recipient
                 << ": " << tx.amount << endl;
        cout << endl;
    }
};

class Blockchain
{
private:
    Block *head;
    Block *tail;
    int size;
    vector<Transaction> pending_transactions;
    double mining_reward;
    unordered_map<string, double> wallet_balances;

public:
    Blockchain() : head(nullptr), tail(nullptr), size(0), mining_reward(10.0)
    {
        create_genesis_block();
    }

    ~Blockchain()
    {
        Block *curr = head;
        while (curr)
        {
            Block *next = curr->next;
            delete curr;
            curr = next;
        }
    }

    void create_genesis_block()
    {
        cout << "Mining genesis block..." << endl;
        Block *genesis = new Block(0, vector<Transaction>(), "0");
        head = tail = genesis;
        size = 1;
        cout << "Genesis block created.\n";
    }

    Block *get_latest_block()
    {
        return tail;
    }

    bool add_transaction(const string &sender, const string &recipient, double amount)
    {
        if (pending_transactions.size() >= MAX_TRANSACTIONS)
        {
            cout << "Pending transaction limit reached\n";
            return false;
        }

        if (sender != "SYSTEM")
        {
            double pending_outflow = 0.0;
            for (const auto &tx : pending_transactions)
            {
                if (tx.sender == sender)
                {
                    pending_outflow += tx.amount;
                }
            }
            double available = wallet_balances[sender] - pending_outflow;
            if (available < amount)
            {
                cout << "Insufficient funds for " << sender << " (Available after pending: " << available
                     << ", Required: " << amount << ")\n";
                return false;
            }
        }

        pending_transactions.emplace_back(sender, recipient, amount);
        cout << "Transaction added: " << sender << " -> " << recipient << ": " << amount << endl;
        return true;
    }

    void mine_pending_transactions(const string &miner_address)
    {
        cout << "Mining new block...\n";
        // Prepare transactions for this block (including reward)
        vector<Transaction> block_transactions = pending_transactions;
        block_transactions.emplace_back("SYSTEM", miner_address, mining_reward);

        Block *new_block = new Block(size, block_transactions, tail->hash);
        tail->next = new_block;
        tail = new_block;
        size++;

        // Update balances for all transactions in the block
        for (const auto &tx : block_transactions)
        {
            if (tx.sender != "SYSTEM")
                wallet_balances[tx.sender] -= tx.amount;
            wallet_balances[tx.recipient] += tx.amount;
        }
        pending_transactions.clear();

        cout << "Block mined and added to blockchain\n";
    }

    void print_chain() const
    {
        cout << "\n===== BLOCKCHAIN =====\n";
        Block *curr = head;
        while (curr)
        {
            curr->print();
            curr = curr->next;
        }
        cout << "======================\n";
    }

    void check_balance(const string &wallet) const
    {
        auto it = wallet_balances.find(wallet);
        double balance = (it != wallet_balances.end()) ? it->second : 0.0;
        cout << "Balance for " << wallet << ": " << balance << endl;
    }

    void list_wallets() const
    {
        cout << "\nKnown Wallet Addresses:\n";
        for (const auto &entry : wallet_balances)
            cout << " - " << entry.first << endl;
    }

    bool is_chain_valid() const
    {
        Block *curr = head;
        while (curr && curr->next)
        {
            Block *next = curr->next;

            string expected_hash = next->calculate_hash();
            if (next->hash != expected_hash)
            {
                cout << "  Invalid hash at block #" << next->index << endl;
                cout << "  Stored:   " << next->hash << endl;
                cout << "  Expected: " << expected_hash << endl;
                return false;
            }

            if (next->previous_hash != curr->hash)
            {
                cout << " Invalid previous hash at block #" << next->index << endl;
                cout << " Stored prev: " << next->previous_hash << endl;
                cout << " Expected:    " << curr->hash << endl;
                return false;
            }

            curr = next;
        }
        return true;
    }
};

int main()
{
    Blockchain blockchain;
    int choice;
    string sender, recipient, miner;
    double amount;

    while (true)
    {
        cout << "\n===== BLOCKCHAIN MENU =====\n";
        cout << "1. Add Transaction\n2. Mine Block\n3. Print Blockchain\n4. Validate Blockchain\n";
        cout << "5. Check Balance\n6. List Wallets\n7. Exit\n8. Test\nChoice: ";
        cin >> choice;
        cin.ignore();

        switch (choice)
        {
        case 1:
            cout << "Enter sender: ";
            getline(cin, sender);
            cout << "Enter recipient: ";
            getline(cin, recipient);
            cout << "Enter amount: ";
            cin >> amount;
            cin.ignore();
            blockchain.add_transaction(sender, recipient, amount);
            break;
        case 2:
            cout << "Enter miner address: ";
            getline(cin, miner);
            blockchain.mine_pending_transactions(miner);
            break;
        case 3:
            blockchain.print_chain();
            break;
        case 4:
            if (blockchain.is_chain_valid())
            {
                cout << "Blockchain is valid!\n";
            }
            else
            {
                cout << "Blockchain is invalid!\n";
            }
            break;
        case 5:
            cout << "Enter wallet address: ";
            getline(cin, sender);
            blockchain.check_balance(sender);
            break;
        case 6:
            blockchain.list_wallets();
            break;
        case 7:
            cout << "Exiting...\n";
            return 0;
        case 8:
        { // For testing purpose
            Block *tampered = blockchain.get_latest_block();
            if (tampered && tampered->index > 0 && !tampered->transactions.empty())
            {
                tampered->transactions[0].amount += 999.99;
                cout << "Block tampered successfully.\n";
            }
            else
            {
                cout << "No tamperable block .\n";
            }
            break;
        }

        default:
            cout << "Invalid choice. Try again.\n";
            break;
        }
    }

    return 0;
}
