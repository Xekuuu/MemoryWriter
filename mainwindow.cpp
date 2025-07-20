#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QLineEdit>
#include <QTextEdit>
#include <QProgressDialog>
#include <QMessageBox>
#include <QString>
#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , isFirstScan(true)
{
    ui->setupUi(this);


    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::onRescanButtonClicked);
    connect(ui->pushButton_2, &QPushButton::clicked, this, &MainWindow::onModifyAllButtonClicked);
    connect(ui->pushButton_3, &QPushButton::clicked, this, &MainWindow::onModifyButtonClicked);
    connect(ui->table, &QTableWidget::cellClicked, this, &MainWindow::onTableClicked);
    connect(ui->pushButton_4, &QPushButton::clicked, this, &MainWindow::onRefreshButtonClicked);



    ui->table->setColumnCount(2);
    ui->table->setHorizontalHeaderLabels({"Process Name", "PID"});
    ui->table->horizontalHeader()->setStretchLastSection(true);

    ui->tableWidget_2->setColumnCount(3);
    ui->tableWidget_2->setHorizontalHeaderLabels({"Address", "Current Value", "Value History"});
    ui->tableWidget_2->horizontalHeader()->setStretchLastSection(true);


    RunningProc(ui->table);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// wchar->qstring conversion convert
QString MainWindow::ToQtString(const wchar_t* wstr) {
    return QString::fromWCharArray(wstr);
}

vector<DWORD> MainWindow::GetPID(const QString& processName)
{
    vector<DWORD> pids;
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return pids;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hProcessSnap, &pe32)) {
        do {
            QString currentProcessName = ToQtString(pe32.szExeFile);
            if (processName == currentProcessName) {
                pids.push_back(pe32.th32ProcessID);
            }
        } while (Process32Next(hProcessSnap, &pe32));
    }

    CloseHandle(hProcessSnap);
    return pids;
}

void MainWindow::RunningProc(QTableWidget* table)
{
    table->setRowCount(0);

    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hProcessSnap, &pe32)) {
        int row = 0;
        do {
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(ToQtString(pe32.szExeFile)));
            table->setItem(row, 1, new QTableWidgetItem(QString::number(pe32.th32ProcessID)));
            row++;
        } while (Process32Next(hProcessSnap, &pe32));
    }

    CloseHandle(hProcessSnap);
}

void MainWindow::onTableClicked()
{
    int row = ui->table->currentRow();
    if (row >= 0) {
        QTableWidgetItem* pidItem = ui->table->item(row, 1);
        if (pidItem) {
            ui->lineEdit->setText(pidItem->text());
        }
    }
}

void MainWindow::UpdateAddressTable()
{
    ui->tableWidget_2->setRowCount(foundAddresses.size());

    for (size_t i = 0; i < foundAddresses.size(); ++i) {
        //Address
        QTableWidgetItem* addressItem = new QTableWidgetItem(
            QString("0x%1").arg(foundAddresses[i].address, 8, 16, QChar('0')).toUpper());
        ui->tableWidget_2->setItem(i, 0, addressItem);

        //Value
        QTableWidgetItem* valueItem = new QTableWidgetItem(
            QString::number(foundAddresses[i].getCurrentValue()));
        ui->tableWidget_2->setItem(i, 1, valueItem);

        // Value Chain
        QTableWidgetItem* historyItem = new QTableWidgetItem(
            QString::fromStdString(foundAddresses[i].getHistoryString()));
        ui->tableWidget_2->setItem(i, 2, historyItem);
    }
}

void MainWindow::ScanMem(DWORD processId, int targetValue)
{
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, processId);

    QProgressDialog progress("Scanning...", "X", 0, 100, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();

    MEMORY_BASIC_INFORMATION mbi;
    uintptr_t address = 0;
    int progressValue = 0;

    while (VirtualQueryEx(hProcess, (LPCVOID)address, &mbi, sizeof(mbi))) {
        if (progress.wasCanceled()) break;

        progressValue = (int)((address * 100) / 0x7FFFFFFF);
        progress.setValue(progressValue);
        QApplication::processEvents();

        if (mbi.State == MEM_COMMIT &&
            (mbi.Protect & PAGE_READWRITE || mbi.Protect & PAGE_EXECUTE_READWRITE)) {

            vector<BYTE> buffer(mbi.RegionSize);
            SIZE_T bytesRead;

            if (ReadProcessMemory(hProcess, mbi.BaseAddress, buffer.data(), mbi.RegionSize, &bytesRead)) {
                for (SIZE_T i = 0; i <= bytesRead - sizeof(int); i += sizeof(int)) {
                    int value = *(int*)(buffer.data() + i);
                    if (value == targetValue) {
                        uintptr_t foundAddress = (uintptr_t)mbi.BaseAddress + i;
                        foundAddresses.emplace_back(foundAddress, targetValue);
                    }
                }
            }
        }

        address = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
    }

    CloseHandle(hProcess);
    progress.setValue(100);
}

