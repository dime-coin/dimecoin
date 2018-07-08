#ifndef DONATEDIALOG_H
#define DONATEDIALOG_H

#include <QDialog>
#include <QImage>

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h" /* for USE_QRCODE */
#endif

namespace Ui {
    class DonateDialog;
}
class ClientModel;

class DonateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DonateDialog(const QString &addr, QWidget *parent = 0);
    ~DonateDialog();

private slots:
    void on_btnOk_clicked();

private:
    Ui::DonateDialog *ui;
    QString address;
    QImage myImage;
#ifdef USE_QRCODE
    void genCode();
#endif
    QString getURI();
};

#endif // DONATEDIALOG_H
