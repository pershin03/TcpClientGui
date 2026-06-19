#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTimer>
#include <QDebug>
#include <QMessageBox>
#include <QRegularExpression>
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
    connect(m_deleteUserButton, &QPushButton::clicked, this, &MainWindow::onDeleteUserButtonClicked);

    connect(m_tcpSocket, &QTcpSocket::connected, this, [this]() {
        m_buffer.clear();
        m_missedHeartbeats = 0;
        updateNetworkStatusUi(true);
        onRefreshClicked();
    });
    connect(m_tcpSocket, &QTcpSocket::disconnected, this, [this]() {
        updateNetworkStatusUi(false);
    });

    updateNetworkStatusUi(false);

    m_tcpSocket->connectToHost("127.0.0.1", 12345);

    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::onTimerTick);
    timer->start(5000);
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
    m_deleteUserButton = new QPushButton("Удалить пользователя", this);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({"ID", "Username", "Email"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);

    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(m_addUserButton);
    mainLayout->addWidget(m_refreshTableButton);
    mainLayout->addWidget(m_deleteUserButton);
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

    if (status != "success")
    {
        std::string err = j.value("message", "");
        showErrorMessage(err);
        return;
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
        std::string answer = j.value("message", "Операция успешно выполнена");
        showSuccessMessage(answer);
        onRefreshClicked();
    }
}

void MainWindow::showErrorMessage(const std::string &err)
{
    QMessageBox::critical(this, "Ошибка", QString::fromStdString(err));
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
    QMessageBox::information(this, "Успех", QString::fromStdString(answer));
}

void MainWindow::updateNetworkStatusUi(bool isConnected)
{
    if(isConnected)
    {
        setWindowTitle("Клиент управления пользователями");
        m_addUserButton->setEnabled(true);
        m_refreshTableButton->setEnabled(true);
        m_deleteUserButton->setEnabled(true);
    }
    else
    {
        setWindowTitle("Клиент (Потеря связи. Переподключение...)");
        m_addUserButton->setEnabled(false);
        m_refreshTableButton->setEnabled(false);
        m_deleteUserButton->setEnabled(false);
    }
}

void MainWindow::onAddUserButtonClicked()
{
    QString username = m_usernameInput->text().trimmed();
    QString email = m_emailInput->text().trimmed();

    if(username.isEmpty() || email.isEmpty())
    {
        showErrorMessage("Пожалуйста, заполните все поля!");
        return;
    }

    QRegularExpression usernameRegex("^[a-zA-Z0-9_-]{3,20}$");
    QRegularExpressionMatch matchUsername = usernameRegex.match(username);

    if(!matchUsername.hasMatch())
    {
        showErrorMessage("Неверный формат имени! Разрешены только латинские буквы, цифры, знаки '_' и '-'. Длина от 3 до 20 символов.");
        return;
    }

    QRegularExpression emailRegex("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,6}$");
    QRegularExpressionMatch matchEmail = emailRegex.match(email);

    if(!matchEmail.hasMatch())
    {
        showErrorMessage("Неверный формат Email!");
        return;
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

void MainWindow::onDeleteUserButtonClicked()
{
    int currentRow = m_table->currentRow();
    if (currentRow < 0)
    {
        showErrorMessage("Пожалуйста, выберите пользователя в таблице для удаления!");
        return;
    }

    QTableWidgetItem* usernameItem = m_table->item(currentRow, 1);
    QTableWidgetItem* emailItem = m_table->item(currentRow, 2);

    if (!usernameItem || !emailItem)
    {
        showErrorMessage("Ошибка чтения данных из таблицы!");
        return;
    }

    QString username = usernameItem->text().trimmed();
    QString email = emailItem->text().trimmed();

    auto reply = QMessageBox::question(this, "Подтверждение удаления",
                                       QString("Вы уверены, что хотите удалить пользователя %1 (%2)?").arg(username, email),
                                       QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::No)
    {
        return;
    }

    json j;
    j["action"] = "delete_user";
    j["username"] = username.toStdString();
    j["email"] = email.toStdString();

    sendCommand(j);

    m_usernameInput->clear();
    m_emailInput->clear();

}

void MainWindow::onReadyRead()
{
    m_missedHeartbeats = 0;
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
    if (m_tcpSocket->error() != QAbstractSocket::RemoteHostClosedError)
    {
        qWarning() << "Ошибка сетевого сокета клиента:" << m_tcpSocket->errorString();
    }
}

void MainWindow::onTimerTick()
{
    if(m_tcpSocket->state() == QTcpSocket::UnconnectedState)
    {
        updateNetworkStatusUi(false);
        m_tcpSocket->connectToHost("127.0.0.1", 12345);
    }
    else if(m_tcpSocket->state() == QTcpSocket::ConnectedState)
    {
        updateNetworkStatusUi(true);
        m_missedHeartbeats++;
        if(m_missedHeartbeats >= 3)
        {
            qDebug() << "Timeout heartbeat.";
            m_tcpSocket->abort();
            showErrorMessage("Сервер перестал отвечать на запросы!");
            return;
        }
    }

    onRefreshClicked();
}


