// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/csvmodelwriter.h>

#include <QAbstractItemModel>
#include <QFile>
#include <QTextStream>

CSVModelWriter::CSVModelWriter(const QString &_filename, QObject *parent) :
    QObject(parent),
    filename(_filename), model(0)
{
}

void CSVModelWriter::setModel(const QAbstractItemModel *_model)
{
    this->model = _model;
}

void CSVModelWriter::addColumn(const QString &title, int column, int role)
{
    Column col;
    col.title = title;
    col.column = column;
    col.role = role;

    columns.append(col);
}

// A spreadsheet application treats a field starting with '=', '+', '@' or '-'
// as a formula, which makes CSV export a code-execution vector for any
// user-controlled text (labels, addresses, comments). Fields that would be
// interpreted that way are prefixed with a single quote. A leading '-' is only
// escaped when the field also contains formula characters, so that ordinary
// negative amounts continue to export unchanged.
static bool needsFormulaGuard(const QString &value)
{
    if (value.isEmpty())
        return false;
    const QChar first = value.at(0);
    if (first == '=' || first == '+' || first == '@' || first == '\t' || first == '\r')
        return true;
    if (first == '-') {
        for (int i = 1; i < value.size(); i++) {
            const QChar c = value.at(i);
            if (c.isLetter() || c == '|' || c == '!' || c == '(' || c == ')')
                return true;
        }
    }
    return false;
}

static void writeValue(QTextStream &f, const QString &value)
{
    QString escaped = value;
    if (needsFormulaGuard(escaped))
        escaped.prepend('\'');
    escaped.replace('"', "\"\"");
    f << "\"" << escaped << "\"";
}

static void writeSep(QTextStream &f)
{
    f << ",";
}

static void writeNewline(QTextStream &f)
{
    f << "\n";
}

bool CSVModelWriter::write()
{
    QFile file(filename);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream out(&file);

    int numRows = 0;
    if(model)
    {
        numRows = model->rowCount();
    }

    // Header row
    for(int i=0; i<columns.size(); ++i)
    {
        if(i!=0)
        {
            writeSep(out);
        }
        writeValue(out, columns[i].title);
    }
    writeNewline(out);

    // Data rows
    for(int j=0; j<numRows; ++j)
    {
        for(int i=0; i<columns.size(); ++i)
        {
            if(i!=0)
            {
                writeSep(out);
            }
            QVariant data = model->index(j, columns[i].column).data(columns[i].role);
            writeValue(out, data.toString());
        }
        writeNewline(out);
    }

    file.close();

    return file.error() == QFile::NoError;
}
