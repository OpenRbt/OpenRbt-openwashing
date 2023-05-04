#include "dia_devicemanager.h"

#include <dirent.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiringPi.h>

#include <stdexcept>
#include <string>

#include "dia_cardreader.h"
#include "dia_ccnet.h"
#include "dia_microcoinsp.h"
#include "dia_nv9usb.h"
#include "dia_vendotek.h"
#include "money_types.h"

void DiaDeviceManager_AddCardReader(DiaDeviceManager *manager) {
    printf("Abstract card reader added to the Device Manager\n");
    manager->_CardReader = new DiaCardReader(manager, DiaDeviceManager_ReportMoney);
}

int DiaDeviceManager_AddVendotek(DiaDeviceManager *manager, std::string host, std::string port) {
    printf("Vendotek card reader added to the Device Manager\n");
    manager->_Vendotek = new DiaVendotek(manager, DiaDeviceManager_ReportMoney, host, port);
    return DiaVendotek_StartPing(manager->_Vendotek);
}

void DiaDeviceManager_StartDeviceScan(DiaDeviceManager *manager) {
    for (auto it = manager->_Devices.begin(); it != manager->_Devices.end(); ++it) {
        (*it)->_CheckStatus = DIAE_DEVICE_STATUS_INITIAL;
    }
}

std::string DiaDeviceManager_ExecBashCommand(const char *cmd, int *error) {
    char buffer[128];
    std::string result = "";
    *error = 0;

    FILE *pipe = popen(cmd, "r");
    if (!pipe) {
        *error = 1;
        return result;
    }

    while (fgets(buffer, sizeof buffer, pipe) != NULL) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}

int DiaDeviceManager_CheckNV9(char *PortName) {
    printf("\nChecking port %s for NV9 device...\n", PortName);

    int error = 0;
    std::string bashOutput = DiaDeviceManager_ExecBashCommand("ls -l /dev/serial/by-id", &error);
    if (error) {
        printf("Error while reading info about serial devices, NV9 check failed\n");
        return 0;
    }

    std::string portName = std::string(PortName);
    size_t maxDiff = 50;

    // Get short name of port, for instance:
    //   /dev/ttyACM0 ==> /ttyACM0
    std::string toCut = portName.substr(0, 4);

    if (toCut != std::string("/dev")) {
        printf("Invlaid port name in NV9 device check: %s\n", PortName);
        return 0;
    }

    std::string shortPortName = portName.substr(4, 8);
    size_t devicePortPosition = bashOutput.find(shortPortName);

    // Check existance of port in list
    if (devicePortPosition != std::string::npos) {
        size_t deviceNamePosition = bashOutput.find("NV9USB");

        // Compare distance between positions with maxDiff const
        if (deviceNamePosition != std::string::npos &&
            devicePortPosition - deviceNamePosition < maxDiff) {
            return 1;
        }
    }

    return 0;
}

int DiaDeviceManager_CheckUIC(char *PortName) {
    printf("\nChecking port %s for UIC device...\n", PortName);

    int error = 0;
    std::string bashOutput = DiaDeviceManager_ExecBashCommand("ls -l /dev/serial/by-id", &error);
    if (error) {
        printf("Error while reading info about serial devices, NV9 check failed\n");
        return 0;
    }

    std::string portName = std::string(PortName);
    size_t maxDiff = 50;

    // Get short name of port, for instance:
    //   /dev/ttyACM0 ==> /ttyACM0
    std::string toCut = portName.substr(0, 4);

    if (toCut != std::string("/dev")) {
        printf("Invlaid port name in UIC device check: %s\n", PortName);
        return 0;
    }

    std::string shortPortName = portName.substr(4, 8);
    size_t devicePortPosition = bashOutput.find(shortPortName);

    // Check existance of port in list
    if (devicePortPosition != std::string::npos) {
        size_t deviceNamePosition = bashOutput.find("UIC");

        // Compare distance between positions with maxDiff const
        if (deviceNamePosition != std::string::npos &&
            devicePortPosition - deviceNamePosition < maxDiff) {
            return 1;
        }
    }

    return 0;
}

void DiaDeviceManager_CheckOrAddDevice(DiaDeviceManager *manager, char *PortName, int isACM) {
    int devInList = 0;

    for (auto it = manager->_Devices.begin(); it != manager->_Devices.end(); ++it) {
        if (strcmp(PortName, (*it)->_PortName) == 0) {
            devInList = 1;
            (*it)->_CheckStatus = DIAE_DEVICE_STATUS_IN_LIST;
        }
    }
    if (!devInList) {
        if (isACM) {
            if (DiaDeviceManager_CheckUIC(PortName)) {
                printf("\nFound UIC on port %s\n\n", PortName);
                printf("Ignoring this port...\n");
                DiaDevice *dev = new DiaDevice(PortName);
                manager->_Devices.push_back(dev);

            } else if (DiaDeviceManager_CheckNV9(PortName)) {
                printf("\nFound NV9 on port %s\n\n", PortName);
                DiaDevice *dev = new DiaDevice(PortName);

                dev->Manager = manager;
                dev->_CheckStatus = DIAE_DEVICE_STATUS_JUST_ADDED;
                dev->Open();
                DiaNv9Usb *newNv9 = new DiaNv9Usb(dev, DiaDeviceManager_ReportMoney);
                DiaNv9Usb_StartDriver(newNv9);
                manager->_Devices.push_back(dev);
            }
        }
        if (!isACM) {
            printf("\nChecking port %s for MicroCoinSp...\n", PortName);
            DiaDevice *dev = new DiaDevice(PortName);

            dev->Manager = manager;
            dev->_CheckStatus = DIAE_DEVICE_STATUS_JUST_ADDED;
            dev->Open();

            int res = DiaMicroCoinSp_Detect(dev);
            if (res) {
                printf("\nFound MicroCoinSp on port %s\n\n", PortName);
                DiaMicroCoinSp *newMicroCoinSp = new DiaMicroCoinSp(dev, DiaDeviceManager_ReportMoney);
                DiaMicroCoinSp_StartDriver(newMicroCoinSp);
                manager->_Devices.push_back(dev);
            } else {
                res = DiaCcnet_Detect(dev);
                if (res) {
                    printf("\nFound CCNET device on port %s\n\n", PortName);
                    DiaCcnet *newCcnet = new DiaCcnet(dev, DiaDeviceManager_ReportMoney);
                    DiaCcnet_StartDriver(newCcnet);
                    manager->_Devices.push_back(dev);
                } else {
                    printf("\nNo devices found on port %s\n", PortName);
                    DiaDevice_CloseDevice(dev);
                }
            }
        }
    }
}

