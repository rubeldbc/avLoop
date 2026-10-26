#pragma once

#include <QDialog>

namespace avloop {

class AboutDialog : public QDialog {
    Q_OBJECT
public:
    explicit AboutDialog(QWidget* parent = nullptr);
};

} // namespace avloop
