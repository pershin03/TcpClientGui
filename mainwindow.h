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
    QTableWidget* m_table;

    QTcpSocket* m_tcpSocket;
    QByteArray m_buffer;

    void parseResponse(const json& j);

private slots:
    void onAddUserButtonClicked();
    void onReadyRead();
    void errorHandle();
};
#endif // MAINWINDOW_H
