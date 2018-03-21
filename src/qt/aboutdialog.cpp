#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include "clientmodel.h"
#include "clientversion.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    // Set current copyright year
    ui->copyrightLabel->setText(
        tr("Copyright") + QString(" &copy; 2009-%1 ").arg(COPYRIGHT_YEAR) + tr("The Bitcoin developers") + QString(",<br>") +
        tr("Copyright") + QString(" &copy; 2013-2018 ") + tr("The Dimecoin developers"));
}

void AboutDialog::setModel(ClientModel *model)
{
    if(model)
    {
#if defined(__x86_64__) || defined(_M_X64)
        ui->versionLabel->setText(model->formatFullVersion() + QString(" (64bit)"));
#else
        ui->versionLabel->setText(model->formatFullVersion());
#endif
    }
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::on_buttonBox_accepted()
{
    close();
}