void MainWindow::NextScan(DWORD processId, int targetValue)
{
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, processId);

    QProgressDialog progress("New scan...", "X", 0, foundAddresses.size(), this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();

    vector<AddressInfo> stillValidAddresses;

    for (size_t i = 0; i < foundAddresses.size(); ++i) {
        if (progress.wasCanceled()) break;

        progress.setValue(i);
        QApplication::processEvents();

        int currentValue;
        SIZE_T bytesRead;

        if (ReadProcessMemory(hProcess, (LPCVOID)foundAddresses[i].address,
                              &currentValue, sizeof(int), &bytesRead)) {

            if (currentValue == targetValue) {

                foundAddresses[i].addValue(targetValue);
                stillValidAddresses.push_back(foundAddresses[i]);
            }

        }
    }

    foundAddresses = stillValidAddresses;
    CloseHandle(hProcess);
    progress.setValue(foundAddresses.size());
}

void MainWindow::onRescanButtonClicked()
{
    QString pidText = ui->lineEdit->text();
    QString valueText = ui->lineEdit_2->text();

    DWORD processId = pidText.toULong();
    int targetValue = valueText.toInt();

    if (isFirstScan) {

        foundAddresses.clear();
        ScanMem(processId, targetValue);
        isFirstScan = false;

        if (!foundAddresses.empty()) {
            QMessageBox::information(this, "First Scan Done",QString("Found %1 addresses with value %2.\n\n").arg(foundAddresses.size()).arg(targetValue));
        } else {
            QMessageBox::information(this, "No Results",QString("").arg(targetValue));
        }
    } else {

        NextScan(processId, targetValue);

        if (!foundAddresses.empty()) {
            QMessageBox::information(this, "New Scan Done",QString("Found %1 addresses that went through the complete value chain.\n\n").arg(foundAddresses.size()));
        }
    }

    UpdateAddressTable();
}

void MainWindow::onModifyAllButtonClicked()
{

    QString pidText = ui->lineEdit->text();
    QString valueText = ui->lineEdit_2->text();

    DWORD processId = pidText.toULong();
    int newValue = valueText.toInt();

    HANDLE hProcess = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, processId);
    if (!hProcess) {
        QMessageBox::critical(this, "Error", "MRAZAM WINDOWS");
        return;
    }

    int successCount = 0;
    for (const auto& addrInfo : foundAddresses) {
        SIZE_T bytesWritten;
        if (WriteProcessMemory(hProcess, (LPVOID)addrInfo.address,
                               &newValue, sizeof(int), &bytesWritten)) {
            successCount++;
        }
    }

    CloseHandle(hProcess);

    QMessageBox::information(this, "Modify All",QString("Sucessfully modified all scanned adresses").arg(successCount).arg(foundAddresses.size()));
}

void MainWindow::onModifyButtonClicked()
{
    QString pidText = ui->lineEdit->text();
    QString addressText = ui->lineEdit_3->text();
    QString valueText = ui->lineEdit_2->text();

    DWORD processId = pidText.toULong();
    uintptr_t address = addressText.toULongLong(nullptr, 16);
    int newValue = valueText.toInt();

    HANDLE hProcess = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, processId);

    SIZE_T bytesWritten;
    if (WriteProcessMemory(hProcess, (LPVOID)address, &newValue, sizeof(int), &bytesWritten)) {
        QMessageBox::information(this, "Success", "Sucessfully modified the specific adress");
    }
    CloseHandle(hProcess);
}

void MainWindow::onRefreshButtonClicked()
{
    //clear refresh reset
    ui->lineEdit->clear();
    ui->lineEdit_2->clear();
    ui->lineEdit_3->clear();

    ui->tableWidget_2->setRowCount(0);

    foundAddresses.clear();
    isFirstScan = true;

    RunningProc(ui->table);

    QMessageBox::information(this, "Refresh Done","Inputs wiped");
}

void MainWindow::onNewScanButtonClicked()
{
    foundAddresses.clear();
    isFirstScan = true;
    ui->tableWidget_2->setRowCount(0);

}

void MainWindow::onRefreshProcessList()
{
    RunningProc(ui->table);
    QMessageBox::information(this, "Refresh", "Inputs wiped");
}
