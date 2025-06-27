#pragma once

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QList>
#include <QFileDialog>
#include <QToolButton>
#include <QTimer>


class PadPage : public QWidget {
    Q_OBJECT

public:
    PadPage(QWidget *parent = nullptr);
    void setPadLabel(int index, const QString& label);

signals:
    void padClicked(int index);
    void sendToNetwork(const QString& message);
    void uploadSoundRequested(int index, const QString& filePath);
    void handleUploadTimeout();

public slots:
    void onSoundReady(int index, const QString& name);

private slots:
    void handlePadClick(int index);
    void handleUpload(int index);

private:
    QList<QToolButton*> padButtons;
    QList<QPushButton*> uploadButtons;
     QTimer* uploadTimer;
};

