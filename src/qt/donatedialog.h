#ifndef DONATEDIALOG_H
#define DONATEDIALOG_H

#include <QDialog>
#include <QImage>

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

    //void setModel(OptionsModel *model);

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
