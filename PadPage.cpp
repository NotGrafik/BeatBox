#include "PadPage.h"
#include <QIcon>
#include <QGridLayout>
#include <QSizePolicy>
#include <QToolButton>
#include <QCoreApplication>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QStandardPaths>
#include "SoundManager.h"

PadPage::PadPage(QWidget *parent) : QWidget(parent) {
    QGridLayout *grid = new QGridLayout(this);

    int padCount = 4;
    int rowCount = 2;
    int colCount = 2;

    for (int i = 0; i < padCount; ++i) {
        QToolButton *pad = new QToolButton();
        QPushButton *upload = new QPushButton("Upload");

        QString imgPath = QString(":/assets/pad_btn.png"); 
        QIcon padIcon(imgPath);
        pad->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        pad->setIcon(padIcon);
        pad->setText(QString("Pad %1").arg(i + 1));
        pad->setIconSize(QSize(300, 300));
        pad->setFixedSize(300, 340);
        pad->setStyleSheet("border: none; font-size: 24px;");

        padButtons.append(pad);
        uploadButtons.append(upload);

        connect(pad, &QToolButton::clicked, this, [this, i]() { handlePadClick(i); });
        connect(upload, &QPushButton::clicked, this, [this, i]() { handleUpload(i); });

        QVBoxLayout *subLayout = new QVBoxLayout;
        subLayout->addWidget(pad, 0, Qt::AlignCenter);
        subLayout->addWidget(upload, 0, Qt::AlignCenter);

        int row = i / colCount;
        int col = i % colCount;
        grid->addLayout(subLayout, row, col);
    }

    uploadTimer = new QTimer(this);
    uploadTimer->setSingleShot(true);
    connect(uploadTimer, &QTimer::timeout, this, &PadPage::handleUploadTimeout);
}

void PadPage::handlePadClick(int index) {
    emit padClicked(index);

    QJsonObject obj;
    obj["type"] = "play";
    obj["index"] = index;
    QJsonDocument doc(obj);
    emit sendToNetwork(doc.toJson(QJsonDocument::Compact) + "\n");
}

void PadPage::handleUpload(int index) {
    QString filePath = QFileDialog::getOpenFileName(this, "Choose a sound file");
    if (!filePath.isEmpty()) {
        // QString fileName = QFileInfo(filePath).fileName();
        // QFile file(filePath);
        
        // if (!file.open(QIODevice::ReadOnly)) {
        //     setPadLabel(index, "File Error");
        //     return;
        // }

        // setPadLabel(index, "Loading...");
        // uploadTimer->start(10000); // 10 secondes timeout

        // QByteArray fileData = file.readAll();
        // file.close();

        // // Envoyer en base64
        // QJsonObject obj;
        // obj["type"] = "upload";
        // obj["index"] = index;
        // obj["name"] = fileName;
        // obj["data"] = QString::fromUtf8(fileData.toBase64());
        // obj["size"] = fileData.size();

        // QJsonDocument doc(obj);
        // emit sendToNetwork(doc.toJson(QJsonDocument::Compact) + "\n");
        SoundManager* soundManager = new SoundManager(this);
        if (soundManager) {
            soundManager->importSound(filePath);
            QFileInfo info(filePath);
            QString name = info.fileName();
            setPadLabel(index, name);

            emit uploadSoundRequested(index, filePath);
            emit sendToNetwork(QString("upload %1 %2").arg(index).arg(name));
        } else {
            qWarning() << "Parent is not a SoundManager instance."; 
        }
    }
}

void PadPage::setPadLabel(int index, const QString &label) {
    if (index >= 0 && index < padButtons.size()) {
        QString displayLabel = label;
        if (displayLabel.endsWith(".wav", Qt::CaseInsensitive)) {
            displayLabel.chop(4);
        }
        static_cast<QToolButton*>(padButtons[index])->setText(displayLabel);
    }
}

void PadPage::onSoundReady(int index, const QString& name) {
    uploadTimer->stop();
    setPadLabel(index, name);
}
