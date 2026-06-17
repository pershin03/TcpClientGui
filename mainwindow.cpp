#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDebug>
#include <vector>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();

    m_tcpSocket = new QTcpSocket(this);

    connect(m_tcpSocket, &QTcpSocket::readyRead, this, &MainWindow::onReadyRead);
    connect(m_tcpSocket, &QTcpSocket::errorOccurred, this, &MainWindow::errorHandle);

    connect(m_addUserButton, &QPushButton::clicked, this, &MainWindow::onAddUserButtonClicked);
    connect(m_refreshTableButton, &QPushButton::clicked, this, &MainWindow::onRefreshClicked);

    m_tcpSocket->connectToHost("127.0.0.1", 12345);

}

MainWindow::~MainWindow()
{
    m_tcpSocket->disconnectFromHost();
}

void MainWindow::setupUI()
{
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    QHBoxLayout* formLayout = new QHBoxLayout();

    m_usernameInput = new QLineEdit(this);
    m_emailInput = new QLineEdit(this);

    formLayout->addWidget(new QLabel("Имя пользователя:", this));
    formLayout->addWidget(m_usernameInput);
    formLayout->addWidget(new QLabel("Email:", this));
    formLayout->addWidget(m_emailInput);

    m_addUserButton = new QPushButton("Добавить пользователя", this);
    m_refreshTableButton = new QPushButton("Обновить таблицу", this);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({"ID", "Username", "Email"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(m_addUserButton);
    mainLayout->addWidget(m_refreshTableButton);
    mainLayout->addWidget(m_table);

    setCentralWidget(centralWidget);

    setWindowTitle("Клиент управления пользователями");
    resize(600, 400);
}

void MainWindow::sendCommand(const json& j)
{
    if(m_tcpSocket->state() == QTcpSocket::UnconnectedState)
    {
        m_tcpSocket->connectToHost("127.0.0.1", 12345);
    }

    if (m_tcpSocket->state() == QAbstractSocket::ConnectedState)
    {
        std::string requestStr = j.dump() + "\n";
        m_tcpSocket->write(requestStr.c_str(), requestStr.length());
        m_tcpSocket->flush();
    }
}

void MainWindow::parseResponse(const json& j)
{
    std::string status = j.value("status", "");

    // status: error, ok(ping pong), success
    if (status != "success")
    {
        std::string err = j.value("message", "");
        showErrorMessage(err);
    }

    if (j.contains("users") && j["users"].is_array())
    {
        std::vector<User> receivedUsers;
        for (const auto& jsonUser : j["users"])
        {
            User user;
            user.id = jsonUser.value("id", 0);
            user.username = jsonUser.value("username", "");
            user.email = jsonUser.value("email", "");
            receivedUsers.push_back(user);
        }
        renderUserTable(receivedUsers);
    }
    else
    {
        std::string answer = j.value("message", "Пользователь успешно добавлен");
        showSuccessMessage(answer);
    }


}

void MainWindow::showErrorMessage(const std::string &err)
{

}

void MainWindow::renderUserTable(const std::vector<User> &users)
{
    m_table->setRowCount(0);
    int row = 0;
    for (const auto& user : users) {
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(QString::number(user.id)));
        m_table->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(user.username)));
        m_table->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(user.email)));
        row++;
    }
}

void MainWindow::showSuccessMessage(const std::string &answer)
{

}

void MainWindow::onAddUserButtonClicked()
{
    QString username = m_usernameInput->text().trimmed();
    QString email = m_emailInput->text().trimmed();

    if(username.isEmpty() || email.isEmpty())
    {
        showErrorMessage("Пожалуйста, заполните все поля!");
    }

    json j;
    j["action"] = "add_user";
    j["username"] = username.toStdString();
    j["email"] = email.toStdString();

    m_usernameInput->clear();
    m_emailInput->clear();

    sendCommand(j);
}

void MainWindow::onRefreshClicked()
{
    json j;
    j["action"] = "get_users";
    sendCommand(j);
}

void MainWindow::onReadyRead()
{
    m_buffer.append(m_tcpSocket->readAll());

    while(m_buffer.contains('\n'))
    {
        int separatorIndex = m_buffer.indexOf('\n');
        QByteArray reply = m_buffer.left(separatorIndex);
        m_buffer.remove(0, separatorIndex + 1);
        if (!reply.isEmpty())
        {
            try
            {
                json response = json::parse(reply.begin(), reply.end());
                parseResponse(response);
            }
            catch (const json::parse_error& e)
            {
                qDebug() << "Error parse json from server: " << e.what();
            }

        }
    }
}

void MainWindow::errorHandle()
{

}


