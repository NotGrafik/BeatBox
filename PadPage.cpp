#include "PadPage.h"
#include <QIcon>
#include <QGridLayout>
#include <QSizePolicy>
#include <QToolButton>
#include <QCoreApplication>


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


        if (padIcon.isNull()) {
            qDebug() << "Icon failed to load!";
        }

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
}

void PadPage::handlePadClick(int index) {
    emit padClicked(index);
}

void PadPage::handleUpload(int index) {
    QString filePath = QFileDialog::getOpenFileName(this, "Choose a sound file");
    if (!filePath.isEmpty()) {
        QString fileName = QFileInfo(filePath).fileName();
        QString destPath = QCoreApplication::applicationDirPath() + "/uploads/" + fileName;

        QDir().mkpath(QCoreApplication::applicationDirPath() + "/uploads"); // Crée le dossier si nécessaire
        QFile::copy(filePath, destPath);

        emit uploadSoundRequested(index, destPath);
    }
}

void PadPage::setPadLabel(int index, const QString &label) {
    if (index >= 0 && index < padButtons.size()) {
        QString displayLabel = label;
        if (displayLabel.endsWith(".wav", Qt::CaseInsensitive)) {
            displayLabel.chop(4); // Remove last 4 characters (".wav")
        }
        static_cast<QToolButton*>(padButtons[index])->setText(displayLabel);
    }
}

