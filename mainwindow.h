#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTcpSocket>
#include <QByteArray>
#include <json.hpp>

using json = nlohmann::json;

struct User
{
    int id;
    std::string username;
    std::string email;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setupUI();
private:
    QLineEdit* m_usernameInput;
    QLineEdit* m_emailInput;
    QPushButton* m_addUserButton;
    QPushButton* m_refreshTableButton;
    QPushButton* m_deleteUserButton;
    QTableWidget* m_table;

    QTcpSocket* m_tcpSocket;
    QByteArray m_buffer;
    int m_missedHeartbeats;

    void sendCommand(const json& j);
    void parseResponse(const json& j);

    void showErrorMessage(const std::string& err);
    void renderUserTable(const std::vector<User>& users);
    void showSuccessMessage(const std::string& answer);
    void updateNetworkStatusUi(bool isConnected);

private slots:
    void onAddUserButtonClicked();
    void onRefreshClicked();
    void onDeleteUserButtonClicked();
    void onReadyRead();
    void errorHandle();
    void onTimerTick();
};
#endif // MAINWINDOW_H