void DiaDeviceManager_ScanDevices(DiaDeviceManager *manager) {
    if (manager == NULL) {
        return;
    }
    struct dirent *entry;
    DIR *dir;
    DiaDeviceManager_StartDeviceScan(manager);
    if ((dir = opendir("/dev")) != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            if (strstr(entry->d_name, "ttyACM")) {
                char buf[1024];
                snprintf(buf, 1023, "/dev/%s", entry->d_name);
                DiaDeviceManager_CheckOrAddDevice(manager, buf, 1);
            } else if (strstr(entry->d_name, "ttyUSB")) {
                char buf[1024];
                snprintf(buf, 1023, "/dev/%s", entry->d_name);
                DiaDeviceManager_CheckOrAddDevice(manager, buf, 0);
            }
        }
        closedir(dir);
    }
}

DiaDeviceManager::DiaDeviceManager() {
    NeedWorking = 1;
    CoinMoney = 0;
    BanknoteMoney = 0;
    ElectronMoney = 0;
    ServiceMoney = 0;
#ifdef SCAN_DEVICES
    pthread_create(&WorkingThread, NULL, DiaDeviceManager_WorkingThread, this);
#endif
}

DiaDeviceManager::~DiaDeviceManager() {
}

void DiaDeviceManager_ReportMoney(void *manager, int moneyType, int money) {
    printf("Entered report money\n");
    DiaDeviceManager *Manager = (DiaDeviceManager *)manager;
    if (moneyType == DIA_BANKNOTES) {
        Manager->BanknoteMoney += money;
    } else if (moneyType == DIA_COINS) {
        Manager->CoinMoney += money;
    } else if (moneyType == DIA_ELECTRON) {
        Manager->ElectronMoney += money;
    } else if (moneyType == DIA_SERVICE) {
        Manager->ServiceMoney += money;
    } else {
        printf("ERROR: Unknown money type %d\n", money);
    }
    printf("Money: %d\n", money);
}

void *DiaDeviceManager_WorkingThread(void *manager) {
    DiaDeviceManager *Manager = (DiaDeviceManager *)manager;

    while (Manager->NeedWorking) {
        DiaDeviceManager_ScanDevices(Manager);
        delay(100);
    }
    return 0;
}

void DiaDeviceManager_PerformTransaction(void *manager, int money) {
    if (manager == NULL) {
        printf("DiaDeviceManager Perform Transaction got NULL driver\n");
        return;
    }
    DiaDeviceManager *Manager = (DiaDeviceManager *)manager;
    printf("DiaDeviceManager got Perform Transaction, money = %d\n", money);
    if (Manager->_CardReader) {
        printf("DiaDeviceManager Perform Transaction CardReader\n");
        DiaCardReader_PerformTransaction(Manager->_CardReader, money);
    } else if (Manager->_Vendotek) {
        printf("DiaDeviceManager Perform Transaction Vendotek\n");
        DiaVendotek_PerformTransaction(Manager->_Vendotek, money);
    }
}

void DiaDeviceManager_AbortTransaction(void *manager) {
    DiaDeviceManager *Manager = (DiaDeviceManager *)manager;
    if (manager == NULL) {
        printf("DiaDeviceManager Abort Transaction got NULL driver\n");
        return;
    }
    if (Manager->_CardReader) {
        DiaCardReader_AbortTransaction(Manager->_CardReader);
    } else if (Manager->_Vendotek) {
        DiaVendotek_AbortTransaction(Manager->_Vendotek);
    }
}

int DiaDeviceManager_GetTransactionStatus(void *manager) {
    DiaDeviceManager *Manager = (DiaDeviceManager *)manager;
    if (manager == NULL) {
        printf("DiaDeviceManager Get Transaction Status got NULL driver\n");
        return -1;
    }
    int res = 0;
    if (Manager->_CardReader) {
        res = DiaCardReader_GetTransactionStatus(Manager->_CardReader);
    } else if (Manager->_Vendotek) {
        res = DiaVendotek_GetTransactionStatus(Manager->_Vendotek);
    }
    return res;
}

int DiaDeviceManager_GetCardReaderStatus(void *manager) {
    DiaDeviceManager *Manager = (DiaDeviceManager *)manager;
    if (manager == NULL) {
        printf("DiaDeviceManager Get Transaction Status got NULL driver\n");
        return -1;
    }
    int res = 0;
    if (Manager->_CardReader) {
        res = 1;
    } else if (Manager->_Vendotek) {
        res = DiaVendotek_GetAvailableStatus(Manager->_Vendotek);
    }
    return res;
}
