#include "donatedialog.h"
#include "ui_donatedialog.h"

#include "guiconstants.h"

#include <QPixmap>
#include <QUrl>

#ifdef USE_QRCODE
#include <qrencode.h>
#endif

DonateDialog::DonateDialog(const QString &addr, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DonateDialog),
    address(addr)
{
    ui->setupUi(this);

    setWindowTitle(QString(tr("Donate")));

#ifdef USE_QRCODE
    genCode();
#else
    ui->lblDonateQRCode->setVisible(false);
    ui->outDonateUri->clear();
    ui->outDonateUri->setPlainText(QString(address));
#endif
}

DonateDialog::~DonateDialog()
{
    delete ui;
}

#ifdef USE_QRCODE
void DonateDialog::genCode()
{
    QString uri = getURI();

    if (uri != "")
    {
        ui->lblDonateQRCode->setText("");

        QRcode *code = QRcode_encodeString(uri.toUtf8().constData(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);
        if (!code)
        {
            ui->lblDonateQRCode->setText(tr("Error encoding URI into QR Code."));
            return;
        }
        myImage = QImage(code->width + 8, code->width + 8, QImage::Format_RGB32);
        myImage.fill(0xffffff);
        unsigned char *p = code->data;
        for (int y = 0; y < code->width; y++)
        {
            for (int x = 0; x < code->width; x++)
            {
                myImage.setPixel(x + 4, y + 4, ((*p & 1) ? 0x0 : 0xffffff));
                p++;
            }
        }
        QRcode_free(code);

        ui->lblDonateQRCode->setPixmap(QPixmap::fromImage(myImage).scaled(300, 300));

        ui->outDonateUri->setPlainText(uri);
    }
}
#endif

QString DonateDialog::getURI()
{
    QString ret = QString("dime:%1?label=Dimecoin").arg(address);
    int paramCount = 0;

    ui->outDonateUri->clear();

    // limit URI length to prevent a DoS against the QR-Code dialog
    if (ret.length() > MAX_URI_LENGTH)
    {
        ui->lblDonateQRCode->setText(tr("Resulting URI too long, try to reduce the text for label / message."));
        return QString("");
    }

    return ret;
}


void DonateDialog::on_btnOk_clicked()
{
    close();
}
