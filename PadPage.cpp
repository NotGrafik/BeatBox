#include "PadPage.h"
#include <QIcon>
#include <QGridLayout>
#include <QSizePolicy>
#include <QToolButton>
#include <QCoreApplication>
#include <QFileDialog>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QStandardPaths>

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
        setPadLabel(index, "Loading...");
        uploadTimer->start(30000); // 30 seconds timeout for large audio files

        // Emit signal to MainWindow to handle the upload
        emit uploadSoundRequested(index, filePath);
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

void PadPage::handleUploadTimeout() {
    qDebug() << "Upload timed out - resetting pad state";
    // Reset all pads that might be in loading state
    for (int i = 0; i < padButtons.size(); ++i) {
        QString currentText = static_cast<QToolButton*>(padButtons[i])->text();
        if (currentText == "Loading...") {
            setPadLabel(i, QString("Pad %1").arg(i + 1));
        }
    }
}
