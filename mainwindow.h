#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressDialog>
#include <QMessageBox>
#include <QApplication>
#include <QHeaderView>
#include <windows.h>
#include <vector>
#include <string>
#include <iostream>
#include <tlhelp32.h>
using namespace std;
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE
struct AddressInfo {
    uintptr_t address;
    vector<int> valueHistory;
    AddressInfo(uintptr_t addr, int initialValue) : address(addr) {
        valueHistory.push_back(initialValue);
    }
    void addValue(int newValue) {
        valueHistory.push_back(newValue);
    }
    int getCurrentValue() const {
        return valueHistory.back();
    }
    string getHistoryString() const {
        string history;
        for (size_t i=0;i<valueHistory.size();++i) {
            history+=to_string(valueHistory[i]);
            if (i<valueHistory.size()-1){
                history+="→";
            }
        }
        return history;
    }
};
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private slots:
    void onModifyAllButtonClicked();
    void onRefreshProcessList();
    void onRescanButtonClicked();
    void onModifyButtonClicked();
    void onNewScanButtonClicked();
    void onTableClicked();
    void onRefreshButtonClicked();
private:
    Ui::MainWindow *ui;
    vector<AddressInfo> foundAddresses;
    bool isFirstScan;
    QString ToQtString(const wchar_t* wstr);
    vector<DWORD> GetPID(const QString& processName);
    void RunningProc(QTableWidget* table);
    void ScanMem(DWORD processId, int targetValue);
    void NextScan(DWORD processId, int targetValue);
    void UpdateAddressTable();
};
#endif
