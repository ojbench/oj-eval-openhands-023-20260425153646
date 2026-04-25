
#include "deque.h"
#include <iostream>
#include <string>

using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    Deque<int> dq;
    string command;
    
    while (cin >> command) {
        if (command == "push_front") {
            int x;
            cin >> x;
            dq.push_front(x);
        } else if (command == "push_back") {
            int x;
            cin >> x;
            dq.push_back(x);
        } else if (command == "pop_front") {
            if (!dq.empty()) {
                dq.pop_front();
            }
        } else if (command == "pop_back") {
            if (!dq.empty()) {
                dq.pop_back();
            }
        } else if (command == "front") {
            if (dq.empty()) {
                cout << "empty" << "\n";
            } else {
                cout << dq.front() << "\n";
            }
        } else if (command == "back") {
            if (dq.empty()) {
                cout << "empty" << "\n";
            } else {
                cout << dq.back() << "\n";
            }
        } else if (command == "size") {
            cout << dq.size() << "\n";
        } else if (command == "empty") {
            cout << (dq.empty() ? "true" : "false") << "\n";
        } else if (command == "clear") {
            dq.clear();
        } else if (command == "exit") {
            break;
        }
    }
    
    return 0;
}
