// Copyright (c) 2011-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/shirecoinunits.h>

#include <QStringList>

ShirecoinUnits::ShirecoinUnits(QObject *parent):
        QAbstractListModel(parent),
        unitlist(availableUnits())
{
}

QList<ShirecoinUnits::Unit> ShirecoinUnits::availableUnits()
{
    QList<ShirecoinUnits::Unit> unitlist;
    unitlist.append(SHIRE);
    unitlist.append(mSHIRE);
    unitlist.append(uSHIRE);
    unitlist.append(SAT);
    return unitlist;
}

bool ShirecoinUnits::valid(int unit)
{
    switch(unit)
    {
    case SHIRE:
    case mSHIRE:
    case uSHIRE:
    case SAT:
        return true;
    default:
        return false;
    }
}

QString ShirecoinUnits::longName(int unit)
{
    switch(unit)
    {
    case SHIRE: return QString("SHIRE");
    case mSHIRE: return QString("mSHIRE");
    case uSHIRE: return QString::fromUtf8("µSHIRE (bits)");
    case SAT: return QString("Satoshi (sat)");
    default: return QString("???");
    }
}

QString ShirecoinUnits::shortName(int unit)
{
    switch(unit)
    {
    case uSHIRE: return QString::fromUtf8("bits");
    case SAT: return QString("sat");
    default: return longName(unit);
    }
}

QString ShirecoinUnits::description(int unit)
{
    switch(unit)
    {
    case SHIRE: return QString("Shirecoins");
    case mSHIRE: return QString("Milli-Shirecoins (1 / 1" THIN_SP_UTF8 "000)");
    case uSHIRE: return QString("Micro-Shirecoins (bits) (1 / 1" THIN_SP_UTF8 "000" THIN_SP_UTF8 "000)");
    case SAT: return QString("Satoshi (sat) (1 / 100" THIN_SP_UTF8 "000" THIN_SP_UTF8 "000)");
    default: return QString("???");
    }
}

qint64 ShirecoinUnits::factor(int unit)
{
    switch(unit)
    {
    case SHIRE: return 100000000;
    case mSHIRE: return 100000;
    case uSHIRE: return 100;
    case SAT: return 1;
    default: return 100000000;
    }
}

int ShirecoinUnits::decimals(int unit)
{
    switch(unit)
    {
    case SHIRE: return 8;
    case mSHIRE: return 5;
    case uSHIRE: return 2;
    case SAT: return 0;
    default: return 0;
    }
}

QString ShirecoinUnits::format(int unit, const CAmount& nIn, bool fPlus, SeparatorStyle separators)
{
    // Note: not using straight sprintf here because we do NOT want
    // localized number formatting.
    if(!valid(unit))
        return QString(); // Refuse to format invalid unit
    qint64 n = (qint64)nIn;
    qint64 coin = factor(unit);
    int num_decimals = decimals(unit);
    qint64 n_abs = (n > 0 ? n : -n);
    qint64 quotient = n_abs / coin;
    QString quotient_str = QString::number(quotient);

    // Use SI-style thin space separators as these are locale independent and can't be
    // confused with the decimal marker.
    QChar thin_sp(THIN_SP_CP);
    int q_size = quotient_str.size();
    if (separators == separatorAlways || (separators == separatorStandard && q_size > 4))
        for (int i = 3; i < q_size; i += 3)
            quotient_str.insert(q_size - i, thin_sp);

    if (n < 0)
        quotient_str.insert(0, '-');
    else if (fPlus && n > 0)
        quotient_str.insert(0, '+');

    if (num_decimals > 0) {
        qint64 remainder = n_abs % coin;
        QString remainder_str = QString::number(remainder).rightJustified(num_decimals, '0');
        return quotient_str + QString(".") + remainder_str;
    } else {
        return quotient_str;
    }
}


// NOTE: Using formatWithUnit in an HTML context risks wrapping
// quantities at the thousands separator. More subtly, it also results
// in a standard space rather than a thin space, due to a bug in Qt's
// XML whitespace canonicalisation
//
// Please take care to use formatHtmlWithUnit instead, when
// appropriate.

QString ShirecoinUnits::formatWithUnit(int unit, const CAmount& amount, bool plussign, SeparatorStyle separators)
{
    return format(unit, amount, plussign, separators) + QString(" ") + shortName(unit);
}

QString ShirecoinUnits::formatHtmlWithUnit(int unit, const CAmount& amount, bool plussign, SeparatorStyle separators)
{
    QString str(formatWithUnit(unit, amount, plussign, separators));
    str.replace(QChar(THIN_SP_CP), QString(THIN_SP_HTML));
    return QString("<span style='white-space: nowrap;'>%1</span>").arg(str);
}


bool ShirecoinUnits::parse(int unit, const QString &value, CAmount *val_out)
{
    if(!valid(unit) || value.isEmpty())
        return false; // Refuse to parse invalid unit or empty string
    int num_decimals = decimals(unit);

    // Ignore spaces and thin spaces when parsing
    QStringList parts = removeSpaces(value).split(".");

    if(parts.size() > 2)
    {
        return false; // More than one dot
    }
    QString whole = parts[0];
    QString decimals;

    if(parts.size() > 1)
    {
        decimals = parts[1];
    }
    if(decimals.size() > num_decimals)
    {
        return false; // Exceeds max precision
    }
    bool ok = false;
    QString str = whole + decimals.leftJustified(num_decimals, '0');

    if(str.size() > 18)
    {
        return false; // Longer numbers will exceed 63 bits
    }
    CAmount retvalue(str.toLongLong(&ok));
    if(val_out)
    {
        *val_out = retvalue;
    }
    return ok;
}

QString ShirecoinUnits::getAmountColumnTitle(int unit)
{
    QString amountTitle = QObject::tr("Amount");
    if (ShirecoinUnits::valid(unit))
    {
        amountTitle += " ("+ShirecoinUnits::shortName(unit) + ")";
    }
    return amountTitle;
}

int ShirecoinUnits::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return unitlist.size();
}

QVariant ShirecoinUnits::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if(row >= 0 && row < unitlist.size())
    {
        Unit unit = unitlist.at(row);
        switch(role)
        {
        case Qt::EditRole:
        case Qt::DisplayRole:
            return QVariant(longName(unit));
        case Qt::ToolTipRole:
            return QVariant(description(unit));
        case UnitRole:
            return QVariant(static_cast<int>(unit));
        }
    }
    return QVariant();
}

CAmount ShirecoinUnits::maxMoney()
{
    return MAX_MONEY;
}
