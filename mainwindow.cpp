#include "MainWindow.h"
#include <QVBoxLayout>
#include <QWidget>
#include <QDebug>
#include <QLineEdit>
#include <QLabel>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) 
    : QMainWindow(parent), isNetworkMode(false) {

    QWidget *central = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(central);
    resize(800, 800);

    // Initialisation du NetworkManager
    networkManager = new NetworkManager(this);

    // Connexions des signaux du NetworkManager
    connect(networkManager, &NetworkManager::sessionCreated, this, &MainWindow::onSessionCreated);
    connect(networkManager, &NetworkManager::clientJoined, this, &MainWindow::onClientJoined);
    connect(networkManager, &NetworkManager::joinedSession, this, &MainWindow::onJoinedSession);
    connect(networkManager, &NetworkManager::sessionStarted, this, &MainWindow::onSessionStarted);
    connect(networkManager, &NetworkManager::connectionError, this, &MainWindow::onConnectionError);

    // Création des boutons
    soloButton = new QPushButton("Solo Mode");
    hostButton = new QPushButton("Host Mode");
    joinButton = new QPushButton("Join Mode");
    startSessionButton = new QPushButton("Start Session");
    startSessionButton->hide();

    sessionCodeLabel = new QLabel("Session Code: ");
    sessionCodeLabel->hide();

    joinCodeInput = new QLineEdit();
    joinCodeInput->setPlaceholderText("Enter session code");
    joinCodeInput->hide();

    joinSubmitButton = new QPushButton("Rejoindre");
    joinSubmitButton->hide();
    backButtonJoin = new QPushButton("Retour");
    backButtonJoin->hide();

    // Ajout des widgets au layout
    layout->addWidget(soloButton);
    layout->addWidget(hostButton);
    layout->addWidget(joinButton);
    layout->addWidget(sessionCodeLabel);
    layout->addWidget(joinCodeInput);
    layout->addWidget(startSessionButton);
    layout->addWidget(joinSubmitButton);
    layout->addWidget(backButtonJoin);

    padPage = new PadPage;
    layout->addWidget(padPage);
    padPage->hide();

    // Connexions des signaux UI
    connect(soloButton, &QPushButton::clicked, this, &MainWindow::handleSoloMode);
    connect(hostButton, &QPushButton::clicked, this, &MainWindow::handleHostMode);
    connect(joinButton, &QPushButton::clicked, this, &MainWindow::handleJoinMode);
    connect(startSessionButton, &QPushButton::clicked, this, &MainWindow::startNetworkSession);
    connect(joinSubmitButton, &QPushButton::clicked, this, &MainWindow::attemptJoinSession);

    connect(joinCodeInput, &QLineEdit::returnPressed, this, &MainWindow::attemptJoinSession);
    connect(backButtonJoin, &QPushButton::clicked, this, [this]() {
        hideJoinInterface();
        showModeButtons();
    });

    // Connexions PadPage
    connect(padPage, &PadPage::padClicked, this, &MainWindow::playPadSound);
    connect(padPage, &PadPage::uploadSoundRequested, this, &MainWindow::uploadSound);

    setCentralWidget(central);
}

void MainWindow::handleSoloMode() {
    hideModeButtons();
    isNetworkMode = false;
    showGameInterface();
    qDebug() << "Solo mode activated";
}

void MainWindow::handleHostMode() {
    hideModeButtons();
    isNetworkMode = true;

    qDebug() << "Starting host mode...";
    networkManager->startHosting();
}

void MainWindow::handleJoinMode() {
    hideModeButtons();
    isNetworkMode = true;

    joinCodeInput->clear();
    joinCodeInput->show();
    joinSubmitButton->show();
    backButtonJoin->show();
    joinCodeInput->setFocus();

    qDebug() << "Join mode activated";
}

void MainWindow::startNetworkSession() {
    if (networkManager->isHost()) {
        qDebug() << "Starting network session...";
        networkManager->startGameSession();
    }
}

void MainWindow::attemptJoinSession() {
    QString code = joinCodeInput->text().trimmed().toUpper();
    if (code.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Veuillez entrer un code de session.");
        return;
    }

    if (code.length() != 6) {
        QMessageBox::warning(this, "Erreur", "Le code de session doit contenir 6 caractères.");
        return;
    }

    qDebug() << "Attempting to join session with code:" << code;
    joinSubmitButton->setEnabled(false);
    joinSubmitButton->setText("Connexion...");

    networkManager->joinSession(code);
}

void MainWindow::playPadSound(int index) {
    qDebug() << "Play sound on pad" << index;

    if (isNetworkMode && networkManager->isConnected()) {
        // En mode réseau, utiliser le SoundManager du NetworkManager
        networkManager->getSoundManager()->playSound(index);
    } else {
        // En mode solo, utiliser le SoundManager local
        soundManager.playSound(index);
    }
}

void MainWindow::uploadSound(int index, const QString &path) {
    qDebug() << "Upload sound for pad" << index << ":" << path;
    padPage->setPadLabel(index, QFileInfo(path).fileName());

    if (isNetworkMode && networkManager->isConnected()) {
        if (networkManager->isHost()) {
            // Host upload: use NetworkManager to sync to all clients
            networkManager->uploadSoundAsHost(path);
        } else {
            // Client upload: use NetworkManager to send to host
            networkManager->uploadSoundAsClient(path);
        }
    } else {
        // Solo mode: use local SoundManager
        soundManager.importSound(path);
    }
}

// Slots pour NetworkManager
void MainWindow::onSessionCreated(const QString &code) {
    qDebug() << "Session created with code:" << code;
    sessionCodeLabel->setText("Session Code: " + code);
    sessionCodeLabel->show();
    startSessionButton->show();

    QMessageBox::information(this, "Session créée", 
        QString("Session créée avec le code: %1\n\nPartagez ce code avec vos amis pour qu'ils puissent rejoindre.").arg(code));
}

void MainWindow::onClientJoined(const QString &name) {
    qDebug() << "Client joined:" << name;

    // Optionnel: afficher une notification
    statusBar()->showMessage(QString("Client connecté: %1").arg(name), 3000);
}

void MainWindow::onJoinedSession() {
    qDebug() << "Successfully joined session!";

    hideJoinInterface();
    showGameInterface();

    QMessageBox::information(this, "Connexion réussie", 
        "Vous avez rejoint la session avec succès!\nEn attente du démarrage par l'hôte...");
}

void MainWindow::onSessionStarted() {
    qDebug() << "Session started!";

    // Masquer les éléments de configuration et afficher le jeu
    sessionCodeLabel->hide();
    startSessionButton->hide();
    showGameInterface();

    QMessageBox::information(this, "Session démarrée", "La session a démarré! Vous pouvez maintenant jouer.");
}

void MainWindow::onConnectionError(const QString &error) {
    qDebug() << "Connection error:" << error;

    // Réactiver le bouton de connexion
    joinSubmitButton->setEnabled(true);
    joinSubmitButton->setText("Rejoindre");

    QMessageBox::critical(this, "Erreur de connexion", 
        QString("Impossible de rejoindre la session:\n%1").arg(error));
}

// Méthodes utilitaires
void MainWindow::hideModeButtons() {
    soloButton->hide();
    hostButton->hide();
    joinButton->hide();
}

void MainWindow::showModeButtons() {
    soloButton->show();
    hostButton->show();
    joinButton->show();
}

void MainWindow::showGameInterface() {
    padPage->show();
}

void MainWindow::hideJoinInterface() {
    joinCodeInput->hide();
    joinSubmitButton->hide();
    backButtonJoin->hide();
}
