#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();

    m_tcpSocket = new QTcpSocket(this);

    connect(m_tcpSocket, &QTcpSocket::readyRead, this, &MainWindow::onReadyRead);
    connect(m_tcpSocket, &QTcpSocket::errorOccurred, this, &MainWindow::errorHandle);

    connect(m_addUserButton, &QPushButton::clicked, this, &MainWindow::onAddUserButtonClicked);

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

    m_table = new QTableWidget(this);
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({"ID", "Username", "Email"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(m_addUserButton);
    mainLayout->addWidget(m_table);

    setCentralWidget(centralWidget);

    setWindowTitle("Клиент управления пользователями");
    resize(600, 400);
}

void MainWindow::parseResponse(const json& j)
{

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
                qDebug << "Error parse json from server" << e.what();
            }

        }
    }
}


